#pragma once

#include "../../physics/engine.hpp"

template<OPT const& T>
struct RockEngine : TemplateEngine<T> {

    std::uniform_int_distribution<int> rockAxisDist;
    std::uniform_real_distribution<float> rockYDist;
    std::uniform_real_distribution<float> rockSizeDist;
    std::uniform_real_distribution<float> rockSpeedDist;
    std::uniform_real_distribution<float> playerXDist;

    Cell* goal = nullptr;

    RockEngine(Server* server, uint16_t id = 0) : TemplateEngine<T>(server, id),
        rockAxisDist(0, 1), 
        rockYDist(-T.MAP_HH * 0.75, T.MAP_HH * 0.75),
        rockSizeDist(500.f, 1500.f),
        rockSpeedDist(10.f, 20.f),
        playerXDist(-T.MAP_HW * 0.8, T.MAP_HW * 0.8) {
        this->hidePerks = false;
        this->dualEnabled = false;
        initGoal();
    }

    ~RockEngine() {};
    
    void addHandle(GameHandle* handle, uint16_t id = 0);
    void delayKill(Control* control, bool replace = false);

    void restart(bool clearMemory = true);
    void initGoal();
    void spawnViruses() {};
    void spawnEXP() {};
    void spawnCYT() {};

    Bot* addBot(uint16_t botID = 0);
    void spawnBots();
    bool spawnBotControl(Control*& c);
    bool spawnPlayerControl(Control*& c);
    void queryTree(AABB& aabb, const std::function<void(Cell*)> func);
    void postResolve();
    void syncState();
};
