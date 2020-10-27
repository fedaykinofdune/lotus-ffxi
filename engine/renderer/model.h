#pragma once

#include "engine/renderer/mesh.h"
#include "acceleration_structure.h"
#include "engine/types.h"
#include "engine/task.h"

namespace lotus
{
    class Model
    {
    public:
        //TODO: when to clean up dead weak_ptrs?
        //TODO: figure out how to get engine out of this call
        template<typename ModelLoader, typename... Args>
        [[nodiscard("Work must be awaited before being used")]]
        static std::pair<std::shared_ptr<Model>, std::optional<Task<>>> LoadModel(Engine* engine, const std::string& modelname, Args... args)
        {
            if (!modelname.empty())
            {
                if (auto found = model_map.find(modelname); found != model_map.end())
                {
                    auto ptr = found->second.lock();
                    if (ptr)
                        return {ptr, std::optional<Task<>>{}};
                }
            }
            auto new_model = std::shared_ptr<Model>(new Model(modelname));
            ModelLoader loader{args...};
            loader.setEngine(engine);
            auto task = loader.LoadModel(new_model);
            if (!modelname.empty())
            {
                return {model_map.emplace(modelname, new_model).first->second.lock(), std::move(task)};
            }
            else
            {
                return {new_model, std::move(task)};
            }
        }

        static std::shared_ptr<Model> getModel(const std::string& modelname)
        {
            if (auto found = model_map.find(modelname); found != model_map.end())
            {
                return found->second.lock();
            }
            return {};
        }

        template<typename T>
        static void forEachModel(T func)
        {
            for (const auto& [name, model] : model_map)
            {
                if (auto ptr = model.lock())
                {
                    func(ptr);
                }
            }
        }

        [[nodiscard]]
        WorkerTask<> InitWork(Engine* engine, std::vector<std::vector<uint8_t>>&& vertex_buffers, std::vector<std::vector<uint8_t>>&& index_buffers, uint32_t vertex_stride);

        [[nodiscard]]
        WorkerTask<> InitWork(Engine* engine, std::vector<uint8_t>&& vertex_buffer, uint32_t vertex_stride, float aabb_dist);

        std::string name;
        std::vector<std::unique_ptr<Mesh>> meshes;
        bool weighted{ false };
        Lifetime lifetime {Lifetime::Short};
        bool rendered{ true };
        uint32_t light_offset{ 0 };

        std::unique_ptr<BottomLevelAccelerationStructure> bottom_level_as;

    protected:
        explicit Model(const std::string& name);

        inline static std::unordered_map<std::string, std::weak_ptr<Model>> model_map{};
    };

    class ModelLoader
    {
    public:
        ModelLoader() {}
        void setEngine(Engine* _engine) { engine = _engine; }
        virtual ~ModelLoader() = default;
        virtual Task<> LoadModel(std::shared_ptr<Model>&) = 0;
    protected:
        Engine* engine {nullptr};
    };
}
