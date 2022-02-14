#include "bot.hpp"
#include "../physics/engine.hpp"

void Bot::setNextAction(float seconds) {
    __nextActionTick = engine->__now.load() + SECOND_TO_NANO * seconds;
}

void Bot::onTick() {
    if (!engine->updateBot || engine->__now.load() < __nextActionTick) return;
    if (engine->usage.load() > 0.75f) {
        setNextAction(10.f);
        return;
    };

    auto AI = engine->getAI();

    std::uniform_int_distribution<int> splitsPicker(0, AI->BOT_MAX_SPLIT_ATTEMPT);

    const float BOT_SPAWN_SIZE = engine->getBotSpawnSize();
    const float BOT_RESPAWN_THRESH = BOT_SPAWN_SIZE * BOT_SPAWN_SIZE * 0.002f;

    if (!control->alive || control->score < BOT_RESPAWN_THRESH) {

        if (!engine->alwaysSpawnBot && AI->BOT_SPAWN_MAX > 0) {
            auto totalMass = engine->botMass + engine->playerMass * 0.25f;
            if (totalMass > AI->BOT_SPAWN_MAX) {
                return setNextAction(10);
            }
        }
        
        // Needs mutex guard because onTick is called from multithread
        engine->sync([&] { control->requestSpawn(); });
        setNextAction(AI->BOT_RESPAWN_CD);
    } else {
        size_t canEjectCount = 0;
        const float PLAYER_MIN_EJECT_SIZE = engine->getMinEjectSize();

        const float maxSplits = AI->BOT_SPLIT_MAX_CELL ? AI->BOT_SPLIT_MAX_CELL : engine->getPlayerMaxCell();

        for (auto c : control->cells) 
            if (c->r > PLAYER_MIN_EJECT_SIZE) canEjectCount++;

        if (canEjectCount > AI->BOT_CENTER_FEED_EJECT) {
            control->ejectMacro = true;
            control->__mouseX = control->viewport.x;
            control->__mouseY = control->viewport.y;
            setNextAction(AI->BOT_IDLE_TIME);
            return;
        } else {
            control->ejectMacro = false;

            auto chance = actionPicker(generator);
            
            if (control->cells.size() < AI->BOT_SOLOTRICK_MAX_CELL && 
                control->score < AI->BOT_SOLOTRICK_MASS && chance < AI->BOT_SOLOTRICK_CHANCE) {
                // Solotrick
                control->ejectMacro = true;
                control->splits = 8;
                setNextAction(AI->BOT_SOLOTRICK_CD);
                return;
            } else if (control->cells.size() < maxSplits && chance > (100 - AI->BOT_SPLIT_CHANCE)) {
                // Random direction
                auto angle = engine->rngAngle();
                control->__mouseX = control->viewport.x + 5000 * sinf(angle);
                control->__mouseY = control->viewport.y + 5000 * cosf(angle);

                control->splits = splitsPicker(generator);
                setNextAction(AI->BOT_SPLIT_CD);
                return;
            }

            // Scale it back to 0-1
            chance = (chance - AI->BOT_SOLOTRICK_CHANCE) / (100.f - AI->BOT_SOLOTRICK_CHANCE - AI->BOT_SPLIT_CHANCE);

            if (AI->BOT_MOVE_MASS > 0.f) {
                const float moveChance = (AI->BOT_MOVE_MASS - control->score) / AI->BOT_MOVE_MASS;
                
                // logger::info("BOT_MOVE_MASS: %.1f, mass: %.1f, "
                //     "moveChance: %.1f, chance: %.1f\n", AI->BOT_MOVE_MASS, control->score,
                //     moveChance, chance);

                if (chance > moveChance) {
                    // Random direction
                    auto angle = engine->rngAngle();
                    control->__mouseX = control->viewport.x + 5000 * sinf(angle);
                    control->__mouseY = control->viewport.y + 5000 * cosf(angle);
                    setNextAction(AI->BOT_IDLE_TIME);
                }
            }

            control->__mouseX = control->viewport.x;
            control->__mouseY = control->viewport.y;
            setNextAction(AI->BOT_IDLE_TIME);
            return;
        }
    }
}