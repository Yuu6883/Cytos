#pragma once

#include <stdio.h>

#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <locale>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#define NOMINMAX
#include <uv.h>

#define MS_TO_NANO 1000000
#define MS_TO_NANO_F 1000000.f
#define SECOND_TO_NANO 1000000000

#define randomZeroToOne ((double)rand() / (RAND_MAX))

using std::string;
using std::string_view;
using std::vector;

static inline uint64_t hrtime() {
    using namespace std::chrono;

    static const high_resolution_clock::time_point time_origin =
        high_resolution_clock::now();

    return duration_cast<nanoseconds>(high_resolution_clock::now() -
                                      time_origin)
        .count();
}

static inline float time_func(uint64_t nano, uint64_t& out) {
    out = hrtime();
    if (out >= nano)
        return (out - nano) / MS_TO_NANO_F;
    else
        return 0;
}

template <typename... Args>
string string_format(string_view format, Args... args) {
    size_t size = snprintf(nullptr, 0, format.data(), args...) +
                  1;  // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.data(), args...);
    return string(buf.get(),
                  buf.get() + size - 1);  // We don't want the '\0' inside
}

// trim from start (in place)
inline string ltrim(string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    [](int ch) { return !std::isspace(ch); }));
    return s;
}

// trim from end (in place)
inline string rtrim(string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](int ch) { return !std::isspace(ch); })
                .base(),
            s.end());
    return s;
}

// trim from both ends (in place)
inline string trim(string s) { return rtrim(ltrim(s)); }

inline std::vector<std::string_view> splitString(string_view strv,
                                                 string_view delims = " ") {
    vector<string_view> output;
    auto first = strv.begin();

    while (first != strv.end()) {
        const auto second = std::find_first_of(
            first, std::cend(strv), std::cbegin(delims), std::cend(delims));
        if (first != second) {
            output.emplace_back(strv.substr(std::distance(strv.begin(), first),
                                            std::distance(first, second)));
        }
        if (second == strv.end()) break;
        first = std::next(second);
    }

    return output;
}

inline string hexDump(unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex;
    for (size_t i = 0; i < len; i++)
        ss << std::setw(2) << std::setfill('0') << (int)data[i];
    return ss.str();
}

inline string getExecutableName() {
    std::string sp;
    std::ifstream("/proc/self/comm") >> sp;
    return sp;
}
