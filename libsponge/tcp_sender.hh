#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>


//重传计时器
class RetransmissionTimer {
  public:
    void start(const unsigned int rto) {  // 开始计时
        _is_started = true;
        _is_expired = false;
        _remaining_time = rto;
    }

    void stop() {  //停止计时
        _is_started = false;
        _is_expired = false;
        _remaining_time = 0;
    }

    // 超时重传 若超出剩余时间，则设定已超时
    // 自上次计时以来
    void tick(const size_t ms_since_last_tick) 
    {
        // 未开始计时
        if (!is_start()) 
        {
            return;
        }
        // 超时
        if (ms_since_last_tick >= _remaining_time) {
            _is_expired = true;
        }
        // 未超时 
        else 
        {
            _remaining_time -= ms_since_last_tick;
        }
    }

    // 超时
    bool is_expired() { return _is_expired && _is_started; }
    bool is_start() { return _is_started; }


  
  private:
    // 剩余时间 
    unsigned int _remaining_time{0};
    // 已超时
    bool _is_expired{false};
    // 已开始计时
    bool _is_started{false};
};

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
//接受一个字节流，将其分成几段并发送这些段，跟踪哪些段仍在传输中，维护重传计时器，并在重传计时器到期时重新传输传输中的段。
class TCPSender {
  private:
    // 初始序列号，即 SYN 号码。
    WrappingInt32 _isn;

    // TCPSender 想要发送的段的出向队列
    std::queue<TCPSegment> _segments_out{};

    // 此连接的重传定时器的时间
    unsigned int _initial_retransmission_timeout;

    // 尚未发送的传出字节流
    ByteStream _stream;

    // 下一个要发送的字节的（绝对）序列号
    uint64_t _next_seqno{0};

  private:
    unsigned int _RTO{_initial_retransmission_timeout};  // 当前超时重传
    unsigned int _retransmission_count{0};               // 连续重传的次数

    // 上一次接收到的
    uint64_t _last_ackno{0};
    // 上一次发送的窗口大小
    uint16_t _last_win{1};

    // 重传器
    RetransmissionTimer _timer{};

    // 尚未发送的段
    std::queue<TCPSegment> _outstanding_seg{};
    
    // 发送段
    void send_segment(TCPSegment &seg);

  public:
    // 初始化TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    // \name "Input" interface for the writer 输入接口
    // 
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }

    // 使TCPSender发送段的方法

    // 收到新的ACK消息
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    // 生成空有效载荷段（用于创建空 ACK 段）
    void send_empty_segment();

    // 创建并发送段以尽可能填充窗口
    void fill_window();

    //! 通知 TCPSender 超时时间已到
    void tick(const size_t ms_since_last_tick);


    //! \name Accessors 访问器


    //! 已发送但尚未确认的段占用了多少个序列号？
    //! 计数在“序列空间”中，即 SYN 和 FIN 各计为一个字节
    //! 参见TCPSegment::length_in_sequence_space()
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    //! 返回连续重传的次数
    unsigned int consecutive_retransmissions() const;

    //! 排队等待传输的 TCPSegments。
    //! 这些必须由 TCPConnection 出队并发送，
    //! TCPConnection 需要在发送之前填写由 TCPReceiver 设置的字段（ackno 和窗口大小）。
    std::queue<TCPSegment> &segments_out() { return _segments_out; }




    //! 下列方法用于测试

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH