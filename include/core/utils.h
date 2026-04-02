/**
 * @file utils.h
 * @brief Utility functions and common components
 * @brief (VI) Hàm tiện ích và các thành phần dùng chung
 * 
 * Provides common functionality such as encoding conversion and thread synchronization.
 * (VI) Cung cấp chức năng dùng chung như chuyển đổi encoding và đồng bộ luồng.
 */

#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <cstdint>

// ============================================================================
// Encoding conversion
// (VI) Chuyển đổi encoding
// ============================================================================

/**
 * @brief Convert a UTF-8 string to a wide string
 * @brief (VI) Chuyển chuỗi UTF-8 sang chuỗi wide
 * @param utf8 UTF-8 encoded string
 * @return Wide string
 */
std::wstring Utf8ToWide(const std::string& utf8);

/**
 * @brief Convert a wide string to a UTF-8 string
 * @brief (VI) Chuyển chuỗi wide sang chuỗi UTF-8
 * @param wide Wide string
 * @return UTF-8 encoded string
 */
std::string WideToUtf8(const std::wstring& wide);

/**
 * @brief Convert bytes in a given code page to a wide string
 * @brief (VI) Chuyển bytes theo code page sang chuỗi wide
 * @param bytes Byte sequence
 * @param codepage Code page (e.g. 936 for GBK)
 * @return Wide string
 */
std::wstring MultiToWide(const std::string& bytes, unsigned int codepage);

// ============================================================================
// RAII critical section lock
// (VI) Khóa critical section theo RAII
// ============================================================================

/**
 * @class CriticalSectionGuard
 * @brief RAII-style critical section auto-lock
 * @brief (VI) Tự động lock/unlock critical section theo RAII
 * 
 * Enters the critical section on construction and leaves on destruction.
 * Ensures exception safety and helps avoid deadlocks.
 *
 * (VI) Vào critical section khi khởi tạo và thoát khi hủy.
 * (VI) Đảm bảo an toàn khi có lỗi và giúp tránh deadlock.
 * 
 * @example
 * @code
 * CriticalSectionLock lock(cs);
 * // Critical section code...
 * // Auto-unlocks when leaving scope
 *
 * // (VI) Code trong critical section...
 * // (VI) Tự unlock khi ra khỏi scope
 * @endcode
 */
class CriticalSectionLock {
public:
    /**
     * @brief Constructor; automatically enters the critical section
     * @brief (VI) Hàm tạo; tự động vào critical section
     * @param cs Reference to the critical section object
     */
    explicit CriticalSectionLock(CRITICAL_SECTION& cs) 
        : m_cs(cs) 
        , m_locked(true) {
        EnterCriticalSection(&m_cs);
    }

    /**
     * @brief Destructor; automatically leaves the critical section
     * @brief (VI) Hàm hủy; tự động rời critical section
     */
    ~CriticalSectionLock() {
        if (m_locked) {
            LeaveCriticalSection(&m_cs);
        }
    }

    // Disable copying
    // (VI) Cấm copy
    CriticalSectionLock(const CriticalSectionLock&) = delete;
    CriticalSectionLock& operator=(const CriticalSectionLock&) = delete;

    /**
     * @brief Manually unlock early
     * @brief (VI) Unlock thủ công trước thời điểm hủy
     */
    void Unlock() {
        if (m_locked) {
            LeaveCriticalSection(&m_cs);
            m_locked = false;
        }
    }

    /**
     * @brief Manually re-lock
     * @brief (VI) Lock lại thủ công
     */
    void Lock() {
        if (!m_locked) {
            EnterCriticalSection(&m_cs);
            m_locked = true;
        }
    }

private:
    CRITICAL_SECTION& m_cs;  ///< Critical section reference
                             ///< (VI) Tham chiếu critical section
    bool m_locked;           ///< Whether currently locked
                             ///< (VI) Trạng thái đang lock hay không
};

// ============================================================================
// RAII critical section lifetime management
// (VI) Quản lý vòng đời critical section theo RAII
// ============================================================================

/**
 * @class CriticalSectionScope
 * @brief Critical section lifetime management class
 * @brief (VI) Lớp quản lý vòng đời critical section
 * 
 * Initializes the critical section on construction and deletes it on destruction.
 * Used together with CriticalSectionLock.
 *
 * (VI) Khởi tạo critical section khi tạo object và hủy khi object bị hủy.
 * (VI) Dùng kèm với CriticalSectionLock.
 */
class CriticalSectionScope {
public:
    CriticalSectionScope() {
        InitializeCriticalSection(&m_cs);
    }

    ~CriticalSectionScope() {
        DeleteCriticalSection(&m_cs);
    }

    // Disable copying
    // (VI) Cấm copy
    CriticalSectionScope(const CriticalSectionScope&) = delete;
    CriticalSectionScope& operator=(const CriticalSectionScope&) = delete;

    /**
     * @brief Get the critical section reference
     * @brief (VI) Lấy tham chiếu critical section
     * @return Reference to the critical section
     */
    CRITICAL_SECTION& Get() { return m_cs; }

private:
    CRITICAL_SECTION m_cs;  ///< Critical section object
                            ///< (VI) Đối tượng critical section
};
