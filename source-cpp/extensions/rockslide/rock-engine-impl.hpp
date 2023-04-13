#pragma once

#include "rock-engine.hpp"
#include "rock.hpp"

template <OPT const& T>
void RockEngine<T>::addHandle(GameHandle* handle, uint16_t id) {
    TemplateEngine<T>::addHandle(handle, id);
    if (!handle->isBot()) {
        handle->perms |= NO_EAT;
        handle->control->overwrites.decay = 0.f;
    }
}

template <OPT const& T>
void RockEngine<T>::delayKill(Control* control, bool replace) {
    Engine::delayKill(control);
}

template <OPT const& T>
void RockEngine<T>::restart(bool clearMemory) {
    TemplateEngine<T>::restart(clearMemory);
    initGoal();
}

template <OPT const& T>
void RockEngine<T>::initGoal() {
    if (goal && goal->type == CYT_TYPE) return;
    auto& c = this->newCell();
    c.x = 0;
    c.y = T.MAP_HH * 0.9f;
    c.r = 1000.f;
    c.type = CYT_TYPE;
    c.shared.aabb = c.toAABB();
    auto cid = this->cell_id(c);
    this->boosts[cid] = {0, 0, 0};
    goal = &c;
}

template <OPT const& T>
Bot* RockEngine<T>::addBot(uint16_t botID) {
    auto bot = new Rock(this->server);
    bot->setEngine(this);
    this->addHandle(bot, botID);
    this->bots.push_back(bot);

    auto rock = bot->control;
    rock->overwrites.cells = 1;
    rock->overwrites.speed = rockSpeedDist(generator);
    rock->overwrites.minSize = rockSizeDist(generator);
    rock->overwrites.canAuto = false;
    rock->overwrites.canMerge = false;
    rock->overwrites.canColli = false;
    rock->overwrites.canEatPellet = false;

    return bot;
}

template <OPT const& T>
void RockEngine<T>::spawnBots() {
    if (this->bots.size() < 40) addBot();
    if (this->bots.size() > 40) {
        auto rock = this->bots.back();
        if (this->freeHandle(rock)) this->bots.pop_back();
    }
}

template <OPT const& T>
bool RockEngine<T>::spawnBotControl(Control*& rock) {
    auto left = rockAxisDist(generator);

    float x = (left ? -T.MAP_HW : T.MAP_HW);
    float y = rockYDist(generator);

    rock->__mouseX = -x;
    rock->__mouseY = rockYDist(generator);

    rock->overwrites.cells = 1;
    rock->overwrites.speed = rockSpeedDist(generator);
    rock->overwrites.minSize = rockSizeDist(generator);

    auto& cell = this->newCell();
    cell.x = x;
    cell.y = y;
    cell.r = 1000.f;
    cell.type = ROCK_TYPE;
    auto cid = this->cell_id(cell);
    this->boosts[cid] = {0, 0, 0};
    cell.updateAABB();

    this->tree->insert(&cell);
    rock->cells.push_back(&cell);

    rock->viewport.x = x;
    rock->viewport.y = y;
    rock->viewport.hw = 1000.f;
    rock->viewport.hh = 1000.f;
    rock->aabb = cell.shared.aabb;

    return true;
}

template <OPT const& T>
bool RockEngine<T>::spawnPlayerControl(Control*& c) {
    auto& cell = this->newCell();
    cell.x = playerXDist(generator);
    cell.y = T.MAP_HH * -0.9f;
    cell.r = 1000.f;
    cell.type = c->id;
    auto cid = this->cell_id(cell);
    this->boosts[cid] = {0, 0, 0};
    cell.updateAABB();

    this->tree->insert(&cell);

    c->cells.push_back(&cell);
    c->viewport.x = cell.x;
    c->viewport.y = cell.y;
    c->viewport.hw =
        instant_opt.PLAYER_VIEW_MIN * instant_opt.PLAYER_VIEW_SCALE;
    c->viewport.hh =
        instant_opt.PLAYER_VIEW_MIN * instant_opt.PLAYER_VIEW_SCALE;
    c->aabb = cell.shared.aabb;
    c->overwrites.maxSize = 1000.f;

    return true;
}

template <OPT const& T>
void RockEngine<T>::queryTree(AABB& aabb,
                              const std::function<void(Cell*)> func) {
    TemplateEngine<T>::queryTree(aabb, func);
    func(goal);
}

template <OPT const& T>
void RockEngine<T>::postResolve() {
    if (goal && goal->type == CYT_TYPE) {
        unordered_set<uint16_t> winnerTypes;

        this->tree->query(*goal, false, [&](Cell* other, uint32_t) {
            float dx = other->x - goal->x;
            float dy = other->y - goal->y;

            float rSum = goal->r + other->r;
            float dSqr = dx * dx + dy * dy;

            if (dSqr >= rSum * rSum) return;
            winnerTypes.insert(other->type);
        });

        for (auto& winnerT : winnerTypes) {
            auto iter = this->controls.find(winnerT);
            if (iter == this->controls.end()) continue;
            Control* c = iter->second;
            if (!c->handle || c->handle->isBot()) continue;
            if (c->handle->perms & TP) continue;

            this->delayKill(c);
            this->infoEvent(c->handle, Engine::EventType::ROCK_WIN);
        }
    }

    TemplateEngine<T>::postResolve();
}

template <OPT const& T>
void RockEngine<T>::syncState() {
    restart(true);
}
