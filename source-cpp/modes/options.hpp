#pragma once

#include <cinttypes>
#include <string_view>
#include "../physics/cell.hpp"

struct BotAI {
    float BOT_RESPAWN_CD = 1.f;
    cell_cord_prec BOT_SOLOTRICK_MASS = 40000.f;
    float BOT_SPAWN_MAX = 0.f;
    cell_cord_prec BOT_MOVE_MASS = 15000.f;
    float BOT_IDLE_TIME = 1.f;
    float BOT_SOLOTRICK_CHANCE = 10.f;
    float BOT_SPLIT_CHANCE = 10.f;
    float BOT_SOLOTRICK_CD = 5.f;
    float BOT_SPLIT_CD = 3.f;
    uint32_t BOT_SPLIT_MAX_CELL = 0;
    uint32_t BOT_CENTER_FEED_EJECT = 3;
    uint32_t BOT_SOLOTRICK_MAX_CELL = 20;
    uint8_t BOT_MAX_SPLIT_ATTEMPT = 3;
};

constexpr BotAI default_ai;

struct OPT {
    size_t CELL_LIMIT = 65536;

    float MAP_HW = 20000.f;
    float MAP_HH = 20000.f;

    float TIME_SCALE = 1;
    uint8_t LBMM_TPS = 2;

    bool STRICT_BORDER = false;
    bool DUAL_ENABLED = true;

    int32_t GRID_PL_SIZE = 512;
    int32_t GRID_EV_SIZE = 512;

    uint8_t QUADTREE_MAX_LEVEL = 16;
    uint8_t QUADTREE_MAX_ITEMS = 16;

    float PERK_INTERVAL = 15;

    float MIN_PERK_SIZE = 10000.f;
    size_t MAX_CYT_CELLS = 1;
    size_t MAX_EXP_CELLS = 1;

    float CYT_CELL_SIZE = 750;
    float EXP_CELL_SIZE = 750;

    float RELAXATION_RATIO_THRESH = 4.0f;
    float M1_RELAXATION = 0.7f;
    float M2_RELAXATION = 0.8f;

    uint16_t MAX_VIRUS_PER_TICK = 1;
    uint16_t MAX_PELLET_PER_TICK = 100;

    uint16_t MAX_SPAWN_TRIES = 128;
    bool PLAYER_CAN_SPAWN = true;
    cell_cord_prec PLAYER_SAFE_SPAWN_RADIUS = 1.5f;
    cell_cord_prec VIRUS_SAFE_SPAWN_RADIUS = 25.f;

    uint32_t PELLET_COUNT = 1;
    cell_cord_prec PELLET_SIZE = 10.f;

    uint32_t VIRUS_COUNT = 10;
    cell_cord_prec VIRUS_SIZE = 100.f;
    cell_cord_prec VIRUS_SPLIT_SIZE = 150.f;
    cell_cord_prec VIRUS_MAX_SIZE = 1500.f;
    cell_cord_prec VIRUS_PUSH_BOOST = 780.f;
    cell_cord_prec VIRUS_SPLIT_BOOST = 850.f;
    cell_cord_prec VIRUS_MAX_BOOST = 1000.f;
    bool VIRUS_EXPLODE = false;
    bool VIRUS_MONOTONE_POP = false;

    bool ULTRA_MERGE = false;
    cell_cord_prec ULTRA_MERGE_DELAY = 150.;

    bool NEW_BOOST_ALGO = false;
    cell_cord_prec BOOST_FACTOR = 1.;
    cell_cord_prec BOOST_AMOUNT = 1.;
    cell_cord_prec BOOST_DELAY = 1.;
    cell_cord_prec BOOST_MULTI = 1.;

    cell_cord_prec PLAYER_SPEED = 1.5;
    float PLAYER_SPAWN_DELAY = 0.75f;
    cell_cord_prec PLAYER_AUTOSPLIT_SIZE = 1500.f;
    float PLAYER_AUTOSPLIT_DELAY = 100.f;
    uint32_t PLAYER_MAX_CELLS = 16;
    cell_cord_prec PLAYER_SPAWN_SIZE = 500.f;
    cell_cord_prec PLAYER_SPLIT_BOOST = 800.f;
    cell_cord_prec PLAYER_SPLIT_DIST = 40.f;
    cell_cord_prec PLAYER_SPLIT_CAP_T1 = 50000.f;
    cell_cord_prec PLAYER_SPLIT_CAP_T2 = 20000.f;
    cell_cord_prec PLAYER_MAX_BOOST = 1000.f;
    cell_cord_prec PLAYER_MIN_SPLIT_SIZE = 60.f;
    cell_cord_prec PLAYER_MIN_EJECT_SIZE = 60.f;
    cell_cord_prec NORMALIZE_THRESH_MASS = 0.f;
    float PLAYER_NO_MERGE_DELAY = 650.f;
    float PLAYER_NO_COLLI_DELAY = 600.f;
    float PLAYER_NO_EJECT_DELAY = 200.f;
    float PLAYER_NO_EJECT_POP_DEALY = 500.f;
    float PLAYER_MERGE_TIME = 1.f;
    cell_cord_prec PLAYER_MERGE_INCREASE = 1.f;
    bool PLAYER_MERGE_NEW_VER = true;
    cell_cord_prec PLAYER_VIEW_SCALE = 1.;
    cell_cord_prec PLAYER_VIEW_MIN = 4000.;
    cell_cord_prec PLAYER_DEAD_DELAY = 5000.;

    cell_cord_prec LOCAL_DECAY = 1.;
    cell_cord_prec STATIC_DECAY = 1.;
    cell_cord_prec GLOBAL_DECAY = 1.;
    cell_cord_prec DECAY_MIN = 1000.;
    
    cell_cord_prec ANTI_CAMP_TIME = 0.;
    cell_cord_prec ANTI_CAMP_MASS = 0.;
    cell_cord_prec ANTI_CAMP_MULT = 0.;

    uint16_t BOTS = 1;
    cell_cord_prec BOT_SPAWN_SIZE = 500;

    cell_cord_prec EJECT_DISPERSION = 0.3;
    cell_cord_prec EJECT_SIZE = 38;
    cell_cord_prec EJECT_LOSS = 43;
    cell_cord_prec EJECT_BOOST = 780;
    cell_cord_prec EJECT_DELAY = 100;
    cell_cord_prec EJECT_MAX_AGE = 10000;

    cell_cord_prec WORLD_RESTART_MULT = 0.75;
    bool WORLD_KILL_OVERSIZE = false;

    cell_cord_prec EAT_OVERLAP = 3;
    cell_cord_prec EAT_MULT = 1.140175425099138;
    cell_cord_prec EX_FAST_BOOST_R = 0;
    cell_cord_prec EX_FAST_MERGE_MASS = 0;

    const BotAI* AI = &default_ai;
};

constexpr OPT default_opt;

#include "ffa.hpp"
#include "instant.hpp"
#include "mega.hpp"
#include "omega.hpp"
#include "sf.hpp"
#include "ultra.hpp"

#include "benchmark/omega.hpp"

#ifndef GLOBAL_ENGINE_FLAGS
    #define GLOBAL_ENGINE_FLAGS 0
#endif

#ifndef ENGINES
    #define ENGINES 1
#endif



