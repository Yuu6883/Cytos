#include <iostream>
#include <fstream>
#include <string>
#include <string_view>

using std::ofstream;
using std::string;
using std::string_view;

class FileWriter {
    ofstream stream;
public:
    FileWriter(string_view file) : stream(file.data()) {};
    ~FileWriter() { 
        stream.flush();
        stream.close(); 
    }

    template<typename O>
    inline void write(O& out) {
        stream.write((const char*) &out, sizeof(O));
    }

    template<typename O>
    inline void write(O&& out) {
        stream.write((const char*) &out, sizeof(O));
    }

    inline void utf8(string_view string) {
        buffer(string);
        write<char>(0);
    }

    inline void buffer(string_view buf) {
        stream.write(buf.data(), buf.size());
    }
};
