# AGENTS.md - 卡布西游微端项目指南

> **文档更新日期**: 2026年3月16日

## 项目概述

这是一个基于 **WebView2** 的 Windows 桌面应用程序，用于卡布西游游戏辅助工具（浮影微端）。

**窗口标题**：`卡布西游浮影微端 V1.02`

**技术栈**：
| 组件 | 技术 |
|------|------|
| 语言 | C++17 (Win32 桌面应用) |
| UI 框架 | WebView2 (Edge Chromium) + HTML/CSS/JavaScript |
| 游戏嵌入 | WebBrowser 控件 (IE/ATL CAxWindow) |
| 构建工具 | CMake 3.16+ |
| 包管理 | vcpkg |
| Hook 框架 | MinHook |
| 内存加载 | MemoryModule（DLL不落地） |
| 压缩库 | zlib + minizip |
| 平台 | Windows x64 |

---

## 核心架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         主窗口 (demo.cpp)                        │
│  ┌─────────────────────┐    ┌─────────────────────────────────┐ │
│  │   WebView2 控件     │    │      WebBrowser 控件 (IE)       │ │
│  │   (辅助工具UI)      │    │      (游戏Flash页面)            │ │
│  └──────────┬──────────┘    └─────────────────────────────────┘ │
│             │                                                    │
│             ▼                                                    │
│  ┌──────────────────────────────────────────────────────────────┤
│  │                    UIBridge (ui_bridge.h)                    │
│  │              C++ ↔ JavaScript 双向通信桥梁                   │
│  └──────────────────────────────────────────────────────────────┤
└─────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────┐
│                    WPE Hook 模块 (wpe_hook.h/cpp)               │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  HookedSend     │  │  HookedRecv     │  │ ResponseWaiter  │ │
│  │  (发送拦截)     │  │  (接收拦截)     │  │  (响应等待)     │ │
│  └────────┬────────┘  └────────┬────────┘  └─────────────────┘ │
│           │                    │                                 │
│           ▼                    ▼                                 │
│  ┌─────────────────────────────────────────────────────────────┤
│  │              ResponseDispatcher (响应分发器)                │
│  │         根据 Opcode+Params 分发到对应处理器                 │
│  └─────────────────────────────────────────────────────────────┤
└─────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────┐
│                  封包处理层 (packet_parser.h/cpp)               │
│  ┌─────────────────┐  ┌─────────────────────────────────────┐  │
│  │  PacketParser   │  │        数据结构定义                  │  │
│  │  (封包解析)     │  │  GamePacket, BattleData, LingyuItem  │  │
│  └─────────────────┘  └─────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────────────────┤
│  │                    Opcode 命名空间                          │
│  │              所有协议操作码的常量定义                        │
│  └─────────────────────────────────────────────────────────────┤
└─────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────┐
│                 封包构建层 (packet_builder.h/cpp)               │
│  ┌─────────────────────────────────────────────────────────────┤
│  │                    PacketBuilder 类                         │
│  │         链式调用构造封包，自动处理小端序编码                 │
│  └─────────────────────────────────────────────────────────────┤
└─────────────────────────────────────────────────────────────────┘
```

---

## 模块职责

| 模块 | 文件 | 职责 |
|------|------|------|
| **主程序** | `demo.cpp` | 窗口创建、WebView2初始化、消息循环、版本检查 |
| **WPE Hook** | `wpe_hook.h/cpp` | 网络封包拦截、发送、响应等待、活动状态管理 |
| **封包解析** | `packet_parser.h/cpp` | 封包解析、战斗数据处理、Opcode定义 |
| **封包构建** | `packet_builder.h/cpp` | 封包构造器，链式调用，自动小端序 |
| **UI桥接** | `ui_bridge.h/cpp` | C++与JavaScript通信桥梁 |
| **数据拦截** | `data_interceptor.h/cpp` | HTTP响应拦截，修改游戏数据文件 |
| **工具函数** | `utils.h/cpp` | 编码转换、线程同步、远程下载 |

---

## 封包协议基础

### 封包格式

```
┌───────────────┬───────────────┬───────────────┬───────────────┬───────────────┐
│  Magic (2B)   │ Length (2B)   │ Opcode (4B)   │ Params (4B)   │  Body (变长)   │
└───────────────┴───────────────┴───────────────┴───────────────┴───────────────┘
     0-1字节         2-3字节         4-7字节         8-11字节        12+字节
```

- **Magic**: `0x5344` ("SD") 普通包，`0x5343` ("SC") 压缩包
- **Length**: Body 长度（小端序）
- **Opcode**: 操作码（小端序）
- **Params**: 参数（小端序）
- **Body**: 数据体（变长，所有数值字段都是小端序）

### Opcode 计算（小端序）

```
Opcode = byte[0] | (byte[1] << 8) | (byte[2] << 16) | (byte[3] << 24)
```

**验证工具**：
```python
int.from_bytes(bytes.fromhex('14141200'), 'little')  # 输出: 1184788
```

---

## 核心工具类

### PacketBuilder（封包构建器）

**位置**: `packet_builder.h`

**使用示例**:
```cpp
auto packet = PacketBuilder()
    .SetOpcode(1185429)
    .SetParams(770)
    .WriteString("game_info")
    .WriteInt32(activityId)
    .Build();
```

### UIBridge（UI桥接器）

**位置**: `ui_bridge.h`

**主要方法**:
- `UpdateHelperText()` - 更新辅助文本
- `UpdateProgress()` - 更新进度显示
- `NotifyTaskComplete()` - 通知任务完成
- `ExecuteJS()` - 执行JavaScript代码

### ResponseWaiter（响应等待器）

**位置**: `wpe_hook.h`（内部类）

**用途**: 高效等待服务器响应，替代 Sleep 轮询

**使用方式**:
```cpp
SendPacket(s, data, size, expectedOpcode, timeoutMs);
```

### ResponseDispatcher（响应分发器）

**位置**: `wpe_hook.h`

**用途**: 根据 Opcode+Params 分发响应到对应处理器

**使用示例**:
```cpp
ResponseDispatcher::Instance().Register(
    Opcode::ACTIVITY_QUERY_BACK, 
    788, 
    ProcessStrawberryResponse
);
```

---

## 项目结构

```
kbwebui/
├── CMakeLists.txt           # CMake 构建配置
├── AGENTS.md                # 项目文档
│
├── demo.cpp                 # 主程序入口
├── wpe_hook.h/cpp           # WPE 网络封包拦截 Hook 模块
├── packet_parser.h/cpp      # 游戏封包解析器 + Opcode定义
├── packet_builder.h/cpp     # 封包构建器
├── ui_bridge.h/cpp          # UI 桥接器
├── data_interceptor.h/cpp   # Data文件拦截修改模块
├── utils.h/cpp              # 工具函数
│
├── scripts/                 # Python 脚本工具
│   ├── embed_dll.py         # DLL 转 C++ 头文件
│   └── embed_html.py        # HTML 转 C++ 头文件
│
├── resources/               # 资源文件
│   ├── app.ico              # 应用图标
│   ├── app.rc               # 资源脚本
│   └── ui.html              # Web 界面源文件
│
├── embedded/                # 嵌入数据头文件（自动生成）
│   ├── webview2loader_data.h
│   ├── zlib_data.h
│   ├── minizip_data.h
│   ├── ui_html.h
│   ├── minhook_data.h
│   └── speed_x64_data.h
│
├── data/                    # 游戏数据缓存（XML）
│   ├── sprite.xml           # 妖怪名称、系别
│   ├── skill.xml            # 技能名称
│   ├── tool.xml             # 道具名称
│   └── map.xml              # 地图名称
│
├── src/                     # AS3反编译源码（游戏协议参考）
│   └── com/game/locators/   # 协议定义 (MsgDoc.as, GameData.as)
│
├── swf_cache/               # SWF缓存目录
│   ├── decompiled/          # 反编译输出
│   └── *.swf                # 下载的SWF文件
│
└── build_new/               # 构建输出目录
```

---

## 构建和运行

### 环境要求

- Windows 10/11 x64
- Visual Studio 2019/2022 (MSVC)
- CMake 3.16+
- vcpkg

### 构建命令

```powershell
cd D:\AItrace\CE\.trae\kbwebui
mkdir build_new
cd build_new
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 运行

```powershell
.\build_new\bin\Release\WebView2Demo.exe
```

---

## 开发规范

### C++ 编码规范

1. **编码**: MSVC 使用 `/utf-8` 强制 UTF-8 编码
2. **标准**: C++17，使用 `std::wstring` 处理中文
3. **命名**:
   - 类名/函数名: `PascalCase`
   - 变量名: `camelCase`
   - 常量: `UPPER_SNAKE_CASE` (命名空间中使用 `constexpr`)
   - 命名空间: `PascalCase`
4. **线程同步**: 使用 RAII 风格的 `CriticalSectionLock`

### 添加新功能的步骤

1. **定义 Opcode**: 在 `packet_parser.h` 的 `Opcode` 命名空间中添加
2. **添加状态**: 在 `wpe_hook.h` 中添加状态结构（如需要）
3. **实现发送函数**: 在 `wpe_hook.cpp` 中实现封包发送
4. **注册响应处理**: 在 `ResponseDispatcher::InitializeDefaultHandlers()` 中注册
5. **添加UI接口**: 在 `demo.cpp` 的消息处理中添加 JavaScript 调用入口

---

## 调试技巧

1. **只能使用 WebView2 控制台输出**，不能使用 debug
2. 注意输出信息编码问题（使用 UTF-8）
3. 使用封包拦截查看实际发送的封包
4. 对比客户端发送的封包与程序构造的封包

---

## 常见问题

### Q: 编译报错找不到头文件
**A**: 检查 vcpkg 路径配置是否正确

### Q: 如何计算封包 Opcode
**A**: 使用小端序计算：`opcode = byte[0] | (byte[1] << 8) | (byte[2] << 16) | (byte[3] << 24)`

### Q: 版本号一致但仍弹出更新对话框
**A**: 确保 `demo.cpp` 中的 `CURRENT_VERSION` 与服务器版本号精度一致（如 `1.02f`）

---

## 版本号更新清单

每次发布新版本时，同步更新以下位置：

| 文件 | 内容 |
|------|------|
| `demo.cpp:84` | `CURRENT_VERSION = X.XXf` |
| `demo.cpp` | 窗口标题 `L"卡布西游浮影微端 VX.XX"` |
| `wpe_hook.cpp` | Hook标题 |
| `resources/ui.html` | UI标题 |
| `resources/app.rc` | 版本信息 |