#!/usr/bin/env python3
"""基础SWF文件分析工具"""
import struct
import zlib
import os

def analyze_swf(filepath):
    """分析SWF文件基本信息"""
    with open(filepath, 'rb') as f:
        data = f.read()
    
    print(f"文件: {filepath}")
    print(f"大小: {len(data)} 字节")
    print()
    
    # 检查SWF签名
    signature = data[:3]
    print(f"签名: {signature}")
    
    if signature == b'FWS':
        print("  未压缩的SWF文件")
        uncompressed_data = data
    elif signature == b'CWS':
        print("  zlib压缩的SWF文件")
        # 解压（跳过前8字节）
        uncompressed_data = data[:8] + zlib.decompress(data[8:])
        print(f"  解压后大小: {len(uncompressed_data)} 字节")
    elif signature == b'ZWS':
        print("  LZMA压缩的SWF文件")
        print("  暂不支持解压")
        return
    else:
        print(f"  未知格式")
        return
    
    # 解析版本
    version = uncompressed_data[3]
    print(f"\n版本: {version}")
    
    # 解析文件长度
    file_length = struct.unpack('<I', uncompressed_data[4:8])[0]
    print(f"文件长度: {file_length} 字节")
    
    # 如果是解压后的文件，检查帧大小和帧率
    if len(uncompressed_data) > 8:
        # 帧大小（RECT结构）- 比较复杂，跳过详细解析
        # 帧率
        if len(uncompressed_data) >= 12:
            frame_rate = struct.unpack('<H', uncompressed_data[8:10])[0] / 256
            frame_count = struct.unpack('<H', uncompressed_data[10:12])[0]
            print(f"帧率: {frame_rate} fps")
            print(f"帧数: {frame_count}")
    
    # 搜索DoABC标签（AS3字节码）
    print("\n搜索AS3字节码标签...")
    search_tags(uncompressed_data)

def search_tags(data):
    """搜索SWF标签"""
    # 简单搜索常见的AS3相关字符串
    as3_signatures = [
        b'DoABC',
        b'ActionScript3',
        b'activity',
        b'game_info',
        b'start_game',
        b'end_game',
        b'checkCode',
        b'opcode',
    ]
    
    for sig in as3_signatures:
        count = data.count(sig)
        if count > 0:
            print(f"  找到 '{sig.decode('latin-1', errors='replace')}': {count} 次")

if __name__ == '__main__':
    swf_path = "D:/AItrace/CE/.trae/kbwebui/swf_cache/PanshiYutianhuo.swf"
    analyze_swf(swf_path)
