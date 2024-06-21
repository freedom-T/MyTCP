#include "wrapping_integers.hh"

#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! 将一个“绝对”的64位序列号（从零开始索引）转换为WrappingInt32
//! \param n 输入的绝对64位序列号
//! \param isn 初始序列号

/*
函数功能：将一个64位的绝对序列号与一个32位的初始序列号相加，然后返回这个新的 WrappingInt32 对象
用于处理TCP协议中的序列号环绕问题
*/
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) 
{
    uint32_t number = isn.raw_value() + static_cast<uint32_t>(n);
    return WrappingInt32{number};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t offset = n - wrap(checkpoint, isn);
    uint64_t pos = checkpoint + offset;

    if (offset > (1u << 31) && pos >= (1ul << 32))
        pos -= (1ul << 32);

    return pos;
}
