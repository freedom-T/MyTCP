#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

// 这个头文件提供了一组精确宽度整数类型的 typedefs，以及一些指定整数类型限制的宏
// 如int32_t
#include <cstdint>
#include <set>
#include <string>


// 一个类，将来自字节流的一系列片段（可能是无序的，可能是重叠的）组装成一个有序的字节流。
// 流的重组器
class StreamReassembler {
  private:

  // 类中有一个结构体，表示一个数据片段
    struct Segment 
    {
        size_t _idx;        // 片段的起始索引
        std::string _data;  // 片段包含的数据

        // 构造函数
        // 默认构造函数将 _idx 初始化为 0，将 _data 初始化为空字符串 ("")
        Segment() : _idx(0), _data() {}

        // 参数化构造函数根据给定的 index 和 data 初始化 _idx 和 _data
        Segment(size_t index, const std::string &data) : _idx(index), _data(data) {}

        // 方法
        // 返回片段数据的长度
        size_t length() const { return _data.length(); }   

        // 返回片段末尾字节的索引
        size_t tail_idx() const { return _idx + length() - 1; }  

        // 运算符重载，用于基于起始索引(_idx)比较两个片段
        bool operator<(const Segment &seg) const { return this->_idx < seg._idx; }
    };


  private:

    // 重新组装的按顺序字节流，组装好的字节流会放入这里
    ByteStream _output;  

    // 最大字节数
    size_t _capacity;   

    // 终止字节的索引
    size_t _eof_index;   

    // 未组装的字节数        
    size_t _unassembled_bytes; 

    // 终止标志  
    bool _eof;        

    // 使用集合存储片段数据，缓冲区,为组装的数据片段的集合        
    std::set<Segment> _buf;      


    // 除去已经写入流中的缓冲区的字符串
    void _buf_erase(const std::set<Segment>::iterator &iter);


    // 负责根据子字符串的索引和长度将其与当前已经接收的片段进行比较和处理
    void _handle_substring(Segment &seg);

    // 处理乱序、重叠的字符串
    void _handle_overlap(Segment &seg);


    // 向缓冲区插入字符串
    void _buf_insert(const Segment &seg);


    // 合并字符串
    void _merge_seg(Segment &seg1, const Segment &seg2);


  public:
    // 构造一个 `StreamReassembler`，其最大存储容量为 `capacity` 字节。
    // 这个容量限制了已经重新组装的字节数
    // 以及尚未重新组装的字节数
    StreamReassembler(const size_t capacity);

    // 接收一个子字符串并将任何新的连续字节写入流中。
    //
    // 将保持在 `capacity` 的内存限制内。
    // 超过容量的字节将被静默丢弃。
    //
    // 子字符串数据
    // 表示 `data` 中第一个字节的索引（序列中的位置）
    // 表示 `data` 的最后一个字节将是整个流的最后一个字节


    // 将子字符串 data 推入流中，将其传递给流重新组装器
    void push_substring(const std::string &data, const uint64_t index, const bool eof);



    // 访问重新组装的字节流
 
    // 分别是用于访问 _output 的常量版本和非常量版本
    const ByteStream &stream_out() const { return _output; }
    
    ByteStream &stream_out() { return _output; }



    // 存储但尚未重新组装的子字符串中的字节数量
    //
    // 如果特定索引处的字节已经被推送多次，则在此函数中，仅应计数一次。

    // 返回存储但尚未重新组装的子字符串中的字节数量
    size_t unassembled_bytes() const;




    // 内部状态是否为空（除了输出流）？
    // 如果没有等待组装的子字符串，则返回 `true`
    bool empty() const;

    // 获取未读取的第一个字节索引，字节流的开始索引
    size_t first_unread() const { return _output.bytes_read(); }

    // 获取未重新组装的第一个字节索引，期望下一个收到的索引值
    size_t first_unassembled() const { return _output.bytes_written(); }

    // 获取第一个不可接受的字节索引
    size_t first_unacceptable() const { return first_unread() + _capacity; }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
