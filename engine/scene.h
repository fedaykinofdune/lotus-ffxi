#pragma once
#include "entity/entity.h"
#include <memory>
#include <vector>
#include "task.h"
#include "engine/core.h"
#include "engine/worker_pool.h"
#include "renderer/acceleration_structure.h"

namespace lotus
{
    class Engine;

    class Scene
    {
    public:
        explicit Scene(Engine* _engine);
        Task<> render();
        void tick_all(time_point time, duration delta);

        template <typename T, typename... Args>
        [[nodiscard("Work must be awaited to be processed")]]
        Task<std::shared_ptr<T>> AddEntity(Args... args)
        {
            auto sp = co_await T::Init(engine, args...);
            new_entities.emplace_back(sp);
            co_return sp;
        }
        template<typename F>
        void forEachEntity(F func)
        {
            for(auto& entity : entities)
            {
                func(entity);
            }
        }
        std::vector<std::shared_ptr<TopLevelAccelerationStructure>> top_level_as;
    protected:
        virtual void tick(time_point time, duration delta) {}

        Engine* engine;
        std::vector<std::shared_ptr<Entity>> entities;
        std::vector<std::shared_ptr<Entity>> new_entities;
    };
}
