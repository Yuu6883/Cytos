#pragma once

#include <list>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <functional>

#define NOMINMAX
#include <uv.h>

using std::pair;
using std::list;
using std::vector;
using std::function;
using std::unordered_set;
using std::unordered_map;

#include "cell.hpp"
#include "quadtree.hpp"
#include "grid.hpp"
#include "../modes/options.hpp"

#include <mutex>
#include <atomic>

using std::mutex;
using std::atomic;
using std::unique_ptr;

#include <chrono>
using namespace std::chrono;

struct Bot;
struct GameHandle;
struct Control;
struct Server;

struct SpawnInfluence {
    cell_cord_prec x0;
    cell_cord_prec y0;
    cell_cord_prec r0;
    Control* control;
    
    inline bool intersect(cell_cord_prec x1, cell_cord_prec y1, cell_cord_prec r1) {
        const cell_cord_prec dx = x0 - x1;
        const cell_cord_prec dy = y0 - y1;
        const cell_cord_prec rSum = r0 + r1;
        return dx * dx + dy * dy < rSum * rSum;
    }
};

struct Engine {

    struct {
        float spawn_cells = 0.f;
        float handle_io = 0.f;
        float spawn_handles = 0.f;
        float update_cells = 0.f;
        float resolve_physics = 0.f;

        struct {
            float phase0 = 0.f;
            float phase1 = 0.f;
            float phase2 = 0.f;
            float phase3 = 0.f;
            float phase4 = 0.f;
            float phase5 = 0.f;
            float phase6 = 0.f;
            float phase7 = 0.f;
        } physics;
    } timings;

    Server* server;

    bool running;
    atomic<float> usage = 0;

    atomic<uint64_t> __now;
    uint64_t __start;
    uint64_t __ltick;
    uint64_t __llog = 0;
    uint64_t __lbmm = 0;
    uint64_t __perk = 0;
    
    atomic<uint16_t> players = 0;
    atomic<cell_cord_prec> playerMass = 0;
    atomic<cell_cord_prec> botMass = 0;

    bool hidePerks = true;
    bool updateBot = true;
    bool ignoreInput = false;
    bool shouldRestart = false;

    Cell* pool;
    uint32_t __next_cell_id;

    atomic<uint32_t> cellCount;

    // All kinds of cells
    vector<Cell*> exps;
    vector<Cell*> cyts;
    vector<Cell*> removedCells;
    vector<Cell*> deadCells;
    vector<Cell*> viruses;
    vector<Cell*> ejected;

    vector<Cell*> virusToSplit;
    vector<pair<Control*, bool>> killArray;
    unordered_set<Control*> spawnSet;

    GameHandle* biggest;
    list<GameHandle*> handles;

    size_t desiredBots = 0;
    bool alwaysSpawnBot = false;
    list<Bot*> bots;
    unordered_map<uint16_t, Control*> controls;
    vector<Control*> aliveControls;

    virtual void addHandle(GameHandle* handle, uint16_t id = 0);
    virtual Bot* addBot(uint16_t botID = 0) { return nullptr; };

    void removeHandle(GameHandle* handle);

    virtual const char* mode() { return "none"; }
    virtual const float getTimeScale() { return 0.f; };
    virtual const cell_cord_prec getMinView() { return 0.; };
    virtual const cell_cord_prec getViewScale() { return 0.; };
    virtual const size_t getPlayerMaxCell() { return 0; };
    virtual const cell_cord_prec getBotSpawnSize() { return 0.; };
    virtual const cell_cord_prec getMinEjectSize() { return 0.; };
    virtual const cell_cord_prec getMinSplitSize() { return 0.; };

    virtual const size_t poolSize() { return 0; };
    virtual const Rect getMap() { return Rect(0, 0, 10000., 10000.); };
    virtual const BotAI* getAI() { return nullptr; };
    virtual const cell_cord_prec minPerkSize() { return 0.; };

    bool dualEnabled;
    bool defaultCanSpawn;

    virtual void reset();
    virtual void tick(float dt);

    virtual void restart(bool clearMemory = true) {};

    virtual uint32_t getPelletCount() { return 0; };
    virtual void spawnPellets() {};
    virtual void spawnViruses() {};
    virtual void updatePerks() {};
    virtual void spawnBots() {};
    virtual void spawnPlayers() {};
    virtual void virus(cell_cord_prec x, cell_cord_prec y) {};

    virtual void handleIO(float dt) {};
    virtual void removeCells() {};
    virtual void updateCells(float dt) {};
    virtual void resolve(float dt) {};
    virtual void postResolve() {};

    // Helper functions
    virtual void delayKill(Control* control, bool replace = false);
    virtual void delaySpawn(Control* control);
    
    virtual void kill(Control* control, bool replace = false) {};
    virtual bool freeHandle(GameHandle* handle);
    
    virtual string_view getInitMessage() { return ""; };
    virtual string_view getExtState() { return ""; };

    virtual void syncState() {};
    virtual void setExtState(string_view buf) {};
    
    std::mt19937 generator;
    mutex m;
    uint16_t id;

    Engine(Server* server, uint16_t id);
    virtual ~Engine();

    bool start();
    bool stop();
    
    uint64_t now() { return __now; };
    float uptime() { return (__now - __start) / 1000 / 1000 / 1000.f; };

    virtual void gc() {};

    template<typename F, typename ... Args>
    void emit(F&& f, Args&& ... args) {
        for (auto h : handles) (h->*f)(args...);
    }
    
    virtual cell_id_t cell_id(Cell*& cell) { return cell - pool; };
    virtual cell_id_t cell_id(Cell& cell) { return &cell - pool; };

    template<typename SyncCallback>
    void sync(const SyncCallback& cb) {
        m.lock();
        cb();
        m.unlock();
    }

    virtual uint32_t extFlags() { return 0; };

    enum class EventType {
        JOIN, LEAVE, ROCK_WIN
    };

    void infoEvent(GameHandle* handle, EventType type);

    virtual cell_cord_prec rngAngle() { return 0.; };

    virtual void queryGridPL(AABB& aabb, const std::function<void(Cell*)> func) {};
    virtual void queryGridEV(AABB& aabb, const std::function<void(Cell*)> func) {};
    virtual void queryTree(AABB& aabb, const std::function<void(Cell*)> func) {};
};

template<OPT const& T>
struct TemplateEngine : Engine {

    std::uniform_real_distribution<cell_cord_prec>randomEject;
    std::uniform_real_distribution<cell_cord_prec>randomAngle;

    cell_cord_prec rngAngle() { return randomAngle(generator); }

    // Templated data structures
    QuadTree tree;
    Grid<T.GRID_PL_SIZE> Grid_PL;
    Grid<T.GRID_EV_SIZE> Grid_EV;

    Rect map = Rect(0, 0, T.MAP_HW, T.MAP_HH);

    vector<SpawnInfluence> influences;

    TemplateEngine(Server* server, uint16_t id = 0);
    virtual ~TemplateEngine() {};

    virtual void addHandle(GameHandle* handle, uint16_t id = 0) { Engine::addHandle(handle, id); };
    
    Cell& newCell();

    const char* mode() { return T.MODE; };
    const float getTimeScale() { return T.TIME_SCALE; };
    const cell_cord_prec getMinView() { return T.PLAYER_VIEW_MIN; };
    const cell_cord_prec getViewScale() { return T.PLAYER_VIEW_SCALE; };
    const size_t getPlayerMaxCell() { return T.PLAYER_MAX_CELLS; };
    const cell_cord_prec getBotSpawnSize() { return T.BOT_SPAWN_SIZE; };
    const cell_cord_prec getMinEjectSize() { return T.PLAYER_MIN_EJECT_SIZE; };
    const cell_cord_prec getMinSplitSize() { return T.PLAYER_MIN_SPLIT_SIZE; };
    
    const size_t poolSize() { return sizeof(Cell) * T.CELL_LIMIT; };
    const Rect getMap() { return map; };
    const BotAI* getAI() { return T.AI; };
    const cell_cord_prec minPerkSize() { return T.MIN_PERK_SIZE; };

    virtual void restart(bool clearMemory = true);

    uint32_t getPelletCount() { return Grid_PL.size(); };
    virtual void spawnPellets();
    virtual void spawnViruses();

    void updatePerks();

    virtual void spawnEXP();
    virtual void spawnCYT();

    virtual void handleIO(float dt);
    virtual void updateCells(float dt);

    virtual void resolve(float dt);
    virtual void postResolve();

    void removeCells();
    void virus(cell_cord_prec x, cell_cord_prec y);

    virtual Bot* addBot(uint16_t botID = 0);
    virtual void spawnBots();
    virtual void spawnPlayers();

    virtual bool spawnBotControl(Control*& c);
    virtual bool spawnPlayerControl(Control*& c);

    void kill(Control* control, bool replace);
    
    inline Point randomPoint(cell_cord_prec size,
        cell_cord_prec xmin = -T.MAP_HW, cell_cord_prec xmax = T.MAP_HW,
        cell_cord_prec ymin = -T.MAP_HH, cell_cord_prec ymax = T.MAP_HH);
    inline BoolPoint getPlayerSpawnPoint(Control* control, cell_cord_prec size, bool avoidCenter = false);
    inline BoolPoint getSafeSpawnPoint(cell_cord_prec size, cell_cord_prec safeSize);
    inline BoolPoint getSafeSpawnFromInflu(cell_cord_prec size, cell_cord_prec safeSize);

    Cell& splitFromCell(Cell& cell, cell_cord_prec size, Boost boost);
    bool boostCell(Cell& cell, float& dt);
    void bounceCell(Cell& cell);
    void movePlayerCell(Cell& cell, float& dt, cell_cord_prec& mouseX, cell_cord_prec& mouseY, 
        uint8_t& lineLocked, uint16_t& flags, cell_cord_prec& multi);

    virtual void syncState();

    virtual void queryGridPL(AABB& aabb, const std::function<void(Cell*)> func) {
        bool escape = false;
        Grid_PL.query(aabb, func, escape);
    }

    virtual void queryGridEV(AABB& aabb, const std::function<void(Cell*)> func) {
        bool escape = false;
        Grid_EV.query(aabb, func, escape);
    }

    virtual void queryTree(AABB& aabb, const std::function<void(Cell*)> func) {
        tree.query(aabb, func);
    }

    void gc() {
        Grid_PL.gc();
        Grid_EV.gc();
    }
};

static void filterCells(vector<Cell*>& cells, vector<Cell*>& removed) {
    uint32_t w_id = 0;
    for (uint32_t i = 0; i < cells.size(); i++) {
        auto& cell = cells[i];
        if (cell->flag & REMOVE_BIT) {
            removed.push_back(cell);   
        } else {
            if (w_id != i) cells[w_id] = cell;
            w_id++;
        }
    }
    cells.resize(w_id);
};

template<OPT const& T, typename MassCallback>
static void distributeMass(cell_cord_prec cellsLeft, cell_cord_prec mass, const MassCallback& cb) {
    if (cellsLeft <= 0) return;
    constexpr cell_cord_prec splitMin = T.PLAYER_MIN_SPLIT_SIZE * T.PLAYER_MIN_SPLIT_SIZE * 0.01f;
    if (T.VIRUS_MONOTONE_POP) {
        const cell_cord_prec amount = std::min(floor(mass / splitMin), cellsLeft);
        const cell_cord_prec perPiece = mass / (amount + 1.f);
        for (int i = 0; i < amount; i++) cb(perPiece);
    } else {
        if (mass / cellsLeft < splitMin) {
            int amount = 2;
            cell_cord_prec perPiece;
            while ((perPiece = mass / (amount + 1.f)) >= splitMin &&
                (amount << 1) <= cellsLeft)
                amount <<= 1;
            for (int i = 0; i < amount; i++) cb(perPiece);
        } else {
            cell_cord_prec nextMass = mass * 0.5f;
            cell_cord_prec massLeft = nextMass;
            while (cellsLeft > 0) {
                if (nextMass / cellsLeft < splitMin) break;
                while (nextMass >= massLeft && cellsLeft > 1)
                    nextMass *= 0.5f;
                cb(nextMass);
                massLeft -= nextMass;
                cellsLeft--;
            }
            nextMass = massLeft / cellsLeft;
            for (int i = 0; i < cellsLeft; i++) cb(nextMass);
        }
    }
};
