# macOS App Host

Current status:

- `kbxyfywd_macos_stub` is a runnable macOS stub target.
- It opens `resources/ui.html` in the default browser.

Build and run on macOS:

```bash
cmake -S . -B build_new
cmake --build build_new
cmake --build build_new --target kbxyfywd_macos_stub
```

Planned direction:

- Host app: Swift + AppKit + WKWebView
- Bridge contract: compatible with current web message schema
- Native core integration: call into extracted cross-platform C++ core

