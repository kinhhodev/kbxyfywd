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

// ============================================================================
// 远程DLL下载功能实现
// ============================================================================

#include <winhttp.h>
#include <vector>

#pragma comment(lib, "winhttp.lib")

std::vector<uint8_t> DownloadBinaryFromUrl(const std::wstring& url, DWORD timeoutMs) {
    std::vector<uint8_t> result;
    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;

    hSession = WinHttpOpen(
        L"KBWebUI/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession) {
        return result;
    }

    WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(URL_COMPONENTS);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        WinHttpCloseHandle(hSession);
        return result;
    }

    std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
    std::wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

    hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
        DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                             SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                             SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, nullptr)) {
        result.clear();
        goto cleanup;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (WinHttpQueryHeaders(hRequest,
                           WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX,
                           &statusCode,
                           &statusCodeSize,
                           WINHTTP_NO_HEADER_INDEX) && statusCode != 200) {
        result.clear();
        goto cleanup;
    }

    DWORD contentLength = 0;
    DWORD contentLengthSize = sizeof(contentLength);
    if (WinHttpQueryHeaders(hRequest,
                           WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX,
                           &contentLength,
                           &contentLengthSize,
                           WINHTTP_NO_HEADER_INDEX)) {
        if (contentLength > 0 && contentLength < 50 * 1024 * 1024) {
            result.reserve(contentLength);
        }
    }

    DWORD bytesRead = 0;
    BYTE buffer[8192];
    DWORD totalSize = 0;
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        result.insert(result.end(), buffer, buffer + bytesRead);
        totalSize += bytesRead;

        if (totalSize > 50 * 1024 * 1024) {
            result.clear();
            break;
        }
    }

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return result;
}
