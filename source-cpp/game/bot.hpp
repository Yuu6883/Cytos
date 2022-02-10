#pragma once

#include <string>
#include <vector>
#include <cinttypes>

#include "handle.hpp"
#include "control.hpp"
#include "../misc/logger.hpp"

using std::vector;

struct Bot : public GameHandle {

    uint64_t __nextActionTick = 0;
    std::uniform_real_distribution<float> actionPicker;

    Bot(Server* server) : GameHandle(server, "bot"), 
        actionPicker(0.f, 100.f) {
        join();
    }

    virtual ~Bot() {};

    bool isBot() { return true; };
    virtual bool spectatable() { return true; }
    void setNextAction(float seconds);

    virtual void onTick();
};