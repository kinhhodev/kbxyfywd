# KBXYFYWD - WebView2 Windows helper
*(VI) KBXYFYWD - Công cụ hỗ trợ Windows dùng WebView2*

[![Version](https://img.shields.io/badge/version-1.04-blue.svg)](https://github.com/lllcc666/kbxyfywd)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey.svg)](https://github.com/lllcc666/kbxyfywd)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

A WebView2-based Windows desktop helper for the game, built with Win32 C++17, ATL/WebBrowser, MinHook, and embedded resources.

*(VI) Ứng dụng desktop Windows hỗ trợ game dựa trên WebView2, dùng Win32 C++17, ATL/WebBrowser, MinHook và tài nguyên nhúng.*

## Features
*(VI) Tính năng*

- Hosts the tool UI using WebView2 (frontend source: `resources/ui.html`)
- Hosts the game page using ATL `CAxWindow` + `IWebBrowser2`
- Provides packet interception, protocol parsing, activity automation, and data bridging
- Embeds HTML/WebView2Loader/zlib resources into the executable at build time
- Primary dev environment: Windows x64 + MSVC + CMake

*(VI)*
- *(VI) Dùng WebView2 để hiển thị UI công cụ (nguồn frontend: `resources/ui.html`)*
- *(VI) Dùng ATL `CAxWindow` + `IWebBrowser2` để nhúng trang game*
- *(VI) Có chặn packet mạng, parse protocol, tự động hóa activity, cầu nối dữ liệu*
- *(VI) Nhúng tài nguyên HTML/WebView2Loader/zlib vào file `.exe` khi build*
- *(VI) Môi trường phát triển chính: Windows x64 + MSVC + CMake*

## Tech stack
*(VI) Công nghệ sử dụng*

| Component | Notes |
|------|------|
| Language | C++17 |
| Desktop framework | Win32 API |
| UI | WebView2 + HTML/CSS/JavaScript |
| Game host | ATL / WebBrowser |
| Hook | MinHook |
| Compression/loader | zlib / MemoryModule |
| Build | CMake 3.16+ |
| Dependency management | vcpkg |

*(VI) Bảng trên: Component = Thành phần, Notes = Mô tả.*

## Requirements
*(VI) Yêu cầu môi trường*

- Windows 10/11 x64
- Visual Studio 2019 or 2022
- CMake 3.16+
- Python 3 available via `python3`
- vcpkg

*(VI)*
- *(VI) Windows 10/11 x64*
- *(VI) Visual Studio 2019 hoặc 2022*
- *(VI) CMake 3.16+*
- *(VI) Python 3 gọi được bằng lệnh `python3`*
- *(VI) vcpkg*

## Build
*(VI) Cách build*

`CMakeLists.txt` currently hardcodes this vcpkg path:

*(VI) `CMakeLists.txt` hiện đang hardcode đường dẫn vcpkg như sau:*

```text
d:/AItrace/CE/.trae/vcpkg-master
```

If your local path differs, update it first or adjust the CMake config.

*(VI) Nếu máy bạn khác đường dẫn này, hãy sửa lại trước hoặc chỉnh cấu hình CMake.*

### Generate project
*(VI) Tạo project*

```powershell
cmake -S . -B build_new -G "Visual Studio 17 2022" -A x64
```

### Build Release
*(VI) Build bản Release*

```powershell
cmake --build build_new --config Release
```

### Run
*(VI) Chạy chương trình*

```powershell
.\build_new\bin\Release\WebView2Demo.exe
```

## Repository layout
*(VI) Cấu trúc thư mục*

```text
kbwebui/
├─ src/
│  ├─ app/                  # App entry & main window bootstrap
│  ├─ core/                 # Bridge/utils/message handling/data interception
│  ├─ hook/                 # Hook core logic & automation
│  └─ protocol/             # Protocol parsing & packet building
├─ include/
│  ├─ activities/           # Activity/feature declarations
│  ├─ core/                 # Core headers
│  ├─ hook/                 # Hook public headers
│  ├─ internal/             # Internal state machines/waiters/private structs
│  └─ protocol/             # Protocol types & declarations
├─ resources/               # UI/icon/RC resources
├─ embedded/                # Embedded headers & embedded assets
├─ data/                    # Local XML data cache
├─ scripts/                 # Python tooling scripts
├─ swf_cache/               # Download cache (not source)
├─ build_new/               # Local build output directory
├─ CMakeLists.txt
├─ README.md
└─ AGENTS.md
```

*(VI) Chú thích nhanh (tương ứng với các dòng `#` ở trên).*

## Key files
*(VI) Các file quan trọng*

- `src/app/demo.cpp`
  - App entry, window lifecycle, WebView2 init, main UI flow
- `src/hook/wpe_hook.cpp`
  - Hook lifecycle, packet interception, response dispatch, automation core
- `src/hook/wpe_hook_helpers.cpp`
  - Hook helpers
- `src/protocol/packet_parser.cpp`
  - Protocol parsing, opcode/params handling, decompression
- `src/protocol/packet_builder.cpp`
  - Packet building helpers
- `src/core/ui_bridge.cpp`
  - C++ to JavaScript bridge
- `src/core/web_message_handler.cpp`
  - Frontend-to-native message entry point
- `src/core/data_interceptor.cpp`
  - HTTP/asset response interception and modification
- `include/activities/*.h`
  - Activity declarations, state access, automation entry points
- `include/internal/*.h`
  - Internal state, waiters, state machines

*(VI) Mô tả tiếng Việt cho list trên tương ứng 1-1 với ý nghĩa tiếng Anh.*

## Assets and generated files
*(VI) Tài nguyên và file sinh ra khi build*

### Source assets
*(VI) Tài nguyên nguồn*

- `resources/ui.html`
- `resources/app.rc`
- `resources/app.ico`

### Generated during build
*(VI) Sinh ra trong quá trình build*

These files are generated by the build and should not be edited manually:

*(VI) Các file dưới đây được sinh ra khi build, không nên sửa tay:*

- `embedded/ui_html.h`
- `embedded/webview2loader_data.h`
- `embedded/zlib_data.h`

### Embedded headers tracked in repo
*(VI) Header nhúng được commit trong repo*

These files are under `embedded/` but are not generated by the current CMake custom commands:

*(VI) Các file dưới nằm trong `embedded/` nhưng không phải do CMake hiện tại tự sinh:*

- `embedded/minhook_data.h`
- `embedded/speed_x64_data.h`
- `embedded/minizip_helper.h`

## Python scripts
*(VI) Script Python*

- `scripts/embed_html.py`
  - Converts `resources/ui.html` into an embedded header
- `scripts/embed_dll.py`
  - Converts a DLL into an embedded header
- `scripts/download_swf.py`
  - Downloads activity SWFs into the local cache directory
- `scripts/fetch_activities.py`
  - Fetches activity data / cache helper

*(VI) Mô tả tiếng Việt tương ứng với các bullet tiếng Anh ở trên.*

## Protocol notes
*(VI) Ghi chú về protocol*

Base packet layout used in this project:

*(VI) Định dạng packet cơ bản trong project:*

```text
Magic(2) + Length(2) + Opcode(4) + Params(4) + Body
```

- Normal packet magic: `0x5344`
- Compressed packet magic: `0x5343`
- Protocol numbers are little-endian

*(VI)*
- *(VI) Magic của packet thường: `0x5344`*
- *(VI) Magic của packet nén: `0x5343`*
- *(VI) Giá trị protocol dùng little-endian*

## Development tips
*(VI) Gợi ý phát triển*

### Adding a new activity/feature
*(VI) Thêm activity/tính năng mới*

Typical steps:

*(VI) Thường làm theo thứ tự:*

1. Add required constants/opcodes/data structures in the protocol layer
2. Add activity declarations in `include/activities/`
3. Add internal state structures in `include/internal/` (if needed)
4. Implement sending/response handling/automation flow in `src/hook/wpe_hook.cpp`
5. Wire the UI entry in `src/core/web_message_handler.cpp` or `src/app/demo.cpp`

*(VI)*
1. *(VI) Bổ sung hằng số/opcode/struct ở layer protocol*
2. *(VI) Thêm khai báo activity ở `include/activities/`*
3. *(VI) Thêm struct trạng thái nội bộ ở `include/internal/` (nếu cần)*
4. *(VI) Implement gửi/xử lý phản hồi/tự động hóa trong `src/hook/wpe_hook.cpp`*
5. *(VI) Nối UI entry trong `src/core/web_message_handler.cpp` hoặc `src/app/demo.cpp`*

### Updating the frontend
*(VI) Sửa frontend*

- Prefer editing `resources/ui.html`
- Rebuild to regenerate `embedded/ui_html.h`
- Do not edit `embedded/ui_html.h` directly

*(VI)*
- *(VI) Ưu tiên sửa `resources/ui.html`*
- *(VI) Build lại để sinh `embedded/ui_html.h` mới*
- *(VI) Không sửa trực tiếp `embedded/ui_html.h`*

### Updating build configuration
*(VI) Sửa cấu hình build*

- Entrypoints and implementation sources are compiled from `src/`
- Public headers are organized under `include/` subfolders
- When adding new source files, also update `CMakeLists.txt`

*(VI)*
- *(VI) Source entry/implementation build từ thư mục `src/`*
- *(VI) Header public được phân loại theo `include/`*
- *(VI) Nếu thêm file source mới, nhớ cập nhật `CMakeLists.txt`*

## Validation
*(VI) Cách kiểm tra/verify*

This repo currently has no automated test framework; common validation is:

*(VI) Repo hiện chưa có test tự động; cách verify thường dùng:*

- Rebuild: `cmake --build build_new --config Release`
- Run: `.\build_new\bin\Release\WebView2Demo.exe`
- Verify the UI loads correctly
- Verify affected activity features, hook flow, and response handling

*(VI)*
- *(VI) Build lại: `cmake --build build_new --config Release`*
- *(VI) Chạy: `.\build_new\bin\Release\WebView2Demo.exe`*
- *(VI) Kiểm tra UI load bình thường*
- *(VI) Kiểm tra activity/hook/response handling còn hoạt động đúng*

## FAQ
*(VI) Câu hỏi thường gặp*

### Build can't find vcpkg dependencies
*(VI) Build không tìm thấy dependency vcpkg*

Check whether `VCPKG_ROOT` in `CMakeLists.txt` matches your local path.

*(VI) Kiểm tra `VCPKG_ROOT` trong `CMakeLists.txt` có đúng đường dẫn máy bạn không.*

### Frontend changes not taking effect
*(VI) Sửa frontend nhưng không thấy hiệu lực*

Make sure you rebuilt so `resources/ui.html` gets regenerated into `embedded/ui_html.h`.

*(VI) Hãy chắc chắn bạn đã build lại để `resources/ui.html` được sinh lại vào `embedded/ui_html.h`.*

### New source file not compiled
*(VI) Thêm file source mới nhưng build không include*

Confirm the file is in the right directory and has been added to `CMakeLists.txt`.

*(VI) Xác nhận file đặt đúng thư mục và đã thêm vào `CMakeLists.txt`.*

## License
*(VI) Giấy phép*

MIT License

## Links
*(VI) Liên kết*

- GitHub repo: `https://github.com/lllcc666/kbxyfywd`
- Issues: `https://github.com/lllcc666/kbxyfywd/issues`
