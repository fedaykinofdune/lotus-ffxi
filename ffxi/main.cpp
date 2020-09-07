#include <SDL2/SDL.h>

#include "engine/core.h"
#include "engine/game.h"
#include "engine/entity/renderable_entity.h"
#include "engine/task/renderable_entity_init.h"
#include "engine/scene.h"
#include "entity/landscape_entity.h"
#include "entity/actor.h"
#include "test_loader.h"
#include "dat/mmb.h"
#include "dat/os2.h"
#include "dat/d3m.h"
#include "engine/entity/free_flying_camera.h"
#include "engine/entity/component/camera_cascades_component.h"
#include "entity/component/third_person_ffxi_entity_input.h"
#include "entity/third_person_ffxi_camera.h"
#include "config.h"

#include "dat/dat_parser.h"
#include "particle_tester.h"

#include <iostream>

class Game : public lotus::Game
{
public:
    Game(const lotus::Settings& settings) : lotus::Game(settings, std::make_unique<FFXIConfig>())
    {
        scene = std::make_unique<lotus::Scene>(engine.get());
        loading_scene = std::make_unique<lotus::Scene>(engine.get());
        auto [default_texture_ptr, work] = lotus::Texture::LoadTexture<TestTextureLoader>(engine.get(), "default");
        default_texture = default_texture_ptr;
        engine->worker_pool->addForegroundWork(work);
        auto path = static_cast<FFXIConfig*>(engine->config.get())->ffxi.ffxi_install_path;
        /* zone dats vtable:
        (i < 256 ? i + 100  : i + 83635) // Model
        (i < 256 ? i + 5820 : i + 84735) // Dialog
        (i < 256 ? i + 6420 : i + 85335) // Actor
        (i < 256 ? i + 6720 : i + 86235) // Event
        */
        //someday, when ranges works better, we can join these and send a view to addWork
        auto [landscape, landscape_work] = loading_scene->AddEntity<FFXILandscapeEntity>(path / "ROM/342/73.DAT");
        //costumeid 3111 (arciela 3074)
        //auto player = scene->AddEntity<Actor>(path / "/ROM/310/3.DAT");
        auto [player, player_work] = loading_scene->AddEntity<Actor>(path / "ROM/309/105.DAT");
        player->setPos(glm::vec3(259.f, -87.f, 99.f));
        auto [camera, camera_work] = loading_scene->AddEntity<ThirdPersonFFXICamera>(std::weak_ptr<lotus::Entity>(player));
        if (engine->config->renderer.render_mode == lotus::Config::Renderer::RenderMode::Rasterization)
        {
            camera->addComponent<lotus::CameraCascadesComponent>();
        }
        engine->set_camera(camera.get());
        player->addComponent<ThirdPersonEntityFFXIInputComponent>(engine->input.get());
        player->addComponent<ParticleTester>(engine->input.get());

        std::ranges::move(landscape_work, std::back_inserter(player_work));
        std::ranges::move(camera_work, std::back_inserter(player_work));

        engine->worker_pool->addBackgroundWork(player_work, [this](lotus::Engine*)
        {
            engine->game->scene = std::move(loading_scene);
        });

        engine->lights->light.diffuse_dir = glm::normalize(-glm::vec3{ -25.f, -100.f, -50.f });
        engine->camera->setPerspective(glm::radians(70.f), engine->renderer->swapchain->extent.width / (float)engine->renderer->swapchain->extent.height, 0.01f, 1000.f);
        //engine->camera->setPos(glm::vec3(259.f, -90.f, 82.f));
    }
    virtual void tick(lotus::time_point, lotus::duration) override
    {
    }
    std::shared_ptr<lotus::Texture> default_texture;
    std::unique_ptr<lotus::Scene> loading_scene;
};

int main(int argc, char* argv[]) {

    lotus::Settings settings;
    settings.app_name = "core-test";
    settings.app_version = VK_MAKE_VERSION(1, 0, 0);
    settings.renderer_settings.landscape_vertex_input_attribute_descriptions = FFXI::MMB::Vertex::getAttributeDescriptions();
    settings.renderer_settings.landscape_vertex_input_binding_descriptions = FFXI::MMB::Vertex::getBindingDescriptions();
    settings.renderer_settings.model_vertex_input_attribute_descriptions = FFXI::OS2::Vertex::getAttributeDescriptions();
    settings.renderer_settings.model_vertex_input_binding_descriptions = FFXI::OS2::Vertex::getBindingDescriptions();
    settings.renderer_settings.particle_vertex_input_attribute_descriptions = FFXI::D3M::Vertex::getAttributeDescriptions();
    settings.renderer_settings.particle_vertex_input_binding_descriptions = FFXI::D3M::Vertex::getBindingDescriptions();
    Game game{settings};

    game.run();

    return EXIT_SUCCESS;
}
