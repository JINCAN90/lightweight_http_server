#pragma once
// File: include/HttpParser.h
#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>             // 用于处理字符串
#include <unordered_map>      // 用于存储键值对，这里将用于存储 HTTP 请求的头部信息

// 定义 HttpRequest 的结构体，用于表示一个 HTTP 请求
struct HttpRequest {
    std::string method;  // 存储 HTTP 请求的方法，例如 GET、POST 等
    std::string path;    // 存储 HTTP 请求的路径，例如 /index.html
    std::string version; // 存储 HTTP 请求的版本，例如 HTTP/1.1
    std::unordered_map<std::string, std::string> headers; //存储 HTTP 请求的头部信息，键为头部字                                                             段名，值为头部字段值
    std::string body;  // 存储 HTTP 请求的主体内容
    bool keep_alive = false;  // 表示是否保持连接，默认值为 false
};

// 定义了一个名为 HttpParser 的类，用于解析 HTTP 请求
class HttpParser
{
public:
    // 声明了一个静态成员函数 parse，该函数用于解析 HTTP 请求
    // const char* data：传入的 HTTP 请求数据的指针
    // size_t len：传入的 HTTP 请求数据的长度
    // HttpRequest& req：引用类型的参数，用于存储解析后的 HTTP 请求信息
    // bool 表示函数的返回值类型，返回 true 表示解析成功，返回 false 表示解析失败
    static bool parse(const char* data, size_t len, HttpRequest& req);
};

#endif

