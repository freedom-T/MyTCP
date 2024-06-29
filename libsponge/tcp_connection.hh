#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"

//! \brief A complete endpoint of a TCP connection
class TCPConnection {
  private:
    TCPConfig _cfg;
    TCPReceiver _receiver{_cfg.recv_capacity};
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};

    //! outbound queue of segments that the TCPConnection wants sent
    //! TCPConnection 想要发送的段的出站队列
    std::queue<TCPSegment> _segments_out{};

    //! Should the TCPConnection stay active (and keep ACKing)
    //! for 10 * _cfg.rt_timeout milliseconds after both streams have ended,
    //! in case the remote TCPConnection doesn't know we've received its whole stream?
    // 在两个流都结束后，TCPConnection 应该保持活动状态（并保持 ACK）10 * _cfg.rt_timeout 毫秒，
    // 以防远程 TCPConnection 不知道我们已经收到了它的整个流
    bool _linger_after_streams_finish{true};

    bool _isactive{true};

    // 自上次收到数据段以来的时间
    size_t _time_since_last_segment_received{0};

  public:
    //! \name "Input" interface for the writer
    //!@{ 写入方的写入接口

    //! \brief Initiate a connection by sending a SYN segment
    // 通过发送SYN段发起连接
    void connect();

    //! \brief Write data to the outbound byte stream, and send it over TCP if possible
    // 将数据写入出站字节流，并在可能的情况下通过 TCP 发送
    //! \returns the number of bytes from `data` that were actually written.
    // 返回实际写入的“数据”字节数。
    size_t write(const std::string &data);

    //! \returns the number of `bytes` that can be written right now.
    // 现在可以写入的“字节”数。
    size_t remaining_outbound_capacity() const;

    //! \brief Shut down the outbound byte stream (still allows reading incoming data)
    // 关闭出站字节流（仍然允许读取传入的数据）
    void end_input_stream();
    //!@}

    //! \name "Output" interface for the reader
    //!@{  读取方的读取接口

    //! \brief The inbound byte stream received from the peer
    // 从对等端接收到的入站字节流
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name Accessors used for testing
    // 用于测试的访问器

    //!@{
    //! \brief number of bytes sent and not yet acknowledged, counting SYN/FIN each as one byte
    // 已发送但尚未确认的字节数，每个 SYN/FIN 算作一个字节
    size_t bytes_in_flight() const;
    //! \brief number of bytes not yet reassembled
    // 尚未重组的字节数
    size_t unassembled_bytes() const;
    //! \brief Number of milliseconds since the last segment was received
    // 自收到最后一个段以来的毫秒数
    size_t time_since_last_segment_received() const;
    //!< \brief summarize the state of the sender, receiver, and the connection
    // 总结发送方、接收方和连接的状态
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name Methods for the owner or operating system to call
    //!@{ 所有者或操作系统调用的方法

    //! Called when a new segment has been received from the network
    // 从网络接收到新段时调用
    void segment_received(const TCPSegment &seg);

    //! Called periodically when time elapses
    // 时间流逝时定期调用
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPSegments that the TCPConnection has enqueued for transmission.
    // 排队等待传输的 TCPSegments。
    //! \note The owner or operating system will dequeue these and
    //! put each one into the payload of a lower-layer datagram (usually Internet datagrams (IP),
    //! but could also be user datagrams (UDP) or any other kind).
    // 所有者或操作系统将出队这些并将每个放入较低层数据报里（通常是互联网数据报（IP），
    // 但也可以是用户数据报（UDP）或任何其他类型）的有效负载中。
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief Is the connection still alive in any way?
    // 连接是否仍然有效？
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    // 如果任一流仍在运行或两个流都完成后 TCPConnection 仍然存在（例如，对来自对等方的 ACK 重新传输），则为“true”
    bool active() const;
    //!@}

    //! Construct a new connection from a configuration
    // 根据配置构建新连接
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name construction and destruction
    // 构造和析构函数
    //! moving is allowed; copying is disallowed; default construction not possible

    //!@{
    ~TCPConnection();  //!< destructor sends a RST if the connection is still open
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}

    void send_segment();
    void send_rst_segment();

    void clean_shutdown();
    void unclean_shutdown();
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH
