#include "wrapping_integers.hh"

#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! 将一个WrappingInt32转换为“绝对”的64位序列号（从零开始索引）
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

//! 将一个WrappingInt32转换为一个“绝对”的64位序列号（从零开始索引）
//! \param n 相对序列号
//! \param isn 初始序列号
//! \param checkpoint 最近的64位绝对序列号
//! \ 返回一个64位序列号，该序列号环绕到`n`并且最接近`checkpoint`
//!
//! \注意 TCP 连接的两条流有各自的ISN。一条流
//！从本地 TCPSender 运行到远程 TCPReceiver，
//! 有一个ISN，另一条流从远程 TCPSender 运行到本地 TCPReceiver，
//! 有一个不同的ISN。
//! TCP 连接的两个数据流各有自己的 ISN。一个数据流从本地 TCPSender 运行到远程 TCPReceiver，
//! 有一个 ISN，另一个数据流从远程 TCPSender 运行到本地 TCPReceiver，有一个不同的 ISN。
/**
 * 将一个 WrappingInt32 类型的对象 n 转换为一个绝对的 64 位序列号，并找到距离 checkpoint 最近的序列号。
 * 这个函数使用了一种算法来计算序列号之间的偏移量，并根据需要进行环绕处理。
 *
 * @param n 要转换的 WrappingInt32 类型的对象。
 * @param isn 用于计算的初始序列号。
 * @param checkpoint 用于计算绝对序列号的参考点。
 * @return 返回一个 64 位序列号，该序列号环绕到 n 并且最接近 checkpoint。
 */
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t offset = n - wrap(checkpoint, isn);
    uint64_t pos = checkpoint + offset;

    if (offset > (1u << 31) && pos >= (1ul << 32))
        pos -= (1ul << 32);

    return pos;
}

