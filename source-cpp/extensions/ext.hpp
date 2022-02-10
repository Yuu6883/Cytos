#pragma once

#include "../physics/engine.hpp"

#ifndef VS_SPAWN_SIZE
    #define VS_SPAWN_SIZE 2236.f
#endif

#ifndef VS_LOSE_MASS
    #define VS_LOSE_MASS 10000
#endif

template<OPT const& T>
struct CustomEngine : TemplateEngine<T> {

    enum class RunStage {
        WAIT, STARTING, RUNNING, ENDED
    };

    RunStage stage = RunStage::WAIT;
    
    std::uniform_int_distribution<int> spawnPosDist;
    bool shrink = false;

    CustomEngine(Server* server, uint16_t id): TemplateEngine<T>(server, id), spawnPosDist(0, 3) {
        this->ignoreInput = true;
        this->updateBot = false;
    };

    void start();
    void spawnPellets();
    void spawnViruses();
    void spawnBots();
    void fixedSpawn();
    void checkWinner(bool forceEnd = false);
    void declareWinner(int32_t result);
    void tick(float dt);
    bool freeHandle(GameHandle* handle);
    void onExtCommand(string_view cmd);

    string_view getInitMessage();
    uint32_t extFlags();
    
    std::tuple<float, float, float, float> getFixedLocation(Rect& map, float multi);
};
