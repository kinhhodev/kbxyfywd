#pragma once

#include <mutex>

#include "wpe_hook.h"

using ResponseHandler = void (*)(const GamePacket&);

// ============================================================================
// Response waiter (internal use)
// Bộ chờ phản hồi (chỉ dùng nội bộ)
// ============================================================================

/**
 * @class ResponseWaiter
 * @brief Response waiter (internal use, used by SendPacket to wait for replies)
 * @brief (VI) Bộ chờ phản hồi (nội bộ, dùng cho SendPacket tự chờ phản hồi)
 *
 * Uses a condition variable for efficient event notification instead of Sleep polling.
 * Optimization: only locks when there are waiting threads, avoiding per-packet locking.
 *
 * (VI) Dùng condition variable để thông báo sự kiện hiệu quả, thay cho polling bằng Sleep.
 * (VI) Tối ưu: chỉ lock khi thực sự có thread đang đợi, tránh lock cho mọi packet.
 */
class ResponseWaiter {
public:
    /**
     * @brief Initialize ResponseWaiter (called in InitializeHooks)
     * @brief (VI) Khởi tạo ResponseWaiter (được gọi trong InitializeHooks)
     */
    static void Initialize();

    /**
     * @brief Cleanup ResponseWaiter (called in UninitializeHooks)
     * @brief (VI) Dọn dẹp ResponseWaiter (được gọi trong UninitializeHooks)
     */
    static void Cleanup();

    /**
     * @brief Wait for a response with a specific opcode
     * @brief (VI) Chờ phản hồi theo opcode cụ thể
     * @param expectedOpcode Expected opcode
     * @param timeoutMs Timeout in milliseconds
     * @return Whether a response was received
     */
    static bool WaitForResponse(
        uint32_t expectedOpcode,
        DWORD timeoutMs
    );

    /**
     * @brief Notify that a response was received (called in ProcessReceivedGamePackets)
     * @brief (VI) Thông báo đã nhận phản hồi (được gọi trong ProcessReceivedGamePackets)
     * @param opcode Received opcode
     *
     * Optimization: uses an atomic fast-path to check whether any thread is waiting,
     * avoiding unnecessary locking.
     *
     * (VI) Tối ưu: dùng biến atomic để kiểm tra nhanh có thread đang đợi hay không,
     * tránh lock không cần thiết.
     */
    static void NotifyResponse(uint32_t opcode);

    /**
     * @brief Cancel waiting
     * @brief (VI) Hủy chờ
     */
    static void CancelWait();

private:
    static CRITICAL_SECTION s_cs;
    static CONDITION_VARIABLE s_cv;
    static bool s_responseReceived;
    static uint32_t s_receivedOpcode;
    static std::atomic<long> s_waitingCount;  // Waiting-thread counter (truly atomic)
                                              // (VI) Bộ đếm thread đang đợi (atomic thật sự)
};

// ============================================================================
// Response dispatcher
// (VI) Bộ phân phối phản hồi
// ============================================================================

/**
 * @brief Response handler type definitions
 * @brief (VI) Định nghĩa kiểu hàm xử lý phản hồi
 */
using ResponseHandler = void (*)(const GamePacket&);
using PacketProgressCallback = void (*)(DWORD, DWORD, const std::string&);

/**
 * @class ResponseDispatcher
 * @brief Response dispatcher - routes responses to handlers by opcode and params
 * @brief (VI) Bộ phân phối phản hồi - điều phối theo opcode và params
 * 
 * Example usage:
 * (VI) Ví dụ sử dụng:
 * @code
 * // Register handler
 * ResponseDispatcher::Instance().Register(
 *     Opcode::ACTIVITY_QUERY_BACK, 
 *     activityId,
 *     ProcessActivityResponse
 * );
 * 
 * // Dispatch inside HookedRecv
 * ResponseDispatcher::Instance().Dispatch(packet);
 * @endcode
 */
class ResponseDispatcher {
public:
    /**
     * @brief Get the singleton instance
     * @brief (VI) Lấy instance singleton
     */
    static ResponseDispatcher& Instance();

    /**
     * @brief Register a response handler (opcode-only match)
     * @brief (VI) Đăng ký handler (chỉ match opcode)
     * @param opcode Opcode
     * @param handler Handler function
     * @return Whether registration succeeded
     */
    BOOL Register(uint32_t opcode, ResponseHandler handler);

    /**
     * @brief Register a response handler (opcode + params match)
     * @brief (VI) Đăng ký handler (match opcode + params)
     * @param opcode Opcode
     * @param params Params value
     * @param handler Handler function
     * @return Whether registration succeeded
     */
    BOOL Register(uint32_t opcode, uint32_t params, ResponseHandler handler);

    /**
     * @brief Unregister a handler
     * @brief (VI) Hủy đăng ký handler
     * @param opcode Opcode
     * @param params Params value (0xFFFFFFFF matches any params)
     */
    void Unregister(uint32_t opcode, uint32_t params = 0xFFFFFFFF);

    /**
     * @brief Dispatch a packet to the matching handler
     * @brief (VI) Điều phối packet tới handler phù hợp
     * @param packet Game packet
     * @return Whether any handler processed the packet
     */
    BOOL Dispatch(const GamePacket& packet);

    /**
     * @brief Clear all handlers
     * @brief (VI) Xóa toàn bộ handler
     */
    void Clear();

    /**
     * @brief Initialize default handlers (called in InitializeHooks)
     * @brief (VI) Khởi tạo handler mặc định (được gọi trong InitializeHooks)
     */
    void InitializeDefaultHandlers();

private:
    ResponseDispatcher() = default;
    ~ResponseDispatcher() = default;
    ResponseDispatcher(const ResponseDispatcher&) = delete;
    ResponseDispatcher& operator=(const ResponseDispatcher&) = delete;

    // Build a unique key: high 32 bits = opcode, low 32 bits = params
    // (VI) Tạo key duy nhất: 32-bit cao = opcode, 32-bit thấp = params
    static uint64_t MakeKey(uint32_t opcode, uint32_t params);

    // Handler map
    // (VI) Bảng ánh xạ handler
    struct HandlerEntry {
        uint64_t key;
        ResponseHandler handler;
    };
    std::vector<HandlerEntry> m_handlers;
    
    // Opcode-only handlers (params = 0xFFFFFFFF)
    // (VI) Handler chỉ theo opcode (params = 0xFFFFFFFF)
    struct OpcodeHandlerEntry {
        uint32_t opcode;
        ResponseHandler handler;
    };
    std::vector<OpcodeHandlerEntry> m_opcodeOnlyHandlers;
    
    // Thread safety
    // (VI) An toàn luồng
    std::mutex m_mutex;
};

// ============================================================================
// Activity state management
// (VI) Quản lý trạng thái hoạt động
// ============================================================================

/**
 * @struct ActivityState
 * @brief Activity state base - common fields shared by all activities
 * @brief (VI) Base trạng thái hoạt động - các field chung cho mọi activity
 */
