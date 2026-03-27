#pragma once

#include <windows.h>

namespace AppMessage {

constexpr UINT kExecuteJs = WM_USER + 101;
constexpr UINT kDecomposeComplete = WM_USER + 102;
constexpr UINT kDecomposeHexPacket = WM_USER + 103;
constexpr UINT kDailyTaskComplete = WM_USER + 104;

}  // namespace AppMessage
