/**
 * @file ui_bridge.h
 * @brief UI bridge - unified JavaScript interaction interface
 * @brief (VI) Cầu nối UI - thống nhất giao tiếp JavaScript
 * 
 * Provides simplified JavaScript invocation helpers, handling memory management and message posting.
 * Refactor goal: reduce ~70% of duplicated JS call code.
 *
 * (VI) Cung cấp các hàm gọi JavaScript gọn hơn, tự quản lý bộ nhớ và gửi message.
 * (VI) Mục tiêu refactor: giảm ~70% code JS gọi lặp lại.
 */

#pragma once

#include <windows.h>
#include <string>

/**
 * @class UIBridge
 * @brief UI bridge singleton
 * @brief (VI) Singleton cầu nối UI
 * 
 * Centralizes all JavaScript interactions with a type-safe interface.
 * Automatically manages memory to prevent leaks.
 *
 * (VI) Quản lý tập trung tương tác JavaScript với interface an toàn kiểu.
 * (VI) Tự quản lý bộ nhớ để tránh leak.
 */
class UIBridge {
public:
    /**
     * @brief Get the singleton instance
     * @brief (VI) Lấy instance singleton
     */
    static UIBridge& Instance();

    /**
     * @brief Initialize the bridge
     * @brief (VI) Khởi tạo bridge
     * @param hWnd Main window handle
     */
    void Initialize(HWND hWnd);

    /**
     * @brief Check whether initialized
     * @brief (VI) Kiểm tra đã khởi tạo hay chưa
     */
    bool IsInitialized() const { return m_hwnd != nullptr; }

    // ================================
    // Common UI update helpers
    // (VI) Các hàm cập nhật UI thường dùng
    // ================================

    /**
     * @brief Update helper text (hint text shown in the UI)
     * @brief (VI) Cập nhật helper text (gợi ý hiển thị trên UI)
     * @param text Text content
     */
    void UpdateHelperText(const std::wstring& text);

    /**
     * @brief Update progress display
     * @brief (VI) Cập nhật hiển thị tiến độ
     * @param current Current progress
     * @param total Total count
     * @param prefix Optional prefix text
     */
    void UpdateProgress(int current, int total, const std::wstring& prefix = L"");

    /**
     * @brief Notify that a task has completed
     * @brief (VI) Thông báo task đã hoàn thành
     * @param taskName Task name
     * @param success Whether it succeeded
     * @param message Optional extra message
     */
    void NotifyTaskComplete(const std::wstring& taskName, bool success, const std::wstring& message = L"");

    /**
     * @brief Show a dialog
     * @brief (VI) Hiển thị hộp thoại
     * @param type Dialog type (info, warning, error)
     * @param message Message content
     */
    void ShowDialog(const std::wstring& type, const std::wstring& message);

    /**
     * @brief Update packet count
     * @brief (VI) Cập nhật số lượng packet
     * @param count Packet count
     */
    void UpdatePacketCount(DWORD count);

    /**
     * @brief Update mute button state
     * @brief (VI) Cập nhật trạng thái nút mute
     * @param muted Whether muted
     */
    void UpdateMuteButtonState(bool muted);

    // ================================
    // Generic JavaScript execution
    // (VI) Thực thi JavaScript dùng chung
    // ================================

    /**
     * @brief Execute JavaScript (async, automatic memory management)
     * @brief (VI) Thực thi JavaScript (bất đồng bộ, tự quản lý bộ nhớ)
     * @param script JavaScript code
     */
    void ExecuteJS(const std::wstring& script);

    // ================================
    // Helpers
    // (VI) Hàm hỗ trợ
    // ================================

    /**
     * @brief Escape a string for JSON
     * @brief (VI) Escape chuỗi để nhúng vào JSON
     * @param input Raw input string
     * @return Escaped string
     */
    static std::wstring EscapeJsonString(const std::wstring& input);

private:
    UIBridge() = default;
    ~UIBridge() = default;
    
    // Disable copying
    // (VI) Cấm copy
    UIBridge(const UIBridge&) = delete;
    UIBridge& operator=(const UIBridge&) = delete;

    HWND m_hwnd = nullptr;  ///< Main window handle
                            ///< (VI) Handle cửa sổ chính
};

// ================================
// Convenience macros (optional)
// (VI) Macro tiện dụng (tùy chọn)
// ================================

/**
 * @brief Quickly update helper text
 * @brief (VI) Cập nhật nhanh helper text
 * Example: UI_UPDATE_TEXT(L"Executing task...")
 * (VI) Ví dụ: UI_UPDATE_TEXT(L"Đang thực thi tác vụ...")
 */
#define UI_UPDATE_TEXT(text) UIBridge::Instance().UpdateHelperText(text)

/**
 * @brief Quickly update progress
 * @brief (VI) Cập nhật nhanh tiến độ
 * Example: UI_UPDATE_PROGRESS(5, 10)
 * (VI) Ví dụ: UI_UPDATE_PROGRESS(5, 10)
 */
#define UI_UPDATE_PROGRESS(current, total) UIBridge::Instance().UpdateProgress(current, total)

/**
 * @brief Quickly notify task completion
 * @brief (VI) Thông báo nhanh task hoàn thành
 */
#define UI_TASK_COMPLETE(name, success) UIBridge::Instance().NotifyTaskComplete(name, success)
