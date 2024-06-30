#include "tcp_connection.hh"

#include <iostream>
#include <numeric>
// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity();}

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received;}

bool TCPConnection::active() const { return _isactive; }

// 从网络接收到新段时调用
void TCPConnection::segment_received(const TCPSegment &seg) {
    if(!_isactive) return;
    // 若连接关闭，返回

    // 重设时间
    _time_since_last_segment_received = 0;

    // 如果RST（reset）标志位为真，将发送端stream和接受端stream设置成error state并终止连接。
    if(seg.header().rst){
        unclean_shutdown();
        return;
    }

    // 把segment传递给TCPReceiver，这样的话，TCPReceiver就能从segment取出它所关心的字段进行处理了：seqno，SYN，payload，FIN。
    _receiver.segment_received(seg);

    // 若处于监听状态，则建立连接。
    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV && 
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED){
            connect();
            return;
    }

    // 如果ACK标志位为真，通知TCPSender有segment被确认，TCPSender关心的字段有ackno和window_size
    if(seg.header().ack)
        _sender.ack_received(seg.header().ackno, seg.header().win);
    
    // 如果收到的segment不为空，TCPConnection必须确保至少给这个segment回复一个ACK，以便远端的发送方更新ackno和window_size
    if(seg.length_in_sequence_space() > 0 && _sender.segments_out().empty())
        _sender.send_empty_segment();
    
    // 此为空数据段，目的为测试连接是否有效
    if(_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 &&
        seg.header().seqno == _receiver.ackno().value() - 1){
            _sender.send_empty_segment();
        }
    
    send_segment();

    clean_shutdown();

    
}

size_t TCPConnection::write(const string &data) {
    // 如果连接关闭，或要传入的数据为空，则返回
    if(!_isactive || data.empty())
        return 0;
    
    // 发送
    size_t len = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segment();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
// 自上次调用此方法以来的毫秒数
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // 若连接关闭，则返回
    if(!active())
        return;
    
    // 超时重传
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received += ms_since_last_tick;
    
    // 若连续重传次数超过最大次数，则发送RST数据段，并关闭连接
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        send_rst_segment();
        unclean_shutdown();
        return;
    }

    // 发送数据，关掉连接
    send_segment();
    clean_shutdown();
}

// 结束输入
void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segment();
}

// 建立连接，并填充缓冲区
void TCPConnection::connect() {
    _sender.fill_window();
    send_segment();
}

void TCPConnection::send_segment() {
    // 待发送队列不为空
    while(!_sender.segments_out().empty()){
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();

        // 填写头部信息
        if(_receiver.ackno().has_value()){
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = (_receiver.window_size() <= numeric_limits<uint16_t>::max()) ? 
                                                            _receiver.window_size() : numeric_limits<uint16_t>::max();
        }

        // 进入发送队列
        _segments_out.push(seg);
    }
}

void TCPConnection::send_rst_segment()
{
    _sender.fill_window();
    // 若待发送队列为空
    if(_sender.segments_out().empty()){
        _sender.send_empty_segment();
        return;
    }

    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    if(_receiver.ackno().has_value()){
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        seg.header().win = (_receiver.window_size() <= numeric_limits<uint16_t>::max()) ? 
                                                _receiver.window_size() : numeric_limits<uint16_t>::max();
    }
    // 设置重连标志
    seg.header().rst = true;

    _segments_out.push(seg);
}


void TCPConnection::clean_shutdown()
{
    // 接收的数据全部写入输出流里的重组器
    // 但发送端的缓存区不为空
    if(_receiver.stream_out().input_ended() && !_sender.stream_in().eof()){
        _linger_after_streams_finish = false;
        // 关闭receiver和sender的stream流结束之后的等待，即linger状态。
    }
    else if(TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && 
            TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV){
                // 若TCP连接处于结束且已确认状态
                // 保留连接活跃为超时重传初始值的10倍时间，确保所有数据都成功发送且成功被接收，然后关闭连接
                if(!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout)
                    _isactive = false;
                    // 关闭连接
            }
}

// 连接异常
void TCPConnection::unclean_shutdown()
{
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _isactive = false;
}

// 析构函数，回收连接
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            send_rst_segment();
            unclean_shutdown();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
