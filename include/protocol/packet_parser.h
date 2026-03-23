/**
 * @file packet_parser.h
 * @brief 游戏封包解析器
 * 
 * 提供游戏协议的解析功能，包括战斗数据、灵玉信息、妖怪背包等
 */

#pragma once

#include <unordered_map>
#include "packet_types.h"
#include "packet_zlib_api.h"

// ============================================================================
// 封包协议常量
// ============================================================================

#ifndef KB_PACKET_PROTOCOL_SHARED
namespace PacketProtocol {

// 封包魔法数字
constexpr uint16_t MAGIC_NORMAL    = 0x5344;  // "SD" - 普通包 (21316)
constexpr uint16_t MAGIC_COMPRESSED = 0x5343;  // "SC" - 压缩包 (21315)

// 封包头部大小
constexpr size_t HEADER_SIZE = 12;  // Magic(2) + Length(2) + Opcode(4) + Params(4)

// 封包最小大小
constexpr size_t MIN_PACKET_SIZE = HEADER_SIZE;

}  // namespace PacketProtocol
#endif


// ============================================================================
// 操作码定义 (Opcode)
// ============================================================================

#ifndef KB_PACKET_PROTOCOL_SHARED
namespace Opcode {

// -------------------------
// 通用协议
// -------------------------

/** 进入世界包响应 - 用于获取卡布号和卡布名 */
constexpr uint32_t ENTER_WORLD = 1314822;

/** 进入地图成功响应 */
constexpr uint32_t ENTER_SCENE_BACK = 1315395;

// -------------------------
// 战斗相关
// -------------------------

/** 战斗开始 */
constexpr uint32_t BATTLE_START = 1317120;

/** 战斗回合开始 */
constexpr uint32_t BATTLE_ROUND_START = 1317121;

/** 回合结算 */
constexpr uint32_t BATTLE_ROUND = 1317122;

/** 战斗结束（需要检查 params == 1） */
constexpr uint32_t BATTLE_END = 1317125;

/** 切换精灵回合 - 响应（收到此包说明我方精灵死亡，需要切换） */
constexpr uint32_t BATTLE_CHANGE_SPIRIT_ROUND = 1317126;

// -------------------------
// 灵玉相关
// -------------------------

/** 查询灵玉列表 - 发送 */
constexpr uint32_t SPIRIT_EQUIP_ALL_SEND = 1185809;

/** 灵玉列表 - 响应 */
constexpr uint32_t SPIRIT_EQUIP_ALL_BACK = 1316881;

/** 单个分解灵玉 - 发送 */
constexpr uint32_t RESOLVE_SPIRIT_SEND = 1185814;

/** 分解灵玉 - 响应 */
constexpr uint32_t RESOLVE_SPIRIT_BACK = 1316886;

/** 批量分解灵玉 - 发送 */
constexpr uint32_t BATCH_RESOLVE_SPIRIT_SEND = 1185819;

/** 批量分解 - 响应 */
constexpr uint32_t BATCH_RESOLVE_BACK = 1316891;

// 兼容旧名称
constexpr uint32_t LINGYU_LIST = SPIRIT_EQUIP_ALL_BACK;
constexpr uint32_t DECOMPOSE_RESPONSE = RESOLVE_SPIRIT_BACK;

// -------------------------
// 妖怪背包相关
// -------------------------

/** 妖怪背包列表包 - 响应 */
constexpr uint32_t MONSTER_LIST = 1318401;

// -------------------------
// 日常活动相关
// -------------------------

/** 深度挖宝响应 (活动ID: 12) */
constexpr uint32_t DEEP_DIG_BACK = 1315861;

/** 活动查询响应 (通用) */
constexpr uint32_t ACTIVITY_QUERY_BACK = 1316501;

// -------------------------
// 场景和聊天相关
// -------------------------

/** 玩家进入场景 - 响应 */
constexpr uint32_t PLAYER_ENTER_SCENE = 1315075;

/** 玩家离开场景 - 响应 */
constexpr uint32_t PLAYER_LEAVE_SCENE = 1315328;

/** 玩家移动 - 响应 */
constexpr uint32_t PLAYER_MOVING = 1315076;

/** 喊话/聊天 - 响应 */
constexpr uint32_t CHAT_MESSAGE = 1315083;

/** 家族聊天 - 响应 */
constexpr uint32_t FAMILY_CHAT = 1316376;

/** 贝贝回血 - 发送 */
constexpr uint32_t BEIBEI_HEAL_SEND = 1186818;

// -------------------------
// 跳舞大赛相关
// -------------------------

/** 跳舞大赛阶段 - 发送 (进入/退出活动) */
constexpr uint32_t DANCE_STAGE_SEND = 1187368;  // 0x121E28

/** 跳舞大赛阶段 - 响应 */
constexpr uint32_t DANCE_STAGE_BACK = 1318440;

/** 跳舞大赛活动 - 发送 */
constexpr uint32_t DANCE_ACTIVITY_SEND = 1187375;  // 0x121E2F

/** 跳舞大赛活动 - 响应 */
constexpr uint32_t DANCE_ACTIVITY_BACK = 1318447;

// -------------------------
// 试炼活动相关
// -------------------------

/** 商城/试炼 - 发送 */
constexpr uint32_t MALL_SEND = 1184833;

/** 商城/试炼 - 响应 */
constexpr uint32_t MALL_BACK = 1316515;

/** 试炼活动响应 (params=142) */
constexpr uint32_t TRIAL_BACK = 1324097;

// -------------------------
// 采集相关
// -------------------------

/** 获取采集状态 - 发送 */
constexpr uint32_t COLLECT_STATUS_SEND = 1187106;

/** 获取采集状态 - 响应 */
constexpr uint32_t COLLECT_STATUS_BACK = 1318178;

/** 账号验证 - 发送 (用于提取登录Key) */
constexpr uint32_t CHECK_ACCOUNT_SEND = 1183744;

/** 进入地图 - 发送 */
constexpr uint32_t ENTER_SCENE_SEND = 1184313;

// -------------------------
// 战斗相关（发起战斗）
// -------------------------

/** 发起战斗准备 */
constexpr uint32_t BATTLE_READY = 1186049;

/** 旁观战斗准备 */
constexpr uint32_t BATTLE_LOOK_READY = 1186233;

/** 战斗结束发送 */
constexpr uint32_t BATTLE_PLAY_OVER = 1186056;

/** 玄塔寻宝发起战斗 */
constexpr uint32_t TOWER_BATTLE_START = 1184788;

/** 点击NPC/发起战斗 - 发送 (OP_CLIENT_CLICK_NPC) */
    constexpr uint32_t CLICK_NPC = 1186048;

// -------------------------
// MD5 图片验证相关 (防机器人验证)
// -------------------------

/** MD5图片验证请求 - 响应 (服务器请求验证) */
constexpr uint32_t BATTLE_MD5_CHECK = 1317264;

/** MD5图片验证结果 - 发送 (提交验证答案) */
constexpr uint32_t BATTLE_MD5_SEND = 1186193;

/** MD5验证失败 - 响应 */
constexpr uint32_t BATTLE_MD5_FAIL = 1317266;

// -------------------------
// 采摘红莓果相关 (Act788)
// -------------------------

/** 新活动协议 - 发送 (OP_CLIENT_ACTIVITY_QINGYANG_NEW, send=1185429, back=1316501)
 *  用于: 采摘红莓果(788)、屠苏祝百寿(643)、宝盆纳万财(768)等新活动
 *  通过params区分不同活动
 */
constexpr uint32_t ACTIVITY_QINGYANG_NEW_SEND = 1185429;

/** 采摘红莓果活动 - 发送 (使用新活动协议，params=788) */
constexpr uint32_t STRAWBERRY_SEND = 1185429;

/** 采摘红莓果活动 - 响应 (params=788) */
constexpr uint32_t STRAWBERRY_BACK = 1316501;

// -------------------------
// 坐骑大赛相关 (Act665)
// -------------------------

/** 坐骑大赛 - 活动命令发送 (Opcode=1185429, Params=665) */
constexpr uint32_t HORSE_COMPETITION_SEND = 1185429;

/** 坐骑大赛 - 活动命令响应 (Opcode=1316501, Params=665) */
constexpr uint32_t HORSE_COMPETITION_BACK = 1316501;

/** 坐骑大赛 - 游戏内命令发送 (USE_ITEM等, Opcode=1185432) */
constexpr uint32_t HORSE_GAME_CMD_SEND = 1185432;

// 坐骑大赛活动ID（全局常量）
constexpr int HORSE_COMPETITION_ACT_ID = 665;

// -------------------------
// 福瑞宝箱相关 (HeavenFurui)
// -------------------------

/** 福瑞宝箱活动 - 发送 (OP_CLIENT_ACTIVITY_QINGYANG, 活动ID=900) */
constexpr uint32_t HEAVEN_FURUI_SEND = 1184833;

/** 福瑞宝箱活动 - 响应 (params=900) */
constexpr uint32_t HEAVEN_FURUI_BACK = 1324097;

// -------------------------
// 精魄赠送系统相关 (SpiritCollect)
// -------------------------

/** 获取精魄列表 - 发送 (OP_CLIENT_REQ_PRESURES) */
constexpr uint32_t SPIRIT_PRESURES_SEND = 1187124;

/** 获取精魄列表 - 响应 */
constexpr uint32_t SPIRIT_PRESURES_BACK = 1330484;

/** 发送精魄 - 发送 (OP_CLIENT_REQ_SEND_SPIRIT, Params=friendId) */
constexpr uint32_t SPIRIT_SEND_SPIRIT_SEND = 1187129;

/** 发送精魄 - 响应 */
constexpr uint32_t SPIRIT_SEND_SPIRIT_BACK = 1330489;

/** 验证玩家信息 - 发送 (OP_GATEWAY_RES_PLAYER_INFO, Params=friendId) */
constexpr uint32_t SPIRIT_PLAYER_INFO_SEND = 1187585;

/** 验证玩家信息 - 响应 */
constexpr uint32_t SPIRIT_PLAYER_INFO_BACK = 1318657;

/** 精魄系统活动协议 - 发送 (OP_CLIENT_ACTIVITY_QINGYANG_NEW, Params=754) */
constexpr uint32_t SPIRIT_COLLECT_SEND = 1185429;

/** 精魄系统活动协议 - 响应 */
constexpr uint32_t SPIRIT_COLLECT_BACK = 1316501;

// 精魄系统活动ID
constexpr int SPIRIT_COLLECT_ACT_ID = 754;

// -------------------------
// 道具相关
// -------------------------

/** 购买道具 - 发送 (OP_CLIENT_BUY_GOODS) */
constexpr uint32_t BUY_GOODS_SEND = 1183760;

/** 购买道具 - 响应 */
constexpr uint32_t BUY_GOODS_BACK = 1314832;

/** 请求背包数据 - 发送 (OP_CLIENT_REQ_PACKAGE_DATA) */
constexpr uint32_t REQ_PACKAGE_DATA_SEND = 1183761;

/** 请求背包数据 - 响应 */
constexpr uint32_t REQ_PACKAGE_DATA_BACK = 1314833;

/** 使用道具(战斗中) - 发送 (OP_CLIENT_USER_OP, param=2) */
constexpr uint32_t USER_OP_SEND = 1186050;  // 小端序: 02 19 12 00

/** 使用道具(通用) - 发送 (OP_CLIENT_USE_PROPS) */
constexpr uint32_t USE_PROPS_SEND = 1184310;

/** 使用道具 - 响应 */
constexpr uint32_t USE_PROPS_BACK = 1315382;

// -------------------------
// 万妖盛会PVP (BattleSix)
// -------------------------

/** 万妖盛会 - 查询战斗信息 - 发送 */
constexpr uint32_t BATTLESIX_COMBAT_INFO_SEND = 1184260;

/** 万妖盛会 - 查询战斗信息 - 响应 */
constexpr uint32_t BATTLESIX_COMBAT_INFO_BACK = 1315344;

/** 万妖盛会 - 开始匹配 - 发送 */
constexpr uint32_t BATTLESIX_MATCH_SEND = 1184262;

/** 万妖盛会 - 开始匹配 - 响应 */
constexpr uint32_t BATTLESIX_MATCH_BACK = 1315347;

/** 万妖盛会 - 取消匹配 - 发送 */
constexpr uint32_t BATTLESIX_CANCEL_MATCH_SEND = 1184266;

/** 万妖盛会 - 取消匹配 - 响应 */
constexpr uint32_t BATTLESIX_CANCEL_MATCH_BACK = 1315351;

/** 万妖盛会 - 请求开始战斗 - 发送 */
constexpr uint32_t BATTLESIX_REQ_START_SEND = 1184268;

/** 万妖盛会 - 请求开始战斗 - 响应 */
constexpr uint32_t BATTLESIX_REQ_START_BACK = 1315353;

/** 万妖盛会 - 准备战斗 - 响应 */
constexpr uint32_t BATTLESIX_PREPARE_COMBAT_BACK = 1315355;

/** 万妖盛会 - 战斗开始 - 响应 */
constexpr uint32_t BATTLESIX_BATTLE_START_BACK = 1317120;

/** 万妖盛会 - 战斗回合开始 - 响应 */
constexpr uint32_t BATTLESIX_BATTLE_ROUND_START_BACK = 1317121;

/** 万妖盛会 - 战斗回合结果 - 响应 */
constexpr uint32_t BATTLESIX_BATTLE_ROUND_RESULT_BACK = 1317122;

/** 万妖盛会 - 战斗结束 - 响应 */
constexpr uint32_t BATTLESIX_BATTLE_END_BACK = 1317125;

/** 万妖盛会 - 战斗操作 (使用USER_OP_SEND) */
constexpr uint32_t BATTLESIX_USER_OP_SEND = 1186050;

}  // namespace Opcode
#endif

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @struct LingyuAttribute
 * @brief 灵玉属性
 */
#ifndef KB_PACKET_SHARED_TYPES_DEFINED
struct LingyuAttribute {
    int32_t nativeEnum;      ///< 属性类型枚举
    int32_t nativeValue;     ///< 属性值
    std::wstring nativeName; ///< 属性名称
};

/**
 * @struct LingyuItem
 * @brief 灵玉物品
 */
struct LingyuItem {
    int32_t symmId;                       ///< 灵玉ID
    std::wstring symmName;                ///< 灵玉名称
    int32_t symmIndex;                    ///< 灵玉索引
    int32_t symmFlag;                     ///< 灵玉标志
    std::wstring petName;                 ///< 装备的妖怪名称
    int32_t symmType;                     ///< 灵玉类型
    std::vector<LingyuAttribute> nativeList; ///< 属性列表
};

/**
 * @struct LingyuData
 * @brief 灵玉数据集合
 */
struct LingyuData {
    int32_t backFlag;                 ///< 返回标志
    std::vector<LingyuItem> items;    ///< 灵玉列表
};

/**
 * @struct BattleSkill
 * @brief 战斗中的技能信息
 */
struct BattleSkill {
    uint32_t id;         ///< 技能ID
    int32_t pp;          ///< 当前PP值 (time in AS3)
    int32_t maxPp;       ///< 最大PP值 (maxtime in AS3)
    int32_t time;        ///< 当前时间
    int32_t maxTime;     ///< 最大时间
    std::wstring name;   ///< 技能名称
};

// ============================================================================
// BufData 常量定义 (参考 AS3 BufData.as)
// ============================================================================

namespace BufDataType {
    // Buff 操作类型
    constexpr int BUF_TYPE_0 = 0;  ///< 移除 Buff
    constexpr int BUF_TYPE_1 = 1;  ///< 添加 Buff (带回合数)
    constexpr int BUF_TYPE_2 = 2;  ///< 添加 Buff (带参数)
    constexpr int BUF_TYPE_3 = 3;  ///< PP 减少
    constexpr int BUF_TYPE_4 = 4;  ///< PP 设置 (指定技能)
    constexpr int BUF_TYPE_5 = 5;  ///< 保留
    constexpr int BUF_TYPE_6 = 6;  ///< PP 增加
    
    // 站点 Buff 操作类型
    constexpr int COMBAT_SITE_ADD = 1;  ///< 添加站点 Buff
    constexpr int COMBAT_SITE_MD  = 2;  ///< 修改站点 Buff
    constexpr int COMBAT_SITE_DEL = 3;  ///< 删除站点 Buff
    
    // 特殊 Buff ID
    constexpr int SITE_RECOVER_ID = 9999;  ///< 站点恢复 ID
    
    // 回合开始时触发的 Buff ID
    constexpr int ROUND_START_ADD[] = {62};  ///< 回合开始加血
    
    // 造成血量变化的 Buff ID (负值)
    constexpr int DEALADD_BLOOD_1[] = {2, 9, 17, 24, 29, 33, 34, 59, 95};
    
    // 造成血量变化的 Buff ID (正值)
    constexpr int DEALADD_BLOOD_2[] = {36, 37, 46, 45, 62, 9999};
}

/**
 * @struct BufData
 * @brief Buff 数据结构 (参考 AS3 BufData.as)
 */
struct BufData {
    int32_t bufId;           ///< Buff ID
    int32_t addOrRemove;     ///< 操作类型 (BUF_TYPE_0~6)
    int32_t atkId;           ///< 攻击者 sid
    int32_t defId;           ///< 防御者 sid (作用对象)
    int32_t round;           ///< 回合数
    int32_t param1;          ///< 参数1
    int32_t param2;          ///< 参数2
    int32_t leftOrRight;     ///< 左右位置 (站点 Buff)
    std::wstring name;       ///< Buff 名称
    std::wstring tipString;  ///< 提示文本
    
    BufData() : bufId(0), addOrRemove(0), atkId(0), defId(0), 
                round(0), param1(0), param2(0), leftOrRight(0) {}
};

/**
 * @struct BattleEntity
 * @brief 战斗中的实体（妖怪）
 */
struct BattleEntity {
    int32_t sid;             ///< 服务端ID
    int32_t groupType;       ///< 队伍类型
    int32_t userId;          ///< 用户ID
    int32_t spiritId;        ///< 妖怪ID
    int32_t uniqueId;        ///< 唯一ID
    int32_t elem;            ///< 属性（系别）
    int32_t state;           ///< 状态 (1=出战中)
    int32_t hp;              ///< 当前血量
    int32_t maxHp;           ///< 最大血量
    int32_t level;           ///< 等级
    int32_t skillNum;        ///< 技能数量
    std::vector<BattleSkill> skills; ///< 技能列表
    std::vector<BufData> bufArr;     ///< Buff 列表
    std::wstring name;       ///< 名称
    bool mySpirit;           ///< 是否为我方妖怪
};

/**
 * @struct BattleData
 * @brief 战斗数据
 */
struct BattleData {
    std::vector<BattleEntity> myPets;     ///< 我方妖怪列表
    std::vector<BattleEntity> otherPets;  ///< 敌方妖怪列表
    int32_t myActiveIndex;                ///< 我方当前妖怪索引
    int32_t otherActiveIndex;             ///< 敌方当前妖怪索引
    int32_t battleType;                   ///< 战斗类型
    int32_t escape;                       ///< 逃跑标志
    int32_t round;                        ///< 回合数
};

/**
 * @struct GamePacket
 * @brief 游戏封包
 */
struct GamePacket {
    uint16_t magic;              ///< 魔法数字 (SD/SC)
    uint16_t length;             ///< Body长度
    uint32_t opcode;             ///< 操作码
    uint32_t params;             ///< 参数
    std::vector<uint8_t> body;   ///< 数据体
    BOOL bSend;                  ///< 是否为发送包
};

/**
 * @struct MonsterGenius
 * @brief 妖怪资质数据（单项）
 */
struct MonsterGenius {
    std::wstring name;  ///< 资质名称（攻击/防御/法术/抗性/体力/速度）
    int32_t value;      ///< 资质原始值（0-31）
    int32_t level;      ///< 资质等级（1-6星，对应 SpiritGenius.cheakGenius）
};

/**
 * @struct MonsterSkill
 * @brief 妖怪技能数据
 */
struct MonsterSkill {
    int32_t id;         ///< 技能ID
    int32_t pp;         ///< 当前PP值
    int32_t maxPp;      ///< 最大PP值
    std::wstring name;  ///< 技能名称
};

/**
 * @struct MonsterSymm
 * @brief 妖怪灵玉数据
 */
struct MonsterSymm {
    int32_t place;      ///< 位置
    int32_t id;         ///< 灵玉ID
    int32_t index;      ///< 灵玉索引
    std::wstring name;  ///< 灵玉名称
};

/**
 * @struct MonsterItem
 * @brief 妖怪数据
 */
struct MonsterItem {
    int32_t id;                              ///< 唯一ID
    int32_t type_id;                         ///< 类型ID (原typeid，避免C++关键字冲突)
    int32_t iid;                             ///< 配置ID
    std::wstring name;                       ///< 名称
    int32_t isfirst;                         ///< 是否首发
    int32_t level;                           ///< 等级
    int32_t exp;                             ///< 经验
    int32_t needExp;                         ///< 升级所需经验
    int32_t type;                            ///< 类型（系别编号）
    std::wstring typeName;                   ///< 系别名称
    int32_t forbitItem;                      ///< 禁用道具
    int32_t attack;                          ///< 攻击
    int32_t defence;                         ///< 防御
    int32_t magic;                           ///< 法术
    int32_t resistance;                      ///< 抗性
    int32_t strength;                        ///< 体力
    int32_t hp;                              ///< 血量
    int32_t speed;                           ///< 速度
    int32_t mold;                            ///< 模型/性格类型
    int32_t state;                           ///< 状态
    int32_t timetxt;                         ///< 时间
    int32_t sex;                             ///< 性别 (0=未知, 1=雌, 2=雄, 3=无性别)
    int32_t geniusType;                      ///< 性格类型编号
    std::wstring geniusName;                 ///< 性格名称
    int32_t aptitude;                        ///< 资质等级
    std::wstring aptitudeName;               ///< 资质名称
    std::vector<MonsterGenius> geniusList;   ///< 性格加成列表
    std::vector<MonsterSkill> skills;        ///< 技能列表
    std::vector<MonsterSymm> symmList;       ///< 灵玉列表
};

/**
 * @struct MonsterData
 * @brief 妖怪背包数据
 */
struct MonsterData {
    int32_t sn;                       ///< 序列号
    int32_t count;                    ///< 数量
    std::vector<MonsterItem> monsters; ///< 妖怪列表
};

// ============================================================================
// 字节序读取辅助函数
// ============================================================================

/**
 * @brief 以小端序读取int32值
 * @param data 数据指针
 * @param offset 偏移量（会被更新）
 * @return 读取的int32值
 */
inline int32_t ReadInt32LE(const uint8_t* data, size_t& offset) {
    int32_t val = static_cast<int32_t>(data[offset]) |
                  (static_cast<int32_t>(data[offset + 1]) << 8) |
                  (static_cast<int32_t>(data[offset + 2]) << 16) |
                  (static_cast<int32_t>(data[offset + 3]) << 24);
    offset += 4;
    return val;
}

/**
 * @brief 以小端序读取uint32值
 */
inline uint32_t ReadUInt32LE(const uint8_t* data, size_t& offset) {
    uint32_t val = static_cast<uint32_t>(data[offset]) |
                   (static_cast<uint32_t>(data[offset + 1]) << 8) |
                   (static_cast<uint32_t>(data[offset + 2]) << 16) |
                   (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return val;
}

/**
 * @brief 以小端序读取int16值
 */
inline int16_t ReadInt16LE(const uint8_t* data, size_t& offset) {
    int16_t val = static_cast<int16_t>(data[offset]) |
                  (static_cast<int16_t>(data[offset + 1]) << 8);
    offset += 2;
    return val;
}

/**
 * @brief 以小端序读取uint16值
 */
inline uint16_t ReadUInt16LE(const uint8_t* data, size_t& offset) {
    uint16_t val = static_cast<uint16_t>(data[offset]) |
                   (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;
    return val;
}

/**
 * @brief 读取单字节
 */
inline uint8_t ReadByte(const uint8_t* data, size_t& offset) {
    return data[offset++];
}

/**
 * @brief 以大端序读取uint32值（兼容旧协议）
 */
inline uint32_t ReadInt32BE(const uint8_t* data, size_t& offset) {
    uint32_t val = (static_cast<uint32_t>(data[offset]) << 24) | 
                   (static_cast<uint32_t>(data[offset + 1]) << 16) | 
                   (static_cast<uint32_t>(data[offset + 2]) << 8) | 
                   static_cast<uint32_t>(data[offset + 3]);
    offset += 4;
    return val;
}

/**
 * @brief 以大端序读取uint16值（兼容旧协议）
 */
inline uint16_t ReadInt16BE(const uint8_t* data, size_t& offset) {
    uint16_t val = (static_cast<uint16_t>(data[offset]) << 8) | 
                   static_cast<uint16_t>(data[offset + 1]);
    offset += 2;
    return val;
}

/**
 * @brief 以大端序读取字符串（兼容旧协议）
 */
inline std::string ReadStringBE(const uint8_t* data, size_t& offset) {
    uint16_t len = ReadInt16BE(data, offset);
    std::string str(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    return str;
}

#endif

// ============================================================================
// 封包解析器类
// ============================================================================

/**
 * @class PacketParser
 * @brief 游戏封包解析器
 * 
 * 提供封包解析、战斗数据处理、灵玉数据解析等功能
 */
class PacketParser {
public:
    /**
     * @brief 初始化解析器
     * @return 初始化是否成功
     */
    static bool Initialize();

    /**
     * @brief 清理解析器资源
     */
    static void Cleanup();

    /**
     * @brief 解析封包数据
     * @param data 数据指针
     * @param size 数据大小
     * @param bSend 是否为发送包
     * @param outPackets 输出的封包列表
     * @return 是否成功解析到封包
     */
    static bool ParsePackets(const uint8_t* data, size_t size, BOOL bSend, 
                             std::vector<GamePacket>& outPackets);

    /**
     * @brief 处理战斗封包
     * @param packet 游戏封包
     */
    static void ProcessBattlePacket(const GamePacket& packet);

    /**
     * @brief 处理灵玉封包
     * @param packet 游戏封包
     */
    static void ProcessLingyuPacket(const GamePacket& packet);

    /**
     * @brief 处理妖怪背包封包
     * @param packet 游戏封包
     */
    static void ProcessMonsterPacket(const GamePacket& packet);

    /**
     * @brief 发送消息到UI
     * @param type 消息类型
     * @param data 消息数据
     */
    static void SendToUI(const std::wstring& type, const std::wstring& data);

    /**
     * @brief 发送BOSS列表到UI
     */
    static void SendBossListToUI();

    /**
     * @brief 获取当前战斗数据
     * @return 当前战斗数据的引用
     */
    static BattleData& GetCurrentBattle() { return g_currentBattle; }

private:
    static std::vector<uint8_t> g_recvBuffer;  ///< 接收包缓冲区（处理黏包）
    static BattleData g_currentBattle;         ///< 当前战斗数据

    /**
     * @brief 解压封包Body
     */
    static bool UncompressBody(const std::vector<uint8_t>& compressed, 
                               std::vector<uint8_t>& decompressed);

    /**
     * @brief 更新UI战斗数据
     */
    static void UpdateUIBattleData();
};

// ============================================================================
// 兼容旧代码的宏定义（将逐步废弃）
// ============================================================================

// 魔法数字
#define MAGIC_NUMBER_D PacketProtocol::MAGIC_NORMAL
#define MAGIC_NUMBER_C PacketProtocol::MAGIC_COMPRESSED

// 战斗相关
#define OPCODE_BATTLE_START Opcode::BATTLE_START
#define OPCODE_BATTLE_ROUND_START Opcode::BATTLE_ROUND_START
#define OPCODE_BATTLE_ROUND Opcode::BATTLE_ROUND
#define OPCODE_BATTLE_END Opcode::BATTLE_END

// 灵玉相关
#define OPCODE_LINGYU_LIST Opcode::LINGYU_LIST
#define OPCODE_DECOMPOSE_RESPONSE Opcode::DECOMPOSE_RESPONSE
#define OPCODE_MONSTER_LIST Opcode::MONSTER_LIST
#define OPCODE_ENTER_WORLD Opcode::ENTER_WORLD

// 跳舞大赛相关
#define OPCODE_DANCE_ACTIVITY_SEND Opcode::DANCE_ACTIVITY_SEND
#define OPCODE_DANCE_ACTIVITY_BACK Opcode::DANCE_ACTIVITY_BACK
#define OPCODE_DANCE_STAGE_SEND Opcode::DANCE_STAGE_SEND
#define OPCODE_DANCE_STAGE_BACK Opcode::DANCE_STAGE_BACK

// 试炼活动相关
#define OPCODE_MALL_SEND Opcode::MALL_SEND
#define OPCODE_MALL_BACK Opcode::MALL_BACK

// 灵玉相关
#define OPCODE_SPIRIT_EQUIP_ALL Opcode::SPIRIT_EQUIP_ALL_SEND
#define OPCODE_SPIRIT_EQUIP_ALL_BACK Opcode::SPIRIT_EQUIP_ALL_BACK
#define OPCODE_RESOLVE_SPIRIT Opcode::RESOLVE_SPIRIT_SEND
#define OPCODE_RESOLVE_SPIRIT_BACK Opcode::RESOLVE_SPIRIT_BACK
#define OPCODE_BATCH_RESOLVE_SPIRIT Opcode::BATCH_RESOLVE_SPIRIT_SEND
#define OPCODE_BATCH_RESOLVE_BACK Opcode::BATCH_RESOLVE_BACK

// ============================================================================
// 地图名称获取
// ============================================================================

/**
 * @brief 获取地图名称
 * @param mapId 地图ID（10进制）
 * @return 地图名称，如果找不到返回空字符串
 */
std::wstring GetMapName(int mapId);

/**
 * @brief 加载HTTP数据（包含地图数据）
 */
void LoadHttpData();

// 外部变量声明
extern std::unordered_map<int, std::wstring> g_petNames;
extern std::unordered_map<int, std::wstring> g_skillNames;
extern std::unordered_map<int, std::wstring> g_elemNames;
extern std::unordered_map<int, std::wstring> g_geniusNames;
extern std::unordered_map<int, int> g_skillPowers;
extern std::unordered_map<int, int> g_petElems;
