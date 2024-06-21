<<<<<<< HEAD
# 基于C++11实现的TCP协议

## 目录树
```cpp
sponge
├── apps
├── build      代码构建
├── CMakeLists.txt
├── compile_commands.json -> build/compile_commands.json
├── doctests
├── etc
├── libsponge  源代码
├── README.md
├── tests
├── tun.sh
├── txrx.sh
└── writeups
```

## 项目组成和结果展示
如图所示
![image-20230313111322059](https://imgbed001.oss-cn-hangzhou.aliyuncs.com/img/image-20230313111322059.png)

本项目一共分为5部分：
- 字节流（byteStream）：对传输的数据进行读写操作、并且支持流量控制；
- 流重组器（StreamReassembler）：对传输过程中TCP段可能会出现乱序、丢失、重复、交叉重叠等错误状态进行处理，使其还原为原来的正确顺序；
- TCP接收器（TCPReceiver）：将接收到的数据送入流重组器中进行处理之后再传入字节流中，同时给发送方返回确认号（ack）和窗口大小（window size）实现对流量的控制；
- TCP发送器（TCPSender）：实现了重传定时器用于TCP的超时重传，同时对传输信息进行加工合成TCP段用于发送；
- TCP连接（TCPConnection）：将TCP接收器和发送器封装在一起从而实现完整的收发数据功能。

基于以上5部分，最终可以实现一个TCP协议。

压力测试如图所示：
```cpp
$ ./apps/tcp_benchmark 
CPU-limited throughput                : 1.00 Gbit/s
CPU-limited throughput with reordering: 0.94 Gbit/s
```
---
2023.03.22
复习时突然想到字节流中的`queue<char> _buff`可以换成`queue<string> _buff`

这样在读写的时候可以从`O(n)->O(1)`，在计算容量的时候由`O(1)->O(n)`但数据传输时读写的操作次数远远大于查询的次数，应该可以得到一个不错的优化。

后续优化等完成后上传。
=======
# MyTCP

#### 介绍
CS144计网大作业实现手写TCP

#### 软件架构
软件架构说明


#### 安装教程

1.  xxxx
2.  xxxx
3.  xxxx

#### 使用说明

1.  xxxx
2.  xxxx
3.  xxxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
>>>>>>> 54ede8c350d72cc24c463c2da5f653aa54b019e7
