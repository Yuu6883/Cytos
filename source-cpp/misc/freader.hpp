#include <iostream>
#include <fstream>
#include <string>
#include <string_view>

using std::ifstream;
using std::string;
using std::string_view;

class FileReader {

    std::streampos pos;
    string_view file;
    ifstream stream;
    bool once;

public:

    FileReader(string_view file, bool once = false) : stream(file.data(), std::ios::ate), file(file), once(once) {
        pos = stream.tellg();
        stream.seekg(std::ios::beg);
    };

    ~FileReader() {
        if (stream.is_open()) stream.close();
        if (once) remove(file.data());
    }

    inline size_t size() { return pos; }

    inline bool good() { return stream.good(); }

    inline void close() { stream.close(); }

    inline bool eof() { return stream.eof(); }

    inline void skip(std::streampos bytes) {
        stream.seekg(stream.tellg() + bytes);
    }

    template<typename T>
    inline T read() {
        T t = 0;
        stream.read((char*) &t, sizeof(T));
        return t;
    }

    inline void read(void* out, std::streamsize size) {
        stream.read((char*) out, size);
    }

    template<typename I>
    inline void read(I& out) {
        stream.read((char*) &out, sizeof(I));
    }

    inline string utf8() {
        string out = "";
        while (true) {
            char c = read<char>();
            if (c) {
                out += c;
            } else break;
        }
        return out;
    }
};
