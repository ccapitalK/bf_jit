#include <fstream>
#include <iostream>
#include <stack>

#include "arguments.hpp"
#include "asmbuf.hpp"
#include "code_generator.hpp"
#include "engine.hpp"
#include "error.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "runtime.hpp"

int main(int argc, char *argv[]) {
    Arguments arguments{argc, argv};
    try {
        switch (arguments.cellBitWidth) {
        case 8: {
            Engine<char> engine(arguments);
            engine.run();
            break;
        }
        case 16: {
            Engine<short> engine(arguments);
            engine.run();
            break;
        }
        case 32: {
            Engine<int> engine(arguments);
            engine.run();
            break;
        }
        }
    } catch (JITError &e) {
        std::cout << "Fatal JITError caught: " << e.what() << '\n';
        return 1;
    }
}
