#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Platform {

struct HttpRequest {
    std::string method;  // e.g. "GET", "POST"
    std::wstring url;
    std::vector<uint8_t> body;
    std::vector<std::wstring> headers;
    int timeoutMs = 10000;
};

struct HttpResponse {
    bool ok = false;
    int statusCode = 0;
    std::vector<uint8_t> body;
    std::wstring errorMessage;
};

class INetworkClient {
public:
    virtual ~INetworkClient() = default;
    virtual HttpResponse Send(const HttpRequest& request) = 0;
};

class IClipboard {
public:
    virtual ~IClipboard() = default;
    virtual bool SetText(const std::wstring& text) = 0;
    virtual bool GetText(std::wstring& outText) = 0;
};

class ISystemShell {
public:
    virtual ~ISystemShell() = default;
    virtual bool OpenUrl(const std::wstring& url) = 0;
};

class IUiDispatcher {
public:
    virtual ~IUiDispatcher() = default;
    virtual bool PostScript(const std::wstring& script) = 0;
};

class ITimerService {
public:
    virtual ~ITimerService() = default;
    virtual uint64_t SetTimeout(int delayMs, std::function<void()> cb) = 0;
    virtual bool Cancel(uint64_t timerId) = 0;
};

}  // namespace Platform

