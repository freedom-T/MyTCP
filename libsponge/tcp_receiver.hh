#ifndef SPONGE_LIBSPONGE_TCP_RECEIVER_HH
#define SPONGE_LIBSPONGE_TCP_RECEIVER_HH

#include "byte_stream.hh"
#include "stream_reassembler.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"


#include <optional>

//! \brief TCP 实现中的 "接收方" 部分

//! 接收并重新组装分段到一个 ByteStream 中，计算确认号和窗口大小，用于发送给远程的 TCPSender。


class TCPReceiver {
    //! 我们用于重新组装字节的数据结构。
    StreamReassembler _reassembler;

    //! 我们将存储的最大字节数量。
    size_t _capacity;


    // 表示这个TCP段是否是一个SYN段的标志。
    // 在建立连接时，SYN段用于在发送方和接收方之间同步序列号。
    bool _syn;            

    // 该TCP段的初始序列号（ISN）。
    // ISN是发送方随机选择的一个数值，用于初始化数据传输的序列号空间。
    // 它确保数据包的序列号在传输中的唯一性，对可靠的数据传输起着重要作用。
    WrappingInt32 _isn;   


  public:
    //! \brief 构造一个TCP接收器
    //!
    //! \param capacity 接收器在任何给定时间内将存储在其缓冲区中的最大字节数。


    // 构造函数初始化 TCPReceiver 对象，并设置了以下成员变量：
    // _reassembler: TCP 数据流的重新组装器，用于将接收到的段重新组装成字节流
    // _capacity: 接收器的缓冲区最大容量，即最多可以存储的字节数
    // _syn: 是否已接收到 SYN 标志的状态，初始为 false，表示尚未接收到 SYN 标志
    // _isn: 初始序列号，初始为 0，用于确定数据流的起始序列号
    TCPReceiver(const size_t capacity)
        : _reassembler(capacity), 
          _capacity(capacity), 
          _syn(false), 
          _isn(0) 
        {}



    //! \name 用于向远程 TCPSender 提供反馈的访问器
    
    //! \brief 应发送给对端的确认号
    //! \returns 如果没有接收到 SYN，则返回空


    //! 这是接收窗口的起始，或者说，接收器尚未接收到的流中第一个字节的序列号。

    std::optional<WrappingInt32> ackno() const;
    

    //! \brief 应发送给对等方的窗口大小
    //!
    //! 操作上：容量减去 TCPReceiver 在其字节流中保存的字节数
    //! （已重新组装但尚未消耗的字节）。
    //!
    //! 形式上：(a) 在窗口之后的第一个字节的序列号（接收器不会接受该字节）与
    //! (b) 窗口开始的序列号（确认号）之间的差异。
    //!
    //! \returns 窗口大小，即接收器当前应该发送给对等方的窗口大小
    size_t window_size() const;
  

    // 存储但尚未重新组装的字节数量
    size_t unassembled_bytes() const { return _reassembler.unassembled_bytes(); }


    // 处理传入的段
    void segment_received(const TCPSegment &seg);


    // 读取器的“输出”接口
    ByteStream &stream_out() { return _reassembler.stream_out(); }
    const ByteStream &stream_out() const { return _reassembler.stream_out(); }
};

#endif  // SPONGE_LIBSPONGE_TCP_RECEIVER_HH
