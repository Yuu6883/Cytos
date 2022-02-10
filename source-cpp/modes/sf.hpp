#pragma once

#include "options.hpp"

constexpr BotAI sf_ai {
    .BOT_SOLOTRICK_MASS = 4000000.f,
    .BOT_SPAWN_MAX = 2000000.f,
    .BOT_SOLOTRICK_CHANCE = 20.f,
    .BOT_SPLIT_CHANCE = 10.f,
    .BOT_SOLOTRICK_CD = 2.f,
    .BOT_SPLIT_CD = 2.f,
    .BOT_CENTER_FEED_EJECT = 10
};

constexpr OPT sf_opt {
    .CELL_LIMIT = 65536,

    .MAP_HW = 32000.f,
    .MAP_HH = 32000.f,

    .TIME_SCALE = 1.2f,
    .DUAL_ENABLED = false,

    .GRID_PL_SIZE = 64,
    .GRID_EV_SIZE = 64,

    .QUADTREE_MAX_LEVEL = 18,
    .QUADTREE_MAX_ITEMS = 20,

    .PERK_INTERVAL = 20,

    .MIN_PERK_SIZE = 250000.f,
    .MAX_CYT_CELLS = 3,
    .MAX_EXP_CELLS = 3,

    .CYT_CELL_SIZE = 2500.f,
    .EXP_CELL_SIZE = 2500.f,

    .MAX_SPAWN_TRIES = 64,
    .VIRUS_SAFE_SPAWN_RADIUS = 10.f,

    .PELLET_COUNT = 1000,
    .PELLET_SIZE = 100.f,

    .VIRUS_COUNT = 0,
    .VIRUS_SIZE = 100.f,
    .VIRUS_SPLIT_SIZE = 150.f,
    .VIRUS_MAX_SIZE = 2500.f,
    .VIRUS_SPLIT_BOOST = 0.f,
    .VIRUS_EXPLODE = false,
    .VIRUS_MONOTONE_POP = false,

    .PLAYER_SPEED = 3.f,
    .PLAYER_AUTOSPLIT_SIZE = 0.f,
    .PLAYER_MAX_CELLS = 256,
    .PLAYER_SPAWN_SIZE = 1000.f,
    .PLAYER_SPLIT_DIST = 50.f,
    .PLAYER_MIN_SPLIT_SIZE = 200.f,
    .PLAYER_MIN_EJECT_SIZE = 70.f,
    .PLAYER_NO_MERGE_DELAY = 1000,
    .PLAYER_MERGE_TIME = 0.f,
    .PLAYER_VIEW_SCALE = 1.5f,

    .LOCAL_DECAY = 1.25f,
    .STATIC_DECAY = 1.25f,
    .DECAY_MIN = 100.f,

    .BOTS = 5,
    .BOT_SPAWN_SIZE = 1000.f,

    .EJECT_SIZE = 60.0f,
    .EJECT_LOSS = 50.2f,
    .EJECT_DELAY = 100.f,

    .AI = &sf_ai
};