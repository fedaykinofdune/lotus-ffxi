#pragma once
#include "worker_pool.h"
#include "config.h"
#include "renderer/vulkan/renderer.h"
#include "entity/camera.h"
#include "input.h"
#include "types.h"
#include "light_manager.h"
#include "random.h"

namespace lotus
{
    struct Settings
    {
        std::string app_name;
        uint32_t app_version;
        Renderer::Settings renderer_settings;
    };

    class Game;

    class Engine
    {
    public:
        Engine(Game* game, Settings settings, std::unique_ptr<Config> config);
        ~Engine();

        void run();
        void close() { closing = true; }

        Game* game;
        std::unique_ptr<Config> config;
        Settings settings;
        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<Input> input;
        Camera* camera {nullptr};
        std::unique_ptr<WorkerPool> worker_pool;
        std::unique_ptr<LightManager> lights;

        void set_camera(Camera* _camera) { camera = _camera; }
        time_point getSimulationTime() { return simulation_time; }

    private:
        time_point simulation_time;
        void mainLoop();
        bool closing{ false };
    };
}
