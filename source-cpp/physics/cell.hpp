#pragma once

#include <atomic>
#include <mutex>

using std::atomic;
using std::mutex;

constexpr uint16_t EJECT_BIT = (1 << 14);
constexpr uint16_t PELLET_TYPE = (1 << 14) - 1;
constexpr uint16_t VIRUS_TYPE = PELLET_TYPE - 1;
constexpr uint16_t DEAD_TYPE = VIRUS_TYPE - 1;
constexpr uint16_t CYT_TYPE = DEAD_TYPE - 1;
constexpr uint16_t EXP_TYPE = CYT_TYPE - 1;
constexpr uint16_t ROCK_TYPE = EXP_TYPE - 1;

#define IS_PLAYER(type) (type <= DEAD_TYPE)
#define IS_NOT_PLAYER(type) (type > DEAD_TYPE)

constexpr uint16_t EXIST_BIT = 0x1;
constexpr uint16_t UPDATE_BIT = 0x2;
constexpr uint16_t INSIDE_BIT = 0x4;
constexpr uint16_t LOCK_BIT = 0x8;

// constexpr uint16_t AUTO_BIT   = 0x10;
constexpr uint16_t REMOVE_BIT = 0x20;
constexpr uint16_t MERGE_BIT = 0x40;
constexpr uint16_t POP_BIT = 0x80;

constexpr uint16_t WALL_BIT = 0x100;
constexpr uint16_t COLL_BIT = 0x200;
constexpr uint16_t NOEAT_BIT = 0x400;
constexpr uint16_t PROT_BIT = 0x800;

constexpr uint16_t CLEAR_BITS = EXIST_BIT;
constexpr uint16_t SKIP_RESOLVE_BITS = INSIDE_BIT | REMOVE_BIT | POP_BIT;

typedef double cell_cord_prec;

template <typename T>
struct TPoint {
    T x, y;
};

template <typename T>
struct TAABB {
    T l, r, b, t;

    template <typename T2 = T>
    inline bool insersect(TAABB<T2>& other) {
        return l < other.r && r > other.l && t > other.b && b < other.t;
    }

    template <typename T2 = T>
    inline bool contains(TAABB<T2>& other) {
        return l < other.l && r > other.r && t > other.t && b < other.b;
    }

    template <typename T2 = T>
    inline bool contains(T2 x, T2 y) {
        return l < x && r > x && t > y && b < y;
    }

    template <typename T2 = T>
    TAABB<T>& operator=(const TAABB<T2>& other) {
        this->l = (T)other.l;
        this->r = (T)other.r;
        this->b = (T)other.b;
        this->t = (T)other.t;
        return *this;
    }

    inline TAABB<T> operator||(TAABB<T> other) {
        return {.l = std::min(l, other.l),
                .r = std::max(r, other.r),
                .b = std::min(b, other.b),
                .t = std::max(t, other.t)};
    }
    inline TAABB<T> operator&&(TAABB<T> other) {
        return {.l = std::max(l, other.l),
                .r = std::min(r, other.r),
                .b = std::max(b, other.b),
                .t = std::min(t, other.t)};
    }
    inline T getArea() { return (r - l) * (t - b); }
};

template <typename T>
struct TRect : TPoint<T> {
    T hw, hh;
    TRect(T x, T y, T hw, T hh) : hw(hw), hh(hh) {
        this->x = x;
        this->y = y;
    };

    template <typename T2 = T>
    TAABB<T2> toAABB() {
        return {
            .l = this->x - hw,
            .r = this->x + hw,
            .b = this->y - hh,
            .t = this->y + hh,
        };
    }

    template <typename T2 = T>
    TRect<T2> operator*(T2 expand) {
        return TRect<T2>(this->x, this->y, hw * expand, hh * expand);
    }
};

typedef TPoint<cell_cord_prec> Point;
typedef TRect<cell_cord_prec> Rect;
typedef TAABB<cell_cord_prec> AABB;

struct BoolPoint : Point {
    bool safe;
};

struct GridRange {
    int32_t t, b, l, r;

    bool include(int32_t i, int32_t j) {
        return i >= l && i <= r && j >= t && j <= b;
    }

    bool intersect(GridRange& other) {
        return l < other.r && r > other.l && t < other.b && b > other.t;
    }

    bool operator==(GridRange& other) {
        return l == other.l && r == other.r && t == other.t && b == other.b;
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

struct alignas(32) Boost {
    cell_cord_prec x, y, d;
    inline void normalize() {
        d = sqrt(x * x + y * y);
        x /= d;
        y /= d;
    }
    inline void operator+=(Boost& other) {
        x = x * d + other.x * other.d;
        y = y * d + other.y * other.d;
        normalize();
    }
};

typedef uint32_t cell_id_t;

typedef TRect<int32_t> IRect;
typedef TAABB<int32_t> IAABB;

#pragma pack(push, 1)
struct alignas(64) Cell {
    atomic<uint16_t> flag = 0;  // 2
    uint16_t type;              // 2
    float age;                  // 4

    cell_cord_prec x;  // 4 or 8
    cell_cord_prec y;  // 4 or 8
    cell_cord_prec r;  // 4 or 8

    cell_id_t eatenByID;     // 4
    int data;                // 4
    void* __root = nullptr;  // 8
    union {
        IAABB aabb;
        GridRange range;
    } shared;  // 16

    IAABB toAABB() {
        return {
            .l = int32_t(x - r),
            .r = int32_t(x + r),
            .b = int32_t(y - r),
            .t = int32_t(y + r),
        };
    }

    void updateAABB() { shared.aabb = toAABB(); }

    void expandeAABB(cell_cord_prec factor) {
        auto rr = r * factor;
        shared.aabb = {
            .l = int32_t(x - rr),
            .r = int32_t(x + rr),
            .b = int32_t(y - rr),
            .t = int32_t(y + rr),
        };
    }
};
#pragma pack(pop)

constexpr size_t CELL_T_SIZE = sizeof(Cell);
constexpr size_t BOOST_T_SIZE = sizeof(Boost);

static_assert(CELL_T_SIZE == 64, "CELL_T_SIZE must be 64");
static_assert(BOOST_T_SIZE == 32, "BOOST_T_SIZE must be 32");

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
    CellCache() : id(0), type(0), x(0), y(0), r(0){};
    CellCache(cell_id_t id, Cell*& cell)
        : id(id),
          type(cell->type),
          x(cell->x / 2.f),
          y(cell->y / 2.f),
          r(cell->r / 2.f) {}

    CellCacheNoID clientState() {
        return {.type = type, .x = x * 2, .y = y * 2, .r = r * 2};
    }
};

constexpr size_t CELL_CACHE_T_SIZE = sizeof(CellCache);