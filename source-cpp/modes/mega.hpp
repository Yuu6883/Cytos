#pragma once

#include "options.hpp"

constexpr BotAI mega_ai {
    .BOT_SPAWN_MAX = 1000000.f,
    .BOT_MOVE_MASS = 150000.f,
};

constexpr OPT mega_opt {
    .CELL_LIMIT = 131072,

    .MAP_HW = 25000.f,
    .MAP_HH = 25000.f,

    .TIME_SCALE = 1.2f,

    .GRID_PL_SIZE = 128,
    .GRID_EV_SIZE = 128,

    .QUADTREE_MAX_LEVEL = 18,
    .QUADTREE_MAX_ITEMS = 20,

    .MIN_PERK_SIZE = 50000.f,
    .MAX_CYT_CELLS = 5,
    .MAX_EXP_CELLS = 5,

    .M1_RELAXATION = 0.8f,
    .M2_RELAXATION = 1.f,

    .MAX_SPAWN_TRIES = 64,
    .VIRUS_SAFE_SPAWN_RADIUS = 10.f,

    .PELLET_COUNT = 5000,
    .PELLET_SIZE = 30.f,

    .VIRUS_COUNT = 75,
    .VIRUS_SIZE = 200,
    .VIRUS_SPLIT_SIZE = 0.f,
    .VIRUS_MAX_SIZE = 2500.f,
    .VIRUS_SPLIT_BOOST = 700.f,
    .VIRUS_EXPLODE = true,
    .VIRUS_MONOTONE_POP = true,

    .NEW_BOOST_ALGO = true,
    .BOOST_FACTOR = 1.2f,
    .BOOST_AMOUNT = 2.f,
    .BOOST_DELAY = 0.75f,
    .BOOST_MULTI = 0.75f,

    .PLAYER_AUTOSPLIT_SIZE = 0.f,
    .PLAYER_MAX_CELLS = 64,
    .PLAYER_SPAWN_SIZE = 400.f,
    .PLAYER_SPLIT_BOOST = 700.f,
    .PLAYER_SPLIT_DIST = 40.f,
    .PLAYER_MAX_BOOST = 700.f,
    .PLAYER_MIN_SPLIT_SIZE = 150.f,
    .PLAYER_NO_COLLI_DELAY = 540,
    .PLAYER_NO_EJECT_DELAY = 200,
    .PLAYER_MERGE_TIME = 5.f,
    .PLAYER_VIEW_SCALE = 1.35f,

    .LOCAL_DECAY = 1.1f,
    .STATIC_DECAY = 1.1f,
    .GLOBAL_DECAY = 3.f,
    .DECAY_MIN = 500.f,

    .BOTS = 100,
    .BOT_SPAWN_SIZE = 800.f,

    .EJECT_DISPERSION = 0.0f,
    .EJECT_SIZE = 44.f,
    .EJECT_LOSS = 44.2f,
    .EJECT_DELAY = 50,

    .EX_FAST_BOOST_R = 2500.f,
    .EX_FAST_MERGE_MASS = 80000.f,

    .AI = &mega_ai
};