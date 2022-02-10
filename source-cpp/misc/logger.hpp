#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <mutex>

#include <string>
#include <codecvt>
using std::u16string;
using std::u16string_view;

#include "misc.hpp"

#define L_DEBUG   0
#define L_VERBOSE 1
#define L_INFO    2
#define L_WARN    3
#define L_ERROR   4
#define L_NOTHING 5

#ifndef LOG_LEVEL
    #define LOG_LEVEL L_INFO
#endif

using std::string_view;

namespace logger {

    inline std::mutex m;
    inline bool color = true;

    template<typename ... Args>
    static inline void debug(string_view message, Args ... args) {
#if LOG_LEVEL <= L_DEBUG
#ifndef SINGLE_THREAD
        std::lock_guard lock(m);
#endif
        if (color) {
            std::cerr << "\r[\033[92mD\033[0m] " << string_format(message, args...);
        } else {
            std::cerr << "[D] " << string_format(message, args...);
        }
#endif
    }

    template<typename ... Args>
    static inline void verbose(string_view message, Args ... args) {
#if LOG_LEVEL <= L_VERBOSE
#ifndef SINGLE_THREAD
        std::lock_guard lock(m);
#endif
        if (color) {
            std::cout << "\r[\033[95mV\033[0m] " << string_format(message, args...);
        } else {
            std::cout << "[V] " << string_format(message, args...);
        }
        std::cout.flush();
#endif
    }

    template<typename ... Args>
    static inline void info(string_view message, Args ... args) {
#if LOG_LEVEL <= L_INFO
#ifndef SINGLE_THREAD
        std::lock_guard lock(m);
#endif
        if (color) {
            std::cout << "\r[\033[96mI\033[0m] " << string_format(message, args...);
        } else {
            std::cout << "[I] " << string_format(message, args...);
        }
        std::cout.flush();
#endif
    }

    template<typename ... Args>
    static inline void warn(string_view message, Args ... args) {
#if LOG_LEVEL <= L_WARN
#ifndef SINGLE_THREAD
        std::lock_guard lock(m);
#endif
        if (color) {
            std::cout << "\r[\033[93mW\033[0m] " << string_format(message, args...);
        } else {
            std::cout << "[W] " << string_format(message, args...);
        }
        std::cout.flush();
#endif
    }

    template<typename ... Args>
    static inline void error(string_view message, Args ... args) {
#if LOG_LEVEL <= L_ERROR
#ifndef SINGLE_THREAD
        std::lock_guard lock(m);
#endif
        if (color) {
            std::cerr << "\r[\033[91mE\033[0m] " << string_format(message, args...);
        } else {
            std::cerr << "[E] " << string_format(message, args...);
        }
#endif
    }

    template<typename ... Args>
    static inline void print(string_view message, Args ... args) {
#ifndef SINGLE_THREAD
        std::lock_guard lock(m);
#endif
        std::cout << string_format(message, args...);
    }
//
//    static inline void print(u16string_view message) {
//#ifndef SINGLE_THREAD
//        std::lock_guard lock(m);
//#endif
//        std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
//        std::cout << converter.to_bytes(u16string(message));
//    }
}