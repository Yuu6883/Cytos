#include <limits>
#include "control.hpp"
#include "handle.hpp"
#include "../physics/engine.hpp"

Control::Control(Engine* engine, uint16_t id):
    engine(engine), id(id), 
    viewport(0, 0, engine->getMinView(), engine->getMinView()) {
        cells.reserve(engine->getPlayerMaxCell());
    }

Control::~Control() {
}

void Control::afterSpawn() {
    spawning = false;
    ejectMacro = false;
    lineLocked = 0;
    autoRespawn = false;

    ejects = 0;
    splits = 0;

    score = 0.0f;
    kills = 0;

    lastSpawned = engine->__now;
    lastDead = 0;
}

bool Control::canSpawn() {
    if (!overwrites.canSpawn) {
        if (alive) engine->delayKill(this);
        return false;
    }

    // Player requesting spawn OR player is dead and requested auto respawn
    return (spawning || (!alive && autoRespawn));
}

void Control::requestSpawn() {
    spawning = true;
    if (canSpawn()) engine->delayKill(this, true);
}

constexpr cell_cord_prec LINE_ANGLE_THRESH = 0.1;

void Control::lockLine() {
    if (cells.size() != 1) return;
    auto cell = cells.front();
    cell_cord_prec x1 = __mouseX, y1 = __mouseY, x2 = cell->x, y2 = cell->y;
    cell_cord_prec a = atan2(x2 - x1, y1 - y2);

    if (fabs(a - M_PI * 0.5) < LINE_ANGLE_THRESH || fabs(a + M_PI * 0.5) < LINE_ANGLE_THRESH) {
        lineLocked = 2;
        y1 = y2;
    } else if (fabs(a - M_PI) < LINE_ANGLE_THRESH || fabs(a) < LINE_ANGLE_THRESH) {
        lineLocked = 3;
        x1 = x2;
    } else {
        lineLocked = 1;
    }

    linearEquation[0] = y1 - y2;
    linearEquation[1] = x2 - x1;
    linearEquation[2] = x1 * y2 - x2 * y1;

    abSqrSumInvL = 1.f / (linearEquation[0] * linearEquation[0] + linearEquation[1] * linearEquation[1]);
}

void Control::calculateViewport() {

    cell_cord_prec size = 0, size_x = 0, size_y = 0;
    cell_cord_prec x = 0, y = 0, factor = 0;

    constexpr float MF = std::numeric_limits<float>::max();
    AABB aabb { MF, -MF, MF, -MF };

    cell_cord_prec score = 0;

    for (auto cell : cells) {
        const cell_cord_prec sqr = cell->r * cell->r;
        x += cell->x * sqr;
        y += cell->y * sqr;
        aabb.l = std::min(aabb.l, cell_cord_prec(cell->shared.aabb.l));
        aabb.r = std::max(aabb.r, cell_cord_prec(cell->shared.aabb.r));
        aabb.b = std::min(aabb.b, cell_cord_prec(cell->shared.aabb.b));
        aabb.t = std::max(aabb.t, cell_cord_prec(cell->shared.aabb.t));
        score += sqr * 0.01;
        size += sqr;
    }

    this->score = score;

    if (!score || size <= 0) return;

    constexpr cell_cord_prec viewExpand = 1.1f;
    constexpr cell_cord_prec cellFactorBase = 100.f;
    constexpr cell_cord_prec factorExponent = 0.05f;

    factor = powf(cells.size() + cellFactorBase, factorExponent) * 0.01f;
    const cell_cord_prec factoredSize = dynamicViewportFactor * (factor + 1) * sqrtf(score * 100);
    const cell_cord_prec vx = x / size, vy = y / size;
    size_x = size_y = std::max(factoredSize, engine->getMinView()) * engine->getViewScale();
    size_x = std::max(size_x, (vx - aabb.l) * viewExpand);
    size_x = std::max(size_x, (aabb.r - vx) * viewExpand);
    size_y = std::max(size_y, (vy - aabb.b) * viewExpand);
    size_y = std::max(size_y, (aabb.t - vy) * viewExpand);

    this->aabb = aabb;
    viewport.x = vx;
    viewport.y = vy;
    viewport.hw = size_x * overwrites.view;
    viewport.hh = size_y * overwrites.view;
};

void Control::resetTimer() {
    lastSplit = 0;
    lastEject = 0;
    lastPopped = 0;
    lastSpawned = 0;
}