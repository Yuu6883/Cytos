#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <string_view>
#include <vector>
#include "../physics/cell.hpp"

using std::vector;

struct Cell;
struct GameHandle;
struct Engine;

// If any field changes, MUST CHANGE state version
struct Overwrites {
    bool canSpawn = true;
    bool canAuto = true;
    bool canEatPellet = true;
    bool canMerge = true;
    bool canColli = true;
    bool instant = false;
    cell_cord_prec view = 1;
    cell_cord_prec spawn = 1;
    cell_cord_prec decay = 1;
    cell_cord_prec speed = 1;
    cell_cord_prec eject = 1;
    cell_cord_prec boost = 1;
    cell_cord_prec minSize = 0;
    cell_cord_prec maxSize = 100000000.;
    int32_t cells = -1;

    void reset() {
        canSpawn = true;
        instant = false;
        view = 1;
        spawn = 1;
        decay = 1;
        speed = 1;
        eject = 1;
        boost = 1;
        minSize = 0;
        maxSize = 100000000.f;
        cells = -1;
    }
};

struct SplitAttempt {
    uint8_t attempt;
    uint8_t tick;
    SplitAttempt(uint8_t attempt, uint8_t tick) : attempt(attempt), tick(tick) {};
};

struct Control {
    uint16_t id;
    Engine* engine;

    GameHandle* handle = nullptr;

    bool alive       = false;
    bool spawning    = false;
    bool ejectMacro  = false;
    bool autoRespawn = false;

    uint8_t lineLocked  = false;
    cell_cord_prec linearEquation[3] = { 0.f, 0.f, 0.f };
    cell_cord_prec abSqrSumInvL = 0.f;

    cell_cord_prec __mouseX = 0.;
    cell_cord_prec __mouseY = 0.;

    uint16_t splits = 0;
    vector<SplitAttempt> splitAttempts;
    uint16_t ejects = 0;
    uint64_t lastSpawnReq = 0;

    uint64_t lastSplit = 0;
    uint64_t lastEject = 0;
    uint64_t lastPopped = 0;
    uint64_t lastSpawned = 0;
    uint64_t lastDead = 0;

    Overwrites overwrites;

    Rect viewport;
    cell_cord_prec dynamicViewportFactor = 1;
    AABB aabb { 0.f, 0.f, 0.f, 0.f };

    cell_cord_prec score = 0.f;
    uint32_t kills = 0;

    vector<Cell*> cells;
    vector<Cell*> sorted;

    Control(Engine* engine, uint16_t id);
    ~Control();
    
    void lockLine();
    void unlockLine() { lineLocked = 0; }
    void toggleLock() { lineLocked ? unlockLine() : lockLine(); }

    void afterSpawn();
    bool canSpawn();
    void requestSpawn();

    void calculateViewport();
    void resetTimer();
};