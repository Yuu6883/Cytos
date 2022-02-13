#include <memory.h>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <stdlib.h>

using namespace std::chrono;

#include "engine.hpp"
#include "../game/bot.hpp"
#include "../game/control.hpp"
#include "../game/handle.hpp"
#include "../misc/logger.hpp"
#include "../misc/writer.hpp"

enum class Action { NONE, COL, EAT, MERGE };

Engine::Engine(Server* server, uint16_t id): 
    server(server),
    id(id),
    running(false),
    usage(0.0f),
    __next_cell_id(1),
    cellCount(0),
    timings({}),
    queries({}),
    tree(nullptr)
{
};

#define DIM std::max(T.MAP_HW, T.MAP_HH)

template<OPT const& T>
TemplateEngine<T>::TemplateEngine(Server* server, uint16_t id) : Engine(server, id),
    Grid_PL(Rect(0, 0, DIM, DIM)),
    Grid_EV(Rect(0, 0, DIM, DIM))
{
    auto bound = std::max(T.MAP_HW, T.MAP_HH);
    int32_t dim = 1;
    while (dim < bound) dim = dim << 1;
    
    this->tree = new LooseQuadTree<0.25f>(IRect(0, 0, dim, dim), T.QUADTREE_MAX_LEVEL, T.QUADTREE_MAX_ITEMS);
    this->dualEnabled = T.DUAL_ENABLED;
    this->defaultCanSpawn = T.PLAYER_CAN_SPAWN;
    this->desiredBots = T.BOTS;

#ifdef WIN32
    pool = static_cast<Cell*>(_aligned_malloc(poolSize(), cell_size));
#else
    pool = static_cast<Cell*>(aligned_alloc(cell_size, poolSize()));
#endif

    memset(pool, 0, poolSize());
}

Engine::~Engine() {
    if (running) stop();
    if (pool) {

#ifdef WIN32
        _aligned_free(pool);
#else
        free(pool);
#endif
        pool = nullptr;
    }

    if (tree) delete tree;

    reset();
};

void Engine::reset() {
    for (auto bot : bots) delete bot;
    bots.clear();
    for (auto [_, control] : controls) delete control;
    controls.clear();
    handles.clear();
    restart();
}

void Engine::addHandle(GameHandle* handle, uint16_t cid) {
    if (handle->control) return;
    if (cid <= 0) cid = 1;
    while (controls.find(cid) != controls.cend()) cid++;
    handle->control = new Control(this, cid);
    handle->control->handle = handle;
    controls.insert({ cid, handle->control });
    handles.push_back(handle);

    string_view gatewayID = handle->gatewayID();
    if (gatewayID.length()) {
        infoEvent(handle, EventType::JOIN);
    }
}

void Engine::removeHandle(GameHandle* handle) {
    if (!running) return;
    if (!handle->control) return;
    delayKill(handle->control, true);
    handle->control->handle = nullptr;
    handle->control = nullptr;
    handles.remove(handle);
    
    string_view gatewayID = handle->gatewayID();
    if (gatewayID.length()) {
        infoEvent(handle, EventType::LEAVE);
    }
}

bool Engine::start() {
    if (running) return false;
    __start = uv_hrtime();
    __ltick = 0;
    running = true;
    return true;
}

void Engine::tick(float dt) {
    if (!running) return;
    if (shouldRestart) restart();

    uint64_t t0 = uv_hrtime(), t1 ,t2, t3, t4, t5;

    spawnPellets();
    spawnViruses();
    updatePerks();

    timings.spawn_cells = time_func(t0, t1);

    handleIO(dt);
    timings.handle_io = time_func(t1, t2);
    
    for (auto [c, replace] : killArray) kill(c, replace);
    killArray.clear();

    spawnBots();
    spawnPlayers();
    timings.spawn_handles = time_func(t2, t3);

    updateCells(dt);
    timings.update_cells = time_func(t3, t4);

    resolve(dt);
    timings.resolve_physics = time_func(t4, t5);
}

bool Engine::stop() {
    if (!running) return false;
    running = false;
    return true;
}

template<OPT const& T>
void TemplateEngine<T>::restart(bool clearMemory) {
    
    if (pool && clearMemory) memset(pool, 0, poolSize());

    __start = __now;
    __next_cell_id = 0;
    cellCount = 0;
    
    tree->clear();
    Grid_EV.clear();
    Grid_PL.clear();
    deadCells.clear();
    removedCells.clear();
    killArray.clear();
    spawnSet.clear();
    ejected.clear();
    viruses.clear();
    exps.clear();
    cyts.clear();

    for (auto [_, c] : controls) {
        c->cells.clear();
        c->alive = false;
        c->score = 0;
        c->resetTimer();
    }

    shouldRestart = false;

    this->desiredBots = T.BOTS;

    logger::verbose("Server restarting\n");
}

void Engine::delayKill(Control* control, bool replace) {
    // logger::debug("Kill 0x%p\n", control);

    if (control && control->alive) killArray.push_back({ control, replace });
}

template<OPT const& T>
void TemplateEngine<T>::kill(Control* control, bool replace) {

    constexpr float MASS_THRESH = (100.f + T.PLAYER_SPAWN_SIZE) * 
        (100.f + T.PLAYER_SPAWN_SIZE) * 0.01f;
        
    if (replace && control->score >= MASS_THRESH) {
        for (auto c : control->cells) {
            auto& n = newCell();
            n.x = c->x;
            n.y = c->y;
            n.r = c->r;
            n.shared.aabb = c->shared.aabb;
            n.boost = c->boost;
            n.type = DEAD_TYPE;
            tree->swap(c, &n);
            memset(c, 0, sizeof(Cell));
            deadCells.push_back(&n);
            cellCount--; // Correct the cell count
        }
    } else {
        for (auto c : control->cells) {
            tree->remove(c);
            memset(c, 0, sizeof(Cell));
        }
        cellCount -= control->cells.size();
    }
    control->alive = false;
    control->cells.clear();
}

void Engine::delaySpawn(Control* control) {
    control->spawning = false;
    spawnSet.insert(control);
}

template<OPT const& T>
void TemplateEngine<T>::spawnPellets() {
    for (uint32_t i = 0; i < T.MAX_PELLET_PER_TICK; i++) {
        if (Grid_PL.size() < T.PELLET_COUNT) {
            auto [x, y] = randomPoint(T.PELLET_SIZE);
            auto& cell = newCell();

            cell.x = x;
            cell.y = y;
            cell.r = T.PELLET_SIZE;
            cell.type = PELLET_TYPE;
            cell.boost = { 0.f, 0.f, 0.f };

            Grid_PL.insert(cell);
        } else break;
    }
}

template<OPT const& T>
void TemplateEngine<T>::spawnViruses() {
    for (uint32_t i = 0; i < T.MAX_VIRUS_PER_TICK; i++) {
        if (viruses.size() < T.VIRUS_COUNT) {
            auto&& point = getSafeSpawnPoint(T.VIRUS_SIZE, T.VIRUS_SAFE_SPAWN_RADIUS);
            // if (!success) break;
            if (!point.safe) continue;
            auto& v = newCell();
            
            v.x = point.x;
            v.y = point.y;
            v.r = T.VIRUS_SIZE;
            v.type = VIRUS_TYPE;
            v.boost = { 0.f, 0.f, 0.f };

            Grid_EV.insert(v);
            viruses.push_back(&v);
        } else break;
    }
}

template <OPT const& T>
Bot* TemplateEngine<T>::addBot(uint16_t botID) {
    auto bot = new Bot(server);
    bot->setEngine(this);
    addHandle(bot, botID);
    bots.push_back(bot);

    return bot;
}

template <OPT const& T>
void TemplateEngine<T>::spawnBots() {
    if (bots.size() < this->desiredBots) {
        addBot();
    }
    
    if (bots.size() > this->desiredBots) {
        bots.sort([](auto a, auto b) { return a->getScore() > b->getScore(); });
        auto bot = bots.back();
        if (freeHandle(bot)) bots.pop_back();
    }
}

template <OPT const& T>
void TemplateEngine<T>::spawnPlayers() {
    unordered_set<Control*> copy = spawnSet;
    for (auto c : copy) {
        // Somehow still alive
        if (c->alive || !c->handle) {
            spawnSet.erase(c);
            continue;
        }

        bool s = c->handle->isBot() ? spawnBotControl(c) : spawnPlayerControl(c);

        if (s) {
            c->alive = true;
            spawnSet.erase(c);
            c->afterSpawn();
        }
    }
    influences.clear();
}

template<OPT const&T>
void TemplateEngine<T>::virus(cell_cord_prec x, cell_cord_prec y) {
    auto& v = newCell();
    v.x = x;
    v.y = y;
    v.r = T.VIRUS_SIZE;
    v.type = VIRUS_TYPE;
    v.boost = { 0.f, 0.f, 0.f };

    bounceCell(v);
    
    Grid_EV.insert(v);
    viruses.push_back(&v);
}

template<OPT const& T>
void TemplateEngine<T>::spawnEXP() {
    if (exps.size() < T.MAX_EXP_CELLS) {
        auto&& point = getSafeSpawnPoint(T.EXP_CELL_SIZE, T.PLAYER_SAFE_SPAWN_RADIUS);
        if (!point.safe) return;

        auto& cell = newCell();

        cell.x = point.x;
        cell.y = point.y;
        cell.r = T.EXP_CELL_SIZE;
        cell.age = 25;
        cell.type = EXP_TYPE;
        cell.boost = { 0.f, 0.f, 0.f };
        cell.updateAABB();

        tree->insert(&cell);
        exps.push_back(&cell);
    }
}

template<OPT const& T>
void TemplateEngine<T>::spawnCYT() {
    if (cyts.size() < T.MAX_CYT_CELLS) {
        auto&& point = getSafeSpawnPoint(T.CYT_CELL_SIZE, T.PLAYER_SAFE_SPAWN_RADIUS);
        if (!point.safe) return;

        auto& cell = newCell();

        cell.x = point.x;
        cell.y = point.y;
        cell.r = T.CYT_CELL_SIZE;
        cell.type = CYT_TYPE;
        cell.age = 5;
        cell.boost = { 0.f, 0.f, 0.f };
        cell.updateAABB();

        tree->insert(&cell);
        cyts.push_back(&cell);
    }
}

template<OPT const& T>
bool TemplateEngine<T>::spawnBotControl(Control*& c) {
    auto&& point = getSafeSpawnFromInflu(T.BOT_SPAWN_SIZE, T.PLAYER_SAFE_SPAWN_RADIUS);
    if (point.safe) {
        auto& cell = newCell();

        cell.x = point.x;
        cell.y = point.y;
        cell.r = T.BOT_SPAWN_SIZE;
        cell.type = c->id;
        cell.boost = { 0.f, 0.f, 0.f };
        cell.updateAABB();

        tree->insert(&cell);
        c->cells.push_back(&cell);

        // Making sure both if both tab try to spawn they spawn together
        c->viewport.x = point.x;
        c->viewport.y = point.y;
        c->viewport.hw = T.PLAYER_VIEW_MIN * T.PLAYER_VIEW_SCALE;
        c->viewport.hh = T.PLAYER_VIEW_MIN * T.PLAYER_VIEW_SCALE;
        c->aabb = cell.shared.aabb;
    }
    return point.safe;
}

template<OPT const& T>
bool TemplateEngine<T>::spawnPlayerControl(Control*& c) {
    Control* target = nullptr;
    if (c->handle->dual && 
        c->handle->dual->control &&
        c->handle->dual->control->alive) target = c->handle->dual->control;

    const cell_cord_prec spawnSize = c->overwrites.spawn * T.PLAYER_SPAWN_SIZE;
    auto&& point = getPlayerSpawnPoint(target, spawnSize, target ? target->lastSplit - __now < 1000 * 1000 * 1000 : false);
    if (point.safe) {
        auto& cell = newCell();

        cell.x = point.x;
        cell.y = point.y;
        cell.r = spawnSize;
        cell.type = c->id;
        cell.boost = { 0.f, 0.f, 0.f };
        cell.updateAABB();

        tree->insert(&cell);
        c->cells.push_back(&cell);

        // Making sure both if both tab try to spawn they spawn together
        c->viewport.x = point.x;
        c->viewport.y = point.y;
        c->viewport.hw = T.PLAYER_VIEW_MIN * T.PLAYER_VIEW_SCALE;
        c->viewport.hh = T.PLAYER_VIEW_MIN * T.PLAYER_VIEW_SCALE;
        c->aabb = cell.shared.aabb;
        c->score = T.PLAYER_SPAWN_SIZE * T.PLAYER_SPAWN_SIZE * 0.01f;
    }

    return point.safe;
}

bool Engine::freeHandle(GameHandle* handle) {
    for (auto h : handles) {
        if (h->spectate == handle) {
            // logger::debug("Removed handle %p spectate target %p\n", h, handle);
            h->spectate = nullptr;
        }
    }
    // logger::debug("Free handle: %p (bot: %s)\n", handle, handle->isBot() ? "true" : "false");
    delete handle;
    return true;
}

// std::uniform_int_distribution<int> jitter(-4000000, 3500000);

template<OPT const& T>
void TemplateEngine<T>::handleIO(float dt) {
    unordered_map<uint16_t, Control*> copy = controls;

    cell_cord_prec playerMass = 0.;
    cell_cord_prec botMass = 0.;
    const cell_cord_prec restartMass = map.hw * map.hh / 100. * T.WORLD_RESTART_MULT;

    biggest = nullptr;

    cell_cord_prec highestScore = 0.0f;

    for (auto h : handles) {
        if (h->spectatable() && h->getScore() > highestScore) {
            highestScore = h->getScore();
            biggest = h;
        }
    }

    for (auto& h : handles) h->syncInput();

    static std::uniform_int_distribution<int> rngBool(0, 1);

    uint64_t t0 = uv_hrtime(), t1, t2, t3;

    aliveControls.clear();
    vector<Control*> queue;
    queue.reserve(controls.size());

    for (auto [id, c] : copy) {
        if (c->handle && c->handle->cleanMe()) freeHandle(c->handle);

        if (!c->cells.size() && !c->handle) {
            // Remove all possible reference to this control
            spawnSet.erase(c);
            killArray.erase(std::remove_if(killArray.begin(), killArray.end(), 
                [c](auto pair) { return pair.first == c; }), killArray.end());
            controls.erase(id);
            // logger::debug("Delete 0x%p\n", c);
            delete c;
            continue;
        }

        if (!c->handle) continue;
        if (c->score > restartMass) {
            // TODO: emit oversize
            if (T.WORLD_KILL_OVERSIZE) {
                delayKill(c);
            } else {
                shouldRestart = true;
            }
        }

        influences.push_back({ c->viewport.x, c->viewport.y, sqrtf(c->score * 100) * 1.25f, c });

        if (c->handle->isBot()) botMass += c->score;
        else playerMass += c->score;
        if (c->handle && !c->handle->isBot()) aliveControls.push_back(c);

        if (ignoreInput) continue;
        queue.push_back(c);
    }

    mutex work_m;
    auto queue_size = queue.size();
    
    // Player cells updates
    for (uint32_t _ = 0; _ < server->threadPool->size(); _++) {
        server->threadPool->enqueue([&] {
            // Accumulate ejected cells locally
            vector<Cell*> local_ejected;
            // Estimate how much memory is needed
            local_ejected.reserve(T.PLAYER_MAX_CELLS * queue_size / server->threadPool->size());

            while (true) {
                Control* c = nullptr;

                {
                    std::scoped_lock lock(work_m);

                    if (!queue.size()) break;
                    else {
                        c = queue.back();
                        queue.pop_back();
                    }
                }

                c->calculateViewport();

                cell_cord_prec minSplitSize = 0.f;
                cell_cord_prec splitRadiusThresh = 0.f;
                if constexpr (T.NORMALIZE_THRESH_MASS > 0.f) {
                    const cell_cord_prec multi = std::max(sqrt(c->score / T.NORMALIZE_THRESH_MASS), 1.);
                    minSplitSize = multi * T.PLAYER_MIN_SPLIT_SIZE;
                    splitRadiusThresh = sqrtf(T.NORMALIZE_THRESH_MASS * 100);
                } else {
                    minSplitSize = T.PLAYER_MIN_SPLIT_SIZE;
                }

                constexpr cell_cord_prec boost = T.PLAYER_SPLIT_BOOST;

                // Split
                uint8_t expireTick = 11;
                if (c->score > T.PLAYER_SPLIT_CAP_T1) expireTick = 9;
                else if (c->score > T.PLAYER_SPLIT_CAP_T2) expireTick = 7;

                auto maxCells = c->overwrites.cells;
                if (maxCells <= 0) maxCells = T.PLAYER_MAX_CELLS;

                uint8_t splits = c->splits;
                c->splits = 0;
                if (splits) c->splitAttempts.push_back(SplitAttempt(splits, 0));

                // Split attempts
                for (auto& s : c->splitAttempts) {
                    s.attempt--;
                    s.tick++;

                    auto copy = c->cells;
                    
                    for (auto cell : copy) {

                        if constexpr (T.ULTRA_MERGE) cell->age = 0;
                        if (c->cells.size() >= maxCells) break;
                        if (cell->r < minSplitSize) continue;
                        cell_cord_prec dx = c->__mouseX - cell->x;
                        cell_cord_prec dy = c->__mouseY - cell->y;
                        cell_cord_prec d = sqrt(dx * dx + dy * dy);
                        if (d < 1) dx = 1, dy = 0, d = 1;
                        else dx /= d, dy /= d;
                        
                        if constexpr (T.NORMALIZE_THRESH_MASS > 0.) {
                            const cell_cord_prec multi2 = std::max(cell->r / splitRadiusThresh, 1.);
                            auto b = std::min(multi2 * boost, T.PLAYER_MAX_BOOST);
                            auto n = splitFromCell(cell, cell->r * M_SQRT1_2, { dx, dy, b });
                            c->cells.push_back(n);
                        } else {
                            if constexpr (T.EX_FAST_BOOST_R > 0.) {
                                auto scaledBoost = cell->r > T.EX_FAST_BOOST_R ? (sqrtf(cell->r / T.EX_FAST_BOOST_R) * boost) : boost;
                                auto n = splitFromCell(cell, cell->r * M_SQRT1_2, { dx, dy, scaledBoost });
                                c->cells.push_back(n);
                            } else {
                                auto n = splitFromCell(cell, cell->r * M_SQRT1_2, { dx, dy, boost });
                                c->cells.push_back(n);
                            }
                        }
                    }

                    c->lastSplit = __now;
                }

                c->splitAttempts.erase(std::remove_if(c->splitAttempts.begin(), c->splitAttempts.end(), 
                    [&] (SplitAttempt& s) { return s.attempt <= 0 || s.tick >= expireTick; }), c->splitAttempts.end());

                uint16_t ejectedCount = 0;
                float maxEjectPerTick = dt / T.EJECT_DELAY;
                // Eject
                auto ejects = c->ejects;
                c->ejects = 0;

                auto macro = c->ejectMacro;
                constexpr uint64_t noEjectPopDelayNano = T.PLAYER_NO_EJECT_POP_DEALY * MS_TO_NANO;

                cell_cord_prec overwriteEjectMulti = c->overwrites.eject;
                constexpr cell_cord_prec ejectLossSqr = T.EJECT_LOSS * T.EJECT_LOSS;
                constexpr cell_cord_prec ejectSizeMin = T.PLAYER_MIN_EJECT_SIZE;
                const cell_cord_prec ejectSize    = overwriteEjectMulti * T.EJECT_SIZE;
                const cell_cord_prec ejectBoost   = c->overwrites.boost * T.EJECT_BOOST;

                // Eject
                if (T.PLAYER_NO_EJECT_POP_DEALY > 0 &&
                    __now > c->lastPopped + noEjectPopDelayNano) {

                    while (c->lastEject <= __now + dt * MS_TO_NANO &&
                        (ejects > 0 || macro) && maxEjectPerTick--) {
                            
                        if (ejects) ejects--;
                        ejectedCount++;

                        uint16_t etype = c->id | EJECT_BIT;
                        for (auto cell : c->cells) {
                            const cell_cord_prec r = cell->r;
                            if (r < ejectSizeMin || cell->age < T.PLAYER_NO_EJECT_DELAY) continue;
                            
                            cell_cord_prec dx = c->__mouseX - cell->x;
                            cell_cord_prec dy = c->__mouseY - cell->y;
                            cell_cord_prec d = sqrt(dx * dx + dy * dy);
                            if (d < 1) dx = 1, dy = 0, d = 1;
                            else dx /= d, dy /= d;

                            const cell_cord_prec sx = cell->x + dx * r;
                            const cell_cord_prec sy = cell->y + dy * r;
                            
                            if constexpr (T.EJECT_DISPERSION > 0.f) {
                                std::uniform_real_distribution<cell_cord_prec> randomEject(-T.EJECT_DISPERSION, T.EJECT_DISPERSION);
                                const cell_cord_prec angle = atan2f(dx, dy) + randomEject(generator);
                                dx = sinf(angle);
                                dy = cosf(angle);
                            }
                            
                            auto& n = newCell();

                            n.x = sx;
                            n.y = sy;
                            n.r = ejectSize;
                            n.type = etype;
                            n.boost = { dx, dy, ejectBoost };
                            
                            local_ejected.push_back(&n);
                            Grid_EV.insert(n);

                            cell->r = sqrtf(r * r - ejectLossSqr);
                            cell->flag |= UPDATE_BIT;
                        }

                        // WTF: floating point error makes solotrick actually OP but breaks after 7 days
                        uint64_t newT = __now + T.EJECT_DELAY * MS_TO_NANO * ejectedCount;
                        uint64_t oldT = __now + T.EJECT_DELAY * MS_TO_NANO_F * ejectedCount;

                        // if (newT != oldT) logger::debug("Diff: %i ms\n", ((long) newT - (long) oldT));
                        // c->lastEject = newT + jitter(generator);
                        c->lastEject = oldT;
                    }
                }

                std::scoped_lock lock(m);
                // Spawn
                if (c->canSpawn()) {
                    auto dt = ((__now - std::max(c->lastSpawned, c->lastDead)) / 1000 / 1000) / 1000.f;
                    if (dt > T.PLAYER_SPAWN_DELAY) delaySpawn(c);
                }
            }
        
            if (!local_ejected.size()) return;
            // Append vector
            std::scoped_lock lock(m);
            ejected.insert(ejected.end(), local_ejected.begin(), local_ejected.end());
        });
    }
    server->threadPool->sync();

    timings.io.phase0 = time_func(t0, t1);;

    tree->restructure();
    this->playerMass.store(playerMass);
    this->botMass.store(botMass);

    timings.io.phase1 = time_func(t1, t2);;

    mutex qm;

    auto hcopy = handles;
    vector<GameHandle*> seq;
    seq.reserve(players.load());

    // Filter out player to be sequentially executed
    auto iter = hcopy.begin();
    while (iter != hcopy.end()) {
        if (!(*iter)->isBot()) {
            seq.push_back(*iter);
            iter = hcopy.erase(iter);
        } else iter++;
    }

    for (uint32_t i = 0; i < server->threadPool->size(); i++) {
        // Basically all bots after we filter it
        server->threadPool->enqueue([&] {
            while (true) {
                GameHandle* h = nullptr;
                qm.lock();
                if (!hcopy.size()) {
                    qm.unlock();
                    break;
                } else {
                    h = hcopy.back();
                    hcopy.pop_back();
                    qm.unlock();
                }
                h->onTick();
            }
        });
    }

    // Single thread because we might call into JS
    for (auto& h : seq) h->onTick();
    server->threadPool->sync();
    
    timings.io.phase2 = time_func(t2, t3);

    // Minimap, not needed if we only have 1 player ):
    // constexpr uint32_t lbmm_repeat = 1000 / T.LBMM_TPS;
    // if (__now > __lbmm + (lbmm_repeat * MS_TO_NANO)) {
    //     __lbmm = __now;

    //     Writer w;
    //     handles.sort([](auto a, auto b) {
    //         return a->getScore() > b->getScore();
    //     });
    //     for (auto h : handles) {
    //         if (h->showOnLBMM && h->isAlive()) {
    //             auto p = h->position();
    //             w.write<uint16_t>(h->control->id);
    //             w.write<uint16_t>(h->dual ? h->dual->control->id : 0);
    //             w.write<float>(p.x);
    //             w.write<float>(p.y);
    //             w.write<float>(h->getScore());
    //         }
    //     }
    //     w.write<uint16_t>(0);
    //     emit(&GameHandle::onLBMMData, w.buffer());
    // }
}

template<OPT const& T>
void TemplateEngine<T>::updatePerks() {
    constexpr uint32_t perk_repeat = 1000 * T.PERK_INTERVAL;
    if (__now > __perk + (perk_repeat * 1000000l)) {
        __perk = __now;
        spawnEXP();
        spawnCYT();
    }
}

template<OPT const& T>
void TemplateEngine<T>::removeCells() {
    const uint32_t step = server->threadPool->size();
    
    for (uint32_t i = 0; i < step; i++) {
        server->threadPool->enqueue([&, i] {
            for (uint32_t j = i; j < removedCells.size(); j += step) {
                auto cell = removedCells[j];
                if (cell->type == VIRUS_TYPE || cell->type & EJECT_BIT) {
                    Grid_EV.remove(*cell);
                } else if (cell->type == PELLET_TYPE) {
                    Grid_PL.remove(*cell);
                }
                if (IS_NOT_PLAYER(cell->type)) cellCount--;
                memset(cell, 0, sizeof(Cell));
            }
        });
    }
    server->threadPool->sync();
    removedCells.clear();
}

template<OPT const& T>
void TemplateEngine<T>::updateCells(float dt) {
    constexpr cell_cord_prec staticDecay = T.STATIC_DECAY * 0.01f;

    removeCells();

    const uint32_t step = server->threadPool->size();
    // Constant boost rate
    for (auto cell : exps) {
        if (boostCell(*cell, dt)) {
            bounceCell(*cell);
            cell->boost.d = T.EJECT_BOOST;
            cell->updateAABB();
            tree->update(cell);
        }
    }

    for (auto cell : cyts) {
        if (boostCell(*cell, dt)) {
            bounceCell(*cell);
            cell->boost.d = T.EJECT_BOOST;
            cell->updateAABB();
            tree->update(cell);
        }
    }

    // Update ejected cells
    if (step <= 1 || ejected.size() < 1000) {
        for (auto cell : ejected) {
            cell->age += dt;
            cell->flag &= CLEAR_BITS;

            if constexpr (T.EJECT_MAX_AGE > 0) {
                if (cell->age > T.EJECT_MAX_AGE) cell->flag |= REMOVE_BIT;
            }

            if (boostCell(*cell, dt)) {
                bounceCell(*cell);
                Grid_EV.update(*cell);
                constexpr size_t s = sizeof(Grid_EV);
            }
        }
    // Multithread
    } else {
        for (int i = 0; i < step; i++) {
            server->threadPool->enqueue([this, i, step, &dt] {
                for (int j = i; j < ejected.size(); j += step) {
                    auto& cell = ejected[j];
                    cell->age += dt;
                    cell->flag &= CLEAR_BITS;
                    
                    if constexpr (T.EJECT_MAX_AGE > 0) {
                        if (cell->age > T.EJECT_MAX_AGE) cell->flag |= REMOVE_BIT;
                    }

                    if (boostCell(*cell, dt)) {
                        bounceCell(*cell);
                        Grid_EV.update(*cell);
                    }
                }
            });
        }
        server->threadPool->sync();
    }

    // Update dead cells
    for (auto cell : deadCells) {
        cell->age += dt;
        cell->flag &= CLEAR_BITS;
        if (boostCell(*cell, dt)) {
            bounceCell(*cell);
            cell->updateAABB();
            tree->update(cell);
        }
    }

    // Update viruses
    if (step <= 1 || viruses.size() < 1000) {
        for (auto cell : viruses) {
            cell->age += dt;
            cell->flag &= CLEAR_BITS;
            if (boostCell(*cell, dt)) {
                bounceCell(*cell);
                Grid_EV.update(*cell);
            }
        }
    } else {
        for (int i = 0; i < step; i++) {
            server->threadPool->enqueue([this, i, step, &dt] {
                for (int j = i; j < viruses.size(); j += step) {
                    auto& cell = viruses[j];
                    cell->age += dt;
                    cell->flag &= CLEAR_BITS;
                    if (boostCell(*cell, dt)) {
                        bounceCell(*cell);
                        Grid_EV.update(*cell);
                    }
                }
            });
        }
        server->threadPool->sync();
    }

    mutex work_m;
    vector<Control*> copy;
    
    copy.reserve(controls.size());
    for (auto [_, c] : controls) copy.push_back(c);

    constexpr cell_cord_prec dMass = 0.01f * T.DECAY_MIN * T.DECAY_MIN;
    constexpr cell_cord_prec localMulti = 0.00001f * T.LOCAL_DECAY;
    const cell_cord_prec mapMass = map.hw * map.hh * 0.04f;
    const cell_cord_prec globalMulti = 1.f + playerMass / mapMass * T.GLOBAL_DECAY;
    const cell_cord_prec pMulti = localMulti * globalMulti;

    // Player cells updates
    for (uint32_t _ = 0; _ < server->threadPool->size(); _++) {
        if (ignoreInput) break;

        server->threadPool->enqueue([&] {

            while (true) {
                Control* c = nullptr;

                {
                    std::scoped_lock lock(work_m);
                    if (!copy.size()) break;
                    else {
                        c = copy.back();
                        copy.pop_back();
                    }
                }

                if (!c->alive) continue;
                cell_cord_prec decayMulti = (c->score - dMass) * pMulti;
                
                // Shrink viewport if camping detected
                if constexpr (T.ANTI_CAMP_TIME > 0) {
                    constexpr cell_cord_prec camp = T.ANTI_CAMP_TIME * 1000 * MS_TO_NANO;
                    const cell_cord_prec nosplit = __now - c->lastSplit;
                    if (nosplit > camp && c->score > T.ANTI_CAMP_MASS) {
                        cell_cord_prec campMulti = nosplit / MS_TO_NANO_F / 1000;
                        if (c->handle && !c->handle->isBot()) {
                            decayMulti *= std::min((1. + T.ANTI_CAMP_MULT * campMulti), 4.);
                            c->dynamicViewportFactor = std::max(0., 1. - T.ANTI_CAMP_MULT * campMulti);
                        }
                    } else {
                        c->dynamicViewportFactor = 1.;
                    }
                }
                
                uint16_t allFlags = 0;
                uint8_t locked = c->lineLocked;
                cell_cord_prec mx = c->__mouseX;
                cell_cord_prec my = c->__mouseY;

                // Remove extra cells
                if (c->overwrites.cells > 0 && c->cells.size() > c->overwrites.cells) {
                    for (int i = c->overwrites.cells; i < c->cells.size(); i++) {
                        auto cell = c->cells[i];
                        tree->remove(cell);
                        memset(cell, 0, sizeof(Cell));
                    }
                    cellCount -= (c->cells.size() - c->overwrites.cells);
                    c->cells.resize(c->overwrites.cells);
                }

                vector<Cell*> cellsCopy = c->cells;

                const bool invincible = c->handle ? c->handle->perms & INVINCIBLE : false;

                auto decay = c->overwrites.decay;
                auto speed = c->overwrites.speed;
                auto minC = c->overwrites.minSize;
                auto maxC = c->overwrites.maxSize;
                auto instant = T.ULTRA_MERGE || c->overwrites.instant;

                for (auto cell : cellsCopy) {
                    cell->age += dt;
                    cell->flag &= CLEAR_BITS;

                    // Boost and bounce cell
                    boostCell(*cell, dt);
                    bounceCell(*cell);

                    // Decay
                    if (cell->r > T.DECAY_MIN && cell->age > T.PLAYER_NO_COLLI_DELAY)
                        cell->r -= decay * decayMulti * cell->r * staticDecay * dt * 0.0001f;

                    cell->r = cell->r > maxC ? maxC : cell->r < minC ? minC : cell->r; 

                    // Calculate collision bit
                    if (c->overwrites.canColli && cell->age > T.PLAYER_NO_COLLI_DELAY)
                        cell->flag |= COLL_BIT;

                    if (invincible) cell->flag |= NOEAT_BIT;

                    // Calculate autosplit
                    if constexpr (T.PLAYER_AUTOSPLIT_SIZE > 0) {
                        if (c->overwrites.canAuto && cell->r > T.PLAYER_AUTOSPLIT_SIZE) {
                            cell_cord_prec angle = rngAngle();
                            auto n = splitFromCell(cell, cell->r * M_SQRT1_2, 
                                { sinf(angle), cosf(angle), T.PLAYER_SPLIT_BOOST });
                            c->cells.push_back(n);
                        }
                    }
                    
                    constexpr float noMergeDelay = T.PLAYER_NO_MERGE_DELAY;
                    float mergeTime;
                    cell_cord_prec initial = 0.;

                    // Calc merge
                    if constexpr (T.PLAYER_MERGE_TIME > 0) {
                        constexpr cell_cord_prec mergeIncrease = T.PLAYER_MERGE_INCREASE;
                        initial = 10000. * T.PLAYER_MERGE_TIME;
                        const cell_cord_prec increase = 100. * cell->r * mergeIncrease;

                        if constexpr (T.PLAYER_MERGE_NEW_VER) {
                            mergeTime = instant ? (T.PLAYER_NO_COLLI_DELAY + T.ULTRA_MERGE_DELAY) : std::max(increase, initial);
                        } else {
                            mergeTime = increase + initial;
                        }
                    } else {
                        mergeTime = noMergeDelay;
                    }

                    if (c->overwrites.canMerge && (cell->age > mergeTime 
                        || (T.EX_FAST_MERGE_MASS && (cell->r < T.PLAYER_MIN_EJECT_SIZE && 
                            c->score > T.EX_FAST_MERGE_MASS && cell->age > initial)))) {

                        cell->flag |= MERGE_BIT;
                    }

                    movePlayerCell(*cell, dt, mx, my, locked, allFlags, speed);
                
                    cell->updateAABB();
                    tree->update(cell);
                }

                // Unlock line if any cell hit wall with normal line
                if ((locked == 1) && (allFlags & WALL_BIT)) {
                    // std::cout << "???" << std::endl;
                    c->unlockLine();
                }
            }
        });
    }

    server->threadPool->sync();
    tree->restructure();
}

/** Super long function incoming */
template<OPT const& T>
void TemplateEngine<T>::resolve(float dt) {
    // 1. Resolve player-player collisions/eat (same type)
    // 2. Resolve player-player eat (different types)
    // 3. Resolve player-pellet eat
    // 4. Resolve player-eject-virus eat
    // 5. Resolve ejected-eject collision and ejected-virus eat

    mutex qm;
    
    uint64_t t0 = uv_hrtime(), t1 ,t2, t3, t4, t5, t6, t7, t8;
    
    vector<Control*> temp;
    temp.reserve(controls.size());
    for (auto [_, c] : controls) if (c->cells.size()) temp.push_back(c);
    std::sort(temp.begin(), temp.end(), [](auto c1, auto c2) { return c1->score > c2->score; });

    atomic<uint64_t> total_queries = 0;
    atomic<uint64_t> effective_queries = 0;
    memset(queries.level_counter, 0, sizeof(queries.level_counter));
    memset(queries.level_efficient, 0, sizeof(queries.level_efficient));
    mutex counter_m;

    vector<Control*> copy = temp;
    for (uint32_t _ = 0; _ < server->threadPool->size(); _++) {
        server->threadPool->enqueue([&] {
            uint64_t effi = 0;
            uint64_t total = 0;
            uint64_t local_level_counter[QUERY_LEVEL] = {};
            uint64_t local_level_efficient[QUERY_LEVEL] = {};

            while (true) {
                Control* c = nullptr;
                qm.lock();
                if (!copy.size()) {
                    qm.unlock();
                    break;
                } else {
                    c = copy.back();
                    copy.pop_back();
                    qm.unlock();
                }

                // Sort the cells
                c->sorted = c->cells;
                std::sort(c->sorted.begin(), c->sorted.end(), [](auto a, auto b) {
                    // if (a->boost.d != b->boost.d) 
                    //     return a->boost.d > b->boost.d;
                    // else
                    return a->r > b->r;
                });

                if (!c->overwrites.canMerge && !c->overwrites.canColli) continue;

                bool instant = T.ULTRA_MERGE || c->overwrites.instant;

                // Player collisions and merge
                for (auto cell : c->sorted) {
                    uint16_t flags = cell->flag;
                    uint16_t type = cell->type;
                    // Skip resolve bits
                    if (flags & SKIP_RESOLVE_BITS) continue;

                    tree->query(*cell, true, [&](Cell* other, uint32_t level) {
                        total++; // TODO: remove
                        if (level < QUERY_LEVEL) local_level_counter[level]++;
                        
                        // This flag is only written to from the same thread, no need for atomic rw here
                        uint16_t otherFlags = other->flag;

                        if (otherFlags & SKIP_RESOLVE_BITS) return;
                        // Double check is good or not??
                        if (cell->r < other->r) return;

                        Action action = Action::NONE;

                        if (type == other->type) {
                            uint16_t flagsAND = flags & otherFlags;
                            if (flagsAND & MERGE_BIT)     action = Action::MERGE;
                            else {
                                if constexpr (T.ULTRA_MERGE) {
                                    if ((flags | otherFlags) & COLL_BIT) action = Action::COL;
                                } else {
                                    if (flagsAND & COLL_BIT) action = Action::COL;
                                }
                            }
                        }

                        // Do nothing
                        if (action == Action::NONE) return;

                        cell_cord_prec r2 = other->r;
                        // Basic condition to eat
                        if (action == Action::EAT && cell->r < r2 * T.EAT_MULT) return;

                        cell_cord_prec dx = other->x - cell->x;
                        cell_cord_prec dy = other->y - cell->y;

                        cell_cord_prec rSum = cell->r + r2;
                        cell_cord_prec dSqr = dx * dx + dy * dy;

                        if (!dSqr || dSqr >= rSum * rSum) return;
                        cell_cord_prec d = sqrt(dSqr);

                        effi++; // Indeed intersection
                        if (level < QUERY_LEVEL) local_level_efficient[level]++;

                        if (action == Action::COL) {
                            cell_cord_prec m = rSum - d;

                            dx /= d;
                            dy /= d;
                        
                            if (d + r2 < cell->r) other->flag |= INSIDE_BIT;
                            
                            cell_cord_prec a = cell->r * cell->r;
                            cell_cord_prec b = r2 * r2;
                            cell_cord_prec sum = a + b;
                            cell_cord_prec aM = b / sum;
                            cell_cord_prec bM = a / sum;

                            cell_cord_prec m1 = (m < cell->r ? m : cell->r) * aM;
                            cell->x -= dx * m1;
                            cell->y -= dy * m1;

                            cell_cord_prec m2 = (m < r2 ? m : r2) * bM;
                            other->x += dx * m2;
                            other->y += dy * m2;

                            // constexpr cell_cord_prec MIN_RELAX = T.PLAYER_MIN_EJECT_SIZE * 3.0f;
                            // if (instant || cell->r > T.RELAXATION_RATIO_THRESH * r2 || r2 < MIN_RELAX) {
                            //     cell_cord_prec m1 = (m < cell->r ? m : cell->r) * aM;
                            //     cell->x -= dx * m1;
                            //     cell->y -= dy * m1;

                            //     constexpr bool boostCutoff = (T.NEW_BOOST_ALGO ? 0.f : 1.f);
                            //     const cell_cord_prec modifier = cell->boost.d > boostCutoff ? 1.5f : 1.f;

                            //     cell_cord_prec m2 = (m < r2 ? m : r2) * bM * modifier;
                            //     other->x += dx * m2;
                            //     other->y += dy * m2;
                            // } else {
                            //     cell_cord_prec m1 = (m < cell->r ? m : cell->r) * aM * T.M1_RELAXATION;
                            //     cell->x -= dx * m1;
                            //     cell->y -= dy * m1;

                            //     cell_cord_prec m2 = (m < r2 ? m : r2) * bM * T.M2_RELAXATION;
                            //     other->x += dx * m2;
                            //     other->y += dy * m2;
                            // }

                            cell->flag = flags | UPDATE_BIT;
                            other->flag = otherFlags | UPDATE_BIT;

                        } else {
                            if (d >= cell->r - r2 / T.EAT_OVERLAP) return;

                            cell->r = sqrt(cell->r * cell->r + r2 * r2);
                            other->eatenByID = cell_id(cell);
                            
                            cell->flag = flags | UPDATE_BIT;
                            other->flag = otherFlags | REMOVE_BIT;
                        }
                    });
                }
            }
        
            total_queries += total;
            effective_queries += effi;

            counter_m.lock();
            for (int i = 0; i < QUERY_LEVEL; i++) {
                queries.level_counter[i] += local_level_counter[i];
                queries.level_efficient[i] += local_level_efficient[i];
            }
            counter_m.unlock();
        });
    }
    server->threadPool->sync();
    timings.physics.phase0 = time_func(t0, t1);
    queries.phase0_total = total_queries.exchange(0);
    queries.phase0_effi = effective_queries.exchange(0);

    copy = temp;
    for (uint32_t i = 0; i < server->threadPool->size(); i++) {
        server->threadPool->enqueue([&] {
            uint64_t effi = 0;
            uint64_t total = 0;

            while (true) {
                Control* c = nullptr;
                qm.lock();
                if (!copy.size()) {
                    qm.unlock();
                    break;
                } else {
                    c = copy.back();
                    copy.pop_back();
                    qm.unlock();
                }
                // No need to sort again

                // uint16_t dualType = 0;
                // if (c->handle && c->handle->dual && c->handle->dual->control) dualType = c->handle->dual->control->id;
                if (!c->handle) continue;
                if (c->handle->perms & NO_EAT) continue;

                if (c->handle->canEatPerk()) {
                    
                    // Player eat (WITH perk implemented)
                    for (auto cell : c->sorted) {
                        uint16_t type = cell->type;

                        // Skip resolve bits
                        if (cell->flag & SKIP_RESOLVE_BITS) continue;

                        tree->query(*cell, true, [&](Cell* other, uint32_t) {
                            total++;

                            uint16_t otherFlags = other->flag.load(std::memory_order_relaxed);

                            if (otherFlags & SKIP_RESOLVE_BITS) return;
                            if (cell->r < other->r) return;

                            if (type == other->type) return;
                            if (!(cell->flag & NOEAT_BIT) && (otherFlags & NOEAT_BIT)) return;

                            cell_cord_prec r2 = other->r;
                            // Basic condition to eat
                            if (cell->r < r2 * T.EAT_MULT) return;

                            cell_cord_prec dx = other->x - cell->x;
                            cell_cord_prec dy = other->y - cell->y;

                            cell_cord_prec rSum = cell->r + r2;
                            cell_cord_prec dSqr = dx * dx + dy * dy;

                            if (!dSqr || dSqr >= rSum * rSum) return;
                            
                            effi++; // Indeed intersection
                            cell_cord_prec d = sqrt(dSqr);

                            if (d >= cell->r - r2 / T.EAT_OVERLAP) return;
                            if (cell->flag & SKIP_RESOLVE_BITS) return;
                            if (!other->flag.compare_exchange_weak(otherFlags, 
                                uint16_t(otherFlags | REMOVE_BIT),
                                std::memory_order_release,
                                std::memory_order_relaxed)) return;

                            other->eatenByID = cell_id(cell);

                            if (other->type == EXP_TYPE) {
                                c->handle->perks.exps += uint16_t(other->age);
                            } else if (other->type == CYT_TYPE) {
                                c->handle->perks.cyts += uint16_t(other->age);
                            } else {
                                cell->r = sqrt(cell->r * cell->r + r2 * r2);
                                cell->flag |= UPDATE_BIT;
                            }
                        });
                    }
                } else {
                    
                    // Player eat (WITHOUT perk implemented)
                    for (auto cell : c->sorted) {
                        uint16_t type = cell->type;

                        // Skip resolve bits
                        if (cell->flag & SKIP_RESOLVE_BITS) continue;

                        tree->query(*cell, true, [&](Cell* other, uint32_t) {
                            total++;

                            uint16_t otherFlags = other->flag.load(std::memory_order_relaxed);

                            if (otherFlags & SKIP_RESOLVE_BITS) return;
                            if (cell->r < other->r) return;

                            if (type == other->type || other->type == EXP_TYPE || other->type == CYT_TYPE) return;
                            if (!(cell->flag & NOEAT_BIT) && (otherFlags & NOEAT_BIT)) return;

                            cell_cord_prec r2 = other->r;
                            // Basic condition to eat
                            if (cell->r < r2 * T.EAT_MULT) return;

                            cell_cord_prec dx = other->x - cell->x;
                            cell_cord_prec dy = other->y - cell->y;

                            cell_cord_prec rSum = cell->r + r2;
                            cell_cord_prec dSqr = dx * dx + dy * dy;

                            if (!dSqr || dSqr >= rSum * rSum) return;
                            
                            effi++;
                            cell_cord_prec d = sqrt(dSqr);

                            if (d >= cell->r - r2 / T.EAT_OVERLAP) return;
                            if (cell->flag & SKIP_RESOLVE_BITS) return;
                            if (!other->flag.compare_exchange_weak(otherFlags, 
                                uint16_t(otherFlags | REMOVE_BIT),
                                std::memory_order_release,
                                std::memory_order_relaxed)) return;

                            other->eatenByID = cell_id(cell);
                            
                            cell->r = sqrt(cell->r * cell->r + r2 * r2);
                            cell->flag |= UPDATE_BIT;
                        });
                    }
                }
            }
        
            total_queries += total;
            effective_queries += effi;
        });
    }
    server->threadPool->sync();
    timings.physics.phase1 = time_func(t1, t2);
    queries.phase1_total = total_queries.exchange(0);
    queries.phase1_effi = effective_queries.exchange(0);
    
    // Dead cell collision resolve
    for (auto cell : deadCells) {

        if (cell->age > T.PLAYER_DEAD_DELAY) {
            cell->flag |= REMOVE_BIT;
            continue;
        }
        
        uint16_t flags = cell->flag;
        uint16_t type = cell->type;
        // Skip resolve bits
        if (flags & SKIP_RESOLVE_BITS) continue;

        cell_cord_prec x = cell->x;
        cell_cord_prec y = cell->y;
        cell_cord_prec r = cell->r;
        cell_cord_prec a = r * r;

        tree->query(*cell, true, [&](Cell* other, uint32_t) {
            uint16_t otherFlags = other->flag;

            if (otherFlags & SKIP_RESOLVE_BITS) return;
            if (r < other->r) return;

            if (other->type != DEAD_TYPE) return;
            
            cell_cord_prec r2 = other->r;
            cell_cord_prec dx = other->x - x;
            cell_cord_prec dy = other->y - y;

            cell_cord_prec rSum = r + r2;
            cell_cord_prec dSqr = dx * dx + dy * dy;

            if (!dSqr || dSqr >= rSum * rSum) return;
            cell_cord_prec d = sqrt(dSqr);
            cell_cord_prec m = rSum - d;

            dx /= d;
            dy /= d;
        
            if (d + r2 < r) other->flag |= INSIDE_BIT;
            
            cell_cord_prec b = r2 * r2;
            cell_cord_prec sum = a + b;
            cell_cord_prec aM = b / sum;
            cell_cord_prec bM = a / sum;

            cell_cord_prec m1 = (m < r ? m : r) * aM;
            x -= dx * m1;
            y -= dy * m1;

            cell_cord_prec m2 = (m < r2 ? m : r2) * bM;
            other->x += dx * m2;
            other->y += dy * m2;

            cell->flag = flags | UPDATE_BIT;
            other->flag = otherFlags | UPDATE_BIT;
        });

        cell->x = x;
        cell->y = y;
    }
    timings.physics.phase2 = time_func(t2, t3);

    // Player cell to ejected cells and virus
    copy = temp;
    for (uint32_t i = 0; i < server->threadPool->size(); i++) {
        server->threadPool->enqueue([&] {
            while (true) {
                Control* c = nullptr;
                qm.lock();
                if (!copy.size()) {
                    qm.unlock();
                    break;
                } else {
                    c = copy.back();
                    copy.pop_back();
                    qm.unlock();
                }
                    
                if (!c->handle) return;
                bool skipOthers = c->handle->perms & NO_EAT;

                for (auto cell : c->sorted) {
                    uint16_t flags = cell->flag;
                    uint16_t type = cell->type;
                    // Skip resolve bits
                    if (flags & SKIP_RESOLVE_BITS) continue;

                    bool escape = false;

                    Grid_EV.query(cell->shared.aabb, [&](Cell* other) {
                        if (skipOthers && type != (other->type & PELLET_TYPE)) return;
                        auto otherFlags = other->flag.load(std::memory_order_relaxed);
                        if (otherFlags & REMOVE_BIT) return;
                        cell_cord_prec r2 = other->r;
                        cell_cord_prec dx = other->x - cell->x;
                        cell_cord_prec dy = other->y - cell->y;
                        cell_cord_prec d = sqrt(dx * dx + dy * dy);
                        if ((cell->r > r2 * T.EAT_MULT) && (d < cell->r - r2 / T.EAT_OVERLAP)) {
                            cell->r = sqrt(cell->r * cell->r + r2 * r2);

                            if (!other->flag.compare_exchange_weak(otherFlags, 
                                uint16_t(otherFlags | REMOVE_BIT),
                                std::memory_order_release,
                                std::memory_order_relaxed)) {
                                    escape = true;
                                    return;
                                }

                            if (other->type == VIRUS_TYPE) {
                                constexpr uint16_t t = UPDATE_BIT | POP_BIT;
                                cell->flag |= t;
                                escape = true;
                            } else {

                                // Boost player cell
                                if constexpr(T.NEW_BOOST_ALGO) {
                                    if (cell->boost.d <= T.PLAYER_MAX_BOOST) {
                                        cell->boost.x *= cell->boost.d;
                                        cell->boost.y *= cell->boost.d;

                                        cell->boost.x += other->boost.x * T.BOOST_AMOUNT;
                                        cell->boost.y += other->boost.y * T.BOOST_AMOUNT;
                                        
                                        cell->boost.d = sqrt(cell->boost.x * cell->boost.x + cell->boost.y * cell->boost.y);
                                        cell->boost.x /= cell->boost.d;
                                        cell->boost.y /= cell->boost.d;
                                    }
                                } else {
                                    cell_cord_prec ratio = other->r / (cell->r + 100.f);
                                    cell->boost.d += ratio * 0.025f * other->boost.d;
                                    if (cell->boost.d >= T.PLAYER_MAX_BOOST) cell->boost.d = T.PLAYER_MAX_BOOST;
                                    
                                    cell_cord_prec bx = cell->boost.x + ratio * 0.02f * other->boost.x;
                                    cell_cord_prec by = cell->boost.y + ratio * 0.02f * other->boost.y;
                                    cell_cord_prec norm = 1.f / sqrtf(bx * bx + by * by);
                                    cell->boost.x = bx * norm;
                                    cell->boost.y = by * norm;
                                }

                                cell->flag |= UPDATE_BIT;
                                other->eatenByID = cell_id(cell);
                            }
                        }
                    }, escape);
                }
            }
        });
    }
    server->threadPool->sync();
    timings.physics.phase3 = time_func(t3, t4);

    mutex removing;

    // Player-Pellets
    copy = temp;
    for (uint32_t _ = 0; _ < server->threadPool->size(); _++) {
        server->threadPool->enqueue([&] {
            while (true) {
                Control* c = nullptr;
                qm.lock();
                if (!copy.size()) {
                    qm.unlock();
                    break;
                } else {
                    c = copy.back();
                    copy.pop_back();
                    qm.unlock();
                }
                if (!c->overwrites.canEatPellet) continue;

                for (auto cell : c->sorted) {
                    uint16_t flags = cell->flag;
                    // Skip resolve bits
                    if (flags & SKIP_RESOLVE_BITS) continue;
                    cell_cord_prec x = cell->x;
                    cell_cord_prec y = cell->y;
                    cell_cord_prec r = cell->r;
                    bool escape = false;
                    Grid_PL.query(cell->shared.aabb, [&](Cell* pellet) {
                        if (pellet->flag & REMOVE_BIT) return;
                        cell_cord_prec r2 = pellet->r;
                        cell_cord_prec dx = pellet->x - x;
                        cell_cord_prec dy = pellet->y - y;
                        cell_cord_prec d = sqrt(dx * dx + dy * dy);
                        if ((r > r2 * T.EAT_MULT) && (d < r - r2 / T.EAT_OVERLAP)) {

                            r = sqrtf(r * r + r2 * r2);
                            pellet->eatenByID = cell_id(cell);

                            cell->flag |= UPDATE_BIT;
                            pellet->flag |= REMOVE_BIT;

                            removing.lock();
                            removedCells.push_back(pellet);
                            removing.unlock();
                        }
                    }, escape);
                    cell->r = r;
                }
            }
        });
    }
    server->threadPool->sync();
    timings.physics.phase4 = time_func(t4, t5);

    // Remove player cells & update
    copy = temp;
    for (uint32_t i = 0; i < server->threadPool->size(); i++) {
        server->threadPool->enqueue([&] {
            uint32_t removeCount = 0;
            while (true) {
                Control* c = nullptr;
                qm.lock();
                if (!copy.size()) {
                    qm.unlock();
                    break;
                } else {
                    c = copy.back();
                    copy.pop_back();
                    qm.unlock();
                }
                for (auto cell : c->sorted) {
                    uint16_t f = cell->flag;
                    if (f & REMOVE_BIT) {
                        removeCount++;
                        continue;
                    } else if (f & POP_BIT) {
                        const bool noPop = (c->handle && c->handle->perms & NO_POP) || c->overwrites.cells == 1;
                        auto maxCells = c->overwrites.cells;
                        if (maxCells <= 0)  maxCells = T.PLAYER_MAX_CELLS;
                        
                        if (!noPop) {
                            distributeMass<T>(maxCells - c->cells.size(), cell->r * cell->r * 0.01f, [&] (cell_cord_prec mass) {
                                cell_cord_prec angle = rngAngle();
                                auto n = splitFromCell(cell, sqrtf(mass * 100),
                                    { sinf(angle), cosf(angle), T.PLAYER_SPLIT_BOOST });
                                c->cells.push_back(n);
                            });
                            c->lastPopped = __now;
                        }
                    }
                    if (cell->flag & LOCK_BIT) {
                        cell_cord_prec x0 = cell->x;
                        cell_cord_prec y0 = cell->y;
                        cell_cord_prec aL = c->linearEquation[0];
                        cell_cord_prec bL = c->linearEquation[1];
                        cell_cord_prec cL = c->linearEquation[2];
                        cell->x = (bL * ( bL * x0 - aL * y0) - aL * cL) * c->abSqrSumInvL;
                        cell->y = (aL * (-bL * x0 + aL * y0) - bL * cL) * c->abSqrSumInvL;
                    }
                    tree->update(cell);
                }
            }
            cellCount -= removeCount;
        });
    }
    server->threadPool->sync();

    for (auto& cell : deadCells) {
        if (cell->flag & REMOVE_BIT) {
            // tree->remove(cell);
            cellCount--;
        } else if (cell->flag & UPDATE_BIT) {
            tree->update(cell);
        }
    }

    // Linear filter to remove cells from list
    for (auto c : temp) {
        filterCells(c->cells, removedCells);
        auto wasAlive = c->alive;
        c->alive = !!c->cells.size();
        if (wasAlive && !c->alive) c->lastDead = __now;
    }

    timings.physics.phase5 = time_func(t5, t6);

    // Ejected cell to ejected cell collision & virus eat
    for (auto e : ejected) {
        if (!(e->flag & UPDATE_BIT)) continue;
        if (e->flag & REMOVE_BIT) continue;

        cell_cord_prec x = e->x;
        cell_cord_prec y = e->y;
        cell_cord_prec r = e->r;

        Grid_EV.query(*e, [&](Cell* other) {
            if (other->flag & REMOVE_BIT) return false;
            cell_cord_prec a = r * r;
            cell_cord_prec r2 = other->r;
            cell_cord_prec dx = other->x - x;
            cell_cord_prec dy = other->y - y;
            cell_cord_prec rSum = r + r2;
            cell_cord_prec dSqr = dx * dx + dy * dy;

            if (!dSqr || dSqr >= rSum * rSum) return false;
            cell_cord_prec d = sqrt(dSqr);

            if (other->type == VIRUS_TYPE) {
                if (r * T.EAT_MULT < r2 && d < r2 - r / T.EAT_OVERLAP) {
                    other->r = sqrt(a + r2 * r2);

                    if constexpr (T.VIRUS_MAX_SIZE > 0.f) {
                        if (other->r > T.VIRUS_MAX_SIZE) {
                            if constexpr (T.VIRUS_EXPLODE) {
                                virusToSplit.push_back(other);
                                other->boost.x = e->boost.x;
                                other->boost.y = e->boost.y;
                            } else {
                                other->r = T.VIRUS_MAX_SIZE;
                            }
                        }
                    }

                    if constexpr (T.VIRUS_SPLIT_SIZE > 0.f) {
                        if (other->r > T.VIRUS_SPLIT_SIZE) {
                            virusToSplit.push_back(other);
                            other->boost.x = e->boost.x;
                            other->boost.y = e->boost.y;
                        }
                    }

                    if constexpr (T.VIRUS_PUSH_BOOST > 0.f) {
                        const cell_cord_prec newBoost = std::min(T.VIRUS_MAX_BOOST, e->boost.d + other->boost.d);
                        const cell_cord_prec bx = other->boost.x * other->boost.d + e->boost.x * e->boost.d;
                        const cell_cord_prec by = other->boost.y * other->boost.d + e->boost.y * e->boost.d;
                        const cell_cord_prec norm = sqrt(bx * bx + by * by);
                        if (norm > 0.f) {
                            other->boost.x = bx / norm;
                            other->boost.y = by / norm;
                            other->boost.d = newBoost;
                        }
                    }

                    e->flag |= REMOVE_BIT;
                    other->flag |= UPDATE_BIT;
                    return true;
                }
            } else {
                cell_cord_prec m = rSum - d;

                dx /= d;
                dy /= d;

                cell_cord_prec b = r2 * r2;
                cell_cord_prec sum = a + b;
                cell_cord_prec aM = b / sum;
                cell_cord_prec bM = a / sum;

                cell_cord_prec m1 = (m < r ? m : r) * aM;
                x -= dx * m1;
                y -= dy * m1;

                cell_cord_prec m2 = (m < r2 ? m : r2) * bM;
                other->x += dx * m2;
                other->y += dy * m2;

                e->flag |= UPDATE_BIT;
                other->flag |= UPDATE_BIT;
            }
            return false;
        });

        e->x = x;
        e->y = y;
    }
    timings.physics.phase6 = time_func(t6, t7);
    postResolve();

    timings.physics.phase7 = time_func(t7, t8);
}

template<OPT const& T>
void TemplateEngine<T>::postResolve() {
    for (auto v : virusToSplit) {
        if constexpr (T.VIRUS_SPLIT_BOOST) {
            auto angle = atan2(v->boost.x, v->boost.y);
            auto& n = newCell();

            n.x = v->x;
            n.y = v->y;
            v->r = n.r = T.VIRUS_SIZE;
            n.type = VIRUS_TYPE;
            n.boost = { sinf(angle), cosf(angle), T.VIRUS_SPLIT_BOOST };

            Grid_EV.insert(n);
            viruses.push_back(&n);
        } else if constexpr (T.VIRUS_EXPLODE)  {
            constexpr int splitTimes = (T.VIRUS_MAX_SIZE * T.VIRUS_MAX_SIZE) / 
                (T.VIRUS_SIZE * T.VIRUS_SIZE);

            for (int i = 0; i < splitTimes; i++) {
                auto angle = rngAngle();
                auto& n = newCell();

                n.x = v->x;
                n.y = v->y;
                n.r = T.VIRUS_SIZE;
                n.type = VIRUS_TYPE;
                std::uniform_real_distribution<cell_cord_prec> boost_dist(5., 20.);

                n.boost = { sinf(angle), cosf(angle), boost_dist(generator) * T.VIRUS_SPLIT_BOOST };

                Grid_EV.insert(n);
                viruses.push_back(&n);
            }
            v->r = T.VIRUS_SIZE;
        }
    }

    virusToSplit.clear();
    tree->restructure();

    filterCells(exps, removedCells);
    filterCells(cyts, removedCells);
    filterCells(ejected, removedCells);
    filterCells(viruses, removedCells);
    filterCells(deadCells, removedCells);
}

template<OPT const& T>
bool TemplateEngine<T>::boostCell(Cell& cell, float& dt) {
    if constexpr(T.NEW_BOOST_ALGO) {
        if (cell.boost.d > 0.) {
            const cell_cord_prec od = cell.boost.d;
            const cell_cord_prec nd = (cell.boost.d = std::max(cell.boost.d - T.BOOST_DELAY * dt, cell_cord_prec(0)));
            const cell_cord_prec dist = (100. * T.BOOST_MULTI) * (exp((0.0025 * T.BOOST_FACTOR) * od) - 
                exp((0.0025 * T.BOOST_FACTOR) * nd));

            cell.x += cell.boost.x * dist;
            cell.y += cell.boost.y * dist;
            cell.flag |= UPDATE_BIT;

            return true;
        } else {
            cell.boost.d = 0.;
            return false;
        }
    } else {
        if (cell.boost.d > 1.) {
            const cell_cord_prec db = cell.boost.d * (T.BOOST_FACTOR * 0.0025) * dt;
            cell.x += cell.boost.x * db;
            cell.y += cell.boost.y * db;
            cell.boost.d -= db;
            cell.flag |= UPDATE_BIT;

            return true;
        } else {
            cell.boost.d = 1.;
            return false;
        }
    }
}

template<OPT const& T>
void TemplateEngine<T>::bounceCell(Cell& cell) {

    AABB box;

    if constexpr (T.STRICT_BORDER) {
        box = AABB { cell.r - map.hw, map.hw - cell.r, cell.r - map.hh, map.hh - cell.r };
    } else {
        box = AABB { cell.r / 2.f - map.hw, map.hw - cell.r / 2.f, cell.r / 2.f - map.hh, map.hh - cell.r / 2.f };
    }

    if (cell.x < box.l) {
        cell.x = box.l;
        cell.flag |= WALL_BIT;
        cell.boost.x = -cell.boost.x;
    } else if (cell.x > box.r) {
        cell.x = box.r;
        cell.flag |= WALL_BIT;
        cell.boost.x = -cell.boost.x;
    }
    if (cell.y < box.b) {
        cell.y = box.b;
        cell.flag |= WALL_BIT;
        cell.boost.y = -cell.boost.y;
    } else if (cell.y > box.t) {
        cell.y = box.t;
        cell.flag |= WALL_BIT;
        cell.boost.y = -cell.boost.y;
    }
}

template<OPT const& T>
void TemplateEngine<T>::movePlayerCell(Cell& cell, float& dt, cell_cord_prec& mouseX, cell_cord_prec& mouseY, uint8_t& lineLocked, uint16_t& flags, cell_cord_prec& multi) {
    if (lineLocked) {
        flags |= cell.flag;
        cell.flag |= LOCK_BIT;
    }

    cell.flag |= UPDATE_BIT;

    cell_cord_prec dx = mouseX - cell.x;
    cell_cord_prec dy = mouseY - cell.y;
    cell_cord_prec d = sqrt(dx * dx + dy * dy);
    if (d < 1) return; dx /= d; dy /= d;
    constexpr cell_cord_prec modifier = 1.76f * T.PLAYER_SPEED / 1.2f;
    // const float speed = modifier * powf(cell.r, -0.4396754f);
    const cell_cord_prec speed = modifier * powf(cell.r, -0.39f) * multi;
    const cell_cord_prec m = std::min(speed, d) * dt;
    cell.x += dx * m;
    cell.y += dy * m;
};

template<OPT const& T>
Point TemplateEngine<T>::randomPoint(cell_cord_prec size,
    cell_cord_prec xmin, cell_cord_prec xmax, cell_cord_prec ymin, cell_cord_prec ymax) {
    
    xmin = std::clamp(xmin, -map.hw, map.hw);
    xmax = std::clamp(xmax, -map.hw, map.hw);

    ymin = std::clamp(ymin, -map.hh, map.hh);
    ymax = std::clamp(ymax, -map.hh, map.hh);

    std::uniform_real_distribution<cell_cord_prec> x_dist(xmin, xmax);
    std::uniform_real_distribution<cell_cord_prec> y_dist(ymin, ymax);

    return { x_dist(generator), y_dist(generator) };
}

template<OPT const& T>
BoolPoint TemplateEngine<T>::getPlayerSpawnPoint(Control* target, cell_cord_prec spawnSize, bool avoidCenter) {
    const cell_cord_prec safe = spawnSize * T.PLAYER_SAFE_SPAWN_RADIUS;
    
    if (!target) {
        if (!aliveControls.size()) {
            return getSafeSpawnFromInflu(spawnSize, T.PLAYER_SAFE_SPAWN_RADIUS);
        };
        // 1/3 chance to spawn close to an alive control
        if (rand() % 3) {
            std::uniform_int_distribution<int> picker(0, aliveControls.size() - 1);
            target = aliveControls[picker(generator)];
        } else {
            return getSafeSpawnFromInflu(spawnSize, T.PLAYER_SAFE_SPAWN_RADIUS);
        }
    }

    cell_cord_prec vx; 
    cell_cord_prec vy;
    const cell_cord_prec mx = target->__mouseX;
    const cell_cord_prec my = target->__mouseY;
    
    if (target->viewport.toAABB().contains(mx, my)) {
        vx = mx;
        vy = my;
    } else {
        vx = target->viewport.x;
        vy = target->viewport.y;
    }
    auto& box = target->aabb;

    uint32_t tries = T.MAX_SPAWN_TRIES;
    const cell_cord_prec f1 = std::max(T.PLAYER_VIEW_MIN, cell_cord_prec(1.2) * (vx - box.l));
    const cell_cord_prec f2 = std::max(T.PLAYER_VIEW_MIN, cell_cord_prec(1.2) * (box.r - vx));
    const cell_cord_prec f3 = std::max(T.PLAYER_VIEW_MIN, cell_cord_prec(1.2) * (vy - box.b));
    const cell_cord_prec f4 = std::max(T.PLAYER_VIEW_MIN, cell_cord_prec(1.2) * (box.t - vy));

    const cell_cord_prec radiusSqaredToAvoid = target->score * 100;

    cell_cord_prec i = 0.f;
    while (++i < tries) {
        const cell_cord_prec f = 0.8 + 0.2 * i / tries;
        const cell_cord_prec xmin = vx - f * f1;
        const cell_cord_prec xmax = vx + f * f2;
        const cell_cord_prec ymin = vy - f * f3;
        const cell_cord_prec ymax = vy + f * f4;
        auto [x, y] = randomPoint(spawnSize, xmin, xmax, ymin, ymax);

        if (avoidCenter) {
            cell_cord_prec dx = target->viewport.x - x;
            cell_cord_prec dy = target->viewport.y - y;
            if (dx * dx + dy * dy < radiusSqaredToAvoid) continue;
        }

        auto skip = false;
        for (auto& influ : influences) {
            if (influ.intersect(x, y, spawnSize)) {
                skip = true;
                break;
            };
        }
        
        if (!skip) {
            bool escape = false;

            AABB aabb = Rect(x, y, spawnSize, spawnSize).toAABB();
            uint32_t e = 0;
            Grid_EV.query(aabb, [&](Cell* other) {
                if (other->type & EJECT_BIT) e++;
                if (other->type == VIRUS_TYPE || e > 10) escape = true;
            }, escape);
            if (!escape) return { x, y, true };
        }
    }

    return { 0, 0, false };
    // return getSafeSpawnPoint(PLAYER_SPAWN_SIZE, safe);
};

template<OPT const& T>
BoolPoint TemplateEngine<T>::getSafeSpawnPoint(cell_cord_prec size, cell_cord_prec safeSize) {
    uint32_t tries = T.MAX_SPAWN_TRIES;
    cell_cord_prec safe = size * safeSize;
    while (--tries) {
        auto [x, y] = randomPoint(size);
        AABB aabb = Rect(x, y, safe, safe).toAABB();
        if (tree->isSafe(aabb)) {
            bool escape = false;
            uint32_t e = 0;
            Grid_EV.query(aabb, [&](Cell* other) {
                if (other->type & EJECT_BIT) e++;
                if (other->type == VIRUS_TYPE || e > 10) escape = true;
            }, escape);
            if (!escape) return { x, y, true };
        }
    }
    return { 0, 0, false };
};

template<OPT const& T>
BoolPoint TemplateEngine<T>::getSafeSpawnFromInflu(cell_cord_prec size, cell_cord_prec safeSize) {
    uint32_t tries = T.MAX_SPAWN_TRIES;
    cell_cord_prec safe = size * safeSize;
    while (--tries) {
        auto [x, y] = randomPoint(size);

        bool skip = false;
        for (auto& influ : influences) {
            if (influ.intersect(x, y, safe)){
                skip = true;
                break;
            }
        }

        if (!skip) {
            bool escape = false;
            uint32_t e = 0;
            AABB aabb = Rect(x, y, safe, safe).toAABB();
            Grid_EV.query(aabb, [&](Cell* other) {
                if (other->type & EJECT_BIT) e++;
                if (other->type == VIRUS_TYPE || e > 10) escape = true;
            }, escape);
            if (!escape) return { x, y, true };
        }
    }
    return { 0, 0, false };
};

template<OPT const& T>
Cell& TemplateEngine<T>::newCell() {
    if (cellCount == T.CELL_LIMIT) {
        // shouldRestart = true;
        // return pool[0];
        abort();
    }

    uint32_t id;

    uint16_t none = 0;
    uint16_t exist = 1;

    while (true) {
        id = __next_cell_id.load();
        while (pool[id].flag.load() & EXIST_BIT) {
            id++;
            if (id >= T.CELL_LIMIT) id = 0;
        }

        if (pool[id].flag.compare_exchange_weak(none, exist, 
            std::memory_order_release, std::memory_order_relaxed)) {
            __next_cell_id.store(id);
            break;
        }

        none = 0;
        exist = 1;
    }

    cellCount++;
    
    Cell& cell = pool[id];
    cell.age = 0.f;
    cell.eatenByID = 0;

    return cell;
}

template<OPT const& T>
Cell* TemplateEngine<T>::splitFromCell(Cell* cell, cell_cord_prec size, Boost boost) {
    cell->r = sqrtf(cell->r * cell->r - size * size);
    cell->flag |= UPDATE_BIT;

    const cell_cord_prec x = cell->x + T.PLAYER_SPLIT_DIST * boost.x;
    const cell_cord_prec y = cell->y + T.PLAYER_SPLIT_DIST * boost.y;

    auto& n = newCell();

    n.x = x;
    n.y = y;
    n.r = size;
    n.type = cell->type;
    n.boost = boost;
    n.updateAABB();

    tree->insert(&n);

    return &n;
};

template<OPT const& T>
void TemplateEngine<T>::syncState() {
    restart(false);

    for (int i = 0; i < T.CELL_LIMIT; i++) {
        Cell& cell = pool[i];

        // Memset funky cells
        if (cell.flag & REMOVE_BIT || !(cell.flag & EXIST_BIT) || !cell.type) {
            memset(&cell, 0, sizeof(Cell));
            continue;
        }

        cellCount++;

        if (cell.type == CYT_TYPE) {
            cyts.push_back(&cell);
            tree->insert(&cell);
        } else if (cell.type == EXP_TYPE) {
            exps.push_back(&cell);
            tree->insert(&cell);
        } else if (cell.type == DEAD_TYPE) {
            deadCells.push_back(&cell);
            cell.updateAABB();
            tree->insert(&cell);
        } else if (cell.type == VIRUS_TYPE) {
            viruses.push_back(&cell);
            Grid_EV.insert(cell);
        } else if (cell.type & EJECT_BIT) {
            ejected.push_back(&cell);
            Grid_EV.insert(cell);
        } else if (cell.type == PELLET_TYPE) {
            Grid_PL.insert(cell);
        } else {
            // Player cell
            auto iter = controls.find(cell.type);
            if (iter == controls.cend()) {
                cell.type = DEAD_TYPE;
                deadCells.push_back(&cell);
            } else {
                iter->second->cells.push_back(&cell);
            }
            cell.updateAABB();
            tree->insert(&cell);
        }
    }

    for (auto [_, c] : controls) {
        c->calculateViewport();
    }
    
    for (auto b : bots) {
        b->control->__mouseX = b->control->viewport.x;
        b->control->__mouseY = b->control->viewport.y;
    }

}