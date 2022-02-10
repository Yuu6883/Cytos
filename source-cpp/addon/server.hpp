#pragma once

#define NOMINMAX
#include <uv.h>
#include <v8.h>

#include <iostream>
#include <chrono>

using namespace v8;
using namespace std::chrono;

// Define Server before including engine templates
struct Server {
    uv_timer_t tick_timer;

    Isolate* isolate;
    class Engine* engine;
    class ThreadPool* threadPool;
    class Player* player;

    int64_t timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    UniquePersistent<Function> jsCellBufferCallback;
    UniquePersistent<Function> jsInfoCallback;
};

#define DECL_V8_EXPORT(func) void func(const FunctionCallbackInfo<Value>& args)
#define CYTOS_IMPL(func) void CytosAddon::func(const FunctionCallbackInfo<Value>& args)

namespace CytosAddon {

    DECL_V8_EXPORT(setInput);
    DECL_V8_EXPORT(getVersion);
    DECL_V8_EXPORT(setThreads);
    DECL_V8_EXPORT(setGameMode);
    DECL_V8_EXPORT(setBufferCallback);
    DECL_V8_EXPORT(setInfoCallback);
    DECL_V8_EXPORT(getTimings);

    DECL_V8_EXPORT(restore);
    DECL_V8_EXPORT(save);

    // Export API
    Server* Main(Local<Object> exports);
}

#undef DECL_V8_EXPORT