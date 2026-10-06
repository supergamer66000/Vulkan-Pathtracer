#include "engine.h"

#include <iostream>
#include <chrono>

void engine::start() {
    auto end_time = std::chrono::high_resolution_clock::now();
    while (running) {
        auto start_time = std::chrono::high_resolution_clock::now();
        double deltatime = std::chrono::duration<double>(end_time - start_time).count();
        end_time = std::chrono::high_resolution_clock::now();

        std::cout << deltatime << '\n';
    }
}
