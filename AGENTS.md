# AGENTS.md - kbwebui repository guide

## Purpose

- This repository builds a Windows desktop helper for the game using Win32 C++17, WebView2, ATL/WebBrowser, MinHook, and embedded assets.
- The main executable target is `WebView2Demo` defined in `CMakeLists.txt`.
- Agentic tools should prefer minimal, focused edits and preserve existing Chinese UI text, protocol constants, and Windows-specific behavior.

## Rule sources checked

- Existing repo guidance was already present in this file and has been expanded.
- No `.cursor/rules/` directory was found.
- No `.cursorrules` file was found.
- No `.github/copilot-instructions.md` file was found.
- If any of those files are added later, treat them as higher-priority repository instructions and merge them into future edits.

## Tech stack

- Language: C++17 with MSVC.
- App model: Win32 desktop app with a `WIN32` executable target.
- UI: WebView2 hosting HTML/CSS/JavaScript from `resources/ui.html`, embedded into headers at build time.
- Embedded browser/game host: ATL `CAxWindow` + `IWebBrowser2`.
- Hooking: MinHook plus custom socket/game packet interception.
- Compression/loading: zlib and MemoryModule.
- Helper scripts: Python 3 scripts under `scripts/` generate embedded headers.

## Important paths

- `CMakeLists.txt`: build definition, compiler flags, linked libs, embedded-header generation.
- `demo.cpp`: application entry, window lifecycle, WebView2 setup, global state, UI bridge wiring.
- `wpe_hook.h` / `wpe_hook.cpp`: hook lifecycle, packet interception, response dispatch, automation logic.
- `packet_parser.h` / `packet_parser.cpp`: protocol constants, packet parsing, opcode definitions.
- `packet_builder.h` / `packet_builder.cpp`: chainable packet construction helpers.
- `ui_bridge.h` / `ui_bridge.cpp`: C++ to JavaScript bridge.
- `web_message_handler.cpp`: JavaScript to native message handling.
- `data_interceptor.cpp`: HTTP response interception/modification.
- `resources/ui.html`: main frontend UI source.
- `embedded/*.h`: generated binary/header assets; treat as build artifacts unless the task is specifically about generated output.
- `scripts/embed_dll.py`, `scripts/embed_html.py`: generate embedded headers consumed by the C++ target.

## Build commands

Use PowerShell or `cmd.exe` on Windows from the repo root.

```powershell
# Configure a fresh build directory
cmake -S . -B build_new -G "Visual Studio 17 2022" -A x64

# Build Release
cmake --build build_new --config Release

# Build Debug
cmake --build build_new --config Debug

# Run the app
.\build_new\bin\Release\WebView2Demo.exe
```

Notes:

- `CMakeLists.txt` hardcodes `VCPKG_ROOT` to `d:/AItrace/CE/.trae/vcpkg-master`; builds may fail on machines where that path does not exist.
- Build generation depends on Python 3 being available as `python3` because custom commands invoke `python3 scripts/embed_dll.py` and `python3 scripts/embed_html.py`.
- The build emits generated headers into `embedded/` for WebView2Loader, zlib, and UI HTML.

## Lint and formatting commands

- There is no configured repo-wide lint target in `CMakeLists.txt`.
- There is no `.clang-format`, `.editorconfig`, `clang-tidy`, or cpplint configuration checked in.
- There is no dedicated `format` or `lint` script.
- If formatting is needed, keep edits consistent with surrounding code rather than reformatting files wholesale.
- If you introduce local validation commands, prefer non-destructive spot checks such as `cmake --build build_new --config Release` on the touched code.

## Test commands

- There is currently no test framework configured in CMake.
- No `enable_testing()`, `add_test(...)`, GoogleTest, Catch2, or standalone test directories were found.
- `ctest` is therefore not expected to run meaningful tests in the current repository state.
- The practical validation path is to build the target and manually verify the affected workflow in the Windows app.

## Single-test guidance

- There are no automated unit/integration tests to run individually today.
- For an agent asked to "run a single test," explain that no formal test target exists and use the narrowest available validation instead.
- Preferred narrow validation options:
  - rebuild only: `cmake --build build_new --config Release`
  - if a future test target is added, run `ctest -N` first to discover names, then `ctest -R <name> --output-on-failure`
  - for script-only changes, run the specific Python script directly with representative arguments where safe

## Runtime validation suggestions

- Launch `build_new/bin/Release/WebView2Demo.exe` after relevant UI/native changes.
- Verify WebView2 UI still loads and that embedded `resources/ui.html` changes propagated to `embedded/ui_html.h` through the build.
- Verify hook-related changes against the in-app workflow rather than assuming compile success proves correctness.
- Be careful with live game/network automation behavior; prefer isolated validation when possible.

## Code style

### Includes and dependencies

- Follow the established include order: Windows/COM headers first, then standard library headers, then third-party headers, then project headers.
- Prefer `#pragma once` in headers.
- Keep header dependencies narrow; forward declare when practical, especially for COM or project types.
- Do not add new external dependencies unless the task explicitly requires them.

### Formatting

- Match the existing style in each file rather than enforcing a new global format.
- Use 4-space indentation in C++ source/header files.
- Keep braces and wrapping consistent with nearby code; the codebase mixes compact Win32-style and documented class-style declarations.
- Preserve existing Chinese comments and user-facing strings unless the task is to revise them.
- Avoid broad cleanup-only diffs.

### Naming conventions

- Classes and major functions use `PascalCase`, for example `PacketBuilder`, `InitializeHooks`, `CreateWebMessageHandler`.
- Local variables and parameters use `camelCase`.
- Global variables use the `g_` prefix, for example `g_hWnd`, `g_webview`, `g_speedhackModule`.
- Namespaces use `PascalCase`, for example `Opcode`, `PacketProtocol`, `WpeHook`.
- Constants use `UPPER_SNAKE_CASE` for macros and `constexpr` values, for example `HEADER_SIZE`, `TIMEOUT_RESPONSE`, `WM_EXECUTE_JS`.
- Typedef-style Windows aliases such as `PACKET`, `BOOL`, `DWORD`, and callback typedefs are already part of the codebase; preserve consistency when editing adjacent code.

### Types and APIs

- Target C++17; avoid using features that require a newer standard unless CMake is updated too.
- This is a Windows-first codebase: Win32, COM, ATL, and Windows integer types are common and acceptable.
- Use `std::wstring` for Unicode/Chinese-facing Windows/UI strings and `std::string` for protocol or narrow text paths where the surrounding code does so.
- Use fixed-width integer types like `uint32_t`, `uint16_t`, and `int32_t` for packet/protocol structures.
- Keep protocol values little-endian; packet comments and builders assume that convention.
- Prefer RAII wrappers already in use (`CComPtr`, lock helpers, standard containers) over manual lifetime management where possible.

### Error handling

- Match local conventions: Win32/COM-heavy code often returns `BOOL`, `bool`, or `HRESULT`-driven success/failure rather than exceptions.
- Check `HRESULT` with `FAILED`/`SUCCEEDED` and return early on failure.
- Check pointer results from `MemoryLoadLibrary`, `MemoryGetProcAddress`, COM activation, and Win32 handle acquisition before use.
- Keep failure paths simple and explicit; this codebase favors guard clauses over deeply nested control flow.
- Do not introduce exceptions across existing Win32 boundaries unless a file already uses them.

### Comments and documentation

- Many headers use Doxygen-style comments in Chinese; continue that style when adding public APIs to those files.
- Keep comments for non-obvious protocol behavior, hook behavior, or concurrency logic.
- Do not restate obvious code with comments.

### Concurrency and globals

- Global state is common in `demo.cpp` and hook code; modify it carefully and minimize new global coupling.
- Synchronization primitives include `CRITICAL_SECTION`, `CONDITION_VARIABLE`, mutexes, and atomics; use the existing primitive already chosen in the file.
- For packet waiting/dispatch logic, preserve the current thread-safety model and timeout behavior.

### UI and frontend assets

- `resources/ui.html` is source; `embedded/ui_html.h` is generated output.
- Prefer editing `resources/ui.html` and letting the build regenerate the embedded header.
- Preserve the established desktop-tool layout and Chinese UI wording unless the task says otherwise.

### Generated files

- Treat `embedded/webview2loader_data.h`, `embedded/ui_html.h`, and `embedded/zlib_data.h` as generated.
- Do not hand-edit generated headers unless the task is specifically about the generator or a generated artifact check.
- If changing embed behavior, edit the Python generator scripts or source assets instead.

## Domain-specific guidance

- Packet layout is `Magic(2) + Length(2) + Opcode(4) + Params(4) + Body`.
- Magic values are `0x5344` for normal packets and `0x5343` for compressed packets.
- Opcode calculation and protocol numbers are little-endian; preserve existing helper usage instead of rewriting protocol code ad hoc.
- When adding a new game feature, the usual path is: add opcode/constants, add any state structures, implement send/response logic, register handlers, then expose UI actions.

## Agent workflow recommendations

- Before editing, inspect nearby code and follow file-local conventions.
- Prefer minimal diffs in large files like `demo.cpp` and `wpe_hook.cpp`.
- Rebuild after native changes whenever feasible.
- Mention clearly when validation is limited because the repository has no automated tests.
- If future Cursor or Copilot rules appear, update this file so agents have a single consolidated guide.
