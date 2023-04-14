#include "player.hpp"

#include <bitset>

#include "../misc/writer.hpp"
#include "../physics/engine.hpp"
#include "server.hpp"

using std::bitset;

#define MAX_CELL_LIMIT 262144

static bitset<MAX_CELL_LIMIT> LAST_VISIBLE;
static bitset<MAX_CELL_LIMIT> CURR_VISIBLE;
static cell_id_t ID_LOOKUP[MAX_CELL_LIMIT];
static vector<CellCache> NEW_CELL_CACHE;

Player::Player(Server* server) : GameHandle(server, "player") {
    dual = new DualHandle(this);
}

void Player::setEngine(Engine* engine) {
    this->engine = engine;

    control = nullptr;
    spectate = nullptr;
    pids[0] = 0;
    pids[1] = 0;

    // Clear cell cache
    cache.clear();
    cache.shrink_to_fit();

    dual->engine = nullptr;
    dual->spectate = nullptr;
    dual->control = nullptr;

    if (!engine) return;
    if (engine->dualEnabled) {
        dual->setEngine(engine);
        dual->join();
        dual->control->overwrites.view = 20;
        dual->control->overwrites.canSpawn = engine->defaultCanSpawn;

        pids[1] = dual->control->id;
    } else {
        pids[1] = 0;
    }

    engine->players++;
    join();
    control->overwrites.view = 20;
    control->overwrites.canSpawn = engine->defaultCanSpawn;

    pids[0] = control->id;
}

void Player::spawn(uint8_t tab) {
    if (tab > 1) return;
    inputs[tab].spawn = true;
}

void Player::spec(uint16_t target) {
    if (!target || !engine) return;
    if (isAlive() || target == pids[0] || target == pids[1]) {
        spectate = nullptr;
        return;
    }

    if (target == 65535) {
        spectate = engine->biggest;
        return;
    }

    auto iter = engine->controls.find(target);
    if (iter == engine->controls.end()) return;

    // Changing spectate pointer needs to be sync'd
    auto h = iter->second->handle;
    auto t = spectate = h->spectatable() ? h : h->dual;

    // Spectating itself...???
    if (spectate == this) {
        spectate = nullptr;
        if (dual) dual->spectate = nullptr;
    } else {
        if (t->dual && dual) {
            dual->spectate = t->dual;
        }
    }
}

void Player::syncInput() {
    if (!isAlive()) {
        for (int tab = 0; tab <= 1; tab++) {
            auto c = control;
            if (tab) {
                if (dual && dual->control) {
                    c = dual->control;
                } else {
                    inputs[tab] = Input();
                    break;
                }
            }

            if (inputs[tab].spawn) {
                inputs[tab].spawn = false;

                c->requestSpawn();
                c->lastSpawnReq = engine->now();
            }
        }
        return;
    }

    // Read from input into control's fields
    for (int tab = 0; tab <= 1; tab++) {
        auto c = control;

        // Tab 2
        if (tab) {
            if (control == c && dual && dual->control) {
                c = dual->control;
            } else {
                inputs[tab] = Input();
                break;
            }
        }

        if (!c->alive && tab == activeTab) {
            c->requestSpawn();
            c->lastSpawnReq = engine->now();
        }

        c->splits = inputs[tab].splits;
        c->ejects = inputs[tab].ejects;
        c->ejectMacro = inputs[tab].macro;

        if (inputs[tab].spawn) {
            inputs[tab].spawn = false;
            c->requestSpawn();
            c->lastSpawnReq = engine->now();
        }

        if (inputs[tab].line) {
            inputs[tab].line = false;
            c->toggleLock();
        }

        c->__mouseX = inputs[tab].mouseX;
        c->__mouseY = inputs[tab].mouseY;

        inputs[tab].splits = 0;
        inputs[tab].ejects = 0;
        inputs[tab].spawn = 0;
        inputs[tab].line = 0;
    }
}

inline uint8_t getFlags(Control*& c) { return c->alive | (c->lineLocked << 1); }

void Player::onTick() {
    if (engine->dualEnabled) {
        if (!wasAlive && isAlive()) {
            actualSpawnTick = engine->now();
        }
    } else {
        if (wasAlive && !isAlive()) {
            control->lastSpawnReq = engine->now();
        }
    }

    wasAlive = isAlive();
    if (dual) dual->wasAlive = wasAlive;

    // Viewport calculation
    vector<AABB> viewports;
    viewports.reserve(2);

    Control* c1 = control;
    Control* c2 = dual ? dual->control : nullptr;

    cell_cord_prec factor = 1.f;

    if (spectate) {
        c1 = spectate->control;
        if (spectate->dual) c2 = spectate->dual->control;

        factor = 1.5f;
    }

    if (!c2) c2 = c1;

    auto map = engine->getMap();
    AABB boxM = map.toAABB();
    // Clip to map size
    AABB box1 = (c1->viewport * factor).toAABB() && boxM;
    AABB box2 = (c2->viewport * factor).toAABB() && boxM;
    bool a1 = c1->alive, a2 = c2->alive;

    if (!a1 && !a2) {
        viewports.push_back(control->viewport.toAABB());
    }
    if (a1 && !a2) {
        viewports.push_back(box1);
    }
    if (!a1 && a2) {
        viewports.push_back(box2);
    }
    if (a1 && a2) {
        // Decide if we want to merge 2 boxes or not
        AABB merged = box1 || box2;
        if (merged.getArea() < box1.getArea() + box2.getArea()) {
            viewports.push_back(merged);
        } else {
            viewports.push_back(box1);
            viewports.push_back(box2);
        }
    }

    // Crazy delta compression protocol I wrote
    LAST_VISIBLE.reset();
    CURR_VISIBLE.reset();

    for (auto& item : cache) LAST_VISIBLE[item.id] = true;

    constexpr cell_cord_prec SKIP_PELLET_VIEW = 25000.;

    for (auto& aabb : viewports) {
        bool skipPellet =
            aabb.getArea() > (SKIP_PELLET_VIEW * SKIP_PELLET_VIEW);

        engine->queryGridEV(aabb, [&](Cell* c) {
            if (!c || !c->age) return;
            cell_id_t id = engine->cell_id(c);
            if (CURR_VISIBLE[id]) return;  // Bit already set
            CURR_VISIBLE[id] = true;
            if (LAST_VISIBLE[id]) return;  // Not seen already
            NEW_CELL_CACHE.push_back(CellCache(id, c));
        });

        if (!skipPellet) {
            engine->queryGridPL(aabb, [&](Cell* c) {
                if (!c) return;
                cell_id_t id = engine->cell_id(c);
                if (CURR_VISIBLE[id]) return;  // Bit already set
                CURR_VISIBLE[id] = true;
                if (LAST_VISIBLE[id]) return;  // Not seen already
                NEW_CELL_CACHE.push_back(CellCache(id, c));
            });
        }

        if (canEatPerk()) {
            engine->queryTree(aabb, [&](auto c) {
                cell_id_t id = engine->cell_id(c);
                if (CURR_VISIBLE[id]) return;  // Bit already set
                CURR_VISIBLE[id] = true;
                if (LAST_VISIBLE[id]) return;  // Not seen already
                NEW_CELL_CACHE.push_back(CellCache(id, c));
            });
        } else {
            engine->queryTree(aabb, [&](auto c) {
                cell_id_t id = engine->cell_id(c);
                if (CURR_VISIBLE[id]) return;  // Bit already set
                if ((c->type == EXP_TYPE || c->type == CYT_TYPE)) return;
                CURR_VISIBLE[id] = true;
                if (LAST_VISIBLE[id]) return;  // Not seen already
                NEW_CELL_CACHE.push_back(CellCache(id, c));
            });
        }
    }

    auto cells = engine->pool;

    Writer w;
    w.write<uint8_t>(spectate ? 1 : 0);
    w.write<uint8_t>(getFlags(c1));
    w.write<uint8_t>(getFlags(c2));
    w.write<uint16_t>(c1->id);
    w.write<uint16_t>(c2->id);
    w.write<uint16_t>(c1->cells.size());
    w.write<uint16_t>(c2->cells.size());
    w.write<float>(c1->score);
    w.write<float>(c2->score);
    w.write<float>(c1->viewport.x);
    w.write<float>(c1->viewport.y);
    w.write<float>(c2->viewport.x);
    w.write<float>(c2->viewport.y);
    w.write<float>(map.hw);
    w.write<float>(map.hh);

    constexpr uint8_t UPD = 0x01 << 6;
    constexpr uint8_t EAT = 0x02 << 6;
    constexpr uint8_t ADD = 0x03 << 6;

    constexpr uint8_t DX_P = 0x01 << 4;
    constexpr uint8_t DX_N = 0x02 << 4;
    constexpr uint8_t XXXX = 0x03 << 4;

    constexpr uint8_t DY_P = 0x01 << 2;
    constexpr uint8_t DY_N = 0x02 << 2;
    constexpr uint8_t YYYY = 0x03 << 2;

    constexpr uint8_t DR_P = 0x01;
    constexpr uint8_t DR_N = 0x02;
    constexpr uint8_t RRRR = 0x03;

    constexpr cell_cord_prec int16range = (1 << 15) - 1;
    constexpr cell_cord_prec uint16range = (1 << 16) - 1;

    memset(ID_LOOKUP, 0, sizeof(ID_LOOKUP));

    auto cache_size = cache.size();
    for (uint32_t i = 0; i < cache_size; i++) ID_LOOKUP[cache[i].id] = i;

    w.write<uint32_t>(cache_size);

    uint32_t w_id = 0;
    // Write
    for (uint32_t i = 0; i < cache_size; i++) {
        auto& item = cache[i];
        auto out = &cache[i];
        uint8_t& flags = w.ref<uint8_t>(0);

        // Move the item
        if (w_id < i) {
            cache[w_id] = item;
            out = &cache[w_id];
        }

        // Cell is eaten
        if (cells[out->id].flag & REMOVE_BIT && cells[out->id].eatenByID &&
            ID_LOOKUP[out->id]) {
            flags |= EAT;
            w.write<uint16_t>(ID_LOOKUP[out->id]);
        } else if (cells[out->id].flag & EXIST_BIT &&
                   cells[out->id].type == out->type && CURR_VISIBLE[out->id]) {
            w_id++;
            flags |= UPD;

            // Delta compression
            const int16_t cx =
                std::clamp(cells[out->id].x * cell_cord_prec(0.5), -int16range,
                           int16range);
            const int16_t cy =
                std::clamp(cells[out->id].y * cell_cord_prec(0.5), -int16range,
                           int16range);
            const uint16_t cr =
                std::clamp(cells[out->id].r * cell_cord_prec(0.5),
                           cell_cord_prec(0), uint16range);

            int16_t dx = cx - out->x;
            out->x = cx;
            if (!dx) {
                // Does nothing
            } else if (dx <= 255 && dx >= -255) {
                if (dx > 0) {
                    flags |= DX_P;
                    w.write<uint8_t>(dx);
                } else {
                    flags |= DX_N;
                    w.write<uint8_t>(-dx);
                }
            } else {
                flags |= XXXX;
                w.write<int16_t>(out->x);
            }

            int16_t dy = cy - out->y;
            out->y = cy;
            if (!dy) {
                // Does nothing
            } else if (dy <= 255 && dy >= -255) {
                if (dy > 0) {
                    flags |= DY_P;
                    w.write<uint8_t>(dy);
                } else {
                    flags |= DY_N;
                    w.write<uint8_t>(-dy);
                }
            } else {
                flags |= YYYY;
                w.write<int16_t>(out->y);
            }

            int16_t dr = cr - out->r;
            out->r = cr;
            if (!dr) {
                // Does nothing
            } else if (dr <= 255 && dr >= -255) {
                if (dr > 0) {
                    flags |= DR_P;
                    w.write<uint8_t>(dr);
                } else {
                    flags |= DR_N;
                    w.write<uint8_t>(-dr);
                }
            } else {
                flags |= RRRR;
                w.write<uint16_t>(out->r);
            }
        } else {
            // Remove but nothing is written
        }
    }

    auto new_cache_size = NEW_CELL_CACHE.size();
    w.write<uint32_t>(new_cache_size);

    for (uint32_t i = 0; i < new_cache_size; i++) {
        auto& item = NEW_CELL_CACHE[i];
        w.write<uint16_t>(item.type);
        w.write<int16_t>(item.x);
        w.write<int16_t>(item.y);
        w.write<uint16_t>(item.r);
    }

    cache.resize(w_id);
    cache.reserve(cache.size() + NEW_CELL_CACHE.size());
    cache.insert(cache.end(), NEW_CELL_CACHE.begin(), NEW_CELL_CACHE.end());
    NEW_CELL_CACHE.clear();

    send(w.finalize());
}