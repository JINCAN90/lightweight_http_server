// File: src/EpollHandler.cpp
#include "EpollHandler.h"
#include <unistd.h> //包含了许多 Unix 标准库的函数声明，如 close 函数，用于关闭文件描述符
#include <fcntl.h> //提供了文件控制相关的函数和常量，不过在当前代码里未直接使用这些内容
#include <cstring> //包含了 C 语言字符串处理函数的声明，例如 memset 函数，用于内存区域的初始化

// 这是 EpollHandler 类的构造函数，接收一个整数参数 max_events，表示 epoll 一次最多能处理的事件数量
//: max_events_(max_events)：使用成员初始化列表将私有成员变量 max_events_ 初始化为传入的 max_events    值。
// 创建epoll实例（挂根结点），即调用 epoll_create1 函数创建一个 epoll 实例，并将返回的文件描述符赋值给私    有成员变量 epoll_fd_。参数 0 表示使用默认选项。
EpollHandler::EpollHandler(int max_events) : max_events_(max_events)
{
    epoll_fd_ = epoll_create1(0);  // 该函数在 <sys/epoll.h> 头文件中声明
    // 与 epoll_create 的区别
            /// 函数	             参数	            功能特性
           // epoll_create	    大小参数	    已废弃，仅保留向后兼容性
           // epoll_create1	    标志参数	 支持设置选项（如 EPOLL_CLOEXEC）

}

// 这是 EpollHandler 类的析构函数，在对象销毁时会被调用
EpollHandler::~EpollHandler()
{
    close(epoll_fd_);  // 即调用 close 函数关闭 epoll 实例的文件描述符 epoll_fd_，释放相关资源
}

//这是 EpollHandler 类的成员函数，用于创建一个监听指定端口的套接字
int EpollHandler::create_listen_socket(int port)
{
    // 调用 socket 函数创建一个 TCP 套接字。AF_INET 表示使用 IPv4 地址族，SOCK_STREAM 表示使用面向连        接的 TCP 协议，0 表示使用默认协议。返回的套接字文件描述符存储在 listen_fd 中
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 设置端口复用（#）
    int opt = 1; //定义一个整数变量 opt 并初始化为 1，用于表示启用端口复用选项
    // 调用 setsockopt 函数设置套接字选项。
    // SOL_SOCKET 表示设置的是套接字级别的选项，SO_REUSEADDR 表示允许地址复用，&opt 是选项值的指针，          sizeof(opt) 是选项值的大小
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定端口
    // 定义一个 sockaddr_in 结构体变量 addr，用于存储套接字地址信息
    struct sockaddr_in addr;
    // 使用 memset 函数将 addr 结构体的内存区域初始化为 0
    memset(&addr, 0, sizeof(addr));
    // 设置地址族为 IPv4
    addr.sin_family = AF_INET;
    // 将端口号 port 转换为网络字节序，并存储在 addr.sin_port 中
    addr.sin_port = htons(port);
    // 设置 IP 地址为任意地址，表示监听所有可用的网络接口
    addr.sin_addr.s_addr = INADDR_ANY;
    // 调用 bind 函数将套接字 listen_fd 绑定到指定的地址和端口
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));

    // 开始监听，即调用 listen 函数将套接字 listen_fd 设置为监听状态，允许最多 1024 个连接请求排队等待
    listen(listen_fd, 1024);
    return listen_fd;
}

// 这是 EpollHandler 类的成员函数，用于将指定的文件描述符 fd 添加到 epoll 实例中，并设置要监听的事件类型   events。
void EpollHandler::add_fd(int fd, uint32_t events)
{
    // 定义一个 epoll_event 结构体变量 ev，用于存储要注册的事件信息
    struct epoll_event ev;
    // 将 ev.events 设置为传入的事件类型 events
    ev.events = events;
    // 将 ev.data.fd 设置为要注册的文件描述符 fd，方便在事件发生时获取对应的文件描述符
    ev.data.fd = fd;
    // 注册事件，即调用 epoll_ctl 函数将文件描述符 fd 添加到 epoll 实例 epoll_fd_ 中，并注册指定的事        件。EPOLL_CTL_ADD 表示添加操作
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
}

// 这是 EpollHandler 类的成员函数，用于等待 epoll 实例中的事件发生。
int EpollHandler::wait(struct epoll_event* events)
{
    // 调用 epoll_wait 函数阻塞当前线程，直到有事件发生。epoll_fd_ 是 epoll 实例的文件描述符，events        是用于存储发生事件信息的数组，max_events_ 是一次最多能处理的事件数量，-1 表示无限期阻塞，直到有事        件发生。函数返回发生的事件数量
    return epoll_wait(epoll_fd_, events, max_events_, -1);  // 阻塞等待，
}

//综上所述，这段代码实现了 EpollHandler 类的各个成员函数，包括创建 epoll 实例、关闭 epoll 实例、创建监听套接字、添加文件描述符到 epoll 实例以及等待事件发生等功能。