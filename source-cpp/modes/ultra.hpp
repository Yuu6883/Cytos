#pragma once

#include "options.hpp"

constexpr BotAI ultra_ai {
    .BOT_SOLOTRICK_MASS = 4000000.f,
    .BOT_SPAWN_MAX = 2000000.f,
    .BOT_SOLOTRICK_CHANCE = 20.f,
    .BOT_SPLIT_CHANCE = 25.f,
    .BOT_SOLOTRICK_CD = 2.f,
    .BOT_SPLIT_CD = 2.f,
    .BOT_CENTER_FEED_EJECT = 10
};

constexpr OPT ultra_opt {
    .MODE = "ultra",
    .CELL_LIMIT = 65536,

    .MAP_HW = 24000.f,
    .MAP_HH = 24000.f,

    .TIME_SCALE = 1.2f,
    .DUAL_ENABLED = false,

    .GRID_PL_SIZE = 64,
    .GRID_EV_SIZE = 64,

    .QUADTREE_MAX_LEVEL = 18,
    .QUADTREE_MAX_ITEMS = 20,

    .PERK_INTERVAL = 30,

    .MIN_PERK_SIZE = 250000.f,
    .MAX_CYT_CELLS = 1,
    .MAX_EXP_CELLS = 1,

    .CYT_CELL_SIZE = 1500.f,
    .EXP_CELL_SIZE = 1500.f,
    
    .M1_RELAXATION = 0.7f,
    .M2_RELAXATION = 0.7f,

    .MAX_SPAWN_TRIES = 64,
    .VIRUS_SAFE_SPAWN_RADIUS = 10.f,

    .PELLET_COUNT = 1000,
    .PELLET_SIZE = 100.f,

    .VIRUS_COUNT = 5,
    .VIRUS_SIZE = 400.f,
    .VIRUS_SPLIT_SIZE = 500.f,
    .VIRUS_PUSH_BOOST = 0.f,

    .ULTRA_MERGE = true,
    .ULTRA_MERGE_DELAY = 50.f,

    .PLAYER_SPEED = 2.f,
    .PLAYER_AUTOSPLIT_SIZE = 0.f,
    .PLAYER_MAX_CELLS = 256,
    .PLAYER_SPAWN_SIZE = 2000.f,
    .PLAYER_SPLIT_DIST = 20.f,
    .PLAYER_MIN_SPLIT_SIZE = 250.f,
    .PLAYER_MIN_EJECT_SIZE = 70.f,
    .PLAYER_NO_MERGE_DELAY = 1100.f,
    .PLAYER_NO_COLLI_DELAY = 500.f,
    .PLAYER_MERGE_TIME = 1.f,
    .PLAYER_VIEW_SCALE = 1.75f,

    .LOCAL_DECAY = 1.1f,
    .STATIC_DECAY = 1.1f,
    .DECAY_MIN = 100.f,

    .BOTS = 40,
    .BOT_SPAWN_SIZE = 1500.f,

    .EJECT_SIZE = 50.0f,
    .EJECT_LOSS = 50.2f,
    .EJECT_DELAY = 200.f,

    .AI = &sf_ai
};