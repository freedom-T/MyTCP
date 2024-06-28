#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! 传出字节流的容量
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! 重新传输最之前的未完成段要等待的初始时间
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
//! 如果设置，则使用初始序列号（否则使用随机 ISN）
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _last_ackno; }

void TCPSender::fill_window() {
    TCPSegment seg;

    //！ 建立新的连接
    if (next_seqno_absolute() == 0) {
        // 设置SYN标志，发送SYN段以建立连接
        seg.header().syn = true;
        send_segment(seg);
        return;
        
    } else if (next_seqno_absolute() > 0 && next_seqno_absolute() == bytes_in_flight()) {
        // 如果接下来要发送的数据序列等于已发送待确认，说明已经发送过数据且还在飞行中。
        // 即_last_ackno （上次接受到的回应序号）不等于0
        return;
    }

    // 获取当前的窗口大小，如果窗口大小为0，设置为1
    uint16_t cur_win = _last_win == 0 ? 1 : _last_win;

    // 计算剩余可发送大小
    size_t remaining_win = cur_win - bytes_in_flight();

    // 循环填充窗口
    while ((remaining_win = cur_win - bytes_in_flight())) {
        // 如果流没有结束且还有未发送的数据
        if (!stream_in().eof() && next_seqno_absolute() > bytes_in_flight()) {
            // 根据窗口大小，调整发送的数据大小
            size_t payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE, remaining_win);
            seg.payload() = Buffer(stream_in().read(payload_size));

            // 如果流结束且长度小于剩余窗口，设置FIN标志
            if (stream_in().eof() && seg.length_in_sequence_space() < remaining_win)
                seg.header().fin = true;

            if (seg.length_in_sequence_space() == 0)
                return;
            // 发送
            send_segment(seg);
        } else if (stream_in().eof()) {
            // 在流结束时，所有的字节都已写入，设置fin标识位
            if (next_seqno_absolute() < stream_in().bytes_written() + 2) {
                seg.header().fin = true;
                send_segment(seg);
                return;
            } else // 若有遗漏字节
                return;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number) 
//! 远程接收方的确认号
//! \param window_size The remote receiver's advertised window size
//! 远程接收方公布的窗口大小
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ack = unwrap(ackno, _isn, _last_ackno);
    //解包出绝对确认号

    // 若确认序列号同要发送序列号不符
    if (abs_ack > _next_seqno)
        return;

    // 若确认序列号大于前一次确认序列号，则更新
    if (abs_ack > _last_ackno) {
        _last_ackno = abs_ack;

        // 若未发送数据段队列不为空
        while (!_outstanding_seg.empty()) {
            const TCPSegment &seg = _outstanding_seg.front();
            // 队首已发送
            if (seg.header().seqno.raw_value() + seg.length_in_sequence_space() <= ackno.raw_value())
                _outstanding_seg.pop();
            else
                break;
        }

        _retransmission_count = 0;
        _RTO = _initial_retransmission_timeout;

        // 队列不为空，则开启计时器
        if (!_outstanding_seg.empty())
            _timer.start(_RTO);
        else
            _timer.stop();
    }

    // 更新窗口大小
    _last_win = window_size;
    // 发送数据
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
// 自上次调用此方法以来的毫秒数
void TCPSender::tick(const size_t ms_since_last_tick) {
    // 判断是否超时
    _timer.tick(ms_since_last_tick);

    // 若未发送队列不为空，且时间时间已过期
    if (!_outstanding_seg.empty() && _timer.is_expired()) {
        _segments_out.push(_outstanding_seg.front());
        if (_last_win > 0) {
            _retransmission_count++;
            _RTO *= 2;
            // 超时时间翻倍，避免拥塞问题
        }

        _timer.start(_RTO);
    } else if (_outstanding_seg.empty())
        // 队列已空
        _timer.stop();
}

// 返回连续重传次数
unsigned int TCPSender::consecutive_retransmissions() const { return _retransmission_count; }

void TCPSender::send_segment(TCPSegment &seg) {
    seg.header().seqno = next_seqno();
    _next_seqno += seg.length_in_sequence_space();

    _segments_out.push(seg);// 待发送队列
    _outstanding_seg.push(seg);// 未发送队列

    if (!_timer.is_start()) {
        _timer.start(_RTO);
    }
}

// 发送空数据段
void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
