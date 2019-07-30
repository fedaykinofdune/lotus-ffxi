#pragma once
#include "core/work_item.h"
#include "core/entity/landscape_entity.h"

namespace lotus
{
    class LandscapeEntityInitTask : public WorkItem
    {
    public:
        LandscapeEntityInitTask(const std::shared_ptr<LandscapeEntity>& entity, std::vector<LandscapeEntity::InstanceInfo>&& instance_info);
        virtual void Process(WorkerThread*) override;
    private:
        void drawModel(WorkerThread* thread, vk::CommandBuffer buffer, vk::Buffer uniform_buffer);
        void drawMesh(WorkerThread* thread, vk::CommandBuffer buffer, const Mesh& model, vk::Buffer uniform_buffer, vk::DeviceSize count);
        void populateInstanceBuffer(WorkerThread* thread);
        std::shared_ptr<LandscapeEntity> entity;
        std::vector<LandscapeEntity::InstanceInfo> instance_info;
        std::unique_ptr<Buffer> staging_buffer;
        vk::UniqueHandle<vk::CommandBuffer, vk::DispatchLoaderDynamic> command_buffer;
    };
}
