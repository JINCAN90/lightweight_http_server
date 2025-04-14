// File: src/main.cpp
#include "EpollHandler.h"
#include "ThreadPool.h"
#include "HttpParser.h"
#include <unistd.h>  // 包含 POSIX 标准的系统调用头文件，提供了如 close、read、write 等系统调用函数
#include <fcntl.h>   // 包含文件控制操作的头文件，用于设置文件描述符的属性，如设置非阻塞模式
#include <csignal>   // 包含信号处理相关的头文件，用于处理系统信号，如 SIGINT 和 SIGTERM
#include <cerrno>    // 包含错误号相关的头文件，用于处理系统调用出错时的错误信息
#include <cstring>   // 包含 C 风格字符串处理函数的头文件，如 strlen 等
#include <memory>    // 包含内存管理相关的头文件，在当前代码中未直接使用，但可能用于更复杂的内存管理场景
#include <iostream>  // 包含标准输入输出流的头文件，用于输出日志信息

// 全局变量
// 定义一个全局变量 stop_server，用于标记服务器是否需要停止。volatile 关键字确保该变量在多线程或信号处理    场景下的可见性，sig_atomic_t 是一种原子类型，保证信号处理函数可以安全地修改该变量
volatile sig_atomic_t stop_server = 0;

// 信号处理函数
// 定义一个信号处理函数，当接收到指定信号时，将 stop_server 置为 1，并输出相应的日志信息
void handle_signal(int sig)
{
    stop_server = 1;
    std::cout << "\nReceived signal " << sig << ", shutting down...\n";
}

// 处理 HTTP 请求函数
// 定义一个处理 HTTP 请求的函数，接收客户端套接字描述符 client_fd 和解析后的 HTTP 请求 req
void handle_request(int client_fd, const HttpRequest& req)
{
    // 定义 HTTP 响应的主体内容
    const std::string body = "Hello from lightweight server!\n";
    // 构建 HTTP 响应头，包括状态行、内容类型、内容长度等
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n";

    // 如果请求中设置了 Keep-Alive，则在响应头中添加 Connection: keep-alive
    if (req.keep_alive) {
        response += "Connection: keep-alive\r\n";
    }
    // 添加空行和响应主体内容
    response += "\r\n" + body;

    // 将响应数据发送给客户端
    ssize_t bytes_sent = write(client_fd, response.c_str(), response.size());
    // 如果发送数据时出错，输出错误信息
    if (bytes_sent < 0) {
        perror("Write error");
    }

    // 如果请求中没有设置 Keep-Alive，则关闭客户端套接字
    if (!req.keep_alive) {
        close(client_fd);
    }
}

// main 函数
int main() {
    // 信号处理
    // 信号处理函数，当接收到 SIGINT（通常是用户按下 Ctrl+C）或 SIGTERM（通常是系统发送的终止信号）时，        调用 handle_signal 函数。
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // 服务器监听的端口号
    const int PORT = 8080;
    // epoll 一次最多处理的事件数量
    const int MAX_EVENTS = 1024;
    // 线程池中的线程数量
    const int THREAD_POOL_SIZE = 4;

    try {
        // 初始化线程池和epoll
        // 创建一个线程池对象，指定线程池中的线程数量
        ThreadPool pool(THREAD_POOL_SIZE);
        // 创建一个 EpollHandler 对象，指定 epoll 一次最多处理的事件数量
        EpollHandler epoll(MAX_EVENTS);

        // 创建监听socket
        // 调用 EpollHandler 对象的 create_listen_socket 方法创建监听套接字，并返回其文件描述符
        int listen_fd = epoll.create_listen_socket(PORT);
        // 如果创建监听套接字失败，抛出异常
        if (listen_fd < 0) {
            throw std::runtime_error("Failed to create listen socket");
        }

        // 设置非阻塞并添加到epoll
        // 将监听套接字设置为非阻塞模式，避免在 accept 或 read 操作时阻塞线程
        fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL) | O_NONBLOCK);
        // 将监听套接字添加到 epoll 实例中，监听读事件，并使用边缘触发模式
        epoll.add_fd(listen_fd, EPOLLIN | EPOLLET);

        // 输出服务器启动信息
        std::cout << "Server started on port " << PORT << std::endl;

        // 定义一个 epoll_event 数组，用于存储 epoll 等待到的事件
        epoll_event events[MAX_EVENTS];
        // 进入主循环，直到 stop_server 被置为 1
        while (!stop_server)
        {
            // 调用 epoll 的 wait 方法等待事件发生，返回发生事件的数量
            int num_events = epoll.wait(events);
            // 如果 epoll_wait 出错且不是被信号中断，输出错误信息并跳出循环
            if (num_events < 0 && errno != EINTR)
            {
                perror("epoll_wait error");
                break;
            }

            // 遍历所有发生的事件
            for (int i = 0; i < num_events; ++i)
            {
                const auto& event = events[i];
                // 如果发生事件的文件描述符是监听套接字，说明有新的客户端连接
                if (event.data.fd == listen_fd)
                {
                    // 循环处理所有新连接
                    while (true)
                    {
                        sockaddr_in client_addr{};
                        socklen_t addr_len = sizeof(client_addr);
                        // 接受新的客户端连接，并将客户端套接字设置为非阻塞模式
                        int client_fd = accept4(listen_fd,
                            reinterpret_cast<sockaddr*>(&client_addr),
                            &addr_len, SOCK_NONBLOCK);

                        // 如果接受连接出错，根据错误类型进行处理
                        if (client_fd < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break; // 没有更多连接
                            }
                            perror("accept error");
                            continue;
                        }

                        // 添加到epoll监控
                        // 将客户端套接字添加到 epoll 实例中，监听读事件，并使用边缘触发模式
                        epoll.add_fd(client_fd, EPOLLIN | EPOLLET);
                    }

                }
                else { // 如果发生事件的文件描述符不是监听套接字，说明有客户端数据可读
                    // 将处理客户端请求的任务提交到线程池
                    pool.enqueue([fd = event.data.fd, &epoll]
                        {
                            // 定义一个缓冲区，用于存储客户端发送的数据
                            char buffer[4096];
                            ssize_t total_read = 0;
                            bool read_error = false;

                            // 边缘触发需要读取所有数据
                            // 循环读取客户端发送的数据，直到没有更多数据或出现错误
                            while (true)
                            {
                                // 从客户端套接字读取数据
                                ssize_t len = read(fd, buffer + total_read,
                                    sizeof(buffer) - total_read);
                                // 如果读取到数据，更新总读取字节数
                                if (len > 0) {
                                    total_read += len;
                                    if (total_read >= sizeof(buffer)) break;
                                }
                                else if (len == 0) { // 如果读取到的数据长度为 0，即客户端关闭了连接
                                    break;
                                }
                                else { // 如果读取数据出错，根据错误类型进行处理
                                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                        break; // 数据读取完成
                                    }
                                    read_error = true;
                                    break;
                                }
                            }

                            // 如果读取数据出错或没有读取到数据，关闭客户端套接字
                            if (read_error || total_read <= 0)
                            {
                                close(fd);
                                return;
                            }

                            // 解析请求
                            // 定义一个 HttpRequest 对象，用于存储解析后的 HTTP 请求
                            HttpRequest req;
                            // 调用 HttpParser 类的 parse 方法解析 HTTP 请求
                            if (HttpParser::parse(buffer, total_read, req))
                            {
                                // 调用 handle_request 函数处理 HTTP 请求
                                handle_request(fd, req);

                                // 保持连接则重新注册
                                // 如果请求中设置了 Keep-Alive，将客户端套接字重新添加到 epoll 实例中，                                继续监听读事件
                                if (req.keep_alive) {
                                    epoll.add_fd(fd, EPOLLIN | EPOLLET);
                                }
                                else { // 如果请求中没有设置 Keep-Alive，关闭客户端套接字
                                    close(fd);
                                }
                            }
                            else {
                                close(fd);
                            }
                        });
                }
            }
        }

        // 清理资源
        close(listen_fd);  // 关闭监听套接字
        // 输出服务器正常关闭信息
        std::cout << "Server shutdown gracefully\n";
    }
    // 捕获并处理异常，输出错误信息并返回失败状态
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // 如果程序正常执行完毕，返回成功状态
    return 0;
}