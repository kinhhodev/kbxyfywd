cmake_minimum_required(VERSION 3.16)

project(WebView2Demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 项目目录结构
set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(EMBEDDED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/embedded)

# 添加可执行文件（Win32 窗口应用程序）
add_executable(WebView2Demo WIN32
    demo.cpp
    wpe_hook.cpp
    utils.cpp
    packet_parser.cpp
    packet_builder.cpp
    data_interceptor.cpp
    ui_bridge.cpp
)

# 设置编译器使用UTF-8编码
if(MSVC)
    target_compile_options(WebView2Demo PRIVATE /utf-8)
endif()

# 设置 vcpkg 路径
set(VCPKG_ROOT "d:/AItrace/CE/.trae/vcpkg-master")
set(VCPKG_INCLUDE_DIR "${VCPKG_ROOT}/installed/x64-windows/include")
set(VCPKG_LIB_DIR "${VCPKG_ROOT}/installed/x64-windows/lib")

# 包含 WebView2 头文件和嵌入式数据目录
include_directories(${VCPKG_INCLUDE_DIR})
include_directories(${EMBEDDED_DIR})

# 链接其他必要库
target_link_libraries(WebView2Demo PRIVATE
    ws2_32.lib
    ole32.lib
    shell32.lib
    shlwapi.lib
    user32.lib
    advapi32.lib
    msimg32.lib
    gdi32.lib
    winhttp.lib
    wininet.lib
    "${VCPKG_LIB_DIR}/MemoryModule.lib"
    "${VCPKG_LIB_DIR}/minhook.x64.lib"
    "${VCPKG_LIB_DIR}/zlib.lib"
)

# 添加资源文件 - 设置正确的工作目录
set_source_files_properties(${RESOURCES_DIR}/app.rc PROPERTIES 
    OBJECT_DEPENDS "${RESOURCES_DIR}/app.ico"
)
target_sources(WebView2Demo PRIVATE ${RESOURCES_DIR}/app.rc)

# 为资源编译器添加包含目录
set_property(SOURCE ${RESOURCES_DIR}/app.rc PROPERTY COMPILE_FLAGS "/I\"${RESOURCES_DIR}\"")

# 添加自定义命令：将WebView2Loader.dll转换为C++头文件
add_custom_command(
    OUTPUT ${EMBEDDED_DIR}/webview2loader_data.h
    COMMAND python3 ${SCRIPTS_DIR}/embed_dll.py
    "${VCPKG_ROOT}/installed/x64-windows/bin/WebView2Loader.dll"
    ${EMBEDDED_DIR}/webview2loader_data.h
    DEPENDS "${VCPKG_ROOT}/installed/x64-windows/bin/WebView2Loader.dll"
    COMMENT "Embedding WebView2Loader.dll into header file..."
)

# 添加自定义命令：将ui.html转换为C++头文件
add_custom_command(
    OUTPUT ${EMBEDDED_DIR}/ui_html.h
    COMMAND python3 ${SCRIPTS_DIR}/embed_html.py
    ${RESOURCES_DIR}/ui.html
    ${EMBEDDED_DIR}/ui_html.h
    DEPENDS ${RESOURCES_DIR}/ui.html
    COMMENT "Embedding UI.html into header file..."
)

# 添加自定义命令：将zlib1.dll转换为C++头文件
add_custom_command(
    OUTPUT ${EMBEDDED_DIR}/zlib_data.h
    COMMAND python3 ${SCRIPTS_DIR}/embed_dll.py
    "${VCPKG_ROOT}/installed/x64-windows/bin/zlib1.dll"
    ${EMBEDDED_DIR}/zlib_data.h
    DEPENDS "${VCPKG_ROOT}/installed/x64-windows/bin/zlib1.dll"
    COMMENT "Embedding zlib1.dll into header file..."
)

# 将生成的头文件添加到源文件列表
target_sources(WebView2Demo PRIVATE
    ${EMBEDDED_DIR}/webview2loader_data.h
    ${EMBEDDED_DIR}/ui_html.h
    ${EMBEDDED_DIR}/zlib_data.h
)

# 设置输出目录
set_target_properties(WebView2Demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 优化 Release 模式体积（增强版）
if(MSVC)
    # 代码生成优化
    target_compile_options(WebView2Demo PRIVATE 
        /Os        # 优化大小优先
        /GL        # 全局优化
        /Gw        # 优化全局变量
        /GF        # 字符串池化
        /Gy        # 函数级链接
        /Ob2       # 内联展开（推荐）
        /GS-       # 禁用安全检查（减少 CRT 体积）
        /GR-       # 禁用 RTTI
        /EHsc      # 异常处理优化
    )
    
    # 链接优化
    target_link_options(WebView2Demo PRIVATE 
        /LTCG              # 链接时代码生成
        /OPT:REF           # 移除未引用函数
        /OPT:ICF           # 合并相同代码
        /MERGE:.rdata=.text  # 合并只读数据段到代码段
        /MERGE:.pdata=.text  # 合并异常处理数据
        /DEBUG:NONE        # 禁用调试信息，减少体积
        /INCREMENTAL:NO   # 禁用增量链接，减少体积
    )
    
    # 使用动态 CRT（兼容 vcpkg 库）
    # 注意：静态 CRT 会与 vcpkg 编译的库冲突
    set_property(TARGET WebView2Demo PROPERTY 
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()