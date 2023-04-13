#pragma once

#include "../omega.hpp"

constexpr OPT omega_bench_opt{.MODE = "bench-omega",
                              .CELL_LIMIT = 262144,

                              .MAP_HW = 64000.f,
                              .MAP_HH = 64000.f,

                              .TIME_SCALE = 1.2f,

                              .GRID_PL_SIZE = 256,
                              .GRID_EV_SIZE = 256,

                              .QUADTREE_MAX_LEVEL = 18,
                              .QUADTREE_MAX_ITEMS = 16,

                              .PERK_INTERVAL = 10.f,
                              .MIN_PERK_SIZE = 100000.f,
                              .MAX_CYT_CELLS = 50,
                              .MAX_EXP_CELLS = 50,

                              .M1_RELAXATION = 0.8f,
                              .M2_RELAXATION = 1.f,

                              .MAX_SPAWN_TRIES = 64,
                              .VIRUS_SAFE_SPAWN_RADIUS = 10.f,

                              .PELLET_COUNT = 10000,
                              .PELLET_SIZE = 30.f,

                              .VIRUS_COUNT = 100,
                              .VIRUS_SIZE = 200,
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

                              .LOCAL_DECAY = 0.7f,
                              .STATIC_DECAY = 1.2f,
                              .GLOBAL_DECAY = 2.f,
                              .DECAY_MIN = 500.f,

                              .ANTI_CAMP_TIME = 1.f,
                              .ANTI_CAMP_MASS = 50000.,
                              .ANTI_CAMP_MULT = 0.05f,

                              .BOTS = 1000,
                              .BOT_SPAWN_SIZE = 750.f,

                              .EJECT_SIZE = 40.f,
                              .EJECT_LOSS = 40.f,
                              .EJECT_DELAY = 100,

                              .EAT_OVERLAP = 3.5,
                              .EAT_MULT = 1.135,

                              .EX_FAST_BOOST_R = 2500.f,
                              .EX_FAST_MERGE_MASS = 0.f,

                              .AI = &omega_ai};
