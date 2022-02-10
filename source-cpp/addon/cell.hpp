#pragma once

#include <math.h>
#include <list>
#include <vector>
#include <algorithm>

#include "colors.hpp"

using std::list;
using std::vector;

constexpr uint16_t EJECT_BIT   = (1 << 14);
constexpr uint16_t PELLET_TYPE = (1 << 14) - 1;
constexpr uint16_t VIRUS_TYPE  = PELLET_TYPE - 1;
constexpr uint16_t DEAD_TYPE   = VIRUS_TYPE - 1;
constexpr uint16_t CYT_TYPE    = DEAD_TYPE - 1;
constexpr uint16_t EXP_TYPE    = CYT_TYPE - 1;
constexpr uint16_t ROCK_TYPE   = EXP_TYPE - 1;

constexpr uint16_t FADE_IN = 0x1;
constexpr uint16_t EATEN   = 0x2;

struct RenderCell {
    uint16_t type;
    uint16_t flags;
    
    float alpha;
    float rotation;
    float animation;

    float oX, oY, oR, cX, cY, cR;
    int32_t nX, nY, nR;

    Color color;

    RenderCell() {};

    void init(uint16_t type, int32_t x, int32_t y, int32_t r) {

        this->type = type;
        oX = cX = nX = x;
        oY = cY = nY = y;
        oR = cR = nR = r;

        flags = FADE_IN;
        alpha = 1;
        rotation = 0;

        if (type == PELLET_TYPE) {
            alpha = 0;
            flags = FADE_IN;
            animation = randomZeroToOne * 1000;
            color =  { 1, 1, 1 };
        } else if (type == VIRUS_TYPE) {
            color = VIRUS_COLOR;
            color.vibrate();
        } else if (type == ROCK_TYPE) {
            color = (randomZeroToOne > 0.5) ? ROCK_COLOR1 : ROCK_COLOR2;
            color.vibrate();
        } else if (type == DEAD_TYPE) {
            color = DEAD_COLOR;
        } else if (type & EJECT_BIT) {
            auto pid = type & PELLET_TYPE;
            color = EJECTS_COLORS[pid % COLOR_COUNT];
            
            alpha = 0;
            flags = FADE_IN;
        } else if (type < ROCK_TYPE) {
            color = CELL_COLORS[type % COLOR_COUNT];
        } else {
            color =  { 1, 1, 1 };
        }
    }

    inline void update(float lerp, float dt, bool animatePellet) {
        if ((type != DEAD_TYPE) && (flags & FADE_IN)) {
            auto duration = (type == PELLET_TYPE) ? 1500 : 500;
            alpha += dt / duration;
            if (alpha >= 1) {
                alpha = 1;
                flags ^= FADE_IN;
            }
        }

        if (type == ROCK_TYPE) {
            auto dx = nX - oX;
            auto dy = nY - oY;
            rotation += (dx > 0 ? -1 : 1) * sqrtf(dx * dx + dy * dy) / cR * dt * 0.01;
        } else if (type == PELLET_TYPE && !(flags & FADE_IN)) {
            if (animatePellet) {
                animation += dt * 0.001f;
                alpha = 0.7f + sinf(animation) * 0.3f;
            } else alpha = 1;
        }

        cX = oX + (nX - oX) * lerp;
        cY = oY + (nY - oY) * lerp;
        cR = oR + (nR - oR) * lerp;
    }

    // Return if this cell should be removed from the container
    inline bool removeUpdate(float dt) {
        animation += dt;

        auto duration = (type == PELLET_TYPE) ? 1500 : 175;
        if (animation > duration) {
            return true;
        } else {
            auto factor = animation / duration;
            alpha = 1 - factor;
            
            if (flags & EATEN) {
                cX = oX + (nX - oX) * factor;
                cY = oY + (nY - oY) * factor;
                cR = oR * (1 - factor);
            }
            return false;
        }
    }
};