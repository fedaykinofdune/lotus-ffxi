#include "worker_thread.h"
#include "worker_pool.h"
#include "core.h"
#include "renderer/vulkan/gpu.h"

namespace lotus
{
    WorkerThread::WorkerThread(Engine* _engine, WorkerPool* _pool) : pool(_pool), engine(_engine)
    {
        graphics_pool = engine->renderer->gpu->createCommandPool(GPU::QueueType::Graphics);
        compute_pool = engine->renderer->gpu->createCommandPool(GPU::QueueType::Compute);

        std::array<vk::DescriptorPoolSize, 2> poolSizes = {};
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(engine->renderer->getImageCount());
        poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(engine->renderer->getImageCount());

        vk::DescriptorPoolCreateInfo poolInfo = {};
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(engine->renderer->getImageCount());
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

        desc_pool = engine->renderer->gpu->device->createDescriptorPoolUnique(poolInfo);
    }

    bool WorkerThread::Busy() const
    {
        return work && dynamic_cast<BackgroundWork*>(work.get()) == nullptr;
    }

    void WorkerThread::Exit()
    {
        active = false;
    }

    void WorkerThread::Join()
    {
    }
}
