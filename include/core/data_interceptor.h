/**
 * @file data_interceptor.h
 * @brief Data file interception and modification module
 * @brief (VI) Module chặn và chỉnh sửa file data
 * 
 * Intercepts the data file downloaded by the game and adds the `display` attribute.
 * This allows the UI to show skill PP values in battle.
 *
 * (VI) Chặn file data game tải về và thêm thuộc tính `display`.
 * (VI) Nhằm hiển thị PP kỹ năng trong chiến đấu.
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace DataInterceptor {

/**
 * @brief Initialize the data interceptor
 * @brief (VI) Khởi tạo data interceptor
 * @return Whether initialization succeeded
 */
BOOL Initialize();

/**
 * @brief Cleanup the data interceptor
 * @brief (VI) Dọn dẹp data interceptor
 */
VOID Cleanup();

/**
 * @brief Check whether the URL is a data file request
 * @brief (VI) Kiểm tra URL có phải request file data hay không
 * @param url Request URL
 * @return Whether it is a data file
 */
BOOL IsDataUrl(const char* url);

/**
 * @brief Process HTTP response data
 * @brief (VI) Xử lý dữ liệu phản hồi HTTP
 * @param url Request URL
 * @param pData Original data
 * @param dwSize Data size
 * @param modifiedData Output: modified data
 * @return Whether the data should be replaced
 */
BOOL ProcessHttpResponse(
    const char* url,
    const BYTE* pData,
    DWORD dwSize,
    std::vector<BYTE>& modifiedData
);

/**
 * @brief Add the `display` attribute to all `spirit` nodes in the XML
 * @brief (VI) Thêm thuộc tính `display` cho mọi node `spirit` trong XML
 * @param xmlData XML data (already decompressed)
 * @return Modified XML data
 */
std::vector<char> AddDisplayAttribute(const std::vector<char>& xmlData);

/**
 * @brief Decompress zlib-compressed data
 * @brief (VI) Giải nén dữ liệu nén zlib
 * @param compressedData Compressed data
 * @param decompressedData Output: decompressed data
 * @return Whether decompression succeeded
 */
BOOL DecompressData(
    const std::vector<BYTE>& compressedData,
    std::vector<BYTE>& decompressedData
);

/**
 * @brief Compress data
 * @brief (VI) Nén dữ liệu
 * @param data Raw data
 * @param compressedData Output: compressed data
 * @return Whether compression succeeded
 */
BOOL CompressData(
    const std::vector<BYTE>& data,
    std::vector<BYTE>& compressedData
);

}  // namespace DataInterceptor
