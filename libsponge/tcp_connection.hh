#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"

// TCP连接的完整端点
class TCPConnection {
  private:
    TCPConfig _cfg;
    TCPReceiver _receiver{_cfg.recv_capacity};
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};

    // TCPConnection 想要发送的段的出站队列
    std::queue<TCPSegment> _segments_out{};

    // 在两个流都结束后，TCPConnection 应该保持活动状态（并保持 ACK）10 * _cfg.rt_timeout 毫秒，
    // 以防远程 TCPConnection 不知道我们已经收到了它的整个流
    bool _linger_after_streams_finish{true};

    bool _isactive{true};

    // 自上次收到数据段以来的时间
    size_t _time_since_last_segment_received{0};

  public:
    // 写入方的写入接口

    // 通过发送SYN段发起连接
    void connect();

    // 将数据写入出站字节流，并在可能的情况下通过 TCP 发送
    // 返回实际写入的“数据”字节数。
    size_t write(const std::string &data);

    // 现在可以写入的“字节”数
    size_t remaining_outbound_capacity() const;

    // 关闭出站字节流（仍然允许读取传入的数据）
    void end_input_stream();


    //  读取方的读取接口

    // 从对等端接收到的入站字节流
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
  

    // 用于测试的访问器

  
    // 已发送但尚未确认的字节数，每个 SYN/FIN 算作一个字节
    size_t bytes_in_flight() const;


    // 尚未重组的字节数
    size_t unassembled_bytes() const;


    // 自收到最后一个段以来的毫秒数
    size_t time_since_last_segment_received() const;


    // 总结发送方、接收方和连接的状态
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    

    // 所有者或操作系统调用的方法

    // 从网络接收到新段时调用
    void segment_received(const TCPSegment &seg);

    // 时间流逝时定期调用
    void tick(const size_t ms_since_last_tick);

    // 排队等待传输的 TCPSegments。
    // 所有者或操作系统将出队这些并将每个放入较低层数据报里（通常是互联网数据报（IP），
    // 但也可以是用户数据报（UDP）或任何其他类型）的有效负载中。
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    // 连接是否仍然有效？
    // 如果任一流仍在运行或两个流都完成后 TCPConnection 仍然存在（例如，对来自对等方的 ACK 重新传输），则为“true”
    bool active() const;
 

    // 根据配置构建新连接
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    // 构造和析构函数

    ~TCPConnection();  
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;

    // 发送段
    void send_segment();

    // 发送 RST 段
    void send_rst_segment();

    // 优雅关闭
    void clean_shutdown();

    // 非优雅关闭
    void unclean_shutdown();
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH
