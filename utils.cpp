#include "utils.h"
#include <windows.h>
#include <string>
#include <vector>

// 将UTF-8字符串转换为宽字符字符串
std::wstring Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    
    // 安全检查：防止过大的输入
    if (utf8.size() > (10 * 1024 * 1024)) { // 最大10MB
        return std::wstring();
    }
    
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (wideSize <= 0) {
        return std::wstring();
    }
    
    // 安全检查：防止整数溢出
    if (wideSize > 1024 * 1024) { // 最大1MB宽字符
        return std::wstring();
    }

    std::vector<wchar_t> wideBuffer(wideSize);
    if (MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wideBuffer.data(), wideSize) <= 0) {
        return std::wstring();
    }
    return std::wstring(wideBuffer.data());
}

// 将宽字符字符串转换为UTF-8字符串
std::string WideToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return std::string();
    
    // 安全检查：防止过大的输入
    if (wide.size() > (5 * 1024 * 1024)) { // 最大5MB宽字符
        return std::string();
    }
    
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8Size <= 0) {
        return std::string();
    }
    
    // 安全检查：防止整数溢出
    if (utf8Size > 4 * 1024 * 1024) { // 最大4MB UTF-8
        return std::string();
    }

    std::vector<char> utf8Buffer(utf8Size);
    if (WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8Buffer.data(), utf8Size, NULL, NULL) <= 0) {
        return std::string();
    }
    return std::string(utf8Buffer.data());
}

std::wstring MultiToWide(const std::string& bytes, unsigned int codepage)
{
    if (bytes.empty()) return std::wstring();
    
    // 安全检查：防止过大的输入
    if (bytes.size() > (10 * 1024 * 1024)) { // 最大10MB
        return std::wstring();
    }
    
    int wideSize = MultiByteToWideChar(codepage, 0, bytes.c_str(), -1, NULL, 0);
    if (wideSize <= 0) {
        return std::wstring();
    }
    
    // 安全检查：防止整数溢出
    if (wideSize > 1024 * 1024) { // 最大1MB宽字符
        return std::wstring();
    }

    std::vector<wchar_t> wideBuffer(wideSize);
    if (MultiByteToWideChar(codepage, 0, bytes.c_str(), -1, wideBuffer.data(), wideSize) <= 0) {
        return std::wstring();
    }
    return std::wstring(wideBuffer.data());
}
