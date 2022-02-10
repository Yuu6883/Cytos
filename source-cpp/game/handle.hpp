#pragma once

#include <list>
#include <vector>
#include <atomic>
#include <string>
#include <cinttypes>
#include <string_view>
#include <unordered_set>
#include "perms.hpp"

#include "../misc/reader.hpp"
#include "../physics/cell.hpp"

using std::list;
using std::vector;
using std::atomic;
using std::string;
using std::string_view;
using std::unordered_set;

struct Engine;
struct Control;
struct Server;

struct Perks {
    uint16_t exps = 0;
    uint16_t cyts = 0;

    inline bool updated() {
        return exps || cyts;
    }

    inline void reset() {
        exps = 0;
        cyts = 0;
    }
};

struct GameHandle {
    string __gid;

    Perks perks;
    
    Server* server;
    Engine* engine;
    GameHandle* dual;
    GameHandle* spectate;
    Control* control;
    uint64_t actualSpawnTick;

    uint16_t perms = 0;
    cell_cord_prec viewArea = 0.f;

    bool showOnLBMM;
    bool wasAlive;

    GameHandle(Server* server, string gid = "") : 
        server(server), engine(nullptr),
        dual(nullptr), control(nullptr),
        spectate(nullptr),
        showOnLBMM(false), 
        wasAlive(false),
        __gid(gid) {};

    virtual ~GameHandle() { remove(); };
    
    virtual bool isAlive();
    virtual cell_cord_prec getScore();
    virtual uint32_t getKills();
    virtual Point position();
    virtual bool canEatPerk();
    virtual bool isBot() { return false; };
    virtual bool spectatable() { return true; }
    virtual bool cleanMe() { return false; };

    virtual string_view gatewayID() { return __gid; };

    virtual void setInfo(string_view infoBuf) {};
    virtual string_view getInfo() { return EMPTY_BUF; };

    virtual void setEngine(Engine* engine);

    void join();
    void remove();

    virtual void syncInput() {};
    virtual void onTick();

    virtual void onLog(string_view message) {};
    virtual void onError(string_view error, int32_t code = 0) {};
    virtual void onInfo(GameHandle* other) {};
    virtual void onLBMMData(string_view buffer) {};
};
