/**
 * @file data_interceptor.h
 * @brief data 文件拦截和修改模块
 * 
 * 拦截游戏下载的 data 文件，为所有妖怪添加 display 属性
 * 使所有妖怪在战斗中都能显示技能 PP 值
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace DataInterceptor {

/**
 * @brief 初始化 data 拦截器
 * @return 初始化是否成功
 */
BOOL Initialize();

/**
 * @brief 清理 data 拦截器
 */
VOID Cleanup();

/**
 * @brief 检查 URL 是否为 data 文件
 * @param url 请求的 URL
 * @return 是否为 data 文件
 */
BOOL IsDataUrl(const char* url);

/**
 * @brief 处理 HTTP 响应数据
 * @param url 请求的 URL
 * @param pData 原始数据
 * @param dwSize 数据大小
 * @param pModifiedData 输出：修改后的数据
 * @param pdwModifiedSize 输出：修改后的大小
 * @return 是否需要替换数据
 */
BOOL ProcessHttpResponse(
    const char* url,
    const BYTE* pData,
    DWORD dwSize,
    std::vector<BYTE>& modifiedData
);

/**
 * @brief 为 XML 中的所有 spirit 节点添加 display 属性
 * @param xmlData XML 数据（已解压）
 * @return 修改后的 XML 数据
 */
std::vector<char> AddDisplayAttribute(const std::vector<char>& xmlData);

/**
 * @brief 解压 zlib 压缩的数据
 * @param compressedData 压缩数据
 * @param decompressedData 输出：解压后的数据
 * @return 解压是否成功
 */
BOOL DecompressData(
    const std::vector<BYTE>& compressedData,
    std::vector<BYTE>& decompressedData
);

/**
 * @brief 压缩数据
 * @param data 原始数据
 * @param compressedData 输出：压缩后的数据
 * @return 压缩是否成功
 */
BOOL CompressData(
    const std::vector<BYTE>& data,
    std::vector<BYTE>& compressedData
);

}  // namespace DataInterceptor
