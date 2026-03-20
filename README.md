# 卡布西游浮影微端

[![Version](https://img.shields.io/badge/version-1.04-blue.svg)](https://github.com/lllcc666/kbxyfywd)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey.svg)](https://github.com/lllcc666/kbxyfywd)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

基于 WebView2 的卡布西游游戏辅助工具。

## 功能特性

- 一键完成游戏活动
- 自动战斗辅助
- 封包拦截与发送
- 游戏加速功能
- 活动状态实时监控

## 技术栈

| 组件 | 技术 |
|------|------|
| 语言 | C++17 (Win32 桌面应用) |
| UI 框架 | WebView2 (Edge Chromium) + HTML/CSS/JavaScript |
| 游戏嵌入 | WebBrowser 控件 (IE/ATL CAxWindow) |
| 构建工具 | CMake 3.16+ |
| 包管理 | vcpkg |
| Hook 框架 | MinHook |

## 环境要求

- Windows 10/11 x64
- Visual Studio 2019/2022 (MSVC)
- CMake 3.16+
- vcpkg

## 构建步骤

```powershell
# 克隆仓库
git clone https://github.com/lllcc666/kbxyfywd.git
cd kbxyfywd

# 创建构建目录
mkdir build_new
cd build_new

# 生成项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release
```

## 运行

```powershell
.\build_new\bin\Release\WebView2Demo.exe
```

## 项目结构

```
kbwebui/
├── demo.cpp                 # 主程序入口
├── wpe_hook.h/cpp           # 网络封包拦截 Hook 模块
├── packet_parser.h/cpp      # 封包解析器 + Opcode 定义
├── packet_builder.h/cpp     # 封包构建器
├── ui_bridge.h/cpp          # C++/JS 通信桥梁
├── data_interceptor.h/cpp   # 数据拦截模块
├── utils.h/cpp              # 工具函数
├── resources/               # 资源文件 (图标、UI)
├── embedded/                # 嵌入数据头文件
├── data/                    # 游戏数据缓存 (XML)
└── scripts/                 # Python 脚本工具
```

## 核心架构

```
┌─────────────────────────────────────────────────────────────┐
│                    主窗口 (demo.cpp)                        │
│  ┌───────────────────┐    ┌───────────────────────────────┐ │
│  │  WebView2 控件    │    │    WebBrowser 控件 (IE)       │ │
│  │  (辅助工具UI)     │    │    (游戏Flash页面)            │ │
│  └─────────┬─────────┘    └───────────────────────────────┘ │
│            │                                                │
│            ▼                                                │
│  ┌─────────────────────────────────────────────────────────┤
│  │              UIBridge (C++ ↔ JS 通信)                   │
│  └─────────────────────────────────────────────────────────┤
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                 WPE Hook 模块                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ HookedSend  │  │ HookedRecv  │  │  ResponseDispatcher │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 开发指南

### 添加新活动

1. 在 `packet_parser.h` 的 `Opcode` 命名空间中定义操作码
2. 在 `wpe_hook.h` 中添加活动状态结构
3. 在 `wpe_hook.cpp` 中实现封包发送函数
4. 在 `ResponseDispatcher::InitializeDefaultHandlers()` 中注册响应处理器
5. 在 `demo.cpp` 中添加 UI 调用入口

### 封包格式

```
| Magic (2B) | Length (2B) | Opcode (4B) | Params (4B) | Body (变长) |
```

- **Magic**: `0x5344` (普通包), `0x5343` (压缩包)
- **Opcode/Params**: 小端序

## 常见问题

**Q: 编译报错找不到头文件**  
A: 检查 vcpkg 路径配置是否正确

**Q: 如何计算封包 Opcode**  
A: 使用小端序计算：`opcode = byte[0] | (byte[1] << 8) | (byte[2] << 16) | (byte[3] << 24)`

## 许可证

MIT License

## 相关链接

- [GitHub 仓库](https://github.com/lllcc666/kbxyfywd)
- [问题反馈](https://github.com/lllcc666/kbxyfywd/issues)
