#pragma once

#include "vector"
#include "algorithm"
#include "cell.hpp"

#include <thread>
#include <mutex>

using std::vector;
using std::mutex;

constexpr int32_t QUAD_NP = -1;
constexpr int32_t QUAD_TL = 0;
constexpr int32_t QUAD_TR = 1;
constexpr int32_t QUAD_BL = 2;
constexpr int32_t QUAD_BR = 3;

static thread_local vector<void*> rts;

struct QuadTree {
private:
    uint32_t maxLevel;
    uint32_t maxItems;
    
    struct QuadNode {
        QuadTree& tree;
        QuadNode* root;
        Rect rect;
        AABB box;
        QuadNode* branches;
        uint32_t level;
        vector<Cell*> items;
        mutex m;

        QuadNode(QuadTree& tree, Rect rect, QuadNode* root = nullptr):
            tree(tree), rect(rect), root(root), branches(nullptr) {
            box = {
                .l = rect.x - rect.hw,
                .r = rect.x + rect.hw,
                .b = rect.y - rect.hh,
                .t = rect.y + rect.hh,
            };
            level = root ? root->level + 1 : 1;
            items.reserve(tree.maxItems);
        };

        ~QuadNode() {
            clear();
        }

        void clear() {
            if (branches) delete[] branches;
            branches = nullptr;
        }

        const int32_t getQuadrant(Cell* cell) {
            if (cell->shared.aabb.b > rect.y) {
                if (cell->shared.aabb.r < rect.x)      return QUAD_TL;
                else if (cell->shared.aabb.l > rect.x) return QUAD_TR;
            } else if (cell->shared.aabb.t < rect.y) {
                if (cell->shared.aabb.r < rect.x)      return QUAD_BL;
                else if (cell->shared.aabb.l > rect.x) return QUAD_BR;
            }
            return QUAD_NP;
        }

        const bool contains(Cell* cell) {
            return  cell->shared.aabb.l > box.l &&
                    cell->shared.aabb.r < box.r &&
                    cell->shared.aabb.t < box.t &&
                    cell->shared.aabb.b > box.b;
        }

        void restructure() {
            uint32_t w_id = 0;
            for (uint32_t k = 0; k < items.size(); k++) {
                if (!(items[k]->flag & REMOVE_BIT)) {
                    if (w_id != k) items[w_id] = items[k];
                    w_id++;
                }
            }
            items.resize(w_id);

            split();
            if (branches) {
                branches[0].restructure();
                branches[1].restructure();
                branches[2].restructure();
                branches[3].restructure();
            }
            merge();
        }

        void split() {
            if (branches || 
                items.size() < tree.maxItems ||
                level >= tree.maxLevel) return;
            
            const cell_cord_prec qw = rect.hw / 2.0f;
            const cell_cord_prec qh = rect.hh / 2.0f;

            branches = new QuadNode[4] {
                QuadNode(tree, Rect { rect.x - qw, rect.y + qh, qw, qh }, this),
                QuadNode(tree, Rect { rect.x + qw, rect.y + qh, qw, qh }, this),
                QuadNode(tree, Rect { rect.x - qw, rect.y - qh, qw, qh }, this),
                QuadNode(tree, Rect { rect.x + qw, rect.y - qh, qw, qh }, this)
            };

            auto iter = items.begin();
            while (iter != items.end()) {
                auto q = getQuadrant(*iter);
                if (q < 0) iter++;
                else {
                    branches[q].items.push_back(*iter);
                    (*iter)->__root = &branches[q];
                    iter = items.erase(iter);
                }
            }
        }

        void merge() {
            if (!branches) return;
            if (branches[0].branches || branches[0].items.size() ||
                branches[1].branches || branches[1].items.size() ||
                branches[2].branches || branches[2].items.size() ||
                branches[3].branches || branches[3].items.size()) return;
            clear();
        }
    };

    
    QuadNode root;
public:
    QuadTree(Rect rect, uint32_t maxLevel, uint32_t maxItems):
        root(*this, rect), maxLevel(maxLevel), maxItems(maxItems) {
        rts.reserve(maxLevel << 2);
    };

    void clear() {
        root.clear();
    }

    void insert(Cell* cell) {
        auto node = &root;
        while (true) {
            if (!node->branches) break;
            auto q = node->getQuadrant(cell);
            if (q < 0) break;
            node = &node->branches[q];
        }

        cell->__root = node;
        node->m.lock();
        node->items.push_back(cell);
        node->m.unlock();
        // node->split();
    }

    void remove(Cell* cell) {
        auto __root = static_cast<QuadNode*>(cell->__root);
        auto iter = std::find(__root->items.begin(), __root->items.end(), cell);
        if (iter != __root->items.end()) __root->items.erase(iter);
        cell->__root = nullptr;
    }

    void update(Cell* cell) {
        auto oldNode = static_cast<QuadNode*>(cell->__root);
        auto newNode = oldNode;

        while (true) {
            if (!newNode->root) break;
            newNode = newNode->root;
            if (newNode->contains(cell)) break;
        }

        while (true) {
            if (!newNode->branches) break;
            auto q = newNode->getQuadrant(cell);
            if (q < 0) break;
            newNode = &newNode->branches[q];
        }

        if (oldNode == newNode) return;

        oldNode->m.lock();
        auto iter = std::find(oldNode->items.begin(), oldNode->items.end(), cell);
        if (iter != oldNode->items.end()) oldNode->items.erase(iter);
        oldNode->m.unlock();

        newNode->m.lock();
        newNode->items.push_back(cell);
        newNode->m.unlock();

        cell->__root = newNode;
        // oldNode->merge();
        // newNode->split();
    }

    void restructure() {
        root.restructure();
    }

    void swap(Cell* cell1, Cell* cell2) {
        cell2->__root = cell1->__root;
        auto oldRoot = static_cast<QuadNode*>(cell2->__root);
        auto iter = std::find(oldRoot->items.begin(), oldRoot->items.end(), cell1);
        if (iter != oldRoot->items.end()) *iter = cell2;
        cell1->__root = nullptr;
    }

    bool isSafe(AABB& aabb) {
        rts.clear();
        rts.push_back(&root);

        while (!rts.empty()) {
            auto curr = static_cast<QuadNode*>(rts.back());
            rts.pop_back();

            if (curr->branches) {
                if (aabb.t > curr->rect.y) {
                    if (aabb.l < curr->rect.x) rts.push_back(&curr->branches[QUAD_TL]);
                    if (aabb.r > curr->rect.x) rts.push_back(&curr->branches[QUAD_TR]);
                }
                if (aabb.b < curr->rect.y) {
                    if (aabb.l < curr->rect.x) rts.push_back(&curr->branches[QUAD_BL]);
                    if (aabb.r > curr->rect.x) rts.push_back(&curr->branches[QUAD_BR]);
                }
            }

            for (auto other : curr->items) {
                if (other->shared.aabb.insersect(aabb)) return false;
            }
        }

        return true;
    }

    template <typename QueryFunc>
    inline void query(Cell& cell, const QueryFunc& cb) {
        rts.clear();
        rts.push_back(&root);

        AABB aabb = cell.shared.aabb;

        while (!rts.empty()) {
            auto curr = static_cast<QuadNode*>(rts.back());
            rts.pop_back();

            if (curr->branches) {
                if (aabb.t > curr->rect.y) {
                    if (aabb.l < curr->rect.x) rts.push_back(&curr->branches[QUAD_TL]);
                    if (aabb.r > curr->rect.x) rts.push_back(&curr->branches[QUAD_TR]);
                }
                if (aabb.b < curr->rect.y) {
                    if (aabb.l < curr->rect.x) rts.push_back(&curr->branches[QUAD_BL]);
                    if (aabb.r > curr->rect.x) rts.push_back(&curr->branches[QUAD_BR]);
                }
            }

            for (auto other : curr->items) {
                if (&cell != other) cb(other);
            }
        }
    }

    template <typename QueryFunc>
    inline void query(AABB& aabb, const QueryFunc& cb) {
        rts.clear();
        rts.push_back(&root);

        while (!rts.empty()) {
            auto curr = static_cast<QuadNode*>(rts.back());
            rts.pop_back();

            if (aabb.contains(curr->box)) {
                auto s = rts.size();
                rts.push_back(curr);

                while (rts.size() > s) {
                    auto curr2 = static_cast<QuadNode*>(rts.back());
                    rts.pop_back();

                    if (curr2->branches) {
                        rts.push_back(&curr2->branches[QUAD_TL]);
                        rts.push_back(&curr2->branches[QUAD_TR]);
                        rts.push_back(&curr2->branches[QUAD_BL]);
                        rts.push_back(&curr2->branches[QUAD_BR]);
                    }
                    for (auto other : curr2->items) cb(other);
                }
            } else {
                if (curr->branches) {
                    if (aabb.t > curr->rect.y) {
                        if (aabb.l < curr->rect.x) rts.push_back(&curr->branches[QUAD_TL]);
                        if (aabb.r > curr->rect.x) rts.push_back(&curr->branches[QUAD_TR]);
                    }
                    if (aabb.b < curr->rect.y) {
                        if (aabb.l < curr->rect.x) rts.push_back(&curr->branches[QUAD_BL]);
                        if (aabb.r > curr->rect.x) rts.push_back(&curr->branches[QUAD_BR]);
                    }
                }
                
                for (auto other : curr->items) if (aabb.insersect(other->shared.aabb)) cb(other);
            }
        }
    }
};