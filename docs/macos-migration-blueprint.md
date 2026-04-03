# macOS Migration Blueprint (Phase 1)

This document defines a safe migration path from the current Windows-first architecture to a cross-platform architecture with a future macOS app target.

## Goals

- Keep the existing Windows app buildable during migration.
- Extract business/protocol logic into platform-independent modules.
- Add clear platform abstraction boundaries before porting runtime behavior.
- Enable an incremental macOS implementation without a high-risk big-bang rewrite.

## Non-goals (for this phase)

- No functional behavior changes in packet/hook logic yet.
- No replacement for MinHook/macOS injection in this phase.
- No removal of existing Windows APIs in current implementation files.

## Target architecture

- `core/` (cross-platform logic)
  - protocol parsing/building
  - activity state machine and task flow
  - pure business rules
- `platform/` (OS adapters)
  - window host, clipboard, network, timers, shell open
- `apps/windows/` and `apps/macos/`
  - entrypoint and UI host integration
- `hook/` (platform-specific interception)
  - isolated from UI and business-core orchestration

## Incremental milestones

### M1: Boundary extraction

1. Introduce platform interfaces:
   - `IClipboard`
   - `INetworkClient`
   - `IUiDispatcher`
   - `ISystemShell`
   - `ITimerService`
2. Keep current Windows logic as the first adapter implementation.
3. Replace direct Win32 calls in core-facing paths with interface calls.

### M2: Core-first refactor

1. Move protocol and activity orchestration into core modules.
2. Keep serialization formats and opcodes unchanged.
3. Add smoke-level unit tests for parser/builder and state transitions.

### M3: macOS shell bootstrap

1. Create `apps/macos` host app (recommended: Swift + WKWebView).
2. Connect JS bridge with the same message contract as current UI.
3. Provide macOS adapters for platform interfaces.

### M4: Hook parity (separate track)

1. Research feasible macOS interception strategies.
2. Build an isolated proof-of-concept.
3. Integrate only after legal/compliance and stability checks.

## Risk register

- Hooking parity risk is high and should be isolated from Phase 1.
- Current code has global-state coupling; extraction must be staged.
- API/packet behavior must remain protocol-compatible during refactor.

## Acceptance checklist for Phase 1

- [ ] No regressions for current Windows build path.
- [ ] Platform interfaces added and documented.
- [ ] New migration folders exist with clear ownership.
- [ ] No runtime behavior changes introduced.

