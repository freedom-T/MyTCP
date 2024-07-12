#include "byte_stream.hh"



// 这是一个虚拟实现的内存中流控制的字节流。

// 在实验0中，请用一个能通过`make check_lab0`运行自动检查的真实实现替换这部分。

// 你需要在 `byte_stream.hh` 的类声明中添加私有成员。

// 这是一个模板函数的声明，typename... Targs 表示模板可以接受任意数量的模板参数，这些参数被统称为 Targs
template <typename... Targs>
// 这是模板函数 DUMMY_CODE 的定义部分。Targs &&... 表示参数包展开，表示可以接受零个或多个右值引用类型的参数
// /* unused */ 表示这些参数在函数体内部没有被使用
void DUMMY_CODE(Targs &&... /* unused */) {}
/*
这是模板函数 DUMMY_CODE 的定义部分
Targs &&... 表示参数包展开,表示可以接受零个或多个右值引用类型的参数
 unused 表示这些参数在函数体内部没有被使用
*/

// 通过 DUMMY_CODE 函数进行占位  这个的目的和Python中的函数实现是pass一样
// ByteStream::ByteStream(const size_t capacity) { DUMMY_CODE(capacity); }


using namespace std;



// 构造函数进行初始化双端队列及成员变量
ByteStream::ByteStream(const size_t _capacity)
    : _buff(),           // 初始化 _buff，使用默认构造函数创建一个空的双端队列
      capacity(_capacity), // 初始化 capacity，用传入的 _capacity 参数
      bytes_r(0),         // 初始化 bytes_r，将其设置为0
      bytes_w(0),         // 初始化 bytes_w，将其设置为0
      _end_input(false),  // 初始化 _end_input，将其设置为false
      _error(false)       // 初始化 _error，将其设置为false
{
    // 构造函数的主体为空，因为成员变量已经在初始化列表中初始化完成
}



// 函数功能：用于向字节流中写入字符串 data
size_t ByteStream::write(const string &data) 
{
    // 如果输入端已经结束，则直接返回0，表示没有写入任何字节
    if (input_ended())
        return 0;

    // 计算实际可以写入的字节数，即 data 的大小和剩余容量的较小值
    size_t write_size = min(data.size(), remaining_capacity());

    // 更新已写入的字节数，用于后续的数据流量统计和管理
    bytes_w += write_size;

    // 将 data 中的字节逐个写入 _buff 中
    for (size_t i = 0; i < write_size; i++)
        _buff.push_back(data[i]);

    // 返回实际写入的字节数
    return write_size;
}



// \param[in] len 字节将从缓冲区的输出端复制
// read 函数中进行调用
string ByteStream::peek_output(const size_t len) const 
{
    // 计算实际可以查看的字节数，即 len 和当前缓冲区中的字节数量的较小值
    size_t peek_size = min(len, _buff.size());

    // 使用 string 类的构造函数，从 _buff 的开头复制 peek_size 个字节到新建的字符串中
    return string(_buff.begin(), _buff.begin() + peek_size);
}




//! \param[in] len 字节将从缓冲区的输出端移除
// read 函数中进行调用
void ByteStream::pop_output(const size_t len) 
{
    // 计算实际可以弹出的字节数，即 len 和当前缓冲区中的字节数量的较小值
    size_t pop_size = min(len, _buff.size());

    // 更新已读取的字节数
    bytes_r += pop_size;

    // 循环弹出指定数量的字节，即从 _buff 的前端依次弹出 pop_size 次
    for (size_t i = 0; i < pop_size; i++)
        _buff.pop_front();
}




//! 读取（即复制然后弹出）流的接下来 "len" 字节
//! \param[in] len 将被弹出并返回的字节数
//! \returns 一个字符串

// 函数功能：读取流中的数据，复制然后弹出
std::string ByteStream::read(const size_t len) {
    // 声明一个字符串变量 r
    std::string r;
    
    // 调用 peek_output(len) 函数，将输出端的前 len 字节复制到 r 中
    r = this->peek_output(len);

    // 调用 pop_output(len) 函数，从输出端移除前 len 字节的数据
    this->pop_output(len);

    // 返回复制到 r 的字节数据
    return r;
}







// 函数功能：标记输入端已经结束
void ByteStream::end_input() 
{
    _end_input = true; // 将 _end_input 标志设置为 true，表示输入端已经结束
}


// 函数功能：用于检查输入端是否已经结束
bool ByteStream::input_ended() const 
{
    return _end_input; // 返回 _end_input 标志，表示输入端是否已经结束
}


// 函数功能：用于返回当前字节流缓冲区的大小
size_t ByteStream::buffer_size() const 
{
    return _buff.size(); // 返回 _buff 的大小，即当前缓冲区中的字节数量
}


// 函数功能：于检查当前字节流缓冲区是否为空
bool ByteStream::buffer_empty() const 
{
    return _buff.empty(); // 返回 _buff 是否为空的结果，即当前缓冲区是否没有字节
}


// 函数功能：用于检查是否已经到达字节流的结束位置
bool ByteStream::eof() const 
{
    // 返回输入端已经结束且缓冲区为空的逻辑与结果
    return input_ended() && buffer_empty();
}


// 函数功能：用于获取当前字节流已写入的字节数
size_t ByteStream::bytes_written() const 
{
    return bytes_w; // 返回成员变量 bytes_w，即已写入的字节数量
}


// 函数功能：用于获取当前字节流已读取的字节数
size_t ByteStream::bytes_read() const 
{
    return bytes_r; // 返回成员变量 bytes_r，即已读取的字节数量
}


// 函数功能：用于计算当前字节流缓冲区的剩余容量
size_t ByteStream::remaining_capacity() const 
{
    // 返回当前字节流的容量减去缓冲区当前的大小，即剩余的可写入容量
    return capacity - _buff.size();
}
