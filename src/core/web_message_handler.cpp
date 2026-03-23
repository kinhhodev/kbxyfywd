#include "web_message_handler.h"

#include <windows.h>
#include <WebView2.h>
#include <shellapi.h>
#include <exdisp.h>
#include <atlbase.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "battle_six.h"
#include "dungeon_jump.h"
#include "activity_minigames.h"
#include "horse_competition.h"
#include "shuangtai.h"
#include "wpe_hook.h"
#include "spirit_collect.h"
#include "utils.h"
#include "ui_bridge.h"

extern CComPtr<IWebBrowser2> g_pWebBrowser;
extern HWND g_hWnd;
extern HWND g_hwnd_webbrowser;
extern std::wstring g_loginKey;
extern bool g_is_packet_window_visible;
extern RECT g_packet_window_rect;

typedef void (__stdcall *PFN_INITIALIZESPEEDHACK)(float);
extern PFN_INITIALIZESPEEDHACK g_pfnInitializeSpeedhack;

bool LoadSpeedhackFromMemory();
bool ToggleProgramVolume();
bool PostScriptToUI(const std::wstring& jsCode);
HRESULT WINAPI ExecuteScriptInWebView2(const WCHAR* script);
void UpdateIEEWindowRegion();
std::wstring ShowSaveFileDialog(const std::wstring& initialDir = L"",
                                const std::wstring& defaultFileName = L"",
                                const std::wstring& filter = L"");
std::wstring ShowOpenFileDialog(const std::wstring& initialDir = L"",
                                const std::wstring& filter = L"");
BOOL SavePacketListToFile(const std::wstring& filePath);
BOOL LoadPacketListFromFile(const std::wstring& filePath);

namespace {

bool TryParseIntInRangeLocal(const std::wstring& text, int minValue, int maxValue, int defaultValue, int& value) {
    const size_t start = text.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) {
        value = defaultValue;
        return false;
    }

    const size_t endPos = text.find_last_not_of(L" \t\r\n");
    const std::wstring trimmed = text.substr(start, endPos - start + 1);

    wchar_t* end = nullptr;
    long parsed = std::wcstol(trimmed.c_str(), &end, 10);
    if (end == trimmed.c_str() || *end != L'\0') {
        value = defaultValue;
        return false;
    }

    if (parsed < minValue) {
        value = minValue;
    } else if (parsed > maxValue) {
        value = maxValue;
    } else {
        value = static_cast<int>(parsed);
    }
    return true;
}

std::wstring UnescapeSerializedWebMessage(std::wstring msg) {
    if (msg.size() < 2 || msg.front() != L'"' || msg.back() != L'"') {
        return msg;
    }

    std::wstring unescaped;
    unescaped.reserve(msg.size());
    for (size_t i = 1; i + 1 < msg.size(); ++i) {
        if (msg[i] == L'\\' && i + 2 < msg.size()) {
            switch (msg[i + 1]) {
                case L'"': unescaped += L'"'; ++i; break;
                case L'\\': unescaped += L'\\'; ++i; break;
                case L'n': unescaped += L'\n'; ++i; break;
                case L'r': unescaped += L'\r'; ++i; break;
                case L't': unescaped += L'\t'; ++i; break;
                default: unescaped += msg[i]; break;
            }
        } else {
            unescaped += msg[i];
        }
    }
    return unescaped;
}

std::wstring GetJsonValue(const std::wstring& msg, const std::wstring& key) {
    const size_t keyPos = msg.find(L"\"" + key + L"\"");
    if (keyPos == std::wstring::npos) {
        return L"";
    }

    const size_t colonPos = msg.find(L":", keyPos);
    if (colonPos == std::wstring::npos) {
        return L"";
    }

    const size_t valueStart = msg.find_first_not_of(L" \t\n\r", colonPos + 1);
    if (valueStart == std::wstring::npos) {
        return L"";
    }

    if (msg[valueStart] == L'"') {
        const size_t valueEnd = msg.find(L"\"", valueStart + 1);
        if (valueEnd != std::wstring::npos) {
            return msg.substr(valueStart + 1, valueEnd - valueStart - 1);
        }
        return L"";
    }

    if (msg[valueStart] == L'[') {
        size_t bracketCount = 1;
        size_t valueEnd = valueStart + 1;
        while (valueEnd < msg.length() && bracketCount > 0) {
            if (msg[valueEnd] == L'[') {
                ++bracketCount;
            } else if (msg[valueEnd] == L']') {
                --bracketCount;
            }
            ++valueEnd;
        }
        if (bracketCount == 0) {
            return msg.substr(valueStart, valueEnd - valueStart);
        }
        return L"";
    }

    const size_t valueEnd = msg.find_first_of(L",}", valueStart);
    if (valueEnd != std::wstring::npos) {
        return msg.substr(valueStart, valueEnd - valueStart);
    }
    return msg.substr(valueStart);
}

void NavigateBrowser(const std::wstring& url) {
    if (!g_pWebBrowser || url.empty()) {
        return;
    }

    VARIANT vEmpty;
    VariantInit(&vEmpty);
    BSTR bstrURL = SysAllocString(url.c_str());
    if (!bstrURL) {
        return;
    }
    g_pWebBrowser->Navigate(bstrURL, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
    SysFreeString(bstrURL);
}

void SetHelperText(const wchar_t* text) {
    PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('" + std::wstring(text) + L"'); }");
}

void CopyLoginKeyToClipboard() {
    if (g_loginKey.empty() || !OpenClipboard(nullptr)) {
        return;
    }

    EmptyClipboard();
    const size_t len = (g_loginKey.length() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hMem) {
        void* locked = GlobalLock(hMem);
        if (locked) {
            memcpy(locked, g_loginKey.c_str(), len);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        } else {
            GlobalFree(hMem);
        }
    }
    CloseClipboard();
}

void ParseIndicesArray(const std::wstring& msg, std::vector<DWORD>& indices) {
    const size_t keyPos = msg.find(L"\"indices\"");
    if (keyPos == std::wstring::npos) {
        return;
    }

    const size_t colonPos = msg.find(L":", keyPos);
    const size_t leftBracket = msg.find(L"[", colonPos);
    const size_t rightBracket = msg.find(L"]", leftBracket);
    if (leftBracket == std::wstring::npos || rightBracket == std::wstring::npos) {
        return;
    }

    const std::wstring arr = msg.substr(leftBracket + 1, rightBracket - leftBracket - 1);
    size_t itemStart = 0;
    while (itemStart <= arr.length()) {
        const size_t itemEnd = arr.find(L',', itemStart);
        std::wstring num = (itemEnd == std::wstring::npos)
            ? arr.substr(itemStart)
            : arr.substr(itemStart, itemEnd - itemStart);
        itemStart = (itemEnd == std::wstring::npos) ? (arr.length() + 1) : (itemEnd + 1);

        const size_t start = num.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos) {
            continue;
        }
        const size_t end = num.find_last_not_of(L" \t\r\n");
        const int value = _wtoi(num.substr(start, end - start + 1).c_str());
        if (value >= 0) {
            indices.push_back(static_cast<DWORD>(value));
        }
    }
}

class WebMessageHandler : public ICoreWebView2WebMessageReceivedEventHandler {
public:
    WebMessageHandler() : m_refCount(1) {}

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_refCount);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (!ppvObject) {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2WebMessageReceivedEventHandler) {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) override {
        (void)sender;
        if (!args) {
            return E_POINTER;
        }

        PWSTR message = nullptr;
        const HRESULT hr = args->get_WebMessageAsJson(&message);
        if (FAILED(hr) || !message) {
            return hr;
        }

        std::wstring msg = UnescapeSerializedWebMessage(std::wstring(message));
        CoTaskMemFree(message);

        if (msg.find(L"window-drag") != std::wstring::npos) {
            ReleaseCapture();
            SendMessage(g_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        } else if (msg.find(L"window-minimize") != std::wstring::npos) {
            ShowWindow(g_hWnd, SW_MINIMIZE);
        } else if (msg.find(L"window-close") != std::wstring::npos) {
            PostMessage(g_hWnd, WM_CLOSE, 0, 0);
        } else if (msg.find(L"update-dialog-show") != std::wstring::npos) {
            if (g_hwnd_webbrowser) ShowWindow(g_hwnd_webbrowser, SW_HIDE);
        } else if (msg.find(L"update-dialog-hide") != std::wstring::npos) {
            if (g_hwnd_webbrowser) ShowWindow(g_hwnd_webbrowser, SW_SHOW);
        } else if (msg.find(L"open-url") != std::wstring::npos) {
            std::wstring url = GetJsonValue(msg, L"url");
            if (!url.empty()) ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } else if (msg.find(L"refresh-game") != std::wstring::npos) {
            NavigateBrowser(L"http://news.4399.com/login/kbxy.html");
        } else if (msg.find(L"refresh-no-login") != std::wstring::npos) {
            if (g_pWebBrowser) g_pWebBrowser->Refresh();
        } else if (msg.find(L"mute-game") != std::wstring::npos) {
            ToggleProgramVolume();
        } else if (msg.find(L"copy-login-key") != std::wstring::npos) {
            CopyLoginKeyToClipboard();
        } else if (msg.find(L"key-login-dialog-show") != std::wstring::npos) {
            ShowWindow(g_hwnd_webbrowser, SW_HIDE);
        } else if (msg.find(L"key-login-dialog-hide") != std::wstring::npos) {
            ShowWindow(g_hwnd_webbrowser, SW_SHOW);
        } else if (msg.find(L"spirit-confirm-dialog-show") != std::wstring::npos) {
            if (g_hwnd_webbrowser) ShowWindow(g_hwnd_webbrowser, SW_HIDE);
        } else if (msg.find(L"spirit-confirm-dialog-hide") != std::wstring::npos) {
            if (g_hwnd_webbrowser) ShowWindow(g_hwnd_webbrowser, SW_SHOW);
        } else if (msg.find(L"key-login") != std::wstring::npos) {
            const std::wstring key = GetJsonValue(msg, L"key");
            if (!key.empty()) NavigateBrowser(BuildLoginUrl(key));
        } else if (msg.find(L"packet_window_visible") != std::wstring::npos) {
            const bool visible = (GetJsonValue(msg, L"visible") == L"true");
            g_is_packet_window_visible = visible;
            if (visible) {
                g_packet_window_rect.left = _wtoi(GetJsonValue(msg, L"left").c_str());
                g_packet_window_rect.top = _wtoi(GetJsonValue(msg, L"top").c_str());
                g_packet_window_rect.right = _wtoi(GetJsonValue(msg, L"width").c_str());
                g_packet_window_rect.bottom = _wtoi(GetJsonValue(msg, L"height").c_str());
                SyncPacketsToUI();
            } else {
                const std::wstring js = L"if(window.updatePacketCount) { window.updatePacketCount(" + std::to_wstring(GetPacketCount()) + L"); }";
                ExecuteScriptInWebView2(js.c_str());
            }
            UpdateIEEWindowRegion();
        } else if (msg.find(L"delete_selected_packets") != std::wstring::npos) {
            std::vector<DWORD> indices;
            ParseIndicesArray(msg, indices);
            if (!indices.empty()) {
                DeleteSelectedPackets(indices);
            }
            static const wchar_t* clearScript = LR"(
                (function(){
                    var pListItems = document.getElementById('packet-list-items');
                    if (pListItems) {
                        pListItems.innerHTML = '';
                    }
                    if (window.updatePacketCount) {
                        var count = pListItems ? pListItems.children.length : 0;
                        window.updatePacketCount(count);
                    }
                })();
            )";
            ExecuteScriptInWebView2(clearScript);
            SyncPacketsToUI();
        } else if (msg.find(L"clear_packets") != std::wstring::npos) {
            ClearPacketList();
        } else if (msg.find(L"start_intercept") != std::wstring::npos) {
            StartIntercept();
        } else if (msg.find(L"stop_intercept") != std::wstring::npos) {
            StopIntercept();
        } else if (msg.find(L"set_intercept_type") != std::wstring::npos) {
            SetInterceptType(GetJsonValue(msg, L"send") == L"true", GetJsonValue(msg, L"recv") == L"true");
        } else if (msg.find(L"send_packet") != std::wstring::npos) {
            const std::wstring hexW = GetJsonValue(msg, L"hex");
            if (!hexW.empty()) {
                struct SendThreadData { std::vector<BYTE> data; };
                SendThreadData* pData = new SendThreadData{StringToHex(WideToUtf8(hexW))};
                CreateThread(nullptr, 0, [](LPVOID lpParam) -> DWORD {
                    SendThreadData* pD = static_cast<SendThreadData*>(lpParam);
                    BOOL result = SendPacket(0, pD->data.data(), static_cast<DWORD>(pD->data.size()));
                    if (!result) {
                        PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('封包发送失败：未连接到游戏服务器'); }");
                    }
                    delete pD;
                    return 0;
                }, pData, 0, nullptr);
            } else {
                PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('封包数据格式错误'); }");
            }
        } else if (msg.find(L"set_speed") != std::wstring::npos) {
            if (LoadSpeedhackFromMemory() && g_pfnInitializeSpeedhack) {
                g_pfnInitializeSpeedhack(static_cast<float>(_wtof(GetJsonValue(msg, L"speed").c_str())));
            }
        } else if (msg.find(L"toggle_auto_heal") != std::wstring::npos) {
            g_autoHeal = (GetJsonValue(msg, L"enabled") == L"true");
        } else if (msg.find(L"set_block_battle") != std::wstring::npos) {
            g_blockBattle = (GetJsonValue(msg, L"enabled") == L"true");
        } else if (msg.find(L"set_auto_go_home") != std::wstring::npos) {
            g_autoGoHome = (GetJsonValue(msg, L"enabled") == L"true");
        } else if (msg.find(L"query_lingyu") != std::wstring::npos) {
            SendQueryLingyuPacket();
        } else if (msg.find(L"query_monsters") != std::wstring::npos) {
            SendQueryMonsterPacket();
        } else if (msg.find(L"refresh_pack_items") != std::wstring::npos) {
            SendReqPackageDataPacket(0xFFFFFFFF);
        } else if (msg.find(L"buy_item") != std::wstring::npos) {
            const uint32_t itemId = static_cast<uint32_t>(_wtol(GetJsonValue(msg, L"itemId").c_str()));
            const uint32_t count = [&]() -> uint32_t {
                const std::wstring value = GetJsonValue(msg, L"count");
                return value.empty() ? 1U : static_cast<uint32_t>(_wtol(value.c_str()));
            }();
            if (itemId > 0) {
                SendReqPackageDataPacket(0xFFFFFFFF);
                Sleep(200);
                if (SendBuyGoodsPacket(itemId, count)) {
                    wchar_t buffer[128];
                    swprintf_s(buffer, L"购买道具成功: %s x%u", GetItemName(itemId).c_str(), count);
                    PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('" + std::wstring(buffer) + L"'); }");
                }
            }
        } else if (msg.find(L"use_item") != std::wstring::npos) {
            const uint32_t itemId = static_cast<uint32_t>(_wtol(GetJsonValue(msg, L"itemId").c_str()));
            if (itemId > 0) {
                if (!g_battleStarted) {
                    PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('使用道具需要在战斗中进行'); }");
                } else {
                    SendReqPackageDataPacket(0xFFFFFFFF);
                    Sleep(200);
                    if (SendUseItemInBattlePacket(itemId)) {
                        wchar_t buffer[128];
                        swprintf_s(buffer, L"使用道具: %s", GetItemName(itemId).c_str());
                        PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('" + std::wstring(buffer) + L"'); }");
                    } else {
                        PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('使用道具失败，背包中无此道具'); }");
                    }
                }
            }
        } else if (msg.find(L"daily_tasks") != std::wstring::npos) {
            SendDailyTasksAsync(static_cast<DWORD>(_wtol(GetJsonValue(msg, L"flags").c_str())));
        } else if (msg.find(L"one_key_collect") != std::wstring::npos) {
            const DWORD flags = static_cast<DWORD>(_wtol(GetJsonValue(msg, L"flags").c_str()));
            if (SendOneKeyCollectPacket(flags)) {
                SetHelperText(L"一键采集已开始，请查看辅助日志");
            } else {
                SetHelperText(L"一键采集启动失败，可能已经在运行或未进入游戏");
            }
        } else if (msg.find(L"buy_dice_18") != std::wstring::npos) {
            if (SendBuyDicePacket()) SetHelperText(L"已购买18个骰子");
        } else if (msg.find(L"buy_dice") != std::wstring::npos) {
            SendBuyDicePacket();
        } else if (msg.find(L"one_key_xuantta") != std::wstring::npos) {
            if (SendOneKeyTowerPacket()) {
                SetHelperText(L"一键玄塔已开始，请查看辅助日志");
            } else {
                SetHelperText(L"一键玄塔启动失败，可能已经在运行或未进入游戏");
            }
        } else if (msg.find(L"query_shuangtai") != std::wstring::npos) {
            QueryShuangTaiMonsters();
        } else if (msg.find(L"start_shuangtai") != std::wstring::npos) {
            std::wstring blockBattleStr = GetJsonValue(msg, L"blockBattle");
            while (!blockBattleStr.empty() && (blockBattleStr.front() == L' ' || blockBattleStr.front() == L'\t')) blockBattleStr.erase(0, 1);
            while (!blockBattleStr.empty() && (blockBattleStr.back() == L' ' || blockBattleStr.back() == L'\t')) blockBattleStr.pop_back();
            if (SendOneKeyShuangTaiPacket(blockBattleStr == L"true")) {
                SetHelperText(L"双台谷刷级已开始，请查看辅助日志");
            } else {
                SetHelperText(L"双台谷刷级启动失败，请先点击查询按钮获取妖怪数据");
            }
        } else if (msg.find(L"stop_shuangtai") != std::wstring::npos) {
            StopShuangTai();
            SetHelperText(L"双台谷刷级已停止");
        } else if (msg.find(L"one_key_strawberry") != std::wstring::npos) {
            std::wstring sweepStr = GetJsonValue(msg, L"sweep");
            while (!sweepStr.empty() && (sweepStr.front() == L' ' || sweepStr.front() == L'\t')) sweepStr.erase(0, 1);
            while (!sweepStr.empty() && (sweepStr.back() == L' ' || sweepStr.back() == L'\t')) sweepStr.pop_back();
            const bool useSweep = (sweepStr == L"true");
            if (SendOneKeyStrawberryPacket(useSweep)) {
                SetHelperText(useSweep ? L"采摘红莓果已开始（扫荡模式），请查看辅助日志" : L"采摘红莓果已开始，请查看辅助日志");
            } else {
                SetHelperText(L"采摘红莓果启动失败，可能已经在运行或未进入游戏");
            }
        } else if (msg.find(L"battlesix_auto_match") != std::wstring::npos) {
            std::wstring matchCountStr = GetJsonValue(msg, L"matchCount");
            while (!matchCountStr.empty() && (matchCountStr.front() == L' ' || matchCountStr.front() == L'\t')) matchCountStr.erase(0, 1);
            while (!matchCountStr.empty() && (matchCountStr.back() == L' ' || matchCountStr.back() == L'\t')) matchCountStr.pop_back();
            int matchCount = 1;
            TryParseIntInRangeLocal(matchCountStr, 1, 999, 1, matchCount);
            wchar_t startMsg[128];
            swprintf_s(startMsg, L"万妖盛会：开始匹配（共%d次）...", matchCount);
            PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('" + std::wstring(startMsg) + L"'); }");
            int* pMatchCount = new int(matchCount);
            HANDLE hThread = CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
                int count = *static_cast<int*>(param);
                delete static_cast<int*>(param);
                SendOneKeyBattleSixPacket(count);
                return 0;
            }, pMatchCount, 0, nullptr);
            if (hThread) CloseHandle(hThread);
        } else if (msg.find(L"battlesix_cancel_match") != std::wstring::npos) {
            g_battleSixAuto.SetAutoMatching(false);
            g_battleSixAuto.SetMatchCount(0);
            SetHelperText(SendCancelBattleSixPacket() ? L"万妖盛会：已取消匹配" : L"万妖盛会：取消匹配失败");
        } else if (msg.find(L"battlesix_set_auto_battle") != std::wstring::npos) {
            std::wstring enabledStr = GetJsonValue(msg, L"enabled");
            while (!enabledStr.empty() && (enabledStr.front() == L' ' || enabledStr.front() == L'\t')) enabledStr.erase(0, 1);
            while (!enabledStr.empty() && (enabledStr.back() == L' ' || enabledStr.back() == L'\t')) enabledStr.pop_back();
            const bool enabled = (enabledStr == L"true");
            g_battleSixAuto.SetAutoBattle(enabled);
            PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('调试：enabled=[" + enabledStr + L"] parsed=" + std::wstring(enabled ? L"true" : L"false") + L"'); }");
            SetHelperText(enabled ? L"万妖盛会：自动战斗已启用" : L"万妖盛会：自动战斗已禁用");
        } else if (msg.find(L"dungeon_jump_start") != std::wstring::npos) {
            std::wstring layerStr = GetJsonValue(msg, L"targetLayer");
            while (!layerStr.empty() && (layerStr.front() == L' ' || layerStr.front() == L'\t')) layerStr.erase(0, 1);
            while (!layerStr.empty() && (layerStr.back() == L' ' || layerStr.back() == L'\t')) layerStr.pop_back();
            int targetLayer = 1;
            TryParseIntInRangeLocal(layerStr, 1, 9999, 1, targetLayer);
            PostScriptToUI(L"if(window.updateDungeonJumpStatus) { window.updateDungeonJumpStatus('副本跳层：准备跳转到第" + std::to_wstring(targetLayer) + L"层...'); }");
            int* pTargetLayer = new int(targetLayer);
            HANDLE hThread = CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
                int layer = *static_cast<int*>(param);
                delete static_cast<int*>(param);
                SendOneKeyDungeonJumpPacket(layer);
                return 0;
            }, pTargetLayer, 0, nullptr);
            if (hThread) CloseHandle(hThread);
        } else if (msg.find(L"dungeon_jump_stop") != std::wstring::npos) {
            StopDungeonJump();
            PostScriptToUI(L"if(window.updateDungeonJumpStatus) { window.updateDungeonJumpStatus('副本跳层：已停止'); }");
        } else if (msg.find(L"one_key_act778") != std::wstring::npos) {
            std::wstring sweepStr = GetJsonValue(msg, L"sweep");
            while (!sweepStr.empty() && (sweepStr.front() == L' ' || sweepStr.front() == L'\t')) sweepStr.erase(0, 1);
            while (!sweepStr.empty() && (sweepStr.back() == L' ' || sweepStr.back() == L'\t')) sweepStr.pop_back();
            const bool useSweep = (sweepStr == L"true");
            if (SendOneKeyAct778Packet(useSweep)) {
                SetHelperText(useSweep ? L"驱傩聚福寿已开始（扫荡模式），请查看辅助日志" : L"驱傩聚福寿已开始（最高分模式），请查看辅助日志");
            } else {
                SetHelperText(L"驱傩聚福寿启动失败");
            }
        } else if (msg.find(L"one_key_act793") != std::wstring::npos) {
            std::wstring sweepStr = GetJsonValue(msg, L"sweep");
            while (!sweepStr.empty() && (sweepStr.front() == L' ' || sweepStr.front() == L'\t')) sweepStr.erase(0, 1);
            while (!sweepStr.empty() && (sweepStr.back() == L' ' || sweepStr.back() == L'\t')) sweepStr.pop_back();
            int targetMedals = Act793::TARGET_MEDALS;
            const std::wstring medalsStr = GetJsonValue(msg, L"medals");
            if (!medalsStr.empty()) {
                TryParseIntInRangeLocal(medalsStr, 1, 100, Act793::TARGET_MEDALS, targetMedals);
            }
            if (SendOneKeyAct793Packet(sweepStr == L"true", targetMedals)) {
                SetHelperText((sweepStr == L"true") ? L"磐石御天火已开始（扫荡模式），请查看辅助日志" : L"磐石御天火已开始（游戏模式），请查看辅助日志");
            } else {
                SetHelperText(L"磐石御天火启动失败");
            }
        } else if (msg.find(L"one_key_act791") != std::wstring::npos) {
            std::wstring sweepStr = GetJsonValue(msg, L"sweep");
            while (!sweepStr.empty() && (sweepStr.front() == L' ' || sweepStr.front() == L'\t')) sweepStr.erase(0, 1);
            while (!sweepStr.empty() && (sweepStr.back() == L' ' || sweepStr.back() == L'\t')) sweepStr.pop_back();
            int targetScore = Act791::TARGET_SCORE;
            const std::wstring scoreStr = GetJsonValue(msg, L"score");
            if (!scoreStr.empty()) {
                TryParseIntInRangeLocal(scoreStr, 1, 250, Act791::TARGET_SCORE, targetScore);
            }
            if (SendOneKeyAct791Packet(sweepStr == L"true", targetScore)) {
                SetHelperText((sweepStr == L"true") ? L"五行镜破封印已开始（扫荡模式），请查看辅助日志" : L"五行镜破封印已开始（游戏模式），请查看辅助日志");
            } else {
                SetHelperText(L"五行镜破封印启动失败");
            }
        } else if (msg.find(L"one_key_horse_competition") != std::wstring::npos) {
            SetHelperText(L"坐骑大赛已开始（临时坐骑），请等待...");
            SetHorseProgressCallback([](const std::wstring& progress) {
                PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('坐骑大赛：" + progress + L"'); }");
            });
            std::thread([]() {
                SendOneKeyHorseCompetitionPacket(true);
            }).detach();
        } else if (msg.find(L"stop_horse_competition") != std::wstring::npos) {
            RequestStopHorseCompetition();
        } else if (msg.find(L"start_heaven_furui") != std::wstring::npos) {
            int maxBoxes = 30;
            const std::wstring maxBoxesStr = GetJsonValue(msg, L"maxBoxes");
            if (!maxBoxesStr.empty()) {
                TryParseIntInRangeLocal(maxBoxesStr, 1, 9999, 30, maxBoxes);
            }
            if (SendOneKeyHeavenFuruiPacket(maxBoxes)) {
                SetHelperText(L"福瑞宝箱：开始遍历地图查找宝箱...");
            } else {
                SetHelperText(L"福瑞宝箱启动失败");
            }
        } else if (msg.find(L"stop_heaven_furui") != std::wstring::npos) {
            StopHeavenFurui();
            SetHelperText(L"福瑞宝箱：已停止");
        } else if (msg.find(L"decompose_lingyu") != std::wstring::npos) {
            if (msg.find(L"decompose_lingyu_batch") != std::wstring::npos) {
                const std::wstring jsonArray = GetJsonValue(msg, L"indices");
                if (!jsonArray.empty()) {
                    SendDecomposeLingyuPacket(jsonArray);
                }
            } else {
                const std::wstring jsonArray = GetJsonValue(msg, L"indices_array");
                if (!jsonArray.empty()) {
                    SendDecomposeLingyuPacket(jsonArray);
                }
            }
        } else if (msg.find(L"set_hijack_enabled") != std::wstring::npos) {
            SetHijackEnabled(GetJsonValue(msg, L"enabled") == L"true");
        } else if (msg.find(L"add_hijack_rule") != std::wstring::npos) {
            const bool isSend = (GetJsonValue(msg, L"isSend") == L"true");
            const bool isBlock = (GetJsonValue(msg, L"isBlock") == L"true");
            AddHijackRule(isBlock ? HIJACK_BLOCK : HIJACK_REPLACE,
                          isSend,
                          !isSend,
                          WideToUtf8(GetJsonValue(msg, L"pattern")),
                          WideToUtf8(GetJsonValue(msg, L"replace")));
        } else if (msg.find(L"clear_hijack_rules") != std::wstring::npos) {
            ClearHijackRules();
        } else if (msg.find(L"save_packet_list") != std::wstring::npos) {
            const std::wstring filePath = ShowSaveFileDialog(L"", L"packets.txt");
            if (!filePath.empty()) {
                SetHelperText(SavePacketListToFile(filePath) ? L"封包列表已保存" : L"保存封包列表失败");
            }
        } else if (msg.find(L"load_packet_list") != std::wstring::npos) {
            const std::wstring filePath = ShowOpenFileDialog(L"");
            if (!filePath.empty()) {
                if (LoadPacketListFromFile(filePath)) {
                    SyncPacketsToUI();
                    SetHelperText(L"封包列表已载入");
                } else {
                    SetHelperText(L"载入封包列表失败");
                }
            }
        } else if (msg.find(L"send_all_packets") != std::wstring::npos) {
            DWORD sendCount = 1;
            DWORD sendDelay = 300;
            const std::wstring sendCountStr = GetJsonValue(msg, L"sendCount");
            if (!sendCountStr.empty()) {
                sendCount = static_cast<DWORD>(_wtoi(sendCountStr.c_str()));
                if (sendCount < 1) sendCount = 1;
            }
            const std::wstring sendDelayStr = GetJsonValue(msg, L"sendDelay");
            if (!sendDelayStr.empty()) {
                sendDelay = static_cast<DWORD>(_wtoi(sendDelayStr.c_str()));
            }
            std::thread([sendCount, sendDelay]() {
                SendAllPackets(sendDelay, sendCount, [](DWORD currentLoop, DWORD packetIndex, const std::string& label) {
                    std::wstring script = L"if(window.updateHelperText) { window.updateHelperText('正在发送第" +
                        std::to_wstring(currentLoop) + L"次，第" + std::to_wstring(packetIndex) + L"个封包";
                    if (!label.empty()) {
                        script += L"，标签：" + Utf8ToWide(label);
                    }
                    script += L"'); }";
                    PostScriptToUI(script);
                });
                PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('封包发送完成'); }");
            }).detach();
        } else if (msg.find(L"stop_send") != std::wstring::npos) {
            StopAutoSend();
            SetHelperText(L"已停止发送封包");
        } else if (msg.find(L"enter_boss_battle") != std::wstring::npos) {
            const uint32_t spiritId = static_cast<uint32_t>(_wtol(GetJsonValue(msg, L"bossId").c_str()));
            if (spiritId > 10000) {
                if (SendBattlePacket(spiritId, 32, 0)) {
                    PostScriptToUI(L"if(window.updateHelperText) { window.updateHelperText('已发送BOSS战斗封包，ID: " + std::to_wstring(spiritId) + L", useId: 32'); }");
                } else {
                    SetHelperText(L"发送战斗封包失败，未连接到游戏服务器");
                }
            } else {
                SetHelperText(L"无效的对象ID，必须大于10000");
            }
        } else if (msg.find(L"spiritCollect") != std::wstring::npos) {
            // 精魄系统消息处理
            const std::wstring action = GetJsonValue(msg, L"action");
            if (action == L"open_ui") {
                // 打开精魄系统 UI
                if (SendSpiritOpenUIPacket()) {
                    SetHelperText(L"精魄系统：正在获取数据...");
                } else {
                    SetHelperText(L"精魄系统：发送请求失败");
                }
            } else if (action == L"getSpirits") {
                // 获取精魄列表
                if (SendSpiritPresuresPacket()) {
                    SetHelperText(L"精魄系统：正在获取精魄列表...");
                } else {
                    SetHelperText(L"精魄系统：获取精魄列表失败");
                }
            } else if (action == L"verifyPlayer") {
                // 验证玩家信息（通过卡布号）
                const std::wstring friendIdStr = GetJsonValue(msg, L"friendId");
                if (!friendIdStr.empty()) {
                    uint32_t friendId = static_cast<uint32_t>(_wtol(friendIdStr.c_str()));
                    g_spiritCollectState.selectedFriendId = friendId;
                    if (SendSpiritPlayerInfoPacket(friendId)) {
                        SetHelperText(L"精魄系统：正在验证玩家信息...");
                    } else {
                        SetHelperText(L"精魄系统：验证玩家信息失败");
                    }
                }
            } else if (action == L"sendSpirit") {
                // 发送精魄
                const std::wstring friendIdStr = GetJsonValue(msg, L"friendId");
                const std::wstring eggIdStr = GetJsonValue(msg, L"eggId");
                if (!friendIdStr.empty() && !eggIdStr.empty()) {
                    uint32_t friendId = static_cast<uint32_t>(_wtol(friendIdStr.c_str()));
                    uint32_t eggId = static_cast<uint32_t>(_wtol(eggIdStr.c_str()));
                    if (SendSpiritGiftPacket(friendId, eggId)) {
                        SetHelperText(L"精魄系统：正在发送精魄...");
                    } else {
                        SetHelperText(L"精魄系统：发送精魄失败");
                    }
                }
            } else if (action == L"history") {
                // 获取历史记录
                const std::wstring typeStr = GetJsonValue(msg, L"recordType");
                if (!typeStr.empty()) {
                    int type = _wtoi(typeStr.c_str());
                    if (SendSpiritHistoryPacket(type)) {
                        SetHelperText(L"精魄系统：正在获取历史记录...");
                    } else {
                        SetHelperText(L"精魄系统：获取历史记录失败");
                    }
                }
            }
        }

        return S_OK;
    }

private:
    ULONG m_refCount;
};

}  // namespace

ICoreWebView2WebMessageReceivedEventHandler* CreateWebMessageHandler() {
    return new WebMessageHandler();
}
