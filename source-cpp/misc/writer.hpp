#pragma once

#include <thread>
#include <memory.h>
#include <string_view>

using std::string_view;

static inline std::unique_ptr<char[]> char_pool(nullptr);

class Writer {
    char* ptr;
public:
    Writer() {
        if (!char_pool.get()) char_pool.reset(new char[10 * 1024 * 1024]);
        ptr = char_pool.get();

        // printf("Thread ID = %i\n", std::this_thread::get_id());
        // printf("pool = 0x%p, &pool = 0x%p\n", char_pool.get(), &char_pool);
    };

    template<typename I>
    I& ref(I init = 0) {
        I& r = * ((I*) ptr);
        r = 0;
        ptr += sizeof(I);
        return r;
    }

    template<typename I, typename O>
    void write(O&& input) {
        * ((I*) ptr) = (I) input;
        ptr += sizeof(I);
    }

    void write(string_view buffer, bool padZero = true, bool asUTF16 = false) {
        if (asUTF16) {
            auto p = buffer.data();
            for (int i = 0; i < buffer.length(); i++)
                write<char16_t>(p[i]);
            if (padZero) write<char16_t>(0);
        } else {
            memcpy(ptr, buffer.data(), buffer.size());
            ptr += buffer.size();
            if (padZero) write<uint8_t>(0);
        }
    }

    void fill(uint8_t v, size_t size) {
        memset(ptr, v, size);
        ptr += size;
    }

    string_view finalize() {
        // printf("Thread ID = %i\n", std::this_thread::get_id());
        // printf("pool = 0x%p, &pool = 0x%p\n", char_pool.get(), &char_pool);

        size_t s = ptr - char_pool.get();
        auto out = static_cast<char*>(malloc(s));
        memcpy(out, char_pool.get(), s);
        ptr = char_pool.get();

        return string_view(out, s);
    }

    string_view buffer() {
        return string_view(char_pool.get(), ptr - char_pool.get());
    }
};