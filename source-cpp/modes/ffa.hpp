#pragma once

#include "options.hpp"

constexpr BotAI ffa_ai {
    .BOT_SOLOTRICK_MASS = 0.f,
    .BOT_SPAWN_MAX = 1000000.f,
    .BOT_IDLE_TIME = 2.5f,
    .BOT_SOLOTRICK_CHANCE = 0.f,
    .BOT_SPLIT_CHANCE = 3.f,
    .BOT_SPLIT_CD = 15.f,
    .BOT_SPLIT_MAX_CELL = 4,
    .BOT_CENTER_FEED_EJECT = 6,
    .BOT_MAX_SPLIT_ATTEMPT = 2
};

constexpr OPT ffa_opt {
    .CELL_LIMIT = 65536,

    .MAP_HW = 16000.f,
    .MAP_HH = 16000.f,

    .TIME_SCALE = 1.2f,

    .GRID_PL_SIZE = 128,
    .GRID_EV_SIZE = 128,

    .QUADTREE_MAX_LEVEL = 18,
    .QUADTREE_MAX_ITEMS = 20,

    .PERK_INTERVAL = 30,

    .MAX_CYT_CELLS = 5,
    .MAX_EXP_CELLS = 5,

    .CYT_CELL_SIZE = 250.f,
    .EXP_CELL_SIZE = 250.f,

    .M1_RELAXATION = 0.8f,
    .M2_RELAXATION = 1.f,

    .MAX_SPAWN_TRIES = 64,
    .VIRUS_SAFE_SPAWN_RADIUS = 10.f,

    .PELLET_COUNT = 10000,
    .PELLET_SIZE = 30.f,

    .VIRUS_COUNT = 100,
    .VIRUS_SIZE = 100.f,
    .VIRUS_SPLIT_SIZE = 150.f,
    .VIRUS_MAX_SIZE = 2500.f,
    .VIRUS_PUSH_BOOST = 0.f,
    .VIRUS_EXPLODE = false,
    .VIRUS_MONOTONE_POP = false,

    .PLAYER_SPEED = 1.f,
    .PLAYER_AUTOSPLIT_SIZE = 1500.f,
    .PLAYER_MAX_CELLS = 16,
    .PLAYER_SPAWN_SIZE = 250.f,
    .PLAYER_SPLIT_DIST = 70.f,
    .PLAYER_MERGE_TIME = 5.f,
    .PLAYER_VIEW_SCALE = 1.5f,

    .LOCAL_DECAY = 2.f,
    .STATIC_DECAY = 2.f,
    .DECAY_MIN = 100.f,

    .BOTS = 50,
    .BOT_SPAWN_SIZE = 500.f,

    .EJECT_DISPERSION = 0.0f,
    .EJECT_SIZE = 39.0f,
    .EJECT_LOSS = 42.2f,
    .EJECT_DELAY = 110.f,

    .AI = &ffa_ai
};
