#pragma once

#include <mutex>
#include <atomic>

using std::mutex;
using std::atomic;

constexpr uint16_t EJECT_BIT   = (1 << 14);
constexpr uint16_t PELLET_TYPE = (1 << 14) - 1;
constexpr uint16_t VIRUS_TYPE  = PELLET_TYPE - 1;
constexpr uint16_t DEAD_TYPE   = VIRUS_TYPE - 1;
constexpr uint16_t CYT_TYPE    = DEAD_TYPE - 1;
constexpr uint16_t EXP_TYPE    = CYT_TYPE - 1;
constexpr uint16_t ROCK_TYPE   = EXP_TYPE - 1;

#define IS_PLAYER(type) (type <= DEAD_TYPE)
#define IS_NOT_PLAYER(type) (type > DEAD_TYPE)

constexpr uint16_t EXIST_BIT  = 0x1;
constexpr uint16_t UPDATE_BIT = 0x2;
constexpr uint16_t INSIDE_BIT = 0x4;
constexpr uint16_t LOCK_BIT   = 0x8;

// constexpr uint16_t AUTO_BIT   = 0x10;
constexpr uint16_t REMOVE_BIT = 0x20;
constexpr uint16_t MERGE_BIT  = 0x40;
constexpr uint16_t POP_BIT    = 0x80;

constexpr uint16_t WALL_BIT   = 0x100;
constexpr uint16_t COLL_BIT   = 0x200;
constexpr uint16_t NOEAT_BIT  = 0x400;
constexpr uint16_t PROT_BIT   = 0x800;

constexpr uint16_t CLEAR_BITS = EXIST_BIT;
constexpr uint16_t SKIP_RESOLVE_BITS = INSIDE_BIT | REMOVE_BIT | POP_BIT;

typedef float cell_cord_prec;

struct AABB {
    cell_cord_prec l, r, b, t;
    inline bool insersect(AABB& other) {
        return l < other.r && r > other.l &&
               t > other.b && b < other.t;
    }
    inline bool contains(AABB& other) {
        return l < other.l && r > other.r &&
               t > other.t && b < other.b;
    }
    inline bool contains(cell_cord_prec x, cell_cord_prec y) {
        return l < x && r > x &&
               t > y && b < y;
    }
    inline AABB operator||(AABB other) {
        return {
            .l = std::min(l, other.l),
            .r = std::max(r, other.r),
            .b = std::min(b, other.b),
            .t = std::max(t, other.t)
        };
    }
    inline AABB operator&&(AABB other) {
        return {
            .l = std::max(l, other.l),
            .r = std::min(r, other.r),
            .b = std::max(b, other.b),
            .t = std::min(t, other.t)
        };
    }
    inline cell_cord_prec getX() { return (r + l) / 2.; }
    inline cell_cord_prec getY() { return (t + b) / 2.; }
    inline cell_cord_prec getArea() { return (r - l) * (t - b); }
};

struct Point {
    cell_cord_prec x;
    cell_cord_prec y;
};

struct BoolPoint : Point {
    bool safe;
};

struct Rect : Point {
    cell_cord_prec hw, hh;
    Rect(cell_cord_prec x, cell_cord_prec y, cell_cord_prec hw, cell_cord_prec hh):
        hw(hw), hh(hh) {
            this->x = x;
            this->y = y;
        };

    AABB toAABB() { 
        return { 
            .l = x - hw,
            .r = x + hw,
            .b = y - hh,
            .t = y + hh,
        };
    }
};

struct GridRange {
    int32_t t, b, l, r;

    bool include(int32_t i, int32_t j) {
        return i >= l && i <= r && j >= t && j <= b;
    }

    bool intersect(GridRange& other) {
        return l < other.r && r > other.l &&
               t < other.b && b > other.t;
    }

    bool operator==(GridRange& other) {
        return l == other.l && r == other.r &&
               t == other.t && b == other.b;
    }

    static GridRange merge(GridRange& r1, GridRange& r2) {
        GridRange r3;
        r3.t = std::max(r1.t, r2.t);
        r3.b = std::min(r1.b, r2.b);
        r3.l = std::min(r1.l, r2.l);
        r3.r = std::max(r1.r, r2.r);
        return r3;
    }
};

struct Boost {
    cell_cord_prec x, y, d;
};

typedef uint32_t cell_id_t;

struct alignas(64) Cell {
// struct alignas(128) Cell {
    void* __root = nullptr; // 8
    atomic<uint16_t> flag = 0; // 2
    uint16_t type; // 2
    cell_cord_prec x; // 4 or 8
    cell_cord_prec y; // 4 or 8
    cell_cord_prec r; // 4
    float age; // 4
    union {
        AABB aabb;
        GridRange range;
    } shared; // 16
    Boost boost; // 12
    cell_id_t eatenByID; // 4

    AABB toAABB() { 
        return { 
            .l = x - r,
            .r = x + r,
            .b = y - r,
            .t = y + r,
        };
    }

    void updateAABB() { shared.aabb = toAABB(); }
};

constexpr size_t cell_size = sizeof(Cell);

struct CellCacheNoID {
    uint32_t type;
    int32_t x;
    int32_t y;
    int32_t r;
};

struct CellCache {
    cell_id_t id;
    uint16_t type;
    int16_t x;
    int16_t y;
    uint16_t r;
    CellCache() : id(0), type(0), x(0), y(0), r(0) {};
    CellCache(cell_id_t id, Cell*& cell) : id(id), 
        type(cell->type), x(cell->x / 2.f), y(cell->y / 2.f), r(cell->r / 2.f) {}

    CellCacheNoID clientState() {
        return {
            .type = type,
            .x = x * 2,
            .y = y * 2,
            .r = r * 2
        };
    }
};

// constexpr size_t CELL_T_SIZE = sizeof(Cell);