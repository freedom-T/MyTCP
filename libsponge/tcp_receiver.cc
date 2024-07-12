#include "tcp_receiver.hh"

// TCP接收器的虚拟实现

// 在实验2中，请用能够通过`make check_lab2`自动检查的真实实现替换此虚拟实现。


template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;


// 函数功能：接收并处理一个入站段
void TCPReceiver::segment_received(const TCPSegment &seg) 
{
    // 如果尚未接收到 SYN 标志
    if (!_syn) {
        // 如果段中没有 SYN 标志，直接返回
        if (!seg.header().syn)
            return;

        // 设置初始序列号为段的序列号，并更新 SYN 标志为 true
        _isn = seg.header().seqno;
        _syn = seg.header().syn;

        // 将段的载荷推入重新组装器，起始索引为 0，结束标志为段的 FIN 标志
        _reassembler.push_substring(seg.payload().copy(), 0, seg.header().fin);
        return;
    }

    // 计算绝对序列号，解包段的序列号，考虑初始序列号和已重新组装但尚未消耗的字节数
    uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, _reassembler.first_unassembled());

    // 将段的载荷推入重新组装器，起始索引为 abs_seqno - 1，结束标志为段的 FIN 标志
    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
}



// 函数功能：返回当前应发送给对等端的确认号（ackno）
// 如果还未接收到SYN段，则返回空的optional
optional<WrappingInt32> TCPReceiver::ackno() const 
{
    if (!_syn) // 如果尚未接收到SYN段，则返回空的optional
        return std::nullopt;

    // 计算确认号，即第一个尚未重新组装的字节的序列号加1
    uint64_t _ackno = _reassembler.first_unassembled() + 1;

    // 如果输出流已结束，确认号再加1
    if (_reassembler.stream_out().input_ended())
        _ackno += 1;

    // 初始化初始序列号
    return wrap(_ackno, _isn);
}



// 函数功能：计算TCP接收器当前的窗口大小
size_t TCPReceiver::window_size() const 
{ 
    // 窗口大小为第一个不可以接受的索引位置  减去  第一个未重组的索引位置
    return _reassembler.first_unacceptable() - _reassembler.first_unassembled(); 
}
