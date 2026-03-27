#!/usr/bin/env python3
"""下载并解压 weeklyactivity 文件"""
import urllib.request
import zlib
from pathlib import Path

# 可能的文件URL列表
URLS = [
    "http://enter.wanwan4399.com/bin-debug/assets/weekly/weeklyactivity",
]

def try_download(url):
    """尝试下载文件"""
    try:
        print(f"尝试: {url}")
        headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
        }
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req, timeout=15) as resp:
            return resp.read()
    except Exception as e:
        print(f"  失败: {e}")
        return None

def decompress_if_needed(data):
    """如果数据是zlib压缩的，则解压"""
    try:
        # 尝试解压
        decompressed = zlib.decompress(data)
        print(f"  数据已解压 (原始: {len(data)} 字节, 解压后: {len(decompressed)} 字节)")
        return decompressed
    except:
        # 不是压缩数据，直接返回
        return data

def main():
    output_dir = Path(__file__).resolve().parents[1] / "swf_cache"
    output_dir.mkdir(exist_ok=True)
    
    print("=" * 60)
    print("下载 weeklyactivity 文件")
    print("=" * 60)
    print()
    
    for url in URLS:
        data = try_download(url)
        if data:
            # 尝试解压
            data = decompress_if_needed(data)
            
            # 确定输出文件名
            if url.endswith('.bin'):
                filename = Path(url).name.replace('.bin', '')
            else:
                filename = Path(url).name
            
            # 如果是二进制数据，添加.bin后缀
            try:
                data.decode('utf-8')
            except:
                if not filename.endswith('.bin'):
                    filename += '.bin'
            
            output_path = output_dir / filename
            with open(output_path, 'wb') as f:
                f.write(data)
            print(f"OK 已保存: {output_path.as_posix()}")
            print()
    
    print("=" * 60)
    print("完成")
    print("=" * 60)

if __name__ == '__main__':
    main()
