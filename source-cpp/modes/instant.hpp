#pragma once

#include "options.hpp"

constexpr BotAI instant_ai {
    .BOT_SPAWN_MAX = 200000.f,
    .BOT_SOLOTRICK_CHANCE = 0.f,
    .BOT_SPLIT_CHANCE = 20.f,
    .BOT_SOLOTRICK_CD = 2.f,
    .BOT_SPLIT_CD = 1.5f,
    .BOT_CENTER_FEED_EJECT = 100,
    .BOT_MAX_SPLIT_ATTEMPT = 64
};

constexpr OPT instant_opt {
    .CELL_LIMIT = 65536,

    .MAP_HW = 16000.f,
    .MAP_HH = 16000.f,

    .TIME_SCALE = 1.2f,
    .DUAL_ENABLED = true,

    .GRID_PL_SIZE = 64,
    .GRID_EV_SIZE = 64,

    .QUADTREE_MAX_LEVEL = 18,
    .QUADTREE_MAX_ITEMS = 20,

    .PERK_INTERVAL = 30,

    .MAX_CYT_CELLS = 5,
    .MAX_EXP_CELLS = 5,

    .CYT_CELL_SIZE = 250.f,
    .EXP_CELL_SIZE = 250.f,

    .M1_RELAXATION = 1.f,
    .M2_RELAXATION = 1.f,

    .MAX_SPAWN_TRIES = 64,
    .VIRUS_SAFE_SPAWN_RADIUS = 10.f,

    .PELLET_COUNT = 1000,
    .PELLET_SIZE = 50.f,

    .VIRUS_COUNT = 30,
    .VIRUS_SIZE = 100.f,
    .VIRUS_SPLIT_SIZE = 150.f,
    .VIRUS_MAX_SIZE = 2500.f,
    .VIRUS_PUSH_BOOST = 0.f,
    .VIRUS_SPLIT_BOOST = 0.f,
    .VIRUS_EXPLODE = false,
    .VIRUS_MONOTONE_POP = false,

    .PLAYER_SPEED = 1.2f,
    .PLAYER_AUTOSPLIT_SIZE = 1500.f,
    .PLAYER_MAX_CELLS = 64,
    .PLAYER_SPAWN_SIZE = 300.f,
    .PLAYER_SPLIT_DIST = 50.f,
    .PLAYER_MIN_SPLIT_SIZE = 100.f,
    .PLAYER_MIN_EJECT_SIZE = 60.f,
    .PLAYER_NO_MERGE_DELAY = 1150,
    .PLAYER_MERGE_TIME = 0.f,
    .PLAYER_VIEW_SCALE = 1.5f,

    .LOCAL_DECAY = 1.25f,
    .STATIC_DECAY = 1.25f,
    .DECAY_MIN = 100.f,

    .BOTS = 50,
    .BOT_SPAWN_SIZE = 500.f,

    .EJECT_SIZE = 39.f,
    .EJECT_LOSS = 42.2f,
    .EJECT_DELAY = 180.f,

    .AI = &instant_ai
};
