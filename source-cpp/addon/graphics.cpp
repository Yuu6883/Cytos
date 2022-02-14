#define NOMINMAX
#include <uv.h>
#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <sstream>
#include "cell.hpp"
#include "../misc/reader.hpp"

using namespace v8;
using std::stringstream;

constexpr uint16_t BOT_BIT = 0x1;
constexpr uint16_t SKIN_VALID_BIT = 0x2;
constexpr uint16_t NAME_VALID_BIT = 0x4;

struct PlayerData {
    uint8_t flags;
    int8_t skinState;
    uint8_t nameTexUnit;
    uint8_t skinTexUnit;
    float nameTexUVsWH[6];
    float skinTexUVs[4];
    Color skinTheme;
};

struct BitmapTextData {
    float width;
    float uvs[4];
};

constexpr uint16_t MAX_PLAYERS = ROCK_TYPE - 1;

#define MAX_CELL_LIMIT 131072

static std::mt19937 generator;
static std::uniform_int_distribution<int> color_offset_dist(0, COLOR_COUNT - 1);

struct ClientState {
    vector<RenderCell*> cells;
    vector<RenderCell*> removing;
    vector<RenderCell*> rendering;
    vector<RenderCell*> freed;

    RenderCell pool[MAX_CELL_LIMIT];

    PlayerData players[MAX_PLAYERS];
    BitmapTextData charText[256];

    int color_offset;

    ClientState() {
        memset(&charText, 0, sizeof(charText));
        init();
    }

    void init() {
        color_offset = color_offset_dist(generator);

        // Not neccesary but in case
        memset(&pool,     0, sizeof(pool));
        memset(&players,  0, sizeof(players));

        cells.clear();
        cells.shrink_to_fit();
        cells.reserve(2048);

        removing.clear();
        removing.shrink_to_fit();
        removing.reserve(2048);

        freed.clear();
        freed.shrink_to_fit();
        freed.reserve(MAX_CELL_LIMIT);

        rendering.clear();
        rendering.shrink_to_fit();
        rendering.reserve(4096);

        for (int i = MAX_CELL_LIMIT - 1; i >= 0; i--) {
            freed.push_back(&this->pool[i]);
        }
    }

    RenderCell* newCell() {
        if (freed.size()) {
            auto ptr = freed.back();
            freed.pop_back();
            cells.push_back(ptr);
            return ptr;
        } else return nullptr;
    }
};

constexpr size_t state_size = sizeof(ClientState);

inline void writeVertices(float* buffer, size_t& i, 
    float x0, float y0, float x1, float y1,
    float uv_x0, float uv_y0, float uv_x1, float uv_y1,
    Color color, float alpha, uint8_t texUnit) {
    
    buffer[i++] = x0;
    buffer[i++] = y0;
    buffer[i++] = uv_x0;
    buffer[i++] = uv_y1;
    buffer[i++] = color.r;
    buffer[i++] = color.g;
    buffer[i++] = color.b;
    buffer[i++] = alpha;
    buffer[i++] = texUnit;

    buffer[i++] = x1;
    buffer[i++] = y0;
    buffer[i++] = uv_x1;
    buffer[i++] = uv_y1;
    buffer[i++] = color.r;
    buffer[i++] = color.g;
    buffer[i++] = color.b;
    buffer[i++] = alpha;
    buffer[i++] = texUnit;

    buffer[i++] = x0;
    buffer[i++] = y1;
    buffer[i++] = uv_x0;
    buffer[i++] = uv_y0;
    buffer[i++] = color.r;
    buffer[i++] = color.g;
    buffer[i++] = color.b;
    buffer[i++] = alpha;
    buffer[i++] = texUnit;

    buffer[i++] = x1;
    buffer[i++] = y0;
    buffer[i++] = uv_x1;
    buffer[i++] = uv_y1;
    buffer[i++] = color.r;
    buffer[i++] = color.g;
    buffer[i++] = color.b;
    buffer[i++] = alpha;
    buffer[i++] = texUnit;

    buffer[i++] = x0;
    buffer[i++] = y1;
    buffer[i++] = uv_x0;
    buffer[i++] = uv_y0;
    buffer[i++] = color.r;
    buffer[i++] = color.g;
    buffer[i++] = color.b;
    buffer[i++] = alpha;
    buffer[i++] = texUnit;

    buffer[i++] = x1;
    buffer[i++] = y1;
    buffer[i++] = uv_x1;
    buffer[i++] = uv_y0;
    buffer[i++] = color.r;
    buffer[i++] = color.g;
    buffer[i++] = color.b;
    buffer[i++] = alpha;
    buffer[i++] = texUnit;
}

inline void writeQuadVertices(float* buffer, size_t& i, 
    float x0, float y0, float x1, float y1, 
    float x2, float y2, float x3, float y3,
    float  uv_x0, float uv_y0, float uv_x1, float uv_y1,
    Color color, float alpha, uint8_t texUnit) {

        buffer[i++] = x0;
        buffer[i++] = y0;
        buffer[i++] = uv_x0;
        buffer[i++] = uv_y1;
        buffer[i++] = color.r;
        buffer[i++] = color.g;
        buffer[i++] = color.b;
        buffer[i++] = alpha;
        buffer[i++] = texUnit;

        buffer[i++] = x1;
        buffer[i++] = y1;
        buffer[i++] = uv_x1;
        buffer[i++] = uv_y1;
        buffer[i++] = color.r;
        buffer[i++] = color.g;
        buffer[i++] = color.b;
        buffer[i++] = alpha;
        buffer[i++] = texUnit;

        buffer[i++] = x2;
        buffer[i++] = y2;
        buffer[i++] = uv_x0;
        buffer[i++] = uv_y0;
        buffer[i++] = color.r;
        buffer[i++] = color.g;
        buffer[i++] = color.b;
        buffer[i++] = alpha;
        buffer[i++] = texUnit;

        buffer[i++] = x1;
        buffer[i++] = y1;
        buffer[i++] = uv_x1;
        buffer[i++] = uv_y1;
        buffer[i++] = color.r;
        buffer[i++] = color.g;
        buffer[i++] = color.b;
        buffer[i++] = alpha;
        buffer[i++] = texUnit;

        buffer[i++] = x2;
        buffer[i++] = y2;
        buffer[i++] = uv_x0;
        buffer[i++] = uv_y0;
        buffer[i++] = color.r;
        buffer[i++] = color.g;
        buffer[i++] = color.b;
        buffer[i++] = alpha;
        buffer[i++] = texUnit;

        buffer[i++] = x3;
        buffer[i++] = y3;
        buffer[i++] = uv_x1;
        buffer[i++] = uv_y0;
        buffer[i++] = color.r;
        buffer[i++] = color.g;
        buffer[i++] = color.b;
        buffer[i++] = alpha;
        buffer[i++] = texUnit;
}

// Welcome to macro HELL

#define field(obj, str) (obj)->Get(ctx, String::NewFromUtf8Literal(iso, str)).ToLocalChecked()
#define fieldTyped(obj, str, type) field(obj, str).As<type>()
#define objField(obj, str) fieldTyped(obj, str, Object)
#define intField(obj, str) field(obj, str)->Int32Value(ctx).ToChecked()
#define numField(obj, str) (float) fieldTyped(obj, str, Value)->NumberValue(ctx).ToChecked()

#define indexObj(obj, i) (obj)->Get(ctx, (i)).ToLocalChecked()
#define indexInt(obj, i) indexObj(obj, i)->Int32Value(ctx).ToChecked()
#define indexFloat(obj, i) ((float) indexObj(obj, i)->NumberValue(ctx).ToChecked())
#define indexTyped(obj, i, type) indexObj(obj, i).As<type>()

void postInit(const FunctionCallbackInfo<Value>& args) {
    auto state = static_cast<ClientState*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();

    auto clientObj = args[0].As<Object>();

    auto floatArr = fieldTyped(clientObj, "massWidth", Float32Array);
    // Don't overflow the buffer
    if (floatArr->Length() < 256) {
        iso->ThrowException(Exception::RangeError(
            String::NewFromUtf8Literal(iso, "charText buffer not big enough")));
        return;
    }
    auto v8buf = floatArr->Buffer();
    auto nodeBuffer = node::Buffer::New(iso, v8buf, 0, v8buf->ByteLength()).ToLocalChecked();
    auto buffer = reinterpret_cast<float*>(node::Buffer::Data(nodeBuffer.As<Object>()));
    
    for (int i = 0; i < 256; i++) {
        state->charText[i].width = buffer[i];
    }

    // Read charTex uvs
    auto jsCharTexArray = fieldTyped(clientObj, "charTex", Map)->AsArray();
    auto charTexLen = jsCharTexArray->Length() / 2;

    for (uint32_t i = 0; i < charTexLen; i++) {
        auto charCode = indexInt(jsCharTexArray, i * 2);
        if (charCode >= 256 || charCode < 0) continue;
        auto charTexObj = indexTyped(jsCharTexArray, i * 2 + 1, Object);

        auto jsUVs = fieldTyped(charTexObj, "uvs", Float32Array);
        auto& uvs = state->charText[charCode].uvs;

        for (uint32_t i = 0; i < 4; i++) {
            uvs[i] = indexFloat(jsUVs, i);
        }
    }
}

void render(const FunctionCallbackInfo<Value>& args) {
    auto state = static_cast<ClientState*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();

    auto clientObj = args[0].As<Object>();
    auto lerp = (float) args[1]->NumberValue(ctx).ToChecked();
    auto dt =   (float) args[2]->NumberValue(ctx).ToChecked();
    auto debug = args[3]->BooleanValue(iso);

    auto debugOutput = fieldTyped(clientObj, "debugOutput", Object);

    // Read player data into packed flat array so indexing will be faster in the render loop
    auto jsPlayerDataArray = fieldTyped(clientObj, "playerData", Map)->AsArray();
    auto players = jsPlayerDataArray->Length() / 2;

    for (uint32_t i = 0; i < players; i++) {
        auto pid = indexInt(jsPlayerDataArray, i * 2);
        if (pid >= MAX_PLAYERS) continue;
        auto playerObj = indexTyped(jsPlayerDataArray, i * 2 + 1, Object);

        auto& data = state->players[pid];
        data.flags = 0;

        auto isBot = field(playerObj, "isBot")->BooleanValue(iso);
        if (isBot) {
            data.flags |= BOT_BIT;
        } else {
            auto nameTex = fieldTyped(playerObj, "nameTex", Object);
            auto nameTexValid = field(nameTex, "valid")->BooleanValue(iso);
            if (nameTexValid) {
                data.flags |= NAME_VALID_BIT;
                auto nameTexUVs = fieldTyped(nameTex, "uvs", Float32Array);
                data.nameTexUnit = intField(nameTex, "unit");
                for (uint32_t i = 0; i < 6; i++) {
                    data.nameTexUVsWH[i] = indexFloat(nameTexUVs, i);
                }
            }

            auto skinTex = fieldTyped(playerObj, "skinTex", Object);
            auto skinTexValid = field(skinTex, "valid")->BooleanValue(iso);
            if (skinTexValid) {
                data.flags |= SKIN_VALID_BIT;
                auto skinTexUVs = fieldTyped(skinTex, "uvs", Float32Array);
                data.skinTexUnit = intField(skinTex, "unit");
                for (uint32_t i = 0; i < 4; i++) {
                    data.skinTexUVs[i] = indexFloat(skinTexUVs, i);
                }
                auto skinTheme = fieldTyped(skinTex, "theme", Float32Array);
                data.skinTheme.r = indexFloat(skinTheme, 0);
                data.skinTheme.g = indexFloat(skinTheme, 1);
                data.skinTheme.b = indexFloat(skinTheme, 2);
            }

            data.skinState = intField(playerObj, "skinState");
        }
    } 

    auto lastRAF = fieldTyped(clientObj, "lastRAF", Value)->IntegerValue(ctx).ToChecked();
   
    // Dumb way to load the pointer in, because electron still hasn't fixed their node.lib
    auto floatArr = fieldTyped(clientObj, "spriteBuffer", Float32Array);
    auto v8buf = floatArr->Buffer();
    auto nodeBuffer = node::Buffer::New(iso, v8buf, 0, v8buf->ByteLength()).ToLocalChecked();
    auto buffer = reinterpret_cast<float*>(node::Buffer::Data(nodeBuffer.As<Object>()));

#define loadUV(prefix, tex) \
    auto prefix ## _UV_X0 = indexFloat(tex, 0); \
    auto prefix ## _UV_Y0 = indexFloat(tex, 1); \
    auto prefix ## _UV_X1 = indexFloat(tex, 2); \
    auto prefix ## _UV_Y1 = indexFloat(tex, 3); \

#define UVs(prefix) prefix ## _UV_X0, prefix ## _UV_Y0, prefix ## _UV_X1, prefix ## _UV_Y1
    
    // printf("Reading " #key " UV\n");
#define loadUVKey(prefix, key) \
    auto key = objField(clientObj, #key); \
    auto key ## UVs = fieldTyped(key, "uvs", Float32Array); \
    loadUV(prefix, key ## UVs) \

#define validTex(key) auto key ## Valid = field(key, "valid")->BooleanValue(iso)

#define viewbox(k) numField(objField(clientObj, "viewbox"), #k)

    auto vT = viewbox(t);
    auto vB = viewbox(b);
    auto vL = viewbox(l);
    auto vR = viewbox(r);

    loadUVKey(CIRCLE, circleTex);
    loadUVKey(RING, ringTex);
    loadUVKey(VIRUS, virusTex);

    auto arr = fieldTyped(clientObj, "botTex", Array);
    auto botTex = indexTyped(arr, (lastRAF / 250) % 4, Object);

    auto botTexUVs = fieldTyped(botTex, "uvs", Float32Array);

    loadUV(BOT, botTexUVs);
    validTex(botTex);

    auto bw = indexFloat(botTexUVs, 4);
    auto bh = indexFloat(botTexUVs, 5);
    auto BOT_W = (bw / std::max(bw, bh)) * 0.25f;
    auto BOT_H = (bh / std::max(bw, bh)) * 0.25f;

    auto expTex = indexTyped(objField(clientObj, "expTex"), (lastRAF / 200) % 4, Object);
    auto expTexUVs = fieldTyped(expTex, "uvs", Float32Array);

    loadUV(EXP, expTexUVs);
    validTex(expTex);

    auto cytTex = indexTyped(objField(clientObj, "expTex"), std::max(int32_t((lastRAF / 100) % 14) - 10, 0), Object);
    auto cytTexUVS = fieldTyped(cytTex, "uvs", Float32Array);

    loadUV(CYT, cytTexUVS);
    validTex(cytTex);

    loadUVKey(ROCK, rockTex);
    validTex(rockTex);

    auto settings = objField(clientObj, "settings");

    const bool renderFood = field(objField(settings, "renderFood"), "v")->BooleanValue(iso);
    const bool renderSkin = field(objField(settings, "renderSkin"), "v")->BooleanValue(iso);
    const bool renderName = field(objField(settings, "renderName"), "v")->BooleanValue(iso);
    const auto massMode   = field(objField(settings, "renderMass"), "v")->Int32Value(ctx).ToChecked();
    const bool longMass = massMode == 2;

    auto textMin = std::min(vR - vL, vT - vB) * 0.03f;

    constexpr float MASS_GAP = 0.85f;
    constexpr float MASS_SCALE_X = 0.23f;
    constexpr float MASS_SCALE_Y = 0.25f;
    constexpr float MASS_Y_OFFSET = -0.33f;

    auto pids = fieldTyped(clientObj, "pids", Int16Array);
    auto pid0 = indexInt(pids, 0);
    auto pid1 = indexInt(pids, 1);

    uint16_t activePID = intField(clientObj, "activeTab") == 0 ? pid0 : pid1;
    uint16_t inactivePID = intField(clientObj, "activeTab") == 1 ? pid0 : pid1;

    auto themes = objField(clientObj, "themes");

    auto autoTheme = field(objField(themes, "autoTheme"), "v")->BooleanValue(iso);
    auto showInactive = field(objField(themes, "showInactiveTabBorder"), "v")->BooleanValue(iso);
    auto animateFood = field(fieldTyped(themes, "foodAnimation", Object), "v")->BooleanValue(iso);

    auto cellBorder = !field(clientObj, "spectating")->BooleanValue(iso) && field(objField(themes, "showBorder"), "v")->BooleanValue(iso);

    auto themeColors = objField(clientObj, "themeComputed");
#define colorField(key) \
    auto key = Color(); \
    { \
       auto temp = fieldTyped(themeColors, #key, Float32Array); \
       key.r = indexFloat(temp, 0); \
       key.g = indexFloat(temp, 1); \
       key.b = indexFloat(temp, 2); \
    } \

    colorField(activeColor);
    colorField(inactiveColor);
    colorField(foodColor);
    colorField(ejectColor);

    auto& cells = state->cells;
    auto& freed = state->freed;
    auto& removing = state->removing;
    auto& rendering = state->rendering;

    if (cells.size() + removing.size() + freed.size() != MAX_CELL_LIMIT) {
        iso->ThrowException(Exception::RangeError(
            String::NewFromUtf8Literal(iso, "Cell pool integrity check failed")));
        return;
    }

    // Filter render cells and sort them
    rendering.clear();
    rendering.reserve(cells.size() + removing.size());

    for (auto& cell : cells) {
        if (!renderFood && cell->type == PELLET_TYPE) continue;
        cell->update(lerp, dt, animateFood);
        if (cell->cX - cell->cR < vR &&
            cell->cX + cell->cR > vL &&
            cell->cY - cell->cR < vT &&
            cell->cY + cell->cR > vB) rendering.push_back(cell);
    }
    
    removing.erase(std::remove_if(removing.begin(), removing.end(), 
        [&](RenderCell*& cell) { 
        if (cell->removeUpdate(dt)) {
            freed.push_back(cell); // "free" the cell
            return true;
        }
        if (!renderFood && cell->type == PELLET_TYPE) return false;
        if (cell->cX - cell->cR < vR &&
            cell->cX + cell->cR > vL &&
            cell->cY - cell->cR < vT &&
            cell->cY + cell->cR > vB) rendering.push_back(cell);
        return false; // Keep the cell in the array
    }), removing.end());

    // Ascend, draw smaller cells first since we are not doing Z-test
    std::sort(rendering.begin(), rendering.end(), [](auto a, auto b) {
        return a->cR <= b->cR;
    });

    clientObj->Set(ctx, String::NewFromUtf8Literal(iso, "rendercells"), Number::New(iso, rendering.size()));

    constexpr float CIRCLE_RADIUS = 512;
    constexpr float CIRCLE_PADDING = 6;
    constexpr float P = 1 + CIRCLE_PADDING / CIRCLE_RADIUS;

    char mass_buffer[64];

    // fprintf(stderr, "Rendering %llu cells, (%llu, %llu)\n", rendering.size(), cells.size(), removing.size());
    auto start = uv_hrtime();

    size_t write_index = 0;
    for (auto cell : rendering) {
        const auto& type = cell->type;
        const auto& x = cell->cX;
        const auto& y = cell->cY;
        const auto& rr = cell->cR;
        const auto& color = cell->color;
        const auto& alpha = cell->alpha;

#define X0Y0X1Y1 x - r, y - r, x + r, y + r

        if ((type & EJECT_BIT) || (type == PELLET_TYPE)) {
            auto r = rr * P;
            auto pid = type & PELLET_TYPE;
            auto& p = state->players[pid];
            if (autoTheme && (p.skinState == 1) && (p.flags & SKIN_VALID_BIT)) {
                writeVertices(buffer, write_index, X0Y0X1Y1, UVs(CIRCLE), p.skinTheme, alpha, 1);
            } else writeVertices(buffer, write_index, X0Y0X1Y1, UVs(CIRCLE), color, alpha, 1);
        } else if (type == DEAD_TYPE) {
            auto r = rr * P;
            writeVertices(buffer, write_index, X0Y0X1Y1, UVs(CIRCLE), color, alpha * 0.5f, 1);
        } else if (type == VIRUS_TYPE) {
            auto r = rr * 1.2f;
            writeVertices(buffer, write_index, X0Y0X1Y1, UVs(VIRUS), color, alpha, 1);
        } else if (type == CYT_TYPE) {
            if (!cytTexValid) continue;
            auto r = rr * 1.1f;
            writeVertices(buffer, write_index, X0Y0X1Y1, UVs(CYT), color, alpha, 1);
        } else if (type == EXP_TYPE) {
            if (!expTexValid) continue;
            auto r = rr * 1.2f;
            writeVertices(buffer, write_index, X0Y0X1Y1, UVs(CYT), color, alpha, 1);
        } else if (type == ROCK_TYPE) {
            if (!rockTexValid) continue;
            const float r = rr * 1.05f;
            const float s = sinf(cell->rotation);
            const float c = sinf(cell->rotation);
            const float x0 = -c + s;
            const float y0 = -c - s;
            writeQuadVertices(buffer, write_index, 
                x + x0, y + y0, x + y0, y - x0,
                x - y0, y + x0, x - x0, y - y0,
                UVs(ROCK), color, alpha, 1);
        } else {
            // Player cell
            auto r = rr * P;
            auto pid = type & PELLET_TYPE;
            auto& p = state->players[pid];

            // Draw skin or circle
            if (renderSkin && !(p.flags & BOT_BIT) && 
                (p.flags & SKIN_VALID_BIT) && p.skinState == 1) {
                writeVertices(buffer, write_index, X0Y0X1Y1,
                    p.skinTexUVs[0], p.skinTexUVs[1], p.skinTexUVs[2], p.skinTexUVs[3],
                    WHITE, alpha, p.skinTexUnit);
            } else {
                writeVertices(buffer, write_index, X0Y0X1Y1,
                    UVs(CIRCLE), color, alpha, 1);
            }

            // Draw border
            if (cellBorder) {
                if (pid == activePID) {
                    if (renderSkin && (!(p.flags & BOT_BIT)) && (p.flags & SKIN_VALID_BIT) && p.skinState == 1) {
                        writeVertices(buffer, write_index, X0Y0X1Y1,
                            UVs(RING), autoTheme ? p.skinTheme : activeColor, alpha, 1);
                    } else {
                        writeVertices(buffer, write_index, X0Y0X1Y1,
                            UVs(RING), autoTheme ? EJECTS_COLORS[(pid + state->color_offset) % COLOR_COUNT] : activeColor, alpha, 1);
                    }
                } else if (showInactive && pid == inactivePID) {
                    writeVertices(buffer, write_index, X0Y0X1Y1,
                        UVs(RING), inactiveColor, alpha, 1);
                }
            }

            // Draw name
            if (renderName && r > textMin) {
                // Draw Bot Icon
                if (p.flags & BOT_BIT) {
                    if (botTexValid) {
                        writeVertices(buffer, write_index,
                            x - BOT_W * r, y - BOT_H * r,
                            x + BOT_W * r, y + BOT_H * r,
                            UVs(BOT), WHITE, alpha, 1);
                        
                    }
                } else {
                    if (p.flags & NAME_VALID_BIT) {
                        auto& uvs = p.nameTexUVsWH;
                        float w = uvs[4] * 0.0015f * r;
                        float h = uvs[5] * 0.0015f * r;
                        writeVertices(buffer, write_index,
                            x - w, y - h, x + w, y + h,
                            uvs[0], uvs[1], uvs[2], uvs[3],
                            WHITE, alpha, p.nameTexUnit);
                    }
                }
            }

            // Draw mass
            if (massMode && r > textMin) {
                const float mass = cell->cR * cell->cR * 0.01f;
                int mass_len;

                if (longMass || mass < 1000) {
                    mass_len = snprintf(mass_buffer, 64, "%i", int(mass));
                } else if (mass > 1000000) {
                    mass_len = snprintf(mass_buffer, 64, "%.2fM", mass / 1000000);
                } else if (mass > 22500) {
                    mass_len = snprintf(mass_buffer, 64, "%iK", int(roundf(mass / 1000)));
                } else {
                    mass_len = snprintf(mass_buffer, 64, "%.1fK", mass / 1000);
                }

                float width = 0.f;
                for (int i = 0; i < mass_len; i++) {
                    // Indexing with char, assume it's greater than 0...
                    width += MASS_GAP * MASS_SCALE_X * state->charText[mass_buffer[i]].width;
                }

                // Center text
                float currX = width * -0.5f;
                for (int i = 0; i < mass_len; i++) {
                    auto& data = state->charText[mass_buffer[i]];
                    auto& uvs = data.uvs;
                    
                    float x0 = x + currX * r;
                    float x1 = x0 + MASS_SCALE_X * data.width * r;
                    float y0 = y + (-0.5f * MASS_SCALE_Y + MASS_Y_OFFSET) * r;
                    float y1 = y + (+0.5f * MASS_SCALE_Y + MASS_Y_OFFSET) * r;

                    currX += MASS_SCALE_X * data.width * MASS_GAP;

                    // BitmapText is on unit2
                    writeVertices(buffer, write_index, x0, y0, x1, y1, 
                        uvs[0], uvs[1], uvs[2], uvs[3], WHITE, alpha, 2);
                }
            }
        }
    }

    auto end = uv_hrtime();

    float debugVar = (end - start) / 1000.f / 1000.f;
    debugOutput->Set(ctx, String::NewFromUtf8Literal(iso, "buffering"), Number::New(iso, debugVar));
    args.GetReturnValue().Set(Number::New(iso, write_index));
}

void parse(const FunctionCallbackInfo<Value>& args) {
    auto state = static_cast<ClientState*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    
    auto clientObj = args[0].As<Object>();
    auto v8buf = args[1].As<ArrayBuffer>();
    auto nodeBuffer = node::Buffer::New(iso, v8buf, 0, v8buf->ByteLength()).ToLocalChecked();
    auto buffer = node::Buffer::Data(nodeBuffer.As<Object>());

    bool error = false;
    Reader r(string_view(buffer, v8buf->ByteLength()), error);

#define str(arg) String::NewFromUtf8Literal(iso, arg)
#define num(arg) Number::New(iso, arg)
#define set(obj, i, v) obj->Set(ctx, i, v)
#define bool(b) Boolean::New(iso, b)

    set(clientObj, str("spectating"), bool(r.read<uint8_t>()));

    auto flags = fieldTyped(clientObj, "flags", Uint8Array);
    set(flags, 0, num(r.read<uint8_t>()));
    set(flags, 1, num(r.read<uint8_t>()));

    auto pids = fieldTyped(clientObj, "pids", Uint16Array);
    set(pids, 0, num(r.read<uint16_t>()));
    set(pids, 1, num(r.read<uint16_t>()));

    auto cellCts = fieldTyped(clientObj, "cellCts", Uint16Array);
    set(cellCts, 0, num(r.read<uint16_t>()));
    set(cellCts, 1, num(r.read<uint16_t>()));

    auto scores = fieldTyped(clientObj, "scores", Float32Array);
    set(scores, 0, num(r.read<float>()));
    set(scores, 1, num(r.read<float>()));

    auto vports = fieldTyped(clientObj, "vports", Float32Array);
    set(vports, 0, num(r.read<float>()));
    set(vports, 1, num(r.read<float>()));
    set(vports, 2, num(r.read<float>()));
    set(vports, 3, num(r.read<float>()));

    auto map = fieldTyped(clientObj, "map", Float32Array);
    set(map, 0, num(r.read<float>()));
    set(map, 1, num(r.read<float>()));

    auto curr_size = r.read<uint16_t>();
    auto& cells = state->cells;
    auto& removing = state->removing;
    auto& freed = state->freed;

#undef str
#undef bool
#undef num
#undef set

    if (cells.size() != curr_size) {
        stringstream s;
        s << "Cell cache integrity check failed: " << cells.size() << " != " << curr_size;
        string out = s.str();

        auto jsString = String::NewFromUtf8(iso, out.c_str()).ToLocalChecked();
        fprintf(stderr, "Error: %s\n", out.c_str());
        
        iso->ThrowException(Exception::Error(jsString));
        return;
    }

    auto copy = cells;

    int w_id = 0;
    // Parse the buffer
    for (uint16_t i = 0; i < curr_size; i++) {
        if (w_id < i) cells[w_id] = cells[i];

        auto cell = cells[w_id];

        cell->oX = cell->cX;
        cell->oY = cell->cY;
        cell->oR = cell->cR;

        uint32_t flags = r.read<uint8_t>();
        uint32_t subop = flags >> 6;

        if (!subop) {
            cell->animation = 0;
            removing.push_back(cell);
        } else if (subop == 1) {
            w_id++;

            // Some delta decompression below
            if (!(flags & 63)) continue;

             uint32_t dx_op = (flags >> 4) & 3;
            if (!dx_op) {
                // Does nothing
            } else if (dx_op == 1) {
                uint16_t d = r.read<uint8_t>();
                cell->nX += d * 2;
            } else if (dx_op == 2) {
                uint16_t d = r.read<uint8_t>();
                cell->nX -= d * 2;
            } else {
                cell->nX = 2 * r.read<int16_t>();
            }

            uint32_t dy_op = (flags >> 2) & 3;
            if (!dy_op) {
                // Does nothing
            } else if (dy_op == 1) {
                uint16_t d = r.read<uint8_t>();
                cell->nY += d * 2;
            } else if (dy_op == 2) {
                uint16_t d = r.read<uint8_t>();
                cell->nY -= d * 2;
            } else {
                cell->nY = 2 * r.read<int16_t>();
            }

            uint32_t dr_op = flags & 3;
            if (!dr_op) {
                // Does nothing
            } else if (dr_op == 1) {
                uint16_t d = r.read<uint8_t>();
                cell->nR += d * 2;
            } else if (dr_op == 2) {
                uint16_t d = r.read<uint8_t>();
                cell->nR -= d * 2;
            } else {
                cell->nR = 2 * r.read<uint16_t>();
            }
        } else if (subop == 2) {
            auto eatenBy = r.read<uint16_t>();
            cell->animation = 0;
            cell->flags |= EATEN;
            cell->nX = cells[eatenBy]->cX;
            cell->nY = cells[eatenBy]->cY;
            removing.push_back(cell);
        } else if (subop == 3) {
            // :dead:
            iso->ThrowException(Exception::Error(String::NewFromUtf8Literal(iso, 
                "Unexpected subop 3")));
        }
    }

    cells.resize(w_id);

    auto newCount = r.read<uint16_t>();

    for (uint16_t i = 0; i < newCount; i++) {
        auto cell = state->newCell();
        // Type, x, y, r
        auto type = r.read<uint16_t>();
        auto x = 2 * r.read<int16_t>();
        auto y = 2 * r.read<int16_t>();
        auto R = 2 * r.read<uint16_t>();
        cell->init(type, x, y, R, state->color_offset);
    }

    if (error) {
        iso->ThrowException(Exception::RangeError(String::NewFromUtf8Literal(iso,
            "Reader error")));
        return;
    } else if (!r.eof()) {
        iso->ThrowException(Exception::RangeError(String::NewFromUtf8Literal(iso,
            "Expecting EOF on reader")));
        return;
    }

    args.GetReturnValue().Set(Number::New(iso, cells.size()));
}

#undef fieldTyped
#undef intField
#undef numField
#undef objField
#undef index
#undef indexFloat
#undef indexInt
#undef loadUV
#undef loadUVKey
#undef validTex
#undef viewbox

void getCellColor(const FunctionCallbackInfo<Value>& args) {
    auto state = static_cast<ClientState*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();

    auto id = args[0]->Int32Value(ctx).ToChecked();
    
    auto color = CELL_COLORS[std::max(0, id + state->color_offset) % COLOR_COUNT];

    auto obj = Array::New(iso, 3);

#define num(arg) Number::New(iso, arg)
#define set(obj, i, v) obj->Set(ctx, i, v)
    set(obj, 0, num(color.r));
    set(obj, 1, num(color.g));
    set(obj, 2, num(color.b));
#undef set
#undef num
    args.GetReturnValue().Set(obj);
}

void clear(const FunctionCallbackInfo<Value>& args) {
    auto state = static_cast<ClientState*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();

    state->init();
    args.GetReturnValue().Set(Boolean::New(iso, true));
}

void getPID(const FunctionCallbackInfo<Value>& args) {
    auto state = static_cast<ClientState*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();

    float x = (float) args[0]->NumberValue(ctx).ToChecked();
    float y = (float) args[1]->NumberValue(ctx).ToChecked();
    
    float maxR = 0.f;
    uint16_t pid = 0;
    for (auto& c : state->rendering) {
        if (c->type > CYT_TYPE) continue;
        if (c->cR < maxR) continue;
        float dx = x - c->cX;
        float dy = y - c->cY;
        float sqr = dx * dx + dy * dy;
        if (sqr > c->cR * c->cR) continue;
        maxR = c->cR;
        pid = c->type;
    }

    args.GetReturnValue().Set(Number::New(iso, pid));
}

// Helper function to set export value
template<typename Str, typename Func>
inline void exportFunc(Isolate*& isolate, Local<Object>& exports, Local<External>& ctx, Str name, Func func) {
    exports->Set(
        isolate->GetCurrentContext(), 
        String::NewFromUtf8(isolate, name, NewStringType::kNormal).ToLocalChecked(), 
        FunctionTemplate::New(isolate, func, ctx)->GetFunction(isolate->GetCurrentContext()).ToLocalChecked()
    ).ToChecked();
}

extern "C" NODE_MODULE_EXPORT void
NODE_MODULE_INITIALIZER(Local<Object> exports, Local<Value> module, Local<Context> context) {
    Isolate* isolate = exports->GetIsolate();
    
    // Per context state
    auto state = new ClientState();
    Local<External> ctx = External::New(isolate, state);

    exportFunc(isolate, exports, ctx, "postInit", postInit);
    exportFunc(isolate, exports, ctx, "render", render);
    exportFunc(isolate, exports, ctx, "parse", parse);
    exportFunc(isolate, exports, ctx, "getCellColor", getCellColor);
    exportFunc(isolate, exports, ctx, "clear", clear);
    exportFunc(isolate, exports, ctx, "getPID", getPID);
}