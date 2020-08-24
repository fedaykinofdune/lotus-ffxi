#include "transform_skeleton.h"
#include "engine/core.h"
#include "engine/worker_thread.h"
#include "engine/entity/component/animation_component.h"

namespace lotus
{
    TransformSkeletonTask::TransformSkeletonTask(std::shared_ptr<DeformableEntity> _entity) : entity(_entity)
    {
    }

    void TransformSkeletonTask::Process(WorkerThread* thread)
    {
        vk::CommandBufferAllocateInfo alloc_info;
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandPool = *thread->compute_pool;
        alloc_info.commandBufferCount = 1;

        auto command_buffers = thread->engine->renderer->gpu->device->allocateCommandBuffersUnique(alloc_info);
        vk::CommandBufferBeginInfo begin_info = {};
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        command_buffer = std::move(command_buffers[0]);

        command_buffer->begin(begin_info);

        auto anim_component = entity->animation_component;
        auto skeleton = anim_component->skeleton.get();
        staging_buffer = thread->engine->renderer->gpu->memory_manager->GetBuffer(sizeof(AnimationComponent::BufferBone) * skeleton->bones.size(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        AnimationComponent::BufferBone* buffer = static_cast<AnimationComponent::BufferBone*>(staging_buffer->map(0, VK_WHOLE_SIZE, {}));
        for (size_t i = 0; i < skeleton->bones.size(); ++i)
        {
            buffer[i].trans = skeleton->bones[i].trans;
            buffer[i].scale = skeleton->bones[i].scale;
            buffer[i].rot.x = skeleton->bones[i].rot.x;
            buffer[i].rot.y = skeleton->bones[i].rot.y;
            buffer[i].rot.z = skeleton->bones[i].rot.z;
            buffer[i].rot.w = skeleton->bones[i].rot.w;
        }
        staging_buffer->unmap();

        vk::BufferCopy copy_region;
        copy_region.srcOffset = 0;
        copy_region.dstOffset = sizeof(AnimationComponent::BufferBone) * skeleton->bones.size() * thread->engine->renderer->getCurrentImage();
        copy_region.size = skeleton->bones.size() * sizeof(AnimationComponent::BufferBone);
        command_buffer->copyBuffer(staging_buffer->buffer, anim_component->skeleton_bone_buffer->buffer, copy_region);

        vk::BufferMemoryBarrier barrier;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = anim_component->skeleton_bone_buffer->buffer;
        barrier.offset = sizeof(AnimationComponent::BufferBone) * skeleton->bones.size() * thread->engine->renderer->getCurrentImage();
        barrier.size = sizeof(AnimationComponent::BufferBone) * skeleton->bones.size();
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, barrier, nullptr);
        command_buffer->end();

        compute.primary = *command_buffer;
    }
}
