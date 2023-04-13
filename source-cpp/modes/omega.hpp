#pragma once

#include "options.hpp"

constexpr BotAI omega_ai{.BOT_SOLOTRICK_MASS = 50000.f,
                         .BOT_SPAWN_MAX = 3000000.f,
                         .BOT_MOVE_MASS = 150000.f,
                         .BOT_IDLE_TIME = 2.5f,
                         .BOT_SOLOTRICK_CHANCE = 15.f,
                         .BOT_SPLIT_CHANCE = 10.f,
                         .BOT_CENTER_FEED_EJECT = 20,
                         .BOT_MAX_SPLIT_ATTEMPT = 4};

constexpr OPT omega_opt{.MODE = "omega",
                        .CELL_LIMIT = 262144,

                        .MAP_HW = 30000.f,
                        .MAP_HH = 30000.f,

                        .TIME_SCALE = 1.2f,

                        .GRID_PL_SIZE = 128,
                        .GRID_EV_SIZE = 128,

                        .QUADTREE_MAX_LEVEL = 18,
                        .QUADTREE_MAX_ITEMS = 20,

                        .PERK_INTERVAL = 10.f,
                        .MIN_PERK_SIZE = 25000.f,

                        .MAX_CYT_CELLS = 20,
                        .MAX_EXP_CELLS = 20,

                        .CYT_CELL_SIZE = 500,
                        .EXP_CELL_SIZE = 500,

                        .M1_RELAXATION = 0.8f,
                        .M2_RELAXATION = 1.f,

                        .MAX_SPAWN_TRIES = 64,
                        .VIRUS_SAFE_SPAWN_RADIUS = 5.f,

                        .PELLET_COUNT = 10000,
                        .PELLET_SIZE = 20.f,

                        .VIRUS_COUNT = 40,
                        .VIRUS_SIZE = 250,
                        .VIRUS_SPLIT_SIZE = 0.f,
                        .VIRUS_MAX_SIZE = 2500.f,
                        .VIRUS_SPLIT_BOOST = 700.f,
                        .VIRUS_MONOTONE_POP = true,

                        .NEW_BOOST_ALGO = true,
                        .BOOST_FACTOR = 1.1f,
                        .BOOST_AMOUNT = 0.75f,
                        .BOOST_DELAY = 1.5f,
                        .BOOST_MULTI = 0.75f,

                        .PLAYER_SPEED = 1.25,
                        .PLAYER_SPAWN_DELAY = 0.5f,
                        .PLAYER_AUTOSPLIT_SIZE = 0.f,
                        .PLAYER_MAX_CELLS = 420,
                        .PLAYER_INIT_SPAWN_SIZE = 830.6623f,
                        .PLAYER_SPAWN_SIZE = 500.f,
                        .PLAYER_SPLIT_BOOST = 700.f,
                        .PLAYER_SPLIT_DIST = 10.f,
                        .PLAYER_SPLIT_CAP_T1 = 100000.f,
                        .PLAYER_SPLIT_CAP_T2 = 75000.f,
                        .PLAYER_SPLIT_ADD_AGE = 0,
                        .PLAYER_MAX_BOOST = 750.f,
                        .PLAYER_MIN_SPLIT_SIZE = 105.f,
                        .PLAYER_NO_COLLI_DELAY = 560,
                        .PLAYER_NO_EJECT_DELAY = 200,
                        .PLAYER_VIEW_SCALE = 1.25f,

                        .LOCAL_DECAY = 4.f,
                        .STATIC_DECAY = 2.5f,
                        .GLOBAL_DECAY = 5.f,
                        .DECAY_MIN = 500.f,

                        .ANTI_CAMP_TIME = 1.f,
                        .ANTI_CAMP_MASS = 50000.,
                        .ANTI_CAMP_MULT = 0.05f,

                        .BOTS = 400,
                        .BOT_SPAWN_SIZE = 400.f,

                        .EJECT_SIZE = 40.f,
                        .EJECT_LOSS = 38.5f,
                        .EJECT_DELAY = 100,

                        .EAT_OVERLAP = 3.5,
                        .EAT_MULT = 1.135,

                        .EX_FAST_BOOST_R = 2500.f,
                        .EX_FAST_MERGE_MASS = 0.f,
                        // .COLLI_RATIO = 5.f,

                        .AI = &omega_ai};
