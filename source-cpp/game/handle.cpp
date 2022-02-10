#define _USE_MATH_DEFINES
#include <cmath> 

#include <math.h>
#include "handle.hpp"
#include "control.hpp"
#include "../physics/engine.hpp"

#include <mutex>
using std::unique_lock;

bool GameHandle::isAlive() { return control && control->alive; };
cell_cord_prec GameHandle::getScore() { return control ? control->score : 0.f; };
uint32_t GameHandle::getKills() { return control ? control->kills : 0; };

void GameHandle::join() {
    if (engine) engine->addHandle(this);
}

void GameHandle::remove() {
    if (engine) engine->removeHandle(this);
}

void GameHandle::setEngine(Engine* engine) {
    if (this->engine) remove();
    this->engine = engine;
}

void GameHandle::onTick() {
    viewArea = control ? control->viewport.toAABB().getArea() : 0.f;
}

Point GameHandle::position() {
    if (control) return { control->viewport.x, control->viewport.y };
    else return { 0, 0 };
}

bool GameHandle::canEatPerk() {
    if (isBot()) return false;
    if (perms & TP) return true;
    if (!engine->hidePerks) return true;
    if (!engine || !wasAlive) return false;
    return getScore() > engine->minPerkSize();
}