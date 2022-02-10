#pragma once

#include "handle.hpp"
#include "control.hpp"

struct DualHandle : public GameHandle {
    DualHandle(GameHandle* dual) : GameHandle(dual->server) {
        this->dual = dual;
    }

    bool spectatable() { return false; }
    cell_cord_prec getScore() { return (control ? control->score : 0) + (dual->control ? dual->control->score : 0); }
};