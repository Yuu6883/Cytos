#pragma once

#define _USE_MATH_DEFINES
#include <cmath> 

#include <math.h>
#include <vector>
#include <algorithm>
#include <mutex>

#include "cell.hpp"
#include "../misc/logger.hpp"

using std::vector;
using std::mutex;

#define int32_floor(arg) int32_t(floor(arg))

template<int32_t Dim>
class Grid {

    AABB aabb;
    float xBinSize, yBinSize;

    // Massive memory block
    vector<Cell*> buckets[Dim][Dim];
    mutex m[Dim][Dim];

    template<typename T>
    GridRange fromAABB(TAABB<T>& box) {
        GridRange rg;

        rg.l = std::max(int32_floor((box.l - aabb.l) / xBinSize), 0);
        rg.r = std::min(int32_floor((box.r - aabb.l) / xBinSize), Dim - 1);
        rg.t = std::max(int32_floor((aabb.t - box.t) / yBinSize), 0);
        rg.b = std::min(int32_floor((aabb.t - box.b) / yBinSize), Dim - 1);

        return rg;
    }

public:
    atomic<int32_t> count;
    Grid(Rect rect) : count(0), aabb(rect.toAABB()) {
        xBinSize = rect.hw * 2 / Dim;
        yBinSize = rect.hh * 2 / Dim;
    };

    int32_t size() { return count; };

    inline void insert(Cell& cell) {
        GridRange& itemRange = cell.shared.range;

        itemRange.l = std::max(int32_floor(((cell.x - cell.r) - aabb.l) / xBinSize), 0);
        itemRange.r = std::min(int32_floor(((cell.x + cell.r) - aabb.l) / xBinSize), Dim - 1);
        itemRange.t = std::max(int32_floor((aabb.t - (cell.y + cell.r)) / yBinSize), 0);
        itemRange.b = std::min(int32_floor((aabb.t - (cell.y - cell.r)) / yBinSize), Dim - 1);
        
        for (int32_t i = itemRange.l; i <= itemRange.r; i++) {
            for (int32_t j = itemRange.t; j <= itemRange.b; j++) {
                m[i][j].lock();
                buckets[i][j].push_back(&cell);
                m[i][j].unlock();
            }
        }

        count++;
    }

    inline void remove(Cell& cell) {
        GridRange& itemRange = cell.shared.range;

        for (int32_t i = itemRange.l; i <= itemRange.r; i++) {
            for (int32_t j = itemRange.t; j <= itemRange.b; j++) {
                auto& bucket = buckets[i][j];
                m[i][j].lock();
                auto iter = std::find(bucket.begin(), bucket.end(), &cell);
                if (iter != bucket.end()) bucket.erase(iter);
                m[i][j].unlock();
            }
        }
        
        count--;
    }

    inline bool update(Cell& cell) {
        GridRange& oldRange = cell.shared.range;
        GridRange newRange;
        
        newRange.l = std::max(int32_floor(((cell.x - cell.r) - aabb.l) / xBinSize), 0);
        newRange.r = std::min(int32_floor(((cell.x + cell.r) - aabb.l) / xBinSize), Dim - 1);
        newRange.t = std::max(int32_floor((aabb.t - (cell.y + cell.r)) / yBinSize), 0);
        newRange.b = std::min(int32_floor((aabb.t - (cell.y - cell.r)) / yBinSize), Dim - 1);

        // Same bucket, no need to update
        if (newRange.l == oldRange.l &&
            newRange.r == oldRange.r &&
            newRange.t == oldRange.t &&
            newRange.b == oldRange.b) return false;

        remove(cell);
        insert(cell);

        return true;
    }

/*
    void removeCells(unsigned int sectorX = 0, unsigned int sectorY = 0, unsigned int block = 1) {
        unsigned int x0 = sectorX * Dim / block;
        unsigned int y0 = sectorY * Dim / block;
        unsigned int x1 = (sectorX + 1) * Dim / block;
        unsigned int y1 = (sectorY + 1) * Dim / block;

        for (unsigned int i = x0; i < x1; i++) {
            for (unsigned int j = y0; j < y1; j++) {
                auto& bucket = buckets[i][j];
                uint32_t w_id = 0;
                for (uint32_t k = 0; k < bucket.size(); k++) {
                    if (!(bucket[k]->flag & REMOVE_BIT)) {
                        if (w_id != k) bucket[w_id] = bucket[k];
                        w_id++;
                    }
                }
                bucket.resize(w_id);
            }
        }
    }
*/

    template <typename QueryFunc>
    inline void query(Cell& cell, const QueryFunc& cb) {
        GridRange& itemRange = cell.shared.range;

        for (int32_t i = itemRange.l; i <= itemRange.r; i++) {
            for (int32_t j = itemRange.t; j <= itemRange.b; j++) {
                for (auto other : buckets[i][j]) 
                    if (&cell != other && cb(other)) return;
            }
        }
    }

    template <typename T, typename QueryFunc>
    inline void query(TAABB<T>& box, const QueryFunc& cb, bool& escape) {
        GridRange rg = fromAABB(box);
    
        for (int32_t i = rg.l; i <= rg.r; i++) {
            for (int32_t j = rg.t; j <= rg.b; j++) {
                for (auto other : buckets[i][j]) {
                    cb(other);
                    if (escape) return;
                }
            }
        }
    }

/*
    template <typename DiffFunc1, typename DiffFunc2>
    void diff(AABB& box1, AABB& box2, const DiffFunc1& func1, const DiffFunc2& func2) {
        GridRange rg1 = fromAABB(box1);
        GridRange rg2 = fromAABB(box2);

        if (rg1 == rg2) return;
        
        if (!rg1.intersect(rg2)) {
            for (int32_t i = rg1.l; i <= rg1.r; i++) {
                for (int32_t j = rg1.t; j <= rg1.b; j++) {
                    for (auto c : buckets[i][j]) func1(c);
                }
            }

            for (int32_t i = rg2.l; i <= rg2.r; i++) {
                for (int32_t j = rg2.t; j <= rg2.b; j++) {
                    for (auto c : buckets[i][j]) func2(c);
                }
            }
            return;
        }

        GridRange merged = GridRange::merge(rg1, rg2);
    
        for (int32_t i = merged.l; i <= merged.r; i++) {
            for (int32_t j = merged.t; j <= merged.b; j++) {
                bool b1 = rg1.include(i, j);
                bool b2 = rg2.include(i, j);
                if (b1 && !b2) {
                    for (auto c : buckets[i][j]) func1(c);
                } else if (!b1 && b2) {
                    for (auto c : buckets[i][j]) func2(c);
                }
            }
        }
    }

    template <typename DiffFunc1, typename DiffFunc2, typename UnionFunc>
    void diff(AABB& box1, AABB& box2, const DiffFunc1& func1, const DiffFunc2& func2, const UnionFunc& func3) {
        GridRange rg1 = fromAABB(box1);
        GridRange rg2 = fromAABB(box2);

        if (rg1 == rg2) {
            for (int32_t i = rg1.l; i <= rg1.r; i++) {
                for (int32_t j = rg1.t; j <= rg1.b; j++) {
                    for (auto c : buckets[i][j]) func3(c);
                }
            }
            return;
        }

        if (!rg1.intersect(rg2)) {
            for (int32_t i = rg1.l; i <= rg1.r; i++) {
                for (int32_t j = rg1.t; j <= rg1.b; j++) {
                    for (auto c : buckets[i][j]) func1(c);
                }
            }

            for (int32_t i = rg2.l; i <= rg2.r; i++) {
                for (int32_t j = rg2.t; j <= rg2.b; j++) {
                    for (auto c : buckets[i][j]) func2(c);
                }
            }
            return;
        }

        GridRange merged = GridRange::merge(rg1, rg2);
    
        for (int32_t i = merged.l; i <= merged.r; i++) {
            for (int32_t j = merged.t; j <= merged.b; j++) {
                bool b1 = rg1.include(i, j);
                bool b2 = rg2.include(i, j);
                if (b1 && b2) for (auto c : buckets[i][j]) func3(c);
                else if (b1 && !b2) for (auto c : buckets[i][j]) func1(c);
                else if (!b1 && b2) for (auto c : buckets[i][j]) func2(c);
            }
        }
    }
*/

    void clear() {
        for (uint32_t i = 0; i < Dim; i++) {
            for (uint32_t j = 0; j < Dim; j++) {
                buckets[i][j].clear();
                buckets[i][j].shrink_to_fit();
            }
        }
        count = 0;
    }

    void gc() {
        for (uint32_t i = 0; i < Dim; i++) {
            for (uint32_t j = 0; j < Dim; j++) {
                buckets[i][j].shrink_to_fit();
            }
        }
    }
};