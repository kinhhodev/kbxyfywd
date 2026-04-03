# Platform Adapter Layer

This directory will host OS-specific adapter implementations for interfaces in:

- `include/platform/platform_interfaces.h`

Planned subdirectories:

- `src/platform/windows/`
- `src/platform/macos/`

The goal is to keep core logic independent from direct OS APIs.

