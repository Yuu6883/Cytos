#pragma once

#include "../../game/bot.hpp"
#include "../../physics/engine.hpp"

struct Rock : Bot {
    
    Rock(Server* server) : Bot(server) {}

    ~Rock() {};

    bool spectatable() { return false; }

    void onTick() {
        if (!control) return;
        if (!control->cells.size()) {
            engine->sync([&] {
                control->requestSpawn();
            });
        } else {
            Cell* cell = control->cells.front();
            float mx = control->__mouseX, my = control->__mouseY;
            float dx = mx - cell->x;
            float dy = my - cell->y;

            if (dx * dx + dy * dy < cell->r * cell->r) {
                engine->sync([&] {
                    control->requestSpawn();
                });
            }
        }
    }
};
