#pragma once

#include "../game/handle.hpp"
#include "../game/dual.hpp"

struct Engine;
struct Server;

struct Input {
    bool macro = 0;
    bool spawn = 0;
    bool line = 0;
    uint8_t splits = 0;
    uint8_t ejects = 0;
    int32_t mouseX = 0;
    int32_t mouseY = 0;
};

struct Player : GameHandle {
    
    Input inputs[2];
    uint16_t pids[2] = { 0, 0 };

    uint8_t activeTab;

    vector<CellCache> cache;

    Player(Server* server);

    bool isAlive() {
        return (control && control->alive) ||
            (dual && dual->control && dual->control->alive);
    }

    cell_cord_prec getScore() {
        return (control ? control->score : 0) + (dual && dual->control ? dual->control->score : 0);
    }

    Point position() {
        float x = 0, y = 0;
        int i = 1;
        if (control && control->alive) {
            x += control->viewport.x;
            y += control->viewport.y;
        }

        if (dual && dual->control && dual->control->alive) {
            x += dual->control->viewport.x;
            y += dual->control->viewport.y;
            i++;
        }

        return { x / i, y / i };
    }

    void setEngine(Engine* engine);

    void spec(uint16_t pid);
    void spawn(uint8_t tab);

    void syncInput() override;
    
    void onTick() override;

    void send(string_view buffer);
};