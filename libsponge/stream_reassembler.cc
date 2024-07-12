#include "stream_reassembler.hh"

#include <iostream>
// 流重组器的示例实现。

// 对于实验 1，请用一个真实的实现替换该示例实现，并通过 `make check_lab1` 运行的自动检查。


// 你需要在 `stream_reassembler.hh` 中的类声明中添加私有成员。


template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}



using namespace std;



StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity),         // 初始化 ByteStream 对象，设置其容量为 capacity
      _capacity(capacity),       // 设置 StreamReassembler 的容量
      _eof_index(0),             // 初始化终止字节的索引为 0
      _unassembled_bytes(0),     // 初始化未组装字节数为 0
      _eof(false),               // 初始化终止标志为 false
      _buf()                  // 初始化用于存储片段的集合为空
{

} 


// 函数功能：除去已经写入流中的缓冲区的字符串
void StreamReassembler::_buf_erase(const set<Segment>::iterator &iter) 
{
    _unassembled_bytes -= iter->length();  // 从未组装字节数中减去该片段的长度
    _buf.erase(iter);                      // 从集合中移除该片段
}


// 函数功能：向未组装片段缓冲区中插入字符串
void StreamReassembler::_buf_insert(const Segment &seg) 
{
    _unassembled_bytes += seg.length();  // 将片段的长度加到未组装字节数中
    _buf.insert(seg);                    // 将片段插入到集合中
}


// 该函数接受来自逻辑流的一个子字符串（也称为片段），可能是无序的，
// 并组装任何新的连续子字符串，将它们按顺序写入输出流。

// 函数功能：将子字符串 data 推入流中，将其传递给流重新组装器
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) 
{
    if (eof) 
    {  // 若 eof 为 true，表明到达了流的末尾，计算出结束索引并将标志符置为 true
        _eof_index = index + data.size();
        _eof = true;
    }

    if (!data.empty()) 
    {  // 如果 data 不为空，则处理该子字符串
        Segment seg{index, data};
        _handle_substring(seg);  // 处理该片段
    }

    // 将连续的片段写入输出流，并从缓冲区中移除
    while (!_buf.empty() && _buf.begin()->_idx == first_unassembled()) 
    {
        const auto &iter = _buf.begin();
        _output.write(iter->_data);
        _buf_erase(iter);  // 从缓冲区中移除已写入到字节流中的片段
    }

    // 如果到达了流的末尾并且所有数据都已重新组装，则结束输入并清空缓冲区
    if (_eof && first_unassembled() == _eof_index) 
    {
        _output.end_input();
        _buf.clear();
    }
}

// 函数功能：处理子字符串，将字符串插入到集合set未组装的集合中
void StreamReassembler::_handle_substring(Segment &seg) 
{
    // 共分为3种情况：
    // 1.索引越界，直接丢弃
    // 2.部分数据越界，裁掉一部分
    // 3.都在范围内
    const size_t index = seg._idx;

    // 情况1：丢弃、
    // 如果该片段索引值已经 大于 不可接收的索引值，return
    // 片段结尾的索引值 小于 字节流当前末尾的索引值，return 
    if (index >= first_unacceptable() || seg.tail_idx() < first_unassembled())
        return;


    // 情况2：截断
    // 如果片段开始索引小于第一个未组装的字节，且片段的尾部索引大于等于第一个未组装的字节
    // 则截取片段数据，使其开始于第一个未组装的字节
    // 前边包重复所以截取
    if (index < first_unassembled() && seg.tail_idx() >= first_unassembled()) 
    {
        seg._data = seg._data.substr(first_unassembled() - index);
        seg._idx = first_unassembled();
    }

    // 如果片段的尾部索引大于第一个不可接受的字节索引，则截取片段数据，使其结束于第一个不可接受的字节之前
    if (index < first_unacceptable() && seg.tail_idx() >= first_unacceptable())
        seg._data = seg._data.substr(0, first_unacceptable() - index);


    // 情况3：将片段插入到缓冲区中，如果缓冲区为空则直接插入
    if (_buf.empty()) 
    {
        _buf_insert(seg);
        return;
    }

    // 插入后，对set集合进行处理，处理重叠部分
    _handle_overlap(seg);
}



// 函数功能：处理乱序、重叠的字符串
void StreamReassembler::_handle_overlap(Segment &seg) 
{
    // 当前处理的片段的起始索引
    const size_t seg_index = seg._idx;     

    // 当前处理的片段的结束索引
    const size_t seg_tail = seg.tail_idx(); 

    // 遍历缓冲区中的每个片段
    for (auto iter = _buf.begin(); iter != _buf.end();) 
    {
        // 缓冲区中的片段起始索引
        const size_t buf_index = iter->_idx;    

        // 缓冲区中的片段结束索引 
        const size_t buf_tail = iter->tail_idx(); 

        // 判断当前处理的片段与缓冲区中的片段是否有重叠部分
        if ((seg_index <= buf_tail && seg_index >= buf_index) || (seg_tail >= buf_index && seg_index <= buf_index)) 
        {
            // 合并重叠部分，将当前处理的片段 `seg` 与缓冲区中的片段 `*iter` 合并
            _merge_seg(seg, *iter); 

            // 从缓冲区中移除已处理的片段，并将迭代器移动到下一个位置
            _buf_erase(iter++);     
        } 
        else 
        {
            // 如果没有重叠部分，则继续检查下一个片段
            ++iter; 
        }
    }

    // 将当前处理的片段 `seg` 插入到缓冲区中
    _buf_insert(seg); 
}


// 函数功能：合并字符串
void StreamReassembler::_merge_seg(Segment &seg1, const Segment &seg2) 
{
    const size_t seg1_tail = seg1.tail_idx(); // seg1 的结束索引
    const size_t seg2_tail = seg2.tail_idx(); // seg2 的结束索引

    // 情况1：seg1 的起始索引小于 seg2 的起始索引，并且 seg1 的结束索引小于等于 seg2 的结束索引
    if (seg1._idx < seg2._idx && seg1_tail <= seg2_tail) 
    {
        // 将 seg2 的数据插入 seg1 的对应位置
        seg1._data = seg1._data.substr(0, seg2._idx - seg1._idx) + seg2._data; 
    } 

    // 情况2：seg1 的起始索引大于等于 seg2 的起始索引，并且 seg1 的结束索引大于 seg2 的结束索引
    else if (seg1._idx >= seg2._idx && seg1_tail > seg2_tail) 
    {
        // 将 seg2 的数据拼接到 seg1 的前部分
        seg1._data = seg2._data + seg1._data.substr(seg2._idx + seg2.length() - seg1._idx); 

        // 更新 seg1 的起始索引为 seg2 的起始索引
        seg1._idx = seg2._idx; 
    } 

    // 情况3：seg1 和 seg2 的起始索引相同，但 seg1 的结束索引小于等于 seg2 的结束索引
    else if (seg1._idx >= seg2._idx && seg1_tail <= seg2_tail) 
    {
        // 直接使用 seg2 的数据替换 seg1 的数据
        seg1._data = seg2._data; 

        // 更新 seg1 的起始索引为 seg2 的起始索引
        seg1._idx = seg2._idx; 
    }
}


// 函数功能：返回存储但尚未重新组装的子字符串中的总字节数
size_t StreamReassembler::unassembled_bytes() const 
{
    // 注意：如果特定索引处的字节被推送多次，则此函数的目的是仅计数一次。
    return _unassembled_bytes;
}


// 函数功能：检查内部状态是否为空（除了输出流以外）
bool StreamReassembler::empty() const 
{
    // 如果没有子字符串等待被重新组装，则返回true。
    return _buf.empty();
}

