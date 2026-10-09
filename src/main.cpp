#include "engine.h"
#include <exception>
#include <iostream>

int main() try {
    vpt::engine e;
    e.start();
    return 0;
} catch (const std::exception &err) {
    std::cerr << "Fatal: " << err.what() << std::endl;
    return 1;
}
