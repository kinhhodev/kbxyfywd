/**
 * @file wpe_hook.h
 * @brief WPE网络封包拦截模块
 * 
 * 提供网络封包的拦截、发送、解析等功能
 * 基于 Windows API Hook 技术实现
 */

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <map>
#include <unordered_map>
#include <queue>

// 包含 packet_parser.h 以使用 MonsterItem 等类型
#include "packet_parser.h"

// 前向声明 GamePacket 结构体
struct GamePacket;

// ============================================================================
// 封包数据结构
// ============================================================================

/**
 * @struct PACKET
 * @brief 封包数据结构
 */
typedef struct _PACKET {
    DWORD dwSize;   ///< 数据大小
    BOOL  bSend;    ///< 是否为发送包
    BYTE* pData;    ///< 数据指针
    DWORD dwTime;   ///< 时间戳
} PACKET, *PPACKET;

// ============================================================================
// 类型定义
// ============================================================================

/** 封包回调函数类型 */
typedef void (*PACKET_CALLBACK)(BOOL bSend, const BYTE* pData, DWORD dwSize);

/** 执行JavaScript脚本的函数指针类型 */
typedef HRESULT (*EXECUTE_SCRIPT_FUNC)(const WCHAR* script);

// ============================================================================
// 常量定义
// ============================================================================

namespace WpeHook {

// 跳舞大赛地图ID（炫舞厅）
constexpr int DANCE_MAP_ID = 1028;

// 跳舞大赛每日最大次数
constexpr int DANCE_MAX_DAILY_COUNT = 3;

// 深度挖宝默认次数（每天2次）
constexpr int DEEP_DIG_DEFAULT_COUNT = 2;

// 试炼活动校验码系数
constexpr int TRIAL_CHECK_CODE_COEFF_NORMAL = 56;  // 火风/火焰试炼
constexpr int TRIAL_CHECK_CODE_COEFF_STORM = 72;   // 风暴试炼（系数不同！）

// 常用超时常量（毫秒）
constexpr DWORD TIMEOUT_SHORT = 100;      // 短延迟
constexpr DWORD TIMEOUT_NORMAL = 300;     // 正常延迟
constexpr DWORD TIMEOUT_RESPONSE = 3000;  // 响应等待超时
constexpr DWORD TIMEOUT_SEND = 5000;      // 发送超时
constexpr DWORD TIMEOUT_RETRY_INTERVAL = 500;  // 重试间隔

}  // namespace WpeHook

// ============================================================================
// Windows 自定义消息
// ============================================================================

#define WM_EXECUTE_JS           (WM_USER + 101)  ///< 执行 JavaScript
#define WM_DECOMPOSE_COMPLETE   (WM_USER + 102)  ///< 分解完成通知
#define WM_DECOMPOSE_HEX_PACKET (WM_USER + 103)  ///< 十六进制封包数据
#define WM_DAILY_TASK_COMPLETE  (WM_USER + 104)  ///< 日常活动完成通知

// ============================================================================
// Hook 初始化与管理
// ============================================================================

/**
 * @brief 初始化Hook
 * @return 初始化是否成功
 */
BOOL InitializeHooks();

/**
 * @brief 卸载Hook
 */
VOID UninitializeHooks();

/**
 * @brief 开始拦截封包
 */
VOID StartIntercept();

/**
 * @brief 停止拦截封包
 */
VOID StopIntercept();

/**
 * @brief 设置拦截类型
 * @param bSend 是否拦截发送包
 * @param bRecv 是否拦截接收包
 */
VOID SetInterceptType(BOOL bSend, BOOL bRecv);

// ============================================================================
// 封包回调设置
// ============================================================================

/**
 * @brief 设置封包回调函数
 * @param callback 回调函数指针
 */
void SetPacketCallback(PACKET_CALLBACK callback);

/**
 * @brief 设置ExecuteScript函数指针
 * @param func 函数指针
 */
void SetExecuteScriptFunction(EXECUTE_SCRIPT_FUNC func);

// ============================================================================
// 响应等待器（内部使用）
// ============================================================================

/**
 * @class ResponseWaiter
 * @brief 响应等待器（内部使用，用于SendPacket自动等待响应）
 *
 * 使用条件变量实现高效的事件通知，替代Sleep轮询。
 * 优化：只在有等待线程时才加锁，避免对每个封包都加锁。
 */
class ResponseWaiter {
public:
    /**
     * @brief 初始化ResponseWaiter（在InitializeHooks中调用）
     */
    static void Initialize();

    /**
     * @brief 清理ResponseWaiter（在UninitializeHooks中调用）
     */
    static void Cleanup();

    /**
     * @brief 等待特定Opcode的响应
     * @param expectedOpcode 期望的Opcode
     * @param timeoutMs 超时时间（毫秒）
     * @return 是否收到响应
     */
    static bool WaitForResponse(
        uint32_t expectedOpcode,
        DWORD timeoutMs
    );

    /**
     * @brief 通知收到响应（在ProcessReceivedGamePackets中调用）
     * @param opcode 收到的Opcode
     *
     * 优化：使用原子变量快速检查是否有等待线程，避免不必要的加锁
     */
    static void NotifyResponse(uint32_t opcode);

    /**
     * @brief 取消等待
     */
    static void CancelWait();

private:
    static CRITICAL_SECTION s_cs;
    static CONDITION_VARIABLE s_cv;
    static bool s_responseReceived;
    static uint32_t s_receivedOpcode;
    static std::atomic<long> s_waitingCount;  // 等待线程计数（真正的原子操作）
};

// ============================================================================
// 响应分发器
// ============================================================================

/**
 * @brief 响应处理器类型定义
 */
using ResponseHandler = std::function<void(const GamePacket&)>;

/**
 * @class ResponseDispatcher
 * @brief 响应分发器 - 基于Opcode和Params分发响应到对应处理器
 * 
 * 使用示例：
 * @code
 * // 注册处理器
 * ResponseDispatcher::Instance().Register(
 *     Opcode::ACTIVITY_QUERY_BACK, 
 *     788, 
 *     ProcessStrawberryResponse
 * );
 * 
 * // 在HookedRecv中分发
 * ResponseDispatcher::Instance().Dispatch(packet);
 * @endcode
 */
class ResponseDispatcher {
public:
    /**
     * @brief 获取单例实例
     */
    static ResponseDispatcher& Instance();

    /**
     * @brief 注册响应处理器（仅匹配Opcode）
     * @param opcode 操作码
     * @param handler 处理器函数
     * @return 注册是否成功
     */
    BOOL Register(uint32_t opcode, ResponseHandler handler);

    /**
     * @brief 注册响应处理器（匹配Opcode + Params）
     * @param opcode 操作码
     * @param params 参数值
     * @param handler 处理器函数
     * @return 注册是否成功
     */
    BOOL Register(uint32_t opcode, uint32_t params, ResponseHandler handler);

    /**
     * @brief 注销处理器
     * @param opcode 操作码
     * @param params 参数值（0xFFFFFFFF表示匹配任意params）
     */
    void Unregister(uint32_t opcode, uint32_t params = 0xFFFFFFFF);

    /**
     * @brief 分发封包到对应处理器
     * @param packet 游戏封包
     * @return 是否有处理器处理了该封包
     */
    BOOL Dispatch(const GamePacket& packet);

    /**
     * @brief 清空所有处理器
     */
    void Clear();

    /**
     * @brief 初始化默认处理器（在InitializeHooks中调用）
     */
    void InitializeDefaultHandlers();

private:
    ResponseDispatcher() = default;
    ~ResponseDispatcher() = default;
    ResponseDispatcher(const ResponseDispatcher&) = delete;
    ResponseDispatcher& operator=(const ResponseDispatcher&) = delete;

    // 生成唯一key: 高32位是opcode，低32位是params
    static uint64_t MakeKey(uint32_t opcode, uint32_t params);

    // 处理器映射表
    std::unordered_map<uint64_t, ResponseHandler> m_handlers;
    
    // 仅匹配opcode的处理器（params = 0xFFFFFFFF）
    std::unordered_map<uint32_t, ResponseHandler> m_opcodeOnlyHandlers;
    
    // 线程安全
    std::mutex m_mutex;
};

// ============================================================================
// 活动状态管理
// ============================================================================

/**
 * @struct ActivityState
 * @brief 活动状态基类 - 包含所有活动共有的状态字段
 */
struct ActivityState {
    std::atomic<bool> isRunning{false};         // 是否正在执行
    std::atomic<bool> waitingResponse{false};   // 是否等待响应
    std::atomic<int> playCount{0};              // 剩余游戏次数
    std::atomic<int> restTime{0};               // 冷却时间（秒）
    std::atomic<int> checkCode{0};              // 服务器校验码
    std::atomic<bool> sweepAvailable{false};    // 是否可扫荡
    
    virtual ~ActivityState() = default;
    
    /**
     * @brief 重置所有状态
     */
    virtual void Reset() {
        isRunning = false;
        waitingResponse = false;
        playCount = 0;
        restTime = 0;
        checkCode = 0;
        sweepAvailable = false;
    }
};

/**
 * @struct StrawberryState
 * @brief 采摘红莓果活动状态
 */
struct StrawberryState : ActivityState {
    std::atomic<int> ruleFlag{0};               // 规则标志
    std::atomic<int> redBerryCount{0};          // 红莓果数量
    std::atomic<bool> useSweep{false};          // 是否使用扫荡
    std::atomic<int> awardType{0};              // 奖励类型
    std::vector<int> awardArr;                  // 奖励物品列表
    std::vector<int> toolArr;                   // 道具列表
    
    void Reset() override {
        ActivityState::Reset();
        ruleFlag = 0;
        redBerryCount = 0;
        useSweep = false;
        awardType = 0;
        awardArr.clear();
        toolArr.clear();
    }
};

/**
 * @struct TrialState
 * @brief 试炼活动状态
 */
struct TrialState : ActivityState {
    std::atomic<int> gameCount{0};              // 游戏次数
    std::atomic<int> awardNum{0};               // 印记数量
    std::atomic<bool> complete{false};          // 是否完成
    std::atomic<bool> waitingResponse{false};   // 等待响应
    
    void Reset() override {
        ActivityState::Reset();
        gameCount = 0;
        awardNum = 0;
        complete = false;
        waitingResponse = false;
    }
};

/**
 * @struct Act778State
 * @brief 驱傩聚福寿活动状态
 */
struct Act778State : ActivityState {
    std::atomic<int> bestScore{0};              // 最高分
    std::atomic<bool> useSweep{false};          // 是否使用扫荡
    std::atomic<bool> sweepSuccess{false};      // 扫荡是否成功
    std::atomic<int> gameTime{0};               // 游戏时间
    
    void Reset() override {
        ActivityState::Reset();
        bestScore = 0;
        useSweep = false;
        sweepSuccess = false;
        gameTime = 0;
    }
};

/**
 * @struct Act793State
 * @brief 磐石御天火活动状态
 */
struct Act793State : ActivityState {
    std::atomic<int> bestScore{0};              // 最高分
    std::atomic<int> medalCount{0};             // 徽章数量
    std::atomic<int> targetMedals{0};           // 目标徽章数
    std::atomic<bool> useSweep{false};          // 是否使用扫荡
    std::atomic<bool> sweepSuccess{false};      // 扫荡是否成功
    
    void Reset() override {
        ActivityState::Reset();
        bestScore = 0;
        medalCount = 0;
        targetMedals = 0;
        useSweep = false;
        sweepSuccess = false;
    }
};

/**
 * @struct Act791State
 * @brief 五行镜破封印活动状态
 */
struct Act791State : ActivityState {
    std::atomic<int> medalNum{0};               // 勋章数量
    std::atomic<int> bestScore{0};              // 最高分
    std::atomic<int> lastScore{0};              // 上次分数
    std::atomic<int> superEvolutionFlag{0};     // 超进化标志
    std::atomic<int> targetScore{0};            // 目标分数
    std::atomic<bool> useSweep{false};          // 是否使用扫荡
    std::atomic<bool> sweepSuccess{false};      // 扫荡是否成功
    
    void Reset() override {
        ActivityState::Reset();
        medalNum = 0;
        bestScore = 0;
        lastScore = 0;
        superEvolutionFlag = 0;
        targetScore = 0;
        useSweep = false;
        sweepSuccess = false;
    }
};

/**
 * @class ActivityStateManager
 * @brief 活动状态管理器 - 统一管理所有活动状态
 * 
 * 使用示例：
 * @code
 * auto& state = ActivityStateManager::Instance().GetAct643State();
 * state.playCount = 5;
 * state.Reset();
 * @endcode
 */
class ActivityStateManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ActivityStateManager& Instance();
    
    /**
     * @brief 获取采摘红莓果状态
     */
    StrawberryState& GetStrawberryState();
    
    /**
     * @brief 获取试炼活动状态
     */
    TrialState& GetTrialState();
    
    /**
     * @brief 获取驱傩聚福寿状态
     */
    Act778State& GetAct778State();
    
    /**
     * @brief 获取磐石御天火状态
     */
    Act793State& GetAct793State();
    
    /**
     * @brief 获取五行镜破封印状态
     */
    Act791State& GetAct791State();
    
    /**
     * @brief 重置所有活动状态
     */
    void ResetAll();

private:
    ActivityStateManager() = default;
    ~ActivityStateManager() = default;
    ActivityStateManager(const ActivityStateManager&) = delete;
    ActivityStateManager& operator=(const ActivityStateManager&) = delete;
    
    StrawberryState m_strawberryState;
    TrialState m_trialState;
    Act778State m_act778State;
    Act793State m_act793State;
    Act791State m_act791State;
};

// ============================================================================
// 封包发送
// ============================================================================

/**
 * @brief 发送封包（支持自动等待响应）
 * @param s Socket句柄（0表示使用最近的游戏Socket）
 * @param pData 数据指针
 * @param dwSize 数据大小
 * @param expectedOpcode 期望的响应Opcode（0表示不等待响应）
 * @param timeoutMs 响应超时时间（毫秒，0表示不等待响应）
 * @return 发送是否成功（如等待响应，超时返回FALSE）
 */
BOOL SendPacket(SOCKET s, const BYTE* pData, DWORD dwSize, uint32_t expectedOpcode = 0, DWORD timeoutMs = 0);

// ============================================================================
// 封包列表管理
// ============================================================================

/**
 * @brief 清空封包列表
 */
VOID ClearPacketList();

/**
 * @brief 删除选中的封包
 * @param indices 要删除的封包索引列表
 */
VOID DeleteSelectedPackets(const std::vector<DWORD>& indices);

/**
 * @brief 获取封包数量
 * @return 封包数量
 */
DWORD GetPacketCount();

/**
 * @brief 获取封包数据
 * @param index 封包索引
 * @param pPacket 输出的封包数据
 * @return 获取是否成功
 */
BOOL GetPacket(DWORD index, PPACKET pPacket);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 十六进制数据转字符串
 * @param pData 数据指针
 * @param dwSize 数据大小
 * @return 十六进制字符串
 */
std::string HexToString(const BYTE* pData, DWORD dwSize);

/**
 * @brief 字符串转十六进制数据
 * @param str 十六进制字符串
 * @return 字节数组
 */
std::vector<BYTE> StringToHex(const std::string& str);

// ============================================================================
// UI 交互函数
// ============================================================================

/**
 * @brief 添加封包到UI
 * @param bSend 是否为发送包
 * @param pData 数据指针
 * @param dwSize 数据大小
 * @param dwTime 时间戳
 */
void AddPacketToUI(BOOL bSend, const BYTE* pData, DWORD dwSize, DWORD dwTime);

/**
 * @brief 同步所有封包到UI
 */
void SyncPacketsToUI();

// ============================================================================
// WPE Hook 模块初始化/清理
// ============================================================================

/**
 * @brief 初始化WPE Hook模块
 * @return 初始化是否成功
 */
BOOL InitializeWpeHook();

/**
 * @brief 清理WPE Hook模块
 */
VOID CleanupWpeHook();

// ============================================================================
// 灵玉相关功能
// ============================================================================

/**
 * @brief 查询灵玉
 * @return 发送是否成功
 */
BOOL SendQueryLingyuPacket();

/**
 * @brief 发送查询背包封包（2个包）
 * @return 发送是否成功
 */
BOOL SendQueryBagPacket();

/**
 * @brief 发送一键分解封包 - 异步版本（多次单个分解）
 * @param jsonArray JSON数组字符串，如 ["37","52"]
 * @return 发送是否成功
 */
BOOL SendDecomposeLingyuPacket(const std::wstring& jsonArray);

// ============================================================================
// 妖怪背包相关功能
// ============================================================================

/**
 * @brief 查询妖怪背包列表
 * @return 发送是否成功
 */
BOOL SendQueryMonsterPacket();

// ============================================================================
// 日常活动功能
// ============================================================================

// -------------------------
// 深度挖宝
// -------------------------

/**
 * @brief 深度挖宝（小游戏，gameType=12）- 执行N次
 * @param count 执行次数，默认为 2 次
 * @return 发送是否成功
 */
BOOL SendDeepDigPacketNTimes(int count = WpeHook::DEEP_DIG_DEFAULT_COUNT);

/**
 * @brief 深度挖宝（单次，向后兼容）
 * @return 发送是否成功
 */
BOOL SendDeepDigPacket();

/**
 * @brief 查询深度挖宝剩余次数
 * @return 发送是否成功
 */
BOOL SendQueryDeepDigCountPacket();

/**
 * @brief 处理深度挖宝响应
 * @param packet 封包数据
 */
void ProcessDeepDigResponse(const GamePacket& packet);

/**
 * @brief 处理深度挖宝查询响应（在HookedRecv中调用）
 * @param packet 封包数据
 */
void ProcessDeepDigQueryResponse(const GamePacket& packet);

// -------------------------
// 简单发送的日常活动
// -------------------------

/** 每日卡牌 */
BOOL SendDailyCardPacket();

/** 每日礼包 */
BOOL SendDailyGiftPacket();

/** 每周礼包 */
BOOL SendWeeklyGiftPacket();

/** 家族考勤 */
BOOL SendFamilyCheckinPacket();

/** 家族报道 */
BOOL SendFamilyReportPacket();

/** 家族保卫战 */
BOOL SendFamilyDefendPacket();

/** 商城惊喜 */
BOOL SendShopSurprisePacket();

// ============================================================================
// 自动回血功能
// ============================================================================

/**
 * @brief 发送贝贝回血封包
 * @return 发送是否成功
 * @note 战斗结束后自动调用
 */
BOOL SendBeibeiHealPacket();

/**
 * @brief 发送MD5图片验证回复封包
 * @param index 选择的索引值（0-3正常，99或其他值用于测试）
 * @return 发送成功返回TRUE，失败返回FALSE
 * @note 收到MD5验证请求时自动调用
 */
BOOL SendMD5CheckReplyPacket(int index);

// ============================================================================
// 跳舞大赛功能
// ============================================================================

/**
 * @brief 进入地图（用于跳舞大赛）
 * @param mapId 地图ID
 * @return 发送是否成功
 */
BOOL SendEnterScenePacket(int mapId);

/**
 * @brief 跳舞大赛 - 发送跳舞活动操作封包
 * @param params 参数
 * @param bodyValues Body值列表
 * @return 发送是否成功
 */
BOOL SendDanceActivityPacketEx(int params, const std::vector<int32_t>& bodyValues);

/**
 * @brief 跳舞大赛 - 发送阶段封包（进入/退出活动）
 * @param params 参数
 * @param bodyValues Body值列表
 * @return 发送是否成功
 */
BOOL SendDanceStagePacketEx(int params, const std::vector<int32_t>& bodyValues);

/**
 * @brief 跳舞大赛 - 进入活动
 * @return 发送是否成功
 */
BOOL SendDanceEnterPacket();

/**
 * @brief 跳舞大赛 - 开始游戏
 * @return 发送是否成功
 */
BOOL SendDanceStartPacket();

/**
 * @brief 跳舞大赛 - 游戏过程封包
 * @param serverTime 服务器时间
 * @param danceCounter 跳舞计数
 * @return 发送是否成功
 */
BOOL SendDanceProcessPacketEx(int serverTime, int danceCounter);

/**
 * @brief 跳舞大赛 - 结束游戏
 * @return 发送是否成功
 */
BOOL SendDanceEndPacketEx();

/**
 * @brief 跳舞大赛 - 退出活动
 * @return 发送是否成功
 */
BOOL SendDanceExitPacketEx();

/**
 * @brief 跳舞大赛 - 提交分数
 * @param serverScore 服务器分数
 * @return 发送是否成功
 */
BOOL SendDanceSubmitScorePacket(int serverScore);

/**
 * @brief 处理跳舞大赛活动响应
 * @param packet 封包数据
 */
void ProcessDanceActivityResponse(const GamePacket& packet);

/**
 * @brief 处理跳舞大赛阶段响应（在HookedRecv中调用）
 * @param packet 封包数据
 */
void ProcessDanceStageResponse(const GamePacket& packet);

/**
 * @brief 跳舞大赛（完整流程）
 * @return 执行是否成功
 */
BOOL SendDanceContestPacket();

// ============================================================================
// 试炼活动功能
// ============================================================================

/**
 * @brief 处理试炼活动响应
 * @param packet 封包数据
 */
void ProcessTrialResponse(const GamePacket& packet);

// -------------------------
// 火风试炼
// -------------------------

/** 火风试炼开始游戏 */
BOOL SendFireWindTrialPacket();

/** 火风试炼结束游戏 */
BOOL SendFireWindEndPacket(int result, int awardCount, int checkCode);

// -------------------------
// 火焰试炼
// -------------------------

/** 火焰试炼开始游戏 */
BOOL SendFireTrialPacket();

/** 火焰试炼结束游戏 */
BOOL SendFireEndPacket(int result, int awardCount, int checkCode);

// -------------------------
// 风暴试炼
// -------------------------

/** 风暴试炼开始游戏 */
BOOL SendStormTrialPacket();

/**
 * @brief 风暴试炼结束游戏
 * @param exitStatus 退出状态
 * @param brand 奖励印记
 * @param checkCode 校验码
 * @note 校验码系数是72，不是56
 */
BOOL SendStormEndPacket(int exitStatus, int brand, int checkCode);

// ============================================================================
// 玄塔寻宝功能
// ============================================================================

// 玄塔活动 Opcode 定义（基于封包分析）
namespace TowerOpcode {
    constexpr uint32_t BUY_DICE_SEND = 1185569;        // 购买骰子 (Opcode 41 14 12 00)
    constexpr uint32_t CLAIM_DICE = 393216;            // 领取骰子 (Opcode 00 00 06 00)
    constexpr uint32_t ENTER_SCENE_SEND = 1184313;    // 进入场景 (Opcode 39 12 12 00)
    constexpr uint32_t START_BATTLE = 1184788;        // 发起战斗 (Opcode 14 14 12 00)
    constexpr uint32_t EXIT_SCENE = 4370;             // 退出场景 (Opcode 12 11 00 00 小端序)
    constexpr uint32_t ACTIVITY_QINGYANG = 1184833;   // 玄塔活动 (Opcode 33 47 12 00)
}

// 玄塔地图ID
constexpr int TOWER_MAP_ID = 16006;

/**
 * @brief 存入妖怪仓库
 * @param param1 第一个参数
 * @param param2 第二个参数
 * @return 发送是否成功
 * @note 封包格式: Opcode=1187333, Params=1, Body=[param1, param2]
 */
BOOL SendPutToSpiritStorePacket(int param1, int param2);

/**
 * @brief 法宝操作（贝贝）
 * @param type 操作类型 (0=召唤, 2=收回)
 * @param userId 卡布UID
 * @return 发送是否成功
 * @note 封包格式: Opcode=1186832, Params=type, Body=[userId, 0]
 */
BOOL SendFabaoPacket(int type, uint32_t userId);

/**
 * @brief 领取免费骰子（5个）
 * @return 发送是否成功
 * @note 封包格式: Opcode=393216, Params=5, Body=[]
 */
BOOL SendClaimFreeDicePacket();

/**
 * @brief 查询玄塔信息（领取骰子第一步）
 * @return 发送是否成功
 * @note 封包格式: Opcode=1185569, Params=341, Body=[1]
 */
BOOL SendTowerCheckInfoPacket();

/**
 * @brief 购买骰子
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184833, Params=341, Body=[6, 18] (BUY_BONES, 数量18)
 */
BOOL SendBuyDicePacket();

/**
 * @brief 购买5个骰子
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184833, Params=341, Body=[6, 5] (BUY_BONES, 数量5)
 */
BOOL SendBuyDice5Packet();

/**
 * @brief 跳转到玄塔地图
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184313, Params=16006 (地图ID)
 */
BOOL SendEnterTowerMapPacket();

/**
 * @brief 投掷骰子
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184833, Params=341, Body=[2]
 */
BOOL SendThrowBonesPacket();

/**
 * @brief 退出世界（回家第一步）
 * @param userId 卡布UID
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184002, Params=userId, Body=[]
 */
BOOL SendExitScenePacket(uint32_t userId);

/**
 * @brief 重新进入玄塔地图（回家第二步）
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184313, Params=1002
 */
BOOL SendReenterTowerMapPacket();

/**
 * @brief 一键玄塔完整流程
 * @return 执行是否成功
 */
BOOL SendOneKeyTowerPacket();

/**
 * @brief 处理玄塔活动响应（在HookedRecv中调用）
 * @param packet 封包数据
 */
void ProcessTowerActivityResponse(const GamePacket& packet);

// ============================================================================
// 一键完成日常活动
// ============================================================================

/**
 * @brief 一键完成所有选中的日常活动
 * @param flags 位标志
 *   - bit0  = 深度挖宝
 *   - bit1  = 每日卡牌
 *   - bit2  = 每日礼包
 *   - bit3  = 每周礼包
 *   - bit4  = 家族考勤
 *   - bit5  = 家族报道
 *   - bit6  = 家族保卫
 *   - bit7  = 商城惊喜
 *   - bit8  = 跳舞大赛
 *   - bit9  = 火风试炼
 *   - bit10 = 火焰试炼
 *   - bit11 = 风暴试炼
 */
void SendDailyTasksAsync(DWORD flags);

// ============================================================================
// 一键采集
// ============================================================================

/**
 * @brief 一键采集所有选中的材料
 * @param flags 位标志，每个bit对应一个采集物品（从bit0开始）
 * @return 执行是否成功
 */
BOOL SendOneKeyCollectPacket(DWORD flags);

/**
 * @brief 处理采集响应（在HookedRecv中调用）
 * @param packet 封包数据
 */
void ProcessCollectResponse(const GamePacket& packet);

// ============================================================================
// 全局变量声明
// ============================================================================

/** 全局卡布号（从进入世界封包获取） */
extern std::atomic<uint32_t> g_userId;

/** 全局窗口句柄，用于UI交互 */
extern HWND g_hWnd;

/** 自动回血（战斗结束后自动发送贝贝回血封包） */
extern std::atomic<bool> g_autoHeal;

/** 屏蔽战斗（勾选后自动屏蔽Opcode 1317120的战斗开始接收包） */
extern std::atomic<bool> g_blockBattle;

/** 自动回家（玄塔投掷完成后自动退出回家） */
extern std::atomic<bool> g_autoGoHome;

extern std::atomic<int32_t> g_battleCounter;  // 战斗counter（用于战斗校验）
extern std::atomic<bool> g_battleStarted;     // 是否已进入战斗

/** MD5图片验证自动回复 */
extern std::atomic<int> g_md5CheckIndex;  // 自动回复的索引值（-1=禁用，0-3=有效索引，99=测试值）

/** 采集相关状态 */
extern std::atomic<bool> g_collectFinished;  // 采集完成标志
extern std::atomic<bool> g_collectAutoMode;  // 一键采集模式
extern std::atomic<int> g_collectStatus;     // 采集状态

// ============================================================================
// 数据结构
// ============================================================================

/**
 * @struct HexPacketData
 * @brief 用于存储十六进制封包数据的结构
 */
struct HexPacketData {
    char* data;   ///< 数据指针
    size_t len;   ///< 数据长度
};

// ============================================================================
// 劫持功能数据结构
// ============================================================================

/**
 * @enum HijackType
 * @brief 劫持类型
 */
enum HijackType {
    HIJACK_NONE = 0,    ///< 不劫持
    HIJACK_BLOCK = 1,   ///< 拦截（屏蔽封包）
    HIJACK_REPLACE = 2  ///< 替换（修改封包内容）
};

/**
 * @struct HijackRule
 * @brief 劫持规则
 */
struct HijackRule {
    HijackType type;           ///< 劫持类型
    bool forSend;              ///< 是否针对发送包
    bool forRecv;              ///< 是否针对接收包
    std::string searchHex;     ///< 搜索的十六进制字符串
    std::string replaceHex;    ///< 替换的十六进制字符串（仅用于替换类型）
};

// ============================================================================
// 封包标签化
// ============================================================================

/**
 * @brief 获取封包标签
 * @param opcode 封包 Opcode
 * @param bSend 是否为发送包
 * @return 标签字符串，如果没有标签则返回空字符串
 */
std::string GetPacketLabel(uint32_t opcode, bool bSend);

// ============================================================================
// 劫持功能
// ============================================================================

/**
 * @brief 添加劫持规则
 * @param type 劫持类型
 * @param forSend 是否针对发送包
 * @param forRecv 是否针对接收包
 * @param searchHex 搜索的十六进制字符串
 * @param replaceHex 替换的十六进制字符串（仅用于替换类型）
 * @return 添加是否成功
 */
BOOL AddHijackRule(HijackType type, bool forSend, bool forRecv, 
                   const std::string& searchHex, const std::string& replaceHex = "");

/**
 * @brief 清空所有劫持规则
 */
VOID ClearHijackRules();

/**
 * @brief 启用/禁用劫持功能
 * @param enable 是否启用
 */
VOID SetHijackEnabled(bool enable);

/**
 * @brief 处理劫持（在 HookedSend/HookedRecv 中调用）
 * @param bSend 是否为发送包
 * @param pData 数据指针
 * @param pdwSize 数据大小指针（可能被修改）
 * @param pModifiedData 输出的修改后数据（需要调用者释放）
 * @return 是否需要劫持（true=拦截/替换，false=放行）
 */
bool ProcessHijack(bool bSend, const BYTE* pData, DWORD* pdwSize, std::vector<BYTE>* pModifiedData);

// ============================================================================
// 保存和载入封包
// ============================================================================

/**
 * @brief 保存发送包列表到文件
 * @param filePath 文件路径
 * @return 保存是否成功
 */
BOOL SavePacketsToFile(const std::string& filePath);

/**
 * @brief 从文件载入封包列表
 * @param filePath 文件路径
 * @return 载入的封包数量，失败返回 -1
 */
int LoadPacketsFromFile(const std::string& filePath);

// ============================================================================
// 自动发送功能
// ============================================================================

/**
 * @brief 发送所有发送包列表中的封包
 * @param intervalMs 发送间隔（毫秒），默认 100ms
 * @param loopCount 循环次数，默认 1（只发送一次）
 * @param progressCallback 进度回调函数（可选）
 * @return 发送的封包数量
 */
DWORD SendAllPackets(DWORD intervalMs = 100, DWORD loopCount = 1, 
                     std::function<void(DWORD, DWORD, const std::string&)> progressCallback = nullptr);

/**

 * @brief 停止自动发送

 */

VOID StopAutoSend();



// ============================================================================

// BOSS专区功能

// ============================================================================



/**

 * @brief 发送BOSS战斗封包

 * @param spiritId BOSS的妖怪ID（大于10000）

 * @return 发送是否成功

 * @note 封包格式（根据易语言代码）：

 *   - Magic: 21316 (0x5344)

 *   - Length: 4

 *   - Opcode: 1186048 (OP_CLIENT_CLICK_NPC)

 *   - Params: spiritId (小端序)

 *   - Body: 00 00 00 00

 */

/**
 * @brief 发起战斗封包
 * @param spiritId 对象ID（NPC ID、BOSS ID等）
 * @param useId 使用道具ID（默认0）
 * @param extraParam 额外参数（默认0，BOSS挑战时不使用）
 * @return 成功返回TRUE，失败返回FALSE
 * 
 * 封包格式：
 * - Magic: 0x5344 (小端序)
 * - Length: 4 (Body长度，4字节)
 * - Opcode: 1186048 (OP_CLIENT_CLICK_NPC)
 * - Params: 高16位=useId，低16位=(extraParam << 8 | spiritId & 0xFF)
 * - Body: counter (小端序)
 * 
 * 编码示例：
 * - BOSS挑战：params = (useId << 16) | spiritId (如 0x2000274c)
 * - 野怪战斗：params = (useId << 16) | (extraParam << 8) | (spiritId & 0xFF) (如 0x10008F1A)
 */
BOOL SendBattlePacket(uint32_t spiritId, uint32_t useId = 0, uint8_t extraParam = 0);

// ============================================================================
// 采摘红莓果功能 (Act788)
// ============================================================================

namespace StrawberryPick {
    // 活动ID
    constexpr int ACTIVITY_ID = 788;
    
    // 游戏时长（秒）
    constexpr int GAME_DURATION = 120;
}

/**
 * @brief 采摘红莓果 - 发送活动操作封包
 * @param operation 操作类型字符串（如 "open_ui", "start_game", "end_game" 等）
 * @param bodyValues Body值列表（可选）
 * @return 发送是否成功
 */
BOOL SendStrawberryPacket(const std::string& operation, const std::vector<int32_t>& bodyValues = {});

/**
 * @brief 采摘红莓果 - 获取游戏信息
 * @return 发送是否成功
 */
BOOL SendStrawberryGameInfoPacket();

/**
 * @brief 采摘红莓果 - 开始游戏
 * @param ruleFlag 规则标志（0=需要看规则，1=已看过规则）
 * @return 发送是否成功
 */
BOOL SendStrawberryStartGamePacket(int ruleFlag = 1);

/**
 * @brief 采摘红莓果 - 游戏中收集物品
 * @param category 物品类别: 1=Monkey(天上掉落), 2=Goods(地面道具)
 * @param id 物品ID (Monkey: 1-73, Goods: 74-103)
 * @param itemType 物品类型 (直接从awardArr/toolArr获取的原始值)
 * @return 发送是否成功
 * @note 封包格式: [game_hit, checkCode, category, id, itemType]
 * @note 校验码: (random 1000-1999) * (checkCode + id) + 8621
 * @note 注意: 过滤掉石头(itemType==16)，只收集有益道具
 */
BOOL SendStrawberryGameHitPacket(int category, int id, int itemType);

/**
 * @brief 采摘红莓果 - 结束游戏
 * @return 发送是否成功
 * @note 校验码: checkCode & (userId % checkCode)
 */
BOOL SendStrawberryEndGamePacket();

/**
 * @brief 采摘红莓果 - 扫荡信息
 * @return 发送是否成功
 */
BOOL SendStrawberrySweepInfoPacket();

/**
 * @brief 采摘红莓果 - 执行扫荡
 * @return 发送是否成功
 */
BOOL SendStrawberrySweepPacket();

/**
 * @brief 采摘红莓果 - 一键完成
 * @param useSweep 是否使用扫荡功能
 * @return 执行是否成功
 */
BOOL SendOneKeyStrawberryPacket(bool useSweep = false);

/**
 * @brief 处理采摘红莓果响应（在HookedRecv中调用）
 * @param packet 封包数据
 */
void ProcessStrawberryResponse(const GamePacket& packet);

// ============================================================================
// 屠苏祝百寿功能 (Act643)
// ============================================================================

// ============================================================================
// 驱傩聚福寿功能 (Act778)
// ============================================================================

namespace Act778 {
    // 活动ID
    constexpr int ACTIVITY_ID = 778;
    
    // 最高分
    constexpr int MAX_SCORE = 1500;
}

/**
 * @brief 驱傩聚福寿 - 发送活动操作封包
 * @param operation 操作类型字符串
 * @param bodyValues Body值列表
 * @return 发送是否成功
 */
BOOL SendAct778Packet(const std::string& operation, const std::vector<int32_t>& bodyValues = {});

/**
 * @brief 驱傩聚福寿 - 获取游戏信息
 * @return 发送是否成功
 */
BOOL SendAct778GameInfoPacket();

/**
 * @brief 驱傩聚福寿 - 开始游戏
 * @return 发送是否成功
 */
BOOL SendAct778StartGamePacket();

/**
 * @brief 驱傩聚福寿 - 发送游戏过程封包
 * @param below 道具类型
 * @return 发送是否成功
 */
BOOL SendAct778GameHitPacket(int below);

/**
 * @brief 驱傩聚福寿 - 结束游戏
 * @param monsterCount 击杀怪物数量
 * @param endType 结束类型 (0=退出, 1=完成)
 * @return 发送是否成功
 */
BOOL SendAct778EndGamePacket(int monsterCount, int endType);

/**
 * @brief 驱傩聚福寿 - 扫荡信息
 * @return 发送是否成功
 */
BOOL SendAct778SweepInfoPacket();

/**
 * @brief 驱傩聚福寿 - 执行扫荡
 * @return 发送是否成功
 */
BOOL SendAct778SweepPacket();

/**
 * @brief 驱傩聚福寿 - 一键完成
 * @param useSweep 是否使用扫荡功能（需要勾选扫荡复选框）
 * @return 执行是否成功
 */
BOOL SendOneKeyAct778Packet(bool useSweep = false);

/**
 * @brief 处理驱傩聚福寿响应
 * @param packet 封包数据
 */
void ProcessAct778Response(const GamePacket& packet);

// ============================================================================
// 磐石御天火功能 (Act793)
// ============================================================================

namespace Act793 {
    // 活动ID
    constexpr int ACTIVITY_ID = 793;
    
    // 最大血量
    constexpr int BLOOD_MAX = 5;
    
    // 最大关卡数
    constexpr int LEVEL_MAX = 5;
    
    // 目标勋章数量（默认）
    constexpr int TARGET_MEDALS = 75;
}

/**
 * @brief 磐石御天火 - 发送活动操作封包
 * @param operation 操作类型字符串（如 "open_ui", "start_game" 等）
 * @param bodyValues Body值列表（可选）
 * @return 发送是否成功
 */
BOOL SendAct793Packet(const std::string& operation, const std::vector<int32_t>& bodyValues = {});

/**
 * @brief 磐石御天火 - 获取游戏信息
 * @return 发送是否成功
 */
BOOL SendAct793GameInfoPacket();

/**
 * @brief 磐石御天火 - 开始游戏
 * @return 发送是否成功
 */
BOOL SendAct793StartGamePacket();

/**
 * @brief 磐石御天火 - 游戏中击中（收集勋章）
 * @param hitCount 击中次数计数
 * @return 发送是否成功
 * @note 封包格式: [game_hit, hitCount + 1000]
 */
BOOL SendAct793GameHitPacket(int hitCount);

/**
 * @brief 磐石御天火 - 结束游戏
 * @param medalCount 收集的勋章数量
 * @return 发送是否成功
 * @note 校验码: checkCode & (userId % checkCode)
 */
BOOL SendAct793EndGamePacket(int medalCount);

/**
 * @brief 磐石御天火 - 扫荡信息
 * @return 发送是否成功
 */
BOOL SendAct793SweepInfoPacket();

/**
 * @brief 磐石御天火 - 执行扫荡
 * @return 发送是否成功
 */
BOOL SendAct793SweepPacket();

/**
 * @brief 磐石御天火 - 一键完成
 * @param useSweep 是否使用扫荡功能
 * @param targetMedals 目标勋章数量（默认50）
 * @return 执行是否成功
 */
BOOL SendOneKeyAct793Packet(bool useSweep = false, int targetMedals = Act793::TARGET_MEDALS);

/**
 * @brief 处理磐石御天火响应（在HookedRecv中调用）
 * @param packet 封包数据
 */
void ProcessAct793Response(const GamePacket& packet);

// ============================================================================
// 五行镜破封印功能 (Act791)
// ============================================================================

namespace Act791 {
    // 活动ID
    constexpr int ACTIVITY_ID = 791;
    
    // 游戏时长（秒）
    constexpr int GAME_DURATION = 60;
    
    // 目标分数（默认）
    constexpr int TARGET_SCORE = 250;
    
    // 额外封包Opcode
    constexpr uint32_t EXTRA_OPCODE = 1184812;
    constexpr int EXTRA_PARAMS = 3;
}

/**
 * @brief 五行镜破封印 - 发送活动操作封包
 * @param operation 操作类型字符串（如 "game_info", "start_game", "end_game" 等）
 * @param bodyValues Body值列表（可选）
 * @return 发送是否成功
 */
BOOL SendAct791Packet(const std::string& operation, const std::vector<int32_t>& bodyValues = {});

/**
 * @brief 五行镜破封印 - 获取游戏信息
 * @return 发送是否成功
 */
BOOL SendAct791GameInfoPacket();

/**
 * @brief 五行镜破封印 - 开始游戏
 * @return 发送是否成功
 */
BOOL SendAct791StartGamePacket();

/**
 * @brief 五行镜破封印 - 结束游戏
 * @param score 游戏分数
 * @return 发送是否成功
 * @note 校验码: userId % 1000 + serverCheckCode + score
 * @note 需要先发送额外封包(Opcode=1184812, Params=3, Body=[3,4039001,0])
 */
BOOL SendAct791EndGamePacket(int score);

/**
 * @brief 五行镜破封印 - 扫荡信息
 * @return 发送是否成功
 */
BOOL SendAct791SweepInfoPacket();

/**
 * @brief 五行镜破封印 - 执行扫荡
 * @return 发送是否成功
 */
BOOL SendAct791SweepPacket();

/**
 * @brief 五行镜破封印 - 一键完成
 * @param useSweep 是否使用扫荡功能
 * @param targetScore 目标分数（默认100）
 * @return 执行是否成功
 */
BOOL SendOneKeyAct791Packet(bool useSweep = false, int targetScore = Act791::TARGET_SCORE);

/**
 * @brief 处理五行镜破封印响应（在HookedRecv中调用）
 * @param packet 封包数据
 */
void ProcessAct791Response(const GamePacket& packet);

// ============================================================================
// 福瑞宝箱功能 (HeavenFurui)
// ============================================================================

namespace HeavenFurui {
    // 活动ID
    constexpr int ACTIVITY_ID = 900;
    
    // 操作类型
    constexpr int OP_QUERY = 1;      // 查询地图宝箱
    constexpr int OP_PICKUP = 2;     // 拾取宝箱
    constexpr int OP_CONFIRM = 4;    // 确认拾取
    
    // 最大宝箱数量
    constexpr int MAX_BOX_COUNT = 10;
}

/**
 * @brief 福瑞宝箱 - 发送活动操作封包
 * @param opType 操作类型 (1=查询, 2=拾取, 4=确认)
 * @param bodyValues Body值列表
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184833, Params=900, Body=[opType, ...bodyValues]
 */
BOOL SendHeavenFuruiPacket(int opType, const std::vector<int32_t>& bodyValues = {});

/**
 * @brief 福瑞宝箱 - 查询地图是否有宝箱
 * @param mapId 地图ID
 * @return 发送是否成功
 */
BOOL SendHeavenFuruiQueryPacket(int mapId);

/**
 * @brief 福瑞宝箱 - 拾取宝箱
 * @param mapId 地图ID
 * @param boxId 宝箱ID
 * @return 发送是否成功
 */
BOOL SendHeavenFuruiPickupPacket(int mapId, int boxId);

/**
 * @brief 福瑞宝箱 - 确认拾取
 * @return 发送是否成功
 */
BOOL SendHeavenFuruiConfirmPacket();

/**
 * @brief 福瑞宝箱 - 一键完成（遍历所有地图拾取宝箱）
 * @param maxBoxes 最大开启宝箱数量（1-30），默认30
 * @return 执行是否成功
 */
BOOL SendOneKeyHeavenFuruiPacket(int maxBoxes = 30);

/**
 * @brief 停止福瑞宝箱
 */
void StopHeavenFurui();

/**
 * @brief 设置福瑞宝箱最大开启数量
 * @param maxBoxes 最大数量（1-30）
 */
void SetHeavenFuruiMaxBoxes(int maxBoxes);

/**
 * @brief 处理福瑞宝箱响应
 * @param packet 封包数据
 */
void ProcessHeavenFuruiResponse(const GamePacket& packet);

// ============================================================================
// 道具功能 (购买/使用)
// ============================================================================

/**
 * @brief 背包物品信息
 */
struct PackItemInfo {
    uint32_t position;    ///< 物品位置索引（使用道具需要）
    uint32_t id;          ///< 物品ID
    uint32_t count;       ///< 数量
    uint32_t packcode;    ///< 包代码
};

/**
 * @brief 道具信息结构
 */
struct ItemInfo {
    uint32_t id;          ///< 道具ID
    std::wstring name;    ///< 道具名称
    uint32_t price;       ///< 价格
    std::wstring desc;    ///< 描述
};

/** 背包物品位置映射：物品ID -> 位置索引 */
extern std::map<uint32_t, uint32_t> g_itemPositionMap;

/** 背包物品列表 */
extern std::vector<PackItemInfo> g_packItems;

/**
 * @brief 请求背包数据
 * @param packType 包类型（0xFFFFFFFF = 全部，1025 = 战斗道具，257 = 其他）
 * @return 发送是否成功
 * @note 封包格式: Opcode=1183761, Params=packType, Body=[]
 */
BOOL SendReqPackageDataPacket(uint32_t packType = 0xFFFFFFFF);

/**
 * @brief 处理背包数据响应
 * @param packet 封包数据
 */
void ProcessPackageDataResponse(const GamePacket& packet);

/**
 * @brief 购买道具
 * @param itemId 道具ID
 * @param count 购买数量
 * @return 发送是否成功
 * @note 封包格式: Opcode=1183760, Params=itemId, Body=[count]
 */
BOOL SendBuyGoodsPacket(uint32_t itemId, uint32_t count);

/**
 * @brief 使用道具（战斗中）
 * @param itemId 道具ID
 * @param position 物品在背包中的位置索引（如果为0，会从g_itemPositionMap查找）
 * @return 发送是否成功
 * @note 封包格式: Opcode=1186050 (OP_CLIENT_USER_OP), Params=2, Body=[1, position, 1]
 * @note 使用前需要先调用 SendReqPackageDataPacket 获取背包数据
 */
BOOL SendUseItemInBattlePacket(uint32_t itemId, uint32_t position = 0);

/**
 * @brief 使用道具（通用）
 * @param itemId 道具ID
 * @param spiritId 妖怪ID（0表示当前妖怪）
 * @param param1 参数1（功能类型，默认1）
 * @param param2 参数2（默认0）
 * @param param3 参数3（默认0）
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184310, Params=itemId, Body=[spiritId, param1, param2, param3]
 */
BOOL SendUsePropsPacket(uint32_t itemId, uint32_t spiritId = 0, 
                        uint32_t param1 = 1, uint32_t param2 = 0, uint32_t param3 = 0);

/**
 * @brief 处理购买道具响应
 * @param packet 封包数据
 */
void ProcessBuyGoodsResponse(const GamePacket& packet);

/**
 * @brief 处理使用道具响应
 * @param packet 封包数据
 */
void ProcessUsePropsResponse(const GamePacket& packet);

/**
 * @brief 获取道具名称
 * @param itemId 道具ID
 * @return 道具名称（如果找不到返回ID字符串）
 */
std::wstring GetItemName(uint32_t itemId);

/**
 * @brief 获取道具价格
 * @param itemId 道具ID
 * @return 道具价格（如果找不到返回0）
 */
uint32_t GetItemPrice(uint32_t itemId);

/**
 * @brief 获取物品位置
 * @param itemId 物品ID
 * @return 物品位置索引，如果未找到返回0
 */
uint32_t GetItemPosition(uint32_t itemId);

// ============================================================================
// 万妖盛会PVP功能 (BattleSix)
// ============================================================================

namespace BattleSix {
    // 活动ID
    constexpr int ACTIVITY_ID = 999;
    
    // 最大精灵数量
    constexpr int MAX_SPIRIT_COUNT = 6;
}

/**
 * @struct BattleSixSkillInfo
 * @brief 万妖盛会技能信息结构
 */
struct BattleSixSkillInfo {
    int skillId;        // 技能ID
    int power;          // 威力
    int currentPP;      // 当前PP值
    int maxPP;          // 最大PP值
    int skillType;      // 0=物理攻击，1=特殊攻击
    int element;        // 系别
    bool available;     // 是否可用
    std::wstring name;  // 技能名称
    
    BattleSixSkillInfo() : skillId(0), power(0), currentPP(0), maxPP(0), 
                           skillType(0), element(0), available(false) {}
};

/**
 * @struct BattleSixSpiritInfo
 * @brief 万妖盛会精灵信息结构
 */
struct BattleSixSpiritInfo {
    int sid;            // 精灵标识ID（用于匹配atkid/defid）
    int spiritId;       // 精灵类型ID
    int uniqueId;       // 唯一ID（战斗中）
    int userId;         // 所属玩家ID（用于区分我方/敌方）
    int hp;             // 当前HP
    int maxHp;          // 最大HP
    int level;          // 等级
    int element;        // 系别
    int position;       // 出场顺序
    bool isDead;        // 是否死亡
    std::wstring name;  // 精灵名称
    std::vector<BattleSixSkillInfo> skills;
    
    BattleSixSpiritInfo() : sid(0), spiritId(0), uniqueId(0), userId(0), hp(0), maxHp(0), 
                            level(0), element(0), position(0), isDead(false) {}
};

/**
 * @class BattleSixAutoBattle
 * @brief 万妖盛会自动战斗类
 */
class BattleSixAutoBattle {
private:
    std::vector<BattleSixSpiritInfo> m_mySpirits;
    std::vector<BattleSixSpiritInfo> m_enemySpirits;
    int m_currentSpiritIndex;
    int m_currentSkillIndex;
    int m_enemySid;        // 敌方目标精灵的sid（用于封包）
    int m_enemyUniqueId;
    int m_myUniqueId;
    bool m_isInBattle;
    bool m_autoBattleEnabled;
    bool m_autoMatching;  // 是否正在自动匹配
    int m_matchCount;     // 匹配次数（剩余）
    int m_totalMatchCount; // 总匹配次数
    int m_winCount;       // 胜利次数
    int m_loseCount;      // 失败次数
    
public:
    BattleSixAutoBattle();
    
    // 战斗状态管理
    void StartBattle();
    void EndBattle();
    bool IsInBattle() const { return m_isInBattle; }
    void SetAutoBattle(bool enabled) { m_autoBattleEnabled = enabled; }
    bool IsAutoBattleEnabled() const { return m_autoBattleEnabled; }
    void SetAutoMatching(bool enabled) { m_autoMatching = enabled; }
    bool IsAutoMatching() const { return m_autoMatching; }
    
    // 多次匹配管理
    void SetMatchCount(int count) { m_matchCount = count; m_totalMatchCount = count; m_winCount = 0; m_loseCount = 0; }
    int GetMatchCount() const { return m_matchCount; }
    int GetTotalMatchCount() const { return m_totalMatchCount; }
    void DecrementMatchCount() { if (m_matchCount > 0) m_matchCount--; }
    void IncrementWinCount() { m_winCount++; }
    void IncrementLoseCount() { m_loseCount++; }
    int GetWinCount() const { return m_winCount; }
    int GetLoseCount() const { return m_loseCount; }
    
    // 精灵管理
    void ParseAllSpiritsFromBattleStart(const GamePacket& packet);
    void UpdateMySpiritHP(int uniqueId, int hp);
    void UpdateEnemySpiritHP(int uniqueId, int hp);
    bool IsMySpiritBySid(int sid) const;  // 根据sid判断是我方还是敌方
    int FindNextAliveSpirit(int currentIndex);
    void LoadCurrentSpiritSkills();
    
    // 战斗操作
    BOOL OnBattleRoundStart();
    void OnBattleRoundResult(const GamePacket& packet);
    BOOL AutoSwitchSpirit();
    int GetAliveSpiritCount();
    int GetEnemyAliveSpiritCount();
    
    // 技能选择
    int SelectBestSkill();
    
    // 状态获取
    std::vector<BattleSixSpiritInfo>& GetMySpirits() { return m_mySpirits; }
    std::vector<BattleSixSpiritInfo>& GetEnemySpirits() { return m_enemySpirits; }
    int GetCurrentSpiritIndex() const { return m_currentSpiritIndex; }
    int GetEnemyUniqueId() const { return m_enemyUniqueId; }
    int GetMyUniqueId() const { return m_myUniqueId; }
    void SetMyUniqueId(int id) { m_myUniqueId = id; }
    void SetEnemyUniqueId(int id) { m_enemyUniqueId = id; }
    void SetCurrentSpiritIndex(int index) { m_currentSpiritIndex = index; }
};

// 万妖盛会全局变量
extern BattleSixAutoBattle g_battleSixAuto;
extern std::atomic<bool> g_battleSixMatching;  // 是否正在匹配
extern std::atomic<bool> g_battleSixMatchSuccess;  // 匹配是否成功
extern std::atomic<int> g_battleSixSwitchTargetId;  // 切换目标精灵uniqueId（-1表示无切换）
extern std::atomic<int> g_battleSixSwitchRetryCount;  // 切换重试次数

/**
 * @brief 万妖盛会 - 发送查询战斗信息封包
 * @return 发送是否成功
 */
BOOL SendBattleSixCombatInfoPacket();

/**
 * @brief 万妖盛会 - 发送开始匹配封包
 * @return 发送是否成功
 */
BOOL SendBattleSixMatchPacket();

/**
 * @brief 万妖盛会 - 发送取消匹配封包
 * @return 发送是否成功
 */
BOOL SendBattleSixCancelMatchPacket();

/**
 * @brief 万妖盛会 - 发送请求开始战斗封包
 * @return 发送是否成功
 */
BOOL SendBattleSixReqStartPacket();

/**
 * @brief 万妖盛会 - 发送战斗操作封包（攻击/技能）
 * @param opType 操作类型（0=攻击，1=技能，2=道具，3=换精灵）
 * @param param1 参数1
 * @param param2 参数2
 * @param param3 参数3（可选）
 * @return 发送是否成功
 */
BOOL SendBattleSixUserOpPacket(int opType, int param1, int param2);

/**
 * @brief 万妖盛会 - 发送确认战斗结束封包
 * @return 发送是否成功
 */
BOOL SendBattleSixEndPacket();

/**
 * @brief 处理万妖盛会匹配响应
 * @param packet 封包数据
 */
void ProcessBattleSixMatchResponse(const GamePacket& packet);

/**
 * @brief 处理万妖盛会准备战斗响应
 * @param packet 封包数据
 */
void ProcessBattleSixPrepareCombatResponse(const GamePacket& packet);

/**
 * @brief 处理万妖盛会请求开始战斗响应
 * @param packet 封包数据
 */
void ProcessBattleSixReqStartResponse(const GamePacket& packet);

/**
 * @brief 处理万妖盛会战斗信息响应
 * @param packet 封包数据
 */
void ProcessBattleSixCombatInfoResponse(const GamePacket& packet);

/**
 * @brief 处理万妖盛会战斗开始响应
 * @param packet 封包数据
 */
void ProcessBattleSixBattleStartResponse(const GamePacket& packet);

/**
 * @brief 处理万妖盛会战斗回合开始响应
 * @param packet 封包数据
 */
void ProcessBattleSixBattleRoundStartResponse(const GamePacket& packet);

/**
 * @brief 处理万妖盛会战斗回合结果响应
 * @param packet 封包数据
 */
void ProcessBattleSixBattleRoundResultResponse(const GamePacket& packet);

/**
 * @brief 处理万妖盛会战斗结束响应
 * @param packet 封包数据
 */
void ProcessBattleSixBattleEndResponse(const GamePacket& packet);

/**
 * @brief 万妖盛会 - 自动匹配并战斗
 * @param matchCount 匹配次数（默认为1）
 * @return 执行是否成功
 */
BOOL SendOneKeyBattleSixPacket(int matchCount = 1);

/**
 * @brief 万妖盛会 - 取消匹配
 * @return 发送是否成功
 */
BOOL SendCancelBattleSixPacket();

// ============================================================================
// 登录 Key 提取（从 OP_CLIENT_CHECK_ACCOUNT 封包）
// ============================================================================

extern std::wstring g_loginKey;          // 整个封包的十六进制字符串（大写）
extern std::atomic<bool> g_loginKeyCaptured;  // 是否已捕获登录 key

/**
 * @brief 从封包数据提取登录 key（整个封包十六进制字符串）
 * @param pData 封包数据指针
 * @param len 封包长度
 * @return 提取是否成功
 */
BOOL ExtractLoginKeyFromPacket(const BYTE* pData, DWORD len);

/**
 * @brief 构造登录 URL
 * @param hexPacket 十六进制封包字符串
 * @return 构造的登录 URL
 *
 * 易语言逻辑：
 * key = 取右边(hexPacket, 28)  // 去掉前28个字符（14字节）
 * URL = "http://enter.wanwan4399.com/bin-debug/KBgameindex.html?" + 十六进制到文本_GBK(key)
 */
std::wstring BuildLoginUrl(const std::wstring& hexPacket);

// ============================================================================
// 副本跳层功能 (DungeonJump)
// ============================================================================

namespace DungeonJump {
    // 副本相关Opcode (Send)
    constexpr uint32_t JUMP_LAYER_SEND = 1186180;        // 跳层封包 (84 19 12 00)
    constexpr uint32_t QUERY_DUNGEON_INFO = 1184317;     // 查询副本信息 (3D 12 12 00)
    constexpr uint32_t PREPARE_BATTLE = 1184782;         // 准备战斗 (0E 14 12 00)
    constexpr uint32_t DUNGEON_ACTIVITY_SEND = 1184833;  // 副本活动操作 (41 14 12 00)
    constexpr uint32_t COMPLETE_JUMP = 1184771;          // 完成跳层 (03 14 12 00)
    constexpr uint32_t PUT_SPIRIT_STORE = 1187333;       // 存入妖怪仓库 (05 1E 12 00)
    constexpr uint32_t SET_FIRST_SPIRIT = 1187344;       // 设置首发妖怪 (10 1E 12 00)
    
    // 副本相关返回Opcode (从AS3 MsgDoc.as分析得到)
    constexpr uint32_t JUMP_LAYER_BACK = 1317252;        // 跳层返回 (OP_GATECRASH)
    constexpr uint32_t QUERY_DUNGEON_BACK = 1315086;     // 查询副本返回 (OP_CLIENT_REQ_SCENE_PLAYER_LIST)
    constexpr uint32_t PREPARE_BATTLE_BACK = 1315854;    // 准备战斗返回 (OP_CLIENT_NPC_LIST)
    constexpr uint32_t ACTIVITY_BACK = 1324097;          // 活动操作返回 (TRIAL_BACK)
    constexpr uint32_t COMPLETE_JUMP_BACK = 1315843;     // 完成跳层返回 (OP_CLIENT_NPC_STATUS)
    // 注意：进入场景返回使用 packet_parser.h 中的 ENTER_SCENE_BACK (1315395)
    
    // 副本相关常量
    constexpr int DUNGEON_MAP_ID = 2000;                 // 副本地图ID
    constexpr int ACTIVITY_ID_900 = 900;                 // 活动ID 900
    constexpr int ACTIVITY_ID_902 = 902;                 // 活动ID 902
    constexpr int ACTIVITY_ID_325 = 325;                 // 活动ID 325
}

/**
 * @brief 副本跳层状态结构
 */
struct DungeonJumpState {
    bool isRunning = false;              // 是否正在执行跳层
    int targetLayer = 1;                 // 目标层数
    std::vector<MonsterItem> highLevelMonsters;  // 等级>50的妖怪列表
    int originalFirstSpiritId = 0;       // 原本首发妖怪ID
    bool monsterDataReceived = false;    // 妖怪背包数据是否已收到
    int storedMonsterCount = 0;          // 已存入仓库的妖怪数量
    int retrievedMonsterCount = 0;       // 已取回的妖怪数量
    
    void Reset() {
        isRunning = false;
        targetLayer = 1;
        highLevelMonsters.clear();
        originalFirstSpiritId = 0;
        monsterDataReceived = false;
        storedMonsterCount = 0;
        retrievedMonsterCount = 0;
    }
};

// 副本跳层全局状态
extern DungeonJumpState g_dungeonJumpState;

/**
 * @brief 发送跳层封包
 * @param layer 目标层数
 * @return 发送是否成功
 * @note 封包格式: Opcode=1185156, Params=0, Body=[layer(4B小端序)]
 */
BOOL SendDungeonJumpLayerPacket(int layer);

/**
 * @brief 发送查询副本信息封包
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184317, Params=0, Body=[]
 */
BOOL SendQueryDungeonInfoPacket();

/**
 * @brief 发送准备战斗封包
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184782, Params=0, Body=[]
 */
BOOL SendPrepareBattlePacket();

/**
 * @brief 发送副本活动操作封包
 * @param activityId 活动ID (900, 902, 325等)
 * @param opType 操作类型 (1=查询, 4=操作等)
 * @param param 附加参数 (如地图ID)
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184833, Params=activityId, Body=[opType, param]
 */
BOOL SendDungeonActivityPacket(int activityId, int opType, int param);

/**
 * @brief 发送完成跳层封包
 * @return 发送是否成功
 * @note 封包格式: Opcode=1184771, Params=19012, Body=[]
 */
BOOL SendCompleteJumpPacket();

/**
 * @brief 发送存入妖怪仓库封包
 * @param spiritId 妖怪唯一ID
 * @return 发送是否成功
 * @note 封包格式: Opcode=1187333, Params=1, Body=[spiritId, 4]
 * @note 易语言: 44530800051E1200 + 文本_反转(ID, 真) + 0400000000000000
 */
BOOL SendPutSpiritToStorePacket(int spiritId);

/**
 * @brief 发送取出妖怪仓库封包
 * @param spiritId 妖怪唯一ID
 * @return 发送是否成功
 * @note 封包格式: Opcode=1187333, Params=1, Body=[spiritId, 0]
 * @note 易语言: 44530800051E1200 + 文本_反转(ID, 真) + 0000000004000000
 */
BOOL SendGetSpiritFromStorePacket(int spiritId);

/**
 * @brief 发送设置首发妖怪封包
 * @param spiritId 妖怪唯一ID
 * @return 发送是否成功
 * @note 封包格式: Opcode=1187360, Params=0, Body=[spiritId]
 * @note 示例: 44 53 04 00 10 1e 12 00 00 00 00 00 56 00 00 00
 */
BOOL SendSetFirstSpiritPacket(int spiritId);

/**
 * @brief 从妖怪列表中筛选等级>50的妖怪
 * @param monsters 妖怪列表
 * @return 等级>50的妖怪列表
 */
std::vector<MonsterItem> FilterHighLevelMonsters(const std::vector<MonsterItem>& monsters);

/**
 * @brief 查找首发妖怪ID
 * @param monsters 妖怪列表
 * @return 首发妖怪ID，找不到返回0
 */
int FindFirstSpiritId(const std::vector<MonsterItem>& monsters);

/**
 * @brief 一键副本跳层（完整流程）
 * @param targetLayer 目标层数
 * @return 执行是否成功
 * 
 * 完整流程：
 * 1. 查询妖怪背包列表
 * 2. 筛选等级>50的妖怪
 * 3. 记录原本首发妖怪ID
 * 4. 将所有等级>50的妖怪存入仓库
 * 5. 发送跳层封包
 * 6. 进入副本地图(2000)
 * 7. 查询副本信息
 * 8. 准备战斗
 * 9. 发送活动操作封包
 * 10. 发送完成跳层封包
 * 11. 等待响应后将妖怪取回背包
 * 12. 设置原本首发妖怪为首发出战
 */
BOOL SendOneKeyDungeonJumpPacket(int targetLayer);

/**
 * @brief 停止副本跳层
 */
void StopDungeonJump();

/**
 * @brief 处理副本跳层相关响应（在HookedRecv中调用）
 * @param packet 封包数据
 */
void ProcessDungeonJumpResponse(const GamePacket& packet);

/**

 * @brief 处理MD5验证并自动回复

 * @param body 封包Body数据

 * @param params 封包Params

 */

void ProcessMD5CheckAndReply(const std::vector<BYTE>& body, uint32_t params);



// ============================================================================

// 双台谷刷级功能 (ShuangTai Leveling)

// ============================================================================



namespace ShuangTai {

    // 副本相关常量
    constexpr int COPY_SCENE_ID = 4;            // 双台谷副本ID (copyScene = 4)
    constexpr int MAP_ID = 14000;               // 双台谷地图ID
    constexpr int FIRST_LAYER = 1;              // 第一层
    constexpr int MAX_PET_COUNT = 6;            // 最大宠物数量
    constexpr int DELAY_BETWEEN_PACKETS = 300;  // 封包间隔(毫秒)
    constexpr int DELAY_SWITCH_PET = 1000;      // 切换宠物间隔(毫秒)
    constexpr int DELAY_BATTLE_ROUND = 500;     // 战斗回合间隔(毫秒)
    
    // 封包Opcode - 按封包实际计算结果
    // 44530000021C120001000000 -> Opcode=1186818 (0x121C02), Params=1
    constexpr uint32_t INIT_SEND = 1186818;
    
    // 445304008419120002000000 -> Opcode=1186180 (0x121984), Params=2
    constexpr uint32_t JUMP_LAYER_SEND = 1186180;
    
    // 445300008819120000000000 -> Opcode=1186184 (0x121988), Params=0
    constexpr uint32_t ENTER_BATTLE_SEND = 1186184;
    
    // 445300000119120000000000 -> Opcode=1186049 (0x121901), Params=0
    constexpr uint32_t BATTLE_OP1_SEND = 1186049;
    
    // 445300000819120000000000 -> Opcode=1186056 (0x121908), Params=0
    constexpr uint32_t BATTLE_OP2_SEND = 1186056;
    
    // 445304000219120001000000XX -> Opcode=1186050 (0x121902), Params=1, Body=宠物ID
    // 44530800021912000000000002000000XX -> Opcode=1186050 (0x121902), Params=0, Body=[2, 技能ID]
    constexpr uint32_t USER_OP_SEND = 1186050;
    
    // 44530000861912002C010000 -> Opcode=1186182 (0x121986), Params=300
    constexpr uint32_t BATTLE_END_SEND = 1186182;

    constexpr uint32_t BATTLE_START_OP_SEND = 1184769;  // 战斗开始操作

    constexpr uint32_t BATTLE_END_OP_SEND = 1184886;    // 战斗结束操作

    

    // 封包返回Opcode

    constexpr uint32_t GATECRASH_BACK = 1317252;        // 跳层返回

    constexpr uint32_t BATTLE_START_BACK = 1317120;     // 战斗开始

    constexpr uint32_t BATTLE_ROUND_START_BACK = 1317121;  // 战斗回合开始

    constexpr uint32_t BATTLE_ROUND_RESULT_BACK = 1317122; // 战斗回合结果

    constexpr uint32_t BATTLE_END_BACK = 1317125;       // 战斗结束

}



/**



 * @brief 双台谷刷级状态枚举



 */



enum class ShuangTaiState {



    IDLE,               // 空闲状态



    INITIALIZING,       // 初始化中



    JUMPING_LAYER,      // 跳层中



    ENTERING_BATTLE,    // 进入战斗中



    IN_BATTLE,          // 战斗中



    SWITCHING_PETS,     // 切换宠物中



    ATTACKING,          // 攻击中



    ENDING_BATTLE,      // 结束战斗中



    STOPPED             // 已停止



};







/**



 * @class ShuangTaiAutoBattle



 * @brief 双台谷刷级状态机 - 异步事件驱动模式



 * 



 * 参考 BattleSixAutoBattle 的设计模式，使用状态机和响应处理器



 * 实现异步事件驱动的刷级流程，避免线程阻塞等待。



 */



class ShuangTaiAutoBattle {



public:



    ShuangTaiAutoBattle();



    



    // ========== 状态查询 ==========



    bool IsRunning() const { return m_running; }



    ShuangTaiState GetState() const { return m_state; }



    



    // ========== 控制方法 ==========



    /**



     * @brief 启动刷级



     * @param blockBattle 是否屏蔽战斗



     * @return 是否成功启动



     */



    bool Start(bool blockBattle);



    



        /**



    



         * @brief 请求停止刷级（完成当前轮次后再停止）



    



         */



    



        void RequestStop();



    



        



    



        /**



    



         * @brief 立即停止刷级



    



         */



    



        void Stop();



    



        



    



        /**



    



         * @brief 检查是否请求停止



    



         */



    



        bool IsStopRequested() const { return m_stopRequested; }



    



        



    



        /**



    



         * @brief 重置状态



    



         */



    



        void Reset();



    



    // ========== 响应处理方法 ==========



    /**



     * @brief 处理战斗开始响应



     */



    void OnBattleStartResponse(const GamePacket& packet);



    



    /**



     * @brief 处理战斗回合开始响应



     */



    void OnBattleRoundStartResponse();



    



    /**



     * @brief 处理战斗回合结果响应



     */



    void OnBattleRoundResultResponse(const GamePacket& packet);



    



    /**



     * @brief 处理战斗结束响应



     */



    void OnBattleEndResponse(const GamePacket& packet);



    



    /**



     * @brief 初始化妖怪数据



     */



    bool InitializePetData();



    



private:



    // ========== 内部方法 ==========



    /**



     * @brief 发送初始化封包



     */



    void SendInitPacket();



    



    /**



     * @brief 发送跳层封包



     */



    void SendJumpLayerPacket();



    



    /**



     * @brief 发送进入战斗封包



     */



    void SendEnterBattlePacket();



    



    /**



     * @brief 发送战斗操作封包



     */



    void SendBattleOp1Packet();



    



    /**



     * @brief 发送切换宠物封包



     */



    void SendSwitchPetPacket();



    



    /**



     * @brief 发送技能攻击封包



     */



    void SendSkillAttackPacket();



    



    /**



     * @brief 发送战斗结束封包



     */



    void SendBattleEndPacket();



    



    /**



     * @brief 切换到下一只宠物



     * @return 是否还有宠物需要切换



     */



    bool SwitchToNextPet();



    



    /**



     * @brief 开始新一轮战斗



     */



    void StartNewRound();



    



    /**



     * @brief 更新状态并通知UI



     */



    void UpdateState(ShuangTaiState newState);



    



private:



    // ========== 状态变量 ==========



    std::atomic<bool> m_running{false};



    std::atomic<bool> m_stopRequested{false};  // 请求停止标志



    std::atomic<ShuangTaiState> m_state{ShuangTaiState::IDLE};



    std::atomic<bool> m_blockBattle{false};



    



    // 宠物相关



    std::atomic<int> m_petIndex{0};             // 当前切换的宠物索引



                std::atomic<int> m_attackRound{0};          // 当前攻击回合



                std::atomic<int> m_maxAttackCount{8};        // 最大攻击次数（技能PP值）



        



                uint32_t m_mainPetId = 0;                   // 主宠ID



                uint32_t m_mainPetSkillId = 0;              // 主宠最高威力技能ID



                int m_mainPetSkillPP = 8;                   // 主宠技能PP值



                std::vector<uint32_t> m_petIds;             // 需要刷级的宠物ID列表



    



    // 统计



    std::atomic<int> m_totalRounds{0};          // 总完成轮数



};







// 全局实例



extern ShuangTaiAutoBattle g_shuangtaiAuto;



/**
 * @brief 查询双台谷妖怪数据并更新UI
 */
BOOL QueryShuangTaiMonsters();

/**
 * @brief 从妖怪背包数据更新双台谷UI
 */
void UpdateShuangTaiUIFromMonsterData();



/**

 * @brief 一键双台谷刷级

 * @param blockBattle 是否屏蔽战斗

 * @return 执行是否成功

 * 

 * 完整流程：

 * 1. 查询妖怪背包列表

 * 2. 获取最后一位妖怪作为主宠

 * 3. 获取主宠威力最高技能

 * 4. 发送初始化封包

 * 5. 跳层到第一层

 * 6. 进入战斗

 * 7. 循环：轮流切换前5只妖怪

 * 8. 切换主宠，使用最高威力技能攻击

 * 9. 战斗结束后重复流程

 */

/**

 * @brief 查询双台谷妖怪数据并更新UI

 */

BOOL QueryShuangTaiMonsters();



/**

 * @brief 一键双台谷刷级（异步启动）

 */

BOOL SendOneKeyShuangTaiPacket(bool blockBattle = false);



/**

 * @brief 停止双台谷刷级

 */

void StopShuangTai();



// ========== 响应处理函数（注册到 ResponseDispatcher）==========



/**

 * @brief 处理双台谷战斗开始响应

 */

void ProcessShuangTaiBattleStartResponse(const GamePacket& packet);



/**

 * @brief 处理双台谷战斗回合开始响应

 */

void ProcessShuangTaiBattleRoundStartResponse(const GamePacket& packet);



/**

 * @brief 处理双台谷战斗回合结果响应

 */

void ProcessShuangTaiBattleRoundResultResponse(const GamePacket& packet);



/**

 * @brief 处理双台谷战斗结束响应

 */

void ProcessShuangTaiBattleEndResponse(const GamePacket& packet);



/**

 * @brief 获取妖怪背包最后一位妖怪ID

 * @return 最后一位妖怪ID，找不到返回0

 */

uint32_t GetLastSpiritId();



/**

 * @brief 获取妖怪威力最高的技能ID

 * @param spiritId 妖怪唯一ID

 * @return 最高威力技能ID，找不到返回0

 */

uint32_t GetHighestPowerSkillId(uint32_t spiritId);








