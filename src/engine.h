#ifndef VULKAN_PATHTRACER_ENGINE_H
#define VULKAN_PATHTRACER_ENGINE_H

class engine {
public:
    engine() = default;
    // ~engine();

    void start();
private:
    bool running = true;
};

#endif //VULKAN_PATHTRACER_ENGINE_H
