/**
 * @file packet_builder.h
 * @brief Packet builder - unified packet construction interface
 * @brief (VI) Packet builder - thống nhất cách dựng packet
 * 
 * Provides a fluent/chained packet-construction API and automatically encodes in little-endian.
 * Refactor goal: reduce ~50% of duplicated packet construction code.
 *
 * (VI) Cung cấp API dựng packet theo kiểu chain, tự encode little-endian.
 * (VI) Mục tiêu refactor: giảm ~50% code dựng packet lặp lại.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "packet_protocol.h"

/**
 * @class PacketBuilder
 * @brief Packet builder
 * @brief (VI) Trình dựng packet
 * 
 * Builds game packets via chained calls and automatically handles little-endian encoding.
 * (VI) Dựng packet game bằng chain call và tự xử lý little-endian.
 * 
 * @example
 * @code
 * auto packet = PacketBuilder()
 *     .SetOpcode(1185429)
 *     .SetParams(770)
 *     .WriteString("game_info")
 *     .Build();
 * @endcode
 */
class PacketBuilder {
public:
    PacketBuilder();
    ~PacketBuilder() = default;

    // ================================
    // Header setters
    // (VI) Thiết lập header
    // ================================

    /**
     * @brief Set Magic (defaults to 0x5344 "SD")
     * @brief (VI) Đặt Magic (mặc định 0x5344 "SD")
     * @param magic Magic value
     * @return Self reference for chaining
     */
    PacketBuilder& SetMagic(uint16_t magic);

    /**
     * @brief Set opcode
     * @brief (VI) Đặt opcode
     * @param opcode Opcode
     * @return Self reference for chaining
     */
    PacketBuilder& SetOpcode(uint32_t opcode);

    /**
     * @brief Set params
     * @brief (VI) Đặt params
     * @param params Params value
     * @return Self reference for chaining
     */
    PacketBuilder& SetParams(uint32_t params);

    // ================================
    // Body writers
    // (VI) Ghi Body
    // ================================

    /**
     * @brief Write a string (with a little-endian length prefix)
     * @brief (VI) Ghi chuỗi (kèm prefix độ dài little-endian)
     * @param str String content
     * @return Self reference for chaining
     */
    PacketBuilder& WriteString(const std::string& str);

    /**
     * @brief Write an int32 (little-endian)
     * @brief (VI) Ghi int32 (little-endian)
     * @param value Integer value
     * @return Self reference for chaining
     */
    PacketBuilder& WriteInt32(int32_t value);

    /**
     * @brief Write a uint32 (little-endian)
     * @brief (VI) Ghi uint32 (little-endian)
     * @param value Integer value
     * @return Self reference for chaining
     */
    PacketBuilder& WriteUInt32(uint32_t value);

    /**
     * @brief Write an int16 (little-endian)
     * @brief (VI) Ghi int16 (little-endian)
     * @param value Integer value
     * @return Self reference for chaining
     */
    PacketBuilder& WriteInt16(int16_t value);

    /**
     * @brief Write a uint16 (little-endian)
     * @brief (VI) Ghi uint16 (little-endian)
     * @param value Integer value
     * @return Self reference for chaining
     */
    PacketBuilder& WriteUInt16(uint16_t value);

    /**
     * @brief Write a single byte
     * @brief (VI) Ghi 1 byte
     * @param value Byte value
     * @return Self reference for chaining
     */
    PacketBuilder& WriteByte(uint8_t value);

    /**
     * @brief Write a byte array
     * @brief (VI) Ghi mảng byte
     * @param data Byte array
     * @return Self reference for chaining
     */
    PacketBuilder& WriteBytes(const std::vector<uint8_t>& data);

    /**
     * @brief Write an int32 array (each element little-endian)
     * @brief (VI) Ghi mảng int32 (mỗi phần tử little-endian)
     * @param values Integer array
     * @return Self reference for chaining
     */
    PacketBuilder& WriteInt32Array(const std::vector<int32_t>& values);

    // ================================
    // Build methods
    // (VI) Hàm build
    // ================================

    /**
     * @brief Build the final packet
     * @brief (VI) Build packet cuối
     * @return Packet bytes
     */
    std::vector<uint8_t> Build();

    /**
     * @brief Clear the buffer and start building from scratch
     * @brief (VI) Xóa buffer và bắt đầu build lại
     */
    void Reset();

    // ================================
    // Utility methods
    // (VI) Hàm tiện ích
    // ================================

    /**
     * @brief Get current body length
     * @brief (VI) Lấy độ dài body hiện tại
     * @return Body length
     */
    size_t GetBodyLength() const { return m_body.size(); }

private:
    // Header fields
    // (VI) Các field header
    uint16_t m_magic;     ///< Magic value (default: 0x5344)
                          ///< (VI) Giá trị Magic (mặc định: 0x5344)
    uint32_t m_opcode;    ///< Opcode
    uint32_t m_params;    ///< Params

    // Body buffer
    // (VI) Buffer body
    std::vector<uint8_t> m_body;  ///< Body bytes
                                  ///< (VI) Dữ liệu body (bytes)

    // State flags
    // (VI) Cờ trạng thái
    bool m_headerSet;     ///< Whether the header has been set
                          ///< (VI) Header đã được set hay chưa
};

// ================================
// Convenience builders
// (VI) Hàm dựng nhanh
// ================================

/**
 * @brief Quickly build an activity-operation packet (common format)
 * @brief (VI) Dựng nhanh packet thao tác activity (định dạng chung)
 * @param opcode Opcode
 * @param activityId Activity ID
 * @param operation Operation string
 * @param bodyValues Body value list
 * @return Full packet bytes
 * 
 * Packet format:
 * (VI) Định dạng packet:
 * - Magic: 0x5344
 * - Length: body length
 * - Opcode: specified value
 * - Params: activity ID
 * - Body: [string length(2B)][string bytes][int32 values...]
 *
 * (VI) - Length: độ dài body
 * (VI) - Opcode: giá trị chỉ định
 * (VI) - Params: ID activity
 * (VI) - Body: [độ dài chuỗi(2B)][bytes chuỗi][các giá trị int32...]
 */
std::vector<uint8_t> BuildActivityPacket(
    uint32_t opcode,
    uint32_t activityId,
    const std::string& operation,
    const std::vector<int32_t>& bodyValues = {}
);
