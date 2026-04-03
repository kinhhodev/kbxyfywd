cmake_minimum_required(VERSION 3.16)

project(WebView2Demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Experimental migration switch. OFF by default to avoid impacting
# the existing Windows-only build pipeline.
option(ENABLE_MACOS_EXPERIMENT "Enable macOS migration scaffolding hooks" OFF)
if(ENABLE_MACOS_EXPERIMENT)
    message(STATUS "ENABLE_MACOS_EXPERIMENT=ON (scaffolding only, no macOS target yet)")
endif()

# This repository is currently Windows-first.
# On non-Windows platforms (e.g. macOS), stop before generating Windows-only
# targets/dependencies to avoid invalid Makefile rules from Windows paths.
if(NOT WIN32)
    add_custom_target(kbxyfywd_macos_stub
        COMMAND ${CMAKE_COMMAND} -E echo "Opening resources/ui.html in default browser..."
        COMMAND /bin/sh -c "open \"${CMAKE_CURRENT_SOURCE_DIR}/resources/ui.html\" || true"
        COMMAND ${CMAKE_COMMAND} -E echo "Stub mode completed. See docs/macos-migration-blueprint.md"
        VERBATIM
    )
    return()
endif()

# Project directory layout
# (VI) Cấu trúc thư mục dự án
set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(EMBEDDED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/embedded)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add executable target (Win32 desktop app)
# (VI) Thêm target executable (ứng dụng desktop Win32)
add_executable(WebView2Demo WIN32
    ${SRC_DIR}/app/demo.cpp
    ${SRC_DIR}/app/app_host.cpp
    ${SRC_DIR}/hook/wpe_hook.cpp
    ${SRC_DIR}/hook/horse_competition.cpp
    ${SRC_DIR}/hook/wpe_hook_helpers.cpp
    ${SRC_DIR}/core/utils.cpp
    ${SRC_DIR}/protocol/packet_parser.cpp
    ${SRC_DIR}/protocol/packet_builder.cpp
    ${SRC_DIR}/core/ui_bridge.cpp
    ${SRC_DIR}/core/web_message_handler.cpp
)

# Force UTF-8 source encoding for compiler
# (VI) Bật encoding UTF-8 cho compiler
if(MSVC)
    target_compile_options(WebView2Demo PRIVATE /utf-8)
endif()

# Configure vcpkg paths
# (VI) Cấu hình đường dẫn vcpkg
set(VCPKG_ROOT "d:/AItrace/CE/.trae/vcpkg-master")
set(VCPKG_INCLUDE_DIR "${VCPKG_ROOT}/installed/x64-windows/include")
set(VCPKG_LIB_DIR "${VCPKG_ROOT}/installed/x64-windows/lib")

# Include WebView2 headers and embedded-data directories
# (VI) Include thư mục header WebView2 và dữ liệu nhúng
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${VCPKG_INCLUDE_DIR})
include_directories(${EMBEDDED_DIR})
include_directories(${INCLUDE_DIR}/activities)
include_directories(${INCLUDE_DIR}/core)
include_directories(${INCLUDE_DIR}/hook)
include_directories(${INCLUDE_DIR}/internal)
include_directories(${INCLUDE_DIR}/protocol)
include_directories(${INCLUDE_DIR}/platform)

# Link required system and third-party libraries
# (VI) Link các thư viện hệ thống và bên thứ ba cần thiết
target_link_libraries(WebView2Demo PRIVATE
    ComCtl32.lib
    Comdlg32.lib
    dwmapi.lib
    mmdevapi.lib
    oleaut32.lib
    ws2_32.lib
    ole32.lib
    shell32.lib
    shlwapi.lib
    uuid.lib
    user32.lib
    advapi32.lib
    msimg32.lib
    gdi32.lib
    winhttp.lib
    wininet.lib
    winmm.lib
    "${VCPKG_LIB_DIR}/MemoryModule.lib"
    "${VCPKG_LIB_DIR}/zlib.lib"
)

# Add resource file and set proper resource dependencies
# (VI) Thêm file resource và thiết lập dependency phù hợp
set_source_files_properties(${RESOURCES_DIR}/app.rc PROPERTIES 
    OBJECT_DEPENDS "${RESOURCES_DIR}/app.ico"
)
target_sources(WebView2Demo PRIVATE ${RESOURCES_DIR}/app.rc)

# Add include path for the resource compiler
# (VI) Thêm include path cho resource compiler
set_property(SOURCE ${RESOURCES_DIR}/app.rc PROPERTY COMPILE_FLAGS "/I\"${RESOURCES_DIR}\"")

# Custom command: convert WebView2Loader.dll into a C++ header
# (VI) Lệnh tùy chỉnh: chuyển WebView2Loader.dll thành header C++
add_custom_command(
    OUTPUT ${EMBEDDED_DIR}/webview2loader_data.h
    COMMAND python3 ${SCRIPTS_DIR}/embed_dll.py
    "${VCPKG_ROOT}/installed/x64-windows/bin/WebView2Loader.dll"
    ${EMBEDDED_DIR}/webview2loader_data.h
    DEPENDS "${VCPKG_ROOT}/installed/x64-windows/bin/WebView2Loader.dll"
    COMMENT "Embedding WebView2Loader.dll into header file..."
)

# Custom command: convert ui.html into a C++ header
# (VI) Lệnh tùy chỉnh: chuyển ui.html thành header C++
add_custom_command(
    OUTPUT ${EMBEDDED_DIR}/ui_html.h
    COMMAND python3 ${SCRIPTS_DIR}/embed_html.py
    ${RESOURCES_DIR}/ui.html
    ${EMBEDDED_DIR}/ui_html.h
    DEPENDS ${RESOURCES_DIR}/ui.html
    COMMENT "Embedding UI.html into header file..."
)

# Custom command: convert zlib1.dll into a C++ header
# (VI) Lệnh tùy chỉnh: chuyển zlib1.dll thành header C++
add_custom_command(
    OUTPUT ${EMBEDDED_DIR}/zlib_data.h
    COMMAND python3 ${SCRIPTS_DIR}/embed_dll.py
    "${VCPKG_ROOT}/installed/x64-windows/bin/zlib1.dll"
    ${EMBEDDED_DIR}/zlib_data.h
    DEPENDS "${VCPKG_ROOT}/installed/x64-windows/bin/zlib1.dll"
    COMMENT "Embedding zlib1.dll into header file..."
)

# Add generated headers to target sources
# (VI) Thêm các header sinh tự động vào danh sách source
target_sources(WebView2Demo PRIVATE
    ${EMBEDDED_DIR}/webview2loader_data.h
    ${EMBEDDED_DIR}/ui_html.h
    ${EMBEDDED_DIR}/zlib_data.h
)

# Set output directory
# (VI) Thiết lập thư mục output
set_target_properties(WebView2Demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Size-focused Release optimizations (extended)
# (VI) Tối ưu kích thước cho Release (mở rộng)
if(MSVC)
    # Code generation optimizations
    # (VI) Tối ưu sinh mã
    target_compile_options(WebView2Demo PRIVATE 
        /Os        # Optimize for size
        /GL        # Whole program optimization
        /Gw        # Optimize global data
        /GF        # String pooling
        /Gy        # Function-level linking
        /Zc:inline # Remove unreferenced COMDAT
        /Ob2       # Inline expansion (recommended)
        /GS-       # Disable security checks (reduce CRT size)
        /GR-       # Disable RTTI
        /EHsc      # Exception handling optimization
    )
    
    # Link-time optimizations
    # (VI) Tối ưu ở bước linker
    target_link_options(WebView2Demo PRIVATE 
        /LTCG              # Link-time code generation
        /OPT:REF           # Remove unreferenced functions
        /OPT:ICF           # Fold identical functions
        /MERGE:.rdata=.text  # Merge read-only data into .text
        /MERGE:.pdata=.text  # Merge exception metadata
        /DEBUG:NONE        # Disable debug info for smaller binary
        /INCREMENTAL:NO   # Disable incremental linking
    )
    
    # Use dynamic CRT for compatibility with vcpkg libraries
    # (VI) Dùng dynamic CRT để tương thích thư viện từ vcpkg
    # Note: static CRT may conflict with vcpkg-built libraries
    # (VI) Lưu ý: static CRT có thể xung đột với thư viện build từ vcpkg
    set_property(TARGET WebView2Demo PROPERTY 
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()
