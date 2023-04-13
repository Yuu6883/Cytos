#include "server.hpp"

#include "../game/handle.hpp"
#include "../misc/pool.hpp"
#include "player.hpp"
// Headers
#include "../extensions/rockslide/rock-engine.hpp"
#include "../physics/engine.hpp"

// Implementations headers
#include <node.h>
#include <node_buffer.h>

#include "../extensions/rockslide/rock-engine-impl.hpp"
#include "../physics/engine-impl.hpp"

constexpr OPT make_rock_opt() {
    OPT temp = instant_opt;
    temp.EJECT_MAX_AGE = 1000;
    return temp;
};

constexpr OPT rock_opt = make_rock_opt();

typedef TemplateEngine<default_opt> DefaultEngine;
typedef TemplateEngine<ffa_opt> FFAEngine;
typedef TemplateEngine<instant_opt> InstantEngine;
typedef TemplateEngine<mega_opt> MegaEngine;
typedef TemplateEngine<omega_opt> OmegaEngine;
typedef TemplateEngine<sf_opt> SfEngine;
typedef TemplateEngine<ultra_opt> UltraEngine;
typedef RockEngine<rock_opt> RockslideEngine;

typedef TemplateEngine<omega_bench_opt> BenchOmegaEngine;

#define PHYSICS_TPS 25

#define MS_TO_NANO 1000000
#define MS_TO_NANO_F 1000000.f

constexpr uint32_t tick_time = 1000 / PHYSICS_TPS;
constexpr uint64_t tickNano = tick_time * MS_TO_NANO;

uint64_t origin = hrtime();
uint64_t total_time() { return hrtime() - origin; };

// Internal ticker
void internal_tick(uv_timer_t* t) {
    auto server = static_cast<Server*>(t->data);
    auto iso = server->isolate;
    auto engine = server->engine;

    auto start = hrtime();
    if (engine) {
        engine->__now = total_time();

        // MILLISECONDS
        float m = engine->getTimeScale() / MS_TO_NANO_F;
        engine->tick((engine->__now - engine->__ltick) * m);

        auto busyTimeNano = total_time() - engine->__now;
        constexpr float t = 1.f / (MS_TO_NANO_F * tick_time);
        engine->usage = busyTimeNano * t;
        engine->__ltick = engine->__now;
    }

    auto totalTimeNano = hrtime() - start;
    if (totalTimeNano > tickNano) {
        uv_timer_start(&server->tick_timer, internal_tick, 1, 0);
    } else {
        uint64_t timeLeft = (tickNano - totalTimeNano) / MS_TO_NANO;
        uv_timer_start(&server->tick_timer, internal_tick, timeLeft, 0);
    }
}

void Player::send(string_view buffer) {
    auto iso = server->isolate;
    HandleScope hs(iso);

    if (server->jsCellBufferCallback != Undefined(iso)) {
        auto func = Local<Function>::New(iso, server->jsCellBufferCallback);

        // fprintf(stderr, "buf ptr = %p\n", buffer.data());
        auto buf = node::Buffer::New(
                       iso, (char*)buffer.data(), buffer.size(),
                       [](auto data, auto) {
                           free(data);
                           // fprintf(stderr, "free buf ptr = %p\n", data);
                       },
                       nullptr)
                       .ToLocalChecked();

        Local<Value> argv[1] = {buf};

        node::MakeCallback(iso, iso->GetCurrentContext()->Global(), func, 1,
                           argv);
    }
}

void Engine::infoEvent(GameHandle* handle, EventType event) {
    if (!handle || !handle->control) return;

    auto iso = server->isolate;

    HandleScope scope(iso);
    auto ctx = iso->GetCurrentContext();

#define lit(arg) String::NewFromUtf8Literal(iso, arg)
#define str(arg) String::NewFromUtf8(iso, arg).ToLocalChecked()
#define num(arg) Number::New(iso, arg)
#define set(obj, i, v) obj->Set(ctx, i, v)

    if (server->jsInfoCallback != Undefined(iso)) {
        auto func = Local<Function>::New(iso, server->jsInfoCallback);
        auto obj = Object::New(iso);

        if (event == EventType::JOIN) {
            set(obj, lit("event"), lit("join"));
            set(obj, lit("id"), str(handle->gatewayID().data()));
            set(obj, lit("pid0"), num(handle->control->id));
            if (handle->dual && handle->dual->control) {
                set(obj, lit("pid1"), num(handle->dual->control->id));
            }

        } else if (event == EventType::LEAVE) {
            // Eh not needed for this
            return;
        } else if (event == EventType::ROCK_WIN) {
            uint32_t time_ms =
                (this->__now - handle->control->lastSpawned) / 1000 / 1000;

            set(obj, lit("event"), lit("join"));
            set(obj, lit("id"), str(handle->gatewayID().data()));
            set(obj, lit("rock"), num(time_ms));
        } else
            return;

        Local<Value> argv[1] = {obj};
        node::MakeCallback(iso, iso->GetCurrentContext()->Global(), func, 1,
                           argv);
    }

#undef str
#undef lit
#undef num
#undef set
}

CYTOS_IMPL(setThreads) {
    auto server =
        static_cast<Server*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();

    int32_t threads = 1;

    if (args.Length() > 0) {
        threads = args[0]
                      .As<Int32>()
                      ->Int32Value(iso->GetCurrentContext())
                      .ToChecked();
        printf("threads = %i\n", threads);
    }

    if (threads <= 0) threads = 1;
    if (threads > std::thread::hardware_concurrency())
        threads = std::thread::hardware_concurrency();

    if (server->threadPool) {
        if (server->threadPool->size() == threads) return;
        delete server->threadPool;
    }
    server->threadPool = new ThreadPool(threads);
}

CYTOS_IMPL(setGameMode) {
    auto server =
        static_cast<Server*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();
    auto& engine = server->engine;

    if (engine) delete engine;

    string mode;

    if (args.Length() >= 1) {
        String::Utf8Value v8Str(iso, args[0]);
        mode = *v8Str;
    }

    if (mode == "ffa") {
        engine = new FFAEngine(server);
    } else if (mode == "instant") {
        engine = new InstantEngine(server);
    } else if (mode == "mega") {
        engine = new MegaEngine(server);
    } else if (mode == "omega") {
        engine = new OmegaEngine(server);
    } else if (mode == "selffeed") {
        engine = new SfEngine(server);
    } else if (mode == "ultra") {
        engine = new UltraEngine(server);
    } else if (mode == "rockslide") {
        engine = new RockslideEngine(server);
    } else if (mode == "bench-omega") {
        engine = new BenchOmegaEngine(server);
        engine->alwaysSpawnBot = true;
    } else if (mode == "debug") {
        engine = new DefaultEngine(server);
    } else {
        logger::warn("Unknown Game Mode: %s\n", mode.c_str());
        // engine = new DefaultEngine(server);
        engine = nullptr;
    }

    server->player->setEngine(engine);

    if (engine) engine->start();
}

CYTOS_IMPL(setBufferCallback) {
    auto iso = args.GetIsolate();
    auto server =
        static_cast<Server*>(Local<External>::Cast(args.Data())->Value());

    if (args.Length() < 1) {
        server->jsCellBufferCallback.Reset(
            iso, Local<Function>::Cast(Undefined(iso)));
    } else {
        server->jsCellBufferCallback.Reset(iso, Local<Function>::Cast(args[0]));
    }
}

CYTOS_IMPL(setInfoCallback) {
    auto iso = args.GetIsolate();
    auto server =
        static_cast<Server*>(Local<External>::Cast(args.Data())->Value());

    if (args.Length() < 1) {
        server->jsInfoCallback.Reset(iso,
                                     Local<Function>::Cast(Undefined(iso)));
    } else {
        server->jsInfoCallback.Reset(iso, Local<Function>::Cast(args[0]));
    }
}

CYTOS_IMPL(getTimings) {
    auto iso = args.GetIsolate();
    auto server =
        static_cast<Server*>(Local<External>::Cast(args.Data())->Value());

    if (!server->engine) {
        args.GetReturnValue().Set(Null(iso));
        return;
    }

    HandleScope scope(iso);
    auto ctx = iso->GetCurrentContext();

#define str(arg) String::NewFromUtf8(iso, arg).ToLocalChecked()
#define lit(arg) String::NewFromUtf8Literal(iso, arg)
#define num(arg) Number::New(iso, arg)
#define set(o, i, v) o->Set(ctx, i, v)

    auto& e = server->engine;
    auto& t = e->timings;

    auto obj = Object::New(iso);

    set(obj, lit("spawn_cells"), num(t.spawn_cells));
    set(obj, lit("handle_io"), num(t.handle_io));
    set(obj, lit("spawn_handles"), num(t.spawn_handles));
    set(obj, lit("update_cells"), num(t.update_cells));
    set(obj, lit("resolve_physics"), num(t.resolve_physics));

    set(obj, lit("threads"), num(server->threadPool->size()));
    set(obj, lit("usage"), num(e->usage.load()));

    uint32_t counters[QUERY_LEVEL];
    e->countTreeItems(counters, QUERY_LEVEL);
    auto tree = Array::New(iso, QUERY_LEVEL);

    for (uint32_t i = 0; i < QUERY_LEVEL; i++) {
        set(tree, i, num(counters[i]));
    }

    set(obj, lit("tree"), tree);

    auto io = Array::New(iso, 3);

    set(io, 0, num(t.io.phase0));
    set(io, 1, num(t.io.phase1));
    set(io, 2, num(t.io.phase2));

    set(obj, lit("io"), io);

    auto phy = Array::New(iso, 8);

    set(phy, 0, num(t.physics.phase0));
    set(phy, 1, num(t.physics.phase1));
    set(phy, 2, num(t.physics.phase2));
    set(phy, 3, num(t.physics.phase3));
    set(phy, 4, num(t.physics.phase4));
    set(phy, 5, num(t.physics.phase5));
    set(phy, 6, num(t.physics.phase6));
    set(phy, 7, num(t.physics.phase7));

    set(obj, lit("physics"), phy);

    auto queries = Array::New(iso, 4);

    set(queries, 0, num(e->queries.phase0_total));
    set(queries, 1, num(e->queries.phase0_effi));
    set(queries, 2, num(e->queries.phase1_total));
    set(queries, 3, num(e->queries.phase1_effi));

    set(obj, lit("queries"), queries);

    auto counter = Array::New(iso, QUERY_LEVEL * 2);

    for (int i = 0; i < QUERY_LEVEL; i++) {
        set(counter, 2 * i, num(e->queries.level_counter[i]));
        set(counter, 2 * i + 1, num(e->queries.level_efficient[i]));
    }

    set(obj, lit("counter"), counter);

    args.GetReturnValue().Set(obj);
}

#define COMPILE_TIME __DATE__ " " __TIME__

CYTOS_IMPL(getVersion) {
    auto iso = args.GetIsolate();
    auto server =
        static_cast<Server*>(Local<External>::Cast(args.Data())->Value());

    HandleScope scope(iso);
    auto ctx = iso->GetCurrentContext();

    auto obj = Object::New(iso);
    set(obj, lit("version"), lit("CYTOS 0.0.3"));
    set(obj, lit("timestamp"), num(server->timestamp));
    set(obj, lit("compile"), lit(COMPILE_TIME));

    args.GetReturnValue().Set(obj);
}

#undef lit
#undef num
#undef set
#undef str

CYTOS_IMPL(setInput) {
    auto iso = args.GetIsolate();
    auto server =
        static_cast<Server*>(Local<External>::Cast(args.Data())->Value());

    HandleScope scope(iso);
    auto ctx = iso->GetCurrentContext();

    auto p = server->player;
    auto inputObj = args[0].As<Object>();

#define field(obj, str) \
    (obj)->Get(ctx, String::NewFromUtf8Literal(iso, str)).ToLocalChecked()
#define fieldTyped(obj, str, type) field(obj, str).As<type>()
#define objField(obj, str) fieldTyped(obj, str, Object)
#define intField(obj, str) field(obj, str)->Int32Value(ctx).ToChecked()
#define boolField(obj, str) field(obj, str)->BooleanValue(iso)
#define indexObj(obj, i) (obj)->Get(ctx, (i)).ToLocalChecked()

    p->spec(intField(inputObj, "spectate"));
    p->activeTab = intField(inputObj, "activeTab");

    auto tabDataArr = fieldTyped(inputObj, "data", Object);
    for (uint32_t i = 0; i <= 1; i++) {
        auto tabObj = indexObj(tabDataArr, i).As<Object>();

        auto& input = p->inputs[i];

        input.line |= boolField(tabObj, "line");
        input.spawn |= boolField(tabObj, "spawn");
        input.macro = boolField(tabObj, "macro");
        input.splits = std::min(input.splits + intField(tabObj, "splits"), 14);
        input.ejects = intField(tabObj, "ejects");
        input.mouseX = intField(tabObj, "mouseX");
        input.mouseY = intField(tabObj, "mouseY");
    }
}

CYTOS_IMPL(restart) {
    auto iso = args.GetIsolate();
    auto server =
        static_cast<Server*>(Local<External>::Cast(args.Data())->Value());

    auto& engine = server->engine;
    auto& player = server->player;

    if (!engine || !player) {
        logger::error("engine & player required\n");
        return;
    }

    // Reset again if error happens
    player->setEngine(nullptr);
    engine->reset();
    player->setEngine(engine);

    args.GetReturnValue().Set(Boolean::New(iso, true));
}

// Helper function to set export value
template <typename Str, typename Func>
inline void exportFunc(Isolate*& isolate, Local<Object>& exports,
                       Local<External>& ctx, Str name, Func func) {
    exports
        ->Set(isolate->GetCurrentContext(),
              String::NewFromUtf8(isolate, name, NewStringType::kNormal)
                  .ToLocalChecked(),
              FunctionTemplate::New(isolate, func, ctx)
                  ->GetFunction(isolate->GetCurrentContext())
                  .ToLocalChecked())
        .ToChecked();
}

Server* CytosAddon::Main(Local<Object> exports) {
    Isolate* iso = exports->GetIsolate();

    auto server = new Server();

    server->engine = nullptr;
    server->isolate = iso;
    server->player = new Player(server);

    // Above 8 threads scalabilty is bad
    uint32_t init_threads =
        std::clamp(std::thread::hardware_concurrency(), 1u, 8u);
    server->threadPool = new ThreadPool(init_threads);

    server->jsCellBufferCallback.Reset(iso,
                                       Local<Function>::Cast(Undefined(iso)));
    server->jsInfoCallback.Reset(iso, Local<Function>::Cast(Undefined(iso)));

    Local<External> serverCtx = External::New(iso, server);

    exportFunc(iso, exports, serverCtx, "setThreads", CytosAddon::setThreads);
    exportFunc(iso, exports, serverCtx, "setGameMode", CytosAddon::setGameMode);

    exportFunc(iso, exports, serverCtx, "onBuffer",
               CytosAddon::setBufferCallback);
    exportFunc(iso, exports, serverCtx, "onInfo", CytosAddon::setInfoCallback);

    exportFunc(iso, exports, serverCtx, "getTimings", CytosAddon::getTimings);
    exportFunc(iso, exports, serverCtx, "getVersion", CytosAddon::getVersion);

    exportFunc(iso, exports, serverCtx, "setInput", CytosAddon::setInput);

    exportFunc(iso, exports, serverCtx, "save", CytosAddon::save);
    exportFunc(iso, exports, serverCtx, "restore", CytosAddon::restore);
    exportFunc(iso, exports, serverCtx, "restart", CytosAddon::restart);

    return server;
}

extern "C" NODE_MODULE_EXPORT void NODE_MODULE_INITIALIZER(
    Local<Object> exports, Local<Value> module, Local<Context> context) {
    auto loop = node::GetCurrentEventLoop(context->GetIsolate());
    /* Register vanilla V8 addon */
    Server* server = CytosAddon::Main(exports);

    uv_timer_init(loop, &server->tick_timer);
    server->tick_timer.data = server;
    uv_timer_start(&server->tick_timer, internal_tick, 0, 0);

    node::AddEnvironmentCleanupHook(
        context->GetIsolate(), [](void* arg) {}, server);
}