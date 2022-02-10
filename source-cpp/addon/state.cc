#include <node_buffer.h>
#include "server.hpp"

#include "../misc/reader.hpp"
#include "../misc/writer.hpp"

#include "../game/control.hpp"
#include "../game/handle.hpp"
#include "player.hpp"
#include "../game/bot.hpp"

#include "../physics/engine.hpp"

CYTOS_IMPL(save) {
    auto server = static_cast<Server*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();

    HandleScope scope(iso);
    auto ctx = iso->GetCurrentContext();

    auto& engine = server->engine;
    auto& player = server->player;

    if (!engine || !player) return;

    Writer w;

    // Serialize engine
    engine->removeCells();
    
    // Write bot ID's
    w.write<uint16_t>(engine->bots.size());
    for (auto bot : engine->bots) {
        w.write<uint16_t>(bot->control->id);
    }

    // Write the entire buffer pool
    auto pool_size = engine->poolSize();
    w.write<size_t>(pool_size);
    w.write(string_view((char*) engine->pool, pool_size), false);

    // Ext buffer
    auto ext_buf = engine->getExtState();
    w.write<size_t>(ext_buf.size());
    w.write(ext_buf, false);

    auto buffer = w.finalize();
    
    auto nbuf = node::Buffer::New(iso, (char*) buffer.data(), buffer.size(), [](auto data, auto) {
        free(data);
        // fprintf(stderr, "free buf ptr = %p\n", data);
    }, nullptr).ToLocalChecked();

    auto result = Object::New(iso);

#define lit(arg) String::NewFromUtf8Literal(iso, arg)
#define str(arg) String::NewFromUtf8(iso, arg).ToLocalChecked()
#define set(obj, i, v) obj->Set(ctx, i, v)
    set(result, lit("mode"), str(engine->mode()));
    set(result, lit("buffer"), nbuf);
#undef lit
#undef str
#undef set

    args.GetReturnValue().Set(result);
}

CYTOS_IMPL(restore) {
    auto server = static_cast<Server*>(Local<External>::Cast(args.Data())->Value());
    auto iso = args.GetIsolate();

    HandleScope scope(iso);
    auto ctx = iso->GetCurrentContext();

    auto& engine = server->engine;
    auto& player = server->player;

    if (!engine || !player) {
        logger::error("engine & player required\n");
        return;
    }

    auto jsMode = String::Utf8Value(iso, args[0]);
    auto mode = string(*jsMode);
    
    if (mode != engine->mode()) {
        logger::error("mode mismatch: requested \"%s\" != current \"%s\"\n", mode.c_str(), engine->mode());
        return;
    }

    // Dumb way to reset states
    player->setEngine(nullptr);
    engine->reset();
    player->setEngine(engine);

    auto floatArr = args[1].As<Uint8Array>();
    auto v8buf = floatArr->Buffer();
    auto nodeBuffer = node::Buffer::New(iso, v8buf, 0, v8buf->ByteLength()).ToLocalChecked();
    auto buffer = reinterpret_cast<char*>(node::Buffer::Data(nodeBuffer.As<Object>()));

    logger::debug("Processing %u bytes\n", v8buf->ByteLength());

    bool error;
    Reader r(string_view(buffer, v8buf->ByteLength()), error);

    // Sync bots
    auto botCount = r.read<uint16_t>();
    for (uint16_t j = 0; j < botCount; j++) {
        auto botID = r.read<uint16_t>();
        engine->addBot(botID);
    }

    // Sync pool
    auto pool_size = r.read<size_t>();
    const size_t POOL_BUF_SIZE = engine->poolSize();
    if (pool_size != POOL_BUF_SIZE) logger::debug("Pool size changed (%u -> %u)\n", pool_size, POOL_BUF_SIZE);
    
    // Memory unsafe, need to skip extra bytes
    if (pool_size > POOL_BUF_SIZE) {
        r.read(engine->pool, POOL_BUF_SIZE);
        r.skip(pool_size - POOL_BUF_SIZE);
    // Memory safe
    } else {
        r.read(engine->pool, pool_size);
    }

    engine->syncState();

    if (!error) {
        args.GetReturnValue().Set(Boolean::New(iso, true));
        return;
    }

    // Reset again if error happens
    player->setEngine(nullptr);
    engine->reset();
    player->setEngine(engine);

    logger::error("Restore parsing failed\n");
}