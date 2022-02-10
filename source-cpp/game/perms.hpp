#pragma once
#include <cinttypes>

constexpr uint16_t BOT_CONTROL = 0x1;
constexpr uint16_t PLAYER_CONTROL = 0x2;
constexpr uint16_t NO_POP = 0x4;
constexpr uint16_t INVINCIBLE = 0x8;

constexpr uint16_t NO_EAT = 0x10;
constexpr uint16_t TP = 0x20;
constexpr uint16_t SPAWN_VIRUS = 0x40;
constexpr uint16_t EJECT_PERKS = 0x80;

constexpr uint16_t ALL_PERM = 0xFFFF;