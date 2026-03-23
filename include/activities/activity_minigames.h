#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

#include "packet_types.h"

namespace StrawberryPick {
constexpr int ACTIVITY_ID = 788;
constexpr int GAME_DURATION = 120;
}  // namespace StrawberryPick

BOOL SendStrawberryPacket(const std::string& operation, const std::vector<int32_t>& bodyValues = {});
BOOL SendStrawberryGameInfoPacket();
BOOL SendStrawberryStartGamePacket(int ruleFlag = 1);
BOOL SendStrawberryGameHitPacket(int category, int id, int itemType);
BOOL SendStrawberryEndGamePacket();
BOOL SendStrawberrySweepInfoPacket();
BOOL SendStrawberrySweepPacket();
BOOL SendOneKeyStrawberryPacket(bool useSweep = false);
void ProcessStrawberryResponse(const GamePacket& packet);

namespace Act778 {
constexpr int ACTIVITY_ID = 778;
constexpr int MAX_SCORE = 1500;
}  // namespace Act778

BOOL SendAct778Packet(const std::string& operation, const std::vector<int32_t>& bodyValues = {});
BOOL SendAct778GameInfoPacket();
BOOL SendAct778StartGamePacket();
BOOL SendAct778GameHitPacket(int below);
BOOL SendAct778EndGamePacket(int monsterCount, int endType);
BOOL SendAct778SweepInfoPacket();
BOOL SendAct778SweepPacket();
BOOL SendOneKeyAct778Packet(bool useSweep = false);
void ProcessAct778Response(const GamePacket& packet);

namespace Act793 {
constexpr int ACTIVITY_ID = 793;
constexpr int BLOOD_MAX = 5;
constexpr int LEVEL_MAX = 5;
constexpr int TARGET_MEDALS = 75;
}  // namespace Act793

BOOL SendAct793Packet(const std::string& operation, const std::vector<int32_t>& bodyValues = {});
BOOL SendAct793GameInfoPacket();
BOOL SendAct793StartGamePacket();
BOOL SendAct793GameHitPacket(int hitCount);
BOOL SendAct793EndGamePacket(int medalCount);
BOOL SendAct793SweepInfoPacket();
BOOL SendAct793SweepPacket();
BOOL SendOneKeyAct793Packet(bool useSweep = false, int targetMedals = Act793::TARGET_MEDALS);
void ProcessAct793Response(const GamePacket& packet);

namespace Act791 {
constexpr int ACTIVITY_ID = 791;
constexpr int GAME_DURATION = 60;
constexpr int TARGET_SCORE = 250;
constexpr uint32_t EXTRA_OPCODE = 1184812;
constexpr int EXTRA_PARAMS = 3;
}  // namespace Act791

BOOL SendAct791Packet(const std::string& operation, const std::vector<int32_t>& bodyValues = {});
BOOL SendAct791GameInfoPacket();
BOOL SendAct791StartGamePacket();
BOOL SendAct791EndGamePacket(int score);
BOOL SendAct791SweepInfoPacket();
BOOL SendAct791SweepPacket();
BOOL SendOneKeyAct791Packet(bool useSweep = false, int targetScore = Act791::TARGET_SCORE);
void ProcessAct791Response(const GamePacket& packet);

namespace HeavenFurui {
constexpr int ACTIVITY_ID = 900;
constexpr int OP_QUERY = 1;
constexpr int OP_PICKUP = 2;
constexpr int OP_CONFIRM = 4;
constexpr int MAX_BOX_COUNT = 10;
}  // namespace HeavenFurui

BOOL SendHeavenFuruiPacket(int opType, const std::vector<int32_t>& bodyValues = {});
BOOL SendHeavenFuruiQueryPacket(int mapId);
BOOL SendHeavenFuruiPickupPacket(int mapId, int boxId);
BOOL SendHeavenFuruiConfirmPacket();
BOOL SendOneKeyHeavenFuruiPacket(int maxBoxes = 30);
void StopHeavenFurui();
void SetHeavenFuruiMaxBoxes(int maxBoxes);
void ProcessHeavenFuruiResponse(const GamePacket& packet);
