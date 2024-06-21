#ifndef SPONGE_LIBSPONGE_BYTE_STREAM_HH
#define SPONGE_LIBSPONGE_BYTE_STREAM_HH
// 预处理器指令，通常用于防止头文件的多重包含

#include <deque>
#include <string>


//! \brief An in-order byte stream.
// \brief 一个按顺序的字节流。

//! Bytes are written on the "input" side and read from the "output"
//! side.  The byte stream is finite: the writer can end the input,
//! and then no more bytes can be written.

//！字节被写入“输入”端并从“输出”端读取。
//！字节流是有限的：写入者可以结束输入，之后就不能再写入更多字节了。

class ByteStream {
  private:
    // Your code here -- add private members as necessary.

    // Hint: This doesn't need to be a sophisticated data structure at
    // all, but if any of your tests are taking longer than a second,
    // that's a sign that you probably want to keep exploring
    // different approaches.

    // 在这里添加你的代码——根据需要添加私有成员。
    // 提示：这并不需要一个复杂的数据结构，
    // 但如果你的任何测试耗时超过一秒，
    // 这表明你可能希望继续探索不同的方法。



    std::deque<char> _buff; // 使用标准库中的双端队列（deque），存储字符类型的数据，用作字节流的缓冲区
    size_t capacity; // 保存缓冲区的容量，即可以存储的最大字节数量
    size_t bytes_r; // 记录已经读取的字节数量
    size_t bytes_w; // 记录已经写入的字节数量。
    bool _end_input; // 表示输入端是否已经结束的标志。如果为真，则表示不再接受新的输入字节
    bool _error{};  // 标志表明流发生了错误。
    /*
    表示流是否发生错误的标志。默认初始化为假（false），表示没有发生错误。
    这些成员变量一起组成了一个简单的字节流管理结构
    用于存储和跟踪字节流的状态、容量、已读取和已写入的字节数量，
    以及流的结束状态和错误状态。
    */

  public:

    // 构建一个容量为 `capacity` 字节的流。
    ByteStream(const size_t capacity);





    //! \name 用于写入的“输入”接口
    //! 将一串字节写入流中。尽可能多地写入，然后返回实际写入的字节数。
    //! \returns 接受到流中的字节数

    // 写入字节流函数，返回成功写入的字节数
    size_t write(const std::string &data);


    // 流中还有空间可容纳的额外字节数
    // 返回剩余可写入的字节数
    size_t remaining_capacity() const;


    // 发出信号，表明字节流已经结束
    void end_input();


    // 指示流发生了错误。
    void set_error() { _error = true; }





    // \name "输出"接口，用于读取
    // 查看流中接下来的 "len" 字节

    // \returns a string
    // 字节将从缓冲区的输出端复制
    std::string peek_output(const size_t len) const;


    // 从缓冲区中移除字节
    void pop_output(const size_t len);


    // 读取（即复制然后弹出）流的下一个 "len" 字节
    //! \returns a string
    std::string read(const size_t len);


    // 如果流的输入已经结束则返回 true
    bool input_ended() const;


    // 如果流发生了错误则返回 true
    bool error() const { return _error; }


    // 当前可以从流中读取的最大量
    size_t buffer_size() const;


    // 判断缓冲区是否为空，空则返回 true
    bool buffer_empty() const;


    // 如果输出已经达到结尾则返回 true
    bool eof() const;




    //! \name 会计核算

    // 总共写入的字节数
    size_t bytes_written() const;

    // 总共弹出的字节数
    size_t bytes_read() const;
};

#endif  // SPONGE_LIBSPONGE_BYTE_STREAM_HH
