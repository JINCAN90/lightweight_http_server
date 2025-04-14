#pragma once
// File: include/EpollHandler.h
#ifndef EPOLL_HANDLER_H
#define EPOLL_HANDLER_H

#include <sys/epoll.h>  //该头文件提供了 Linux 系统中 epoll 机制的相关函数和数据结构的声                                       明，例如 epoll_create1、epoll_ctl 和 epoll_wait 等函数，以                                       及 epoll_event 结构体

//定义了一个名为 EpollHandler 的类，用于封装 epoll 相关的操作，方便使用和管理
class EpollHandler
{
public:

    // 这是 EpollHandler 类的构造函数，
    // 接收一个整数参数，max_events，用于指定 epoll 一次最多可以处理的事件数量。
    // 默认值为 1024。构造函数会在创建 EpollHandler 对象时被调用，进行一些初始化操作。
    EpollHandler(int max_events = 1024);

    // 这是 EpollHandler 类的析构函数，
    // 在 EpollHandler 对象被销毁时自动调用，用于释放对象占用的资源，例如关闭 epoll 实例描述符等  
    ~EpollHandler();

    // 创建监听socket，即声明了一个公共成员函数 create_listen_socket，
    // 该函数接收一个整数参数 port，表示要监听的端口号。
    // 函数的返回值是一个整数，代表创建的监听套接字的文件描述符。                                         // 该函数的作用是创建一个监听指定端口的套接字。
    int create_listen_socket(int port);

    // 添加文件描述符到epoll，即声明了一个公共成员函数
    // 该函数接收两个参数：
    // 一个整数 fd 表示要添加到 epoll 实例中的文件描述符，
    // 一个 uint32_t 类型的 events 表示要监听的事件类型，例如 EPOLLIN（可读事件）、EPOLLOUT（可写事          件）等。该函数的作用是将指定的文件描述符添加到 epoll 实例中，并设置要监听的事件。
    void add_fd(int fd, uint32_t events);

    // 声明了一个公共成员函数 wait，
    // 该函数接收一个struct epoll_event 类型的指针 events，用于存储发生的事件信息。                     // 函数的返回值是一个整数，表示发生的事件数量。该函数的作用是阻塞当前线程，直到有事件发生，并将发生的事        件信息存储在events 数组中。
    int wait(struct epoll_event* events);


private:

    // epoll实例描述符，即声明了一个私有成员变量 epoll_fd_，它是一个整数类型，用于存储 epoll 实例的文件         描述符。这个描述符在创建 epoll 实例时获得，后续的 epoll操作（如添加文件描述符、等待事件等）都需要         使用这个描述符。
    int epoll_fd_;

    // 最大事件数，用于存储 epoll 一次最多可以处理的事件数量。这个值在构造函数中被初始化，可以通过构造函数        的参数进行设置。
    int max_events_;
};

#endif