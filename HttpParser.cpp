// File: src/HttpParser.cpp
#include "HttpParser.h"
#include <algorithm>  // 包含标准库中的 <algorithm> 头文件，不过在当前代码中未实际使用该头文件中的功能
#include <cstring>    // 包含标准 C 库的字符串处理头文件，代码中使用了 memchr 函数，该函数在这个头文件                          中声明

// 这是 HttpParser 类的静态成员函数 parse 的实现，用于解析 HTTP 请求
// const char* data：指向存储 HTTP 请求数据的字符数组
// size_t len：HTTP 请求数据的长度
// HttpRequest& req：引用类型的参数，用于存储解析后的 HTTP 请求信息
bool HttpParser::parse(const char* data, size_t len, HttpRequest& req) {
    // 计算数据的结束位置
    const char* end = data + len;
    // 初始化一个指针 ptr，用于遍历数据
    const char* ptr = data;

    // 解析请求行
    // 使用 memchr 函数在 ptr 到 end 之间查找第一个回车符 \r，如果找到则返回其指针，否则返回 nullptr
    const char* line_end = static_cast<const char*>(memchr(ptr, '\r', end - ptr));
    // 检查是否找到回车符以及回车符后面是否紧跟着换行符 \n，如果未找到则表示请求行格式错误，函数返回 false
    if (!line_end || line_end[1] != '\n') return false;

    // 解析方法
    // 在请求行中查找第一个空格，其位置即为方法的结束位置
    const char* method_end = static_cast<const char*>(memchr(ptr, ' ', line_end - ptr));
    // 如果未找到空格，则表示请求行格式错误，函数返回 false
    if (!method_end) return false;
    // 将从 ptr 到 method_end 的字符赋值给 req.method，即存储 HTTP 请求的方法
    req.method.assign(ptr, method_end - ptr);

    // 解析路径
    // 将指针 ptr 移动到路径的起始位置
    ptr = method_end + 1;
    // 在路径部分查找第一个空格，其位置即为路径的结束位置
    const char* path_end = static_cast<const char*>(memchr(ptr, ' ', line_end - ptr));
    // 如果未找到空格，则表示请求行格式错误，函数返回 false
    if (!path_end) return false;
    // 将从 ptr 到 path_end 的字符赋值给 req.path，即存储 HTTP 请求的路径
    req.path.assign(ptr, path_end - ptr);

    // 解析HTTP版本
    // 将指针 ptr 移动到 HTTP 版本的起始位置
    ptr = path_end + 1;
    // 将 version_end 指向请求行的结束位置
    const char* version_end = line_end;
    // 将从 ptr 到 version_end 的字符赋值给 req.version，即存储 HTTP 请求的版本
    req.version.assign(ptr, version_end - ptr);

    ptr = line_end + 2; // 将指针 ptr 移动到下一行的起始位置，跳过回车换行符 \r\n

    // 解析请求头
    // 循环遍历请求头部分，直到数据结束
    while (ptr < end)
    {
        // 查找当前行的回车符
        line_end = static_cast<const char*>(memchr(ptr, '\r', end - ptr));
        // 如果未找到回车符或回车符后面不是换行符，则跳出循环
        if (!line_end || line_end[1] != '\n') break;
        // 如果当前行是空行，说明请求头结束，跳过回车换行符并跳出循环
        if (ptr == line_end) {
            ptr += 2;
            break;
        }

        // 查找当前行中的冒号，冒号前面是头部字段名，后面是头部字段值
        const char* colon = static_cast<const char*>(memchr(ptr, ':', line_end - ptr));
        if (colon) {
            // 提取头部字段名
            std::string key(ptr, colon - ptr);

            // 将指针移动到头部字段值的起始位置
            const char* value_start = colon + 1;
            // 跳过头部字段值前面的空格或制表符
            while (value_start < line_end && (*value_start == ' ' || *value_start == '\t'))
                ++value_start;

            // 提取头部字段值
            std::string value(value_start, line_end - value_start);
            // 将头部字段名和值以键值对的形式插入到 req.headers 中
            req.headers.emplace(std::move(key), std::move(value));
        }
        // 将指针移动到下一行的起始位置
        ptr = line_end + 2;
    }

    // 解析Keep-Alive
    // 默认情况下，如果 HTTP 版本大于等于 HTTP/1.1，则认为 Keep-Alive 为 true
    req.keep_alive = (req.version >= "HTTP/1.1");
    // 查找请求头中是否包含 Connection 字段
    if (auto it = req.headers.find("Connection"); it != req.headers.end())
    {
        // 如果 Connection 字段的值为 close，则将 Keep-Alive 设置为 false
        if (it->second == "close") req.keep_alive = false;
        // 如果 Connection 字段的值为 keep-alive，则将 Keep-Alive 设置为 true
        else if (it->second == "keep-alive") req.keep_alive = true;
    }

    // 解析请求体
    // 将从 ptr 到 end 的字符赋值给 req.body，即存储 HTTP 请求的主体内容
    req.body.assign(ptr, end - ptr);
    // 表示解析成功，函数返回 true
    return true;
}
