#!/usr/bin/env python3
"""下载最新的weeklyactivity XML文件"""
import urllib.request
import urllib.error
import os
import sys

# 游戏服务器基地址（按优先级排序）
BASE_URLS = [
    "http://kbws.4399.com/",
    "https://kbws.4399.com/",
    "http://www.4399.com/kabuxiyou/",
]

# 文件列表
FILES_TO_DOWNLOAD = [
    "weeklyactivity_new.xml",
    "weeklyactivity.xml",
]

def download_file(filename):
    """下载单个文件，尝试多个备选URL"""
    output_path = f'D:/AItrace/CE/.trae/kbwebui/swf_cache/{filename}'
    
    for base_url in BASE_URLS:
        url = base_url + filename
        print(f'尝试下载: {url}')
        
        try:
            # 创建请求
            headers = {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
                'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
            }
            request = urllib.request.Request(url, headers=headers)
            
            # 下载文件
            with urllib.request.urlopen(request, timeout=30) as response:
                data = response.read()
                
                # 保存文件
                with open(output_path, 'wb') as f:
                    f.write(data)
                
                print(f'✓ 成功下载: {filename} ({len(data)} 字节)')
                print(f'  保存到: {output_path}')
                return True
                
        except urllib.error.HTTPError as e:
            print(f'  ✗ HTTP错误: {e.code} - {e.reason}')
            continue
        except urllib.error.URLError as e:
            print(f'  ✗ URL错误: {e.reason}')
            continue
        except Exception as e:
            print(f'  ✗ 错误: {e}')
            continue
    
    print(f'✗ 所有URL都失败，无法下载: {filename}')
    return False

def main():
    """主函数"""
    print('=' * 60)
    print('卡布西游每周活动文件下载工具')
    print('=' * 60)
    print()
    
    # 确保输出目录存在
    output_dir = 'D:/AItrace/CE/.trae/kbwebui/swf_cache'
    os.makedirs(output_dir, exist_ok=True)
    
    success_count = 0
    fail_count = 0
    
    for filename in FILES_TO_DOWNLOAD:
        if download_file(filename):
            success_count += 1
        else:
            fail_count += 1
        print()
    
    print('=' * 60)
    print(f'下载完成: 成功 {success_count} 个, 失败 {fail_count} 个')
    print('=' * 60)
    
    return 0 if fail_count == 0 else 1

if __name__ == '__main__':
    sys.exit(main())
