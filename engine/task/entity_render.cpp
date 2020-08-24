#include "entity_render.h"
#include "../worker_thread.h"
#include "engine/core.h"
#include "engine/entity/renderable_entity.h"
#include "engine/entity/deformable_entity.h"
#include "engine/entity/particle.h"

#include "engine/game.h"
#include "engine/entity/component/animation_component.h"

namespace lotus
{
    EntityRenderTask::EntityRenderTask(std::shared_ptr<RenderableEntity>& _entity, float _priority) : WorkItem(), entity(_entity)
    {
        priority = _priority;
    }

    void EntityRenderTask::Process(WorkerThread* thread)
    {
        auto image_index = thread->engine->renderer->getCurrentImage();
        updateUniformBuffer(thread, image_index, entity.get());
        if (auto deformable = dynamic_cast<DeformableEntity*>(entity.get()))
        {
            updateAnimationVertices(thread, image_index, deformable);
        }
        if (thread->engine->config->renderer.RasterizationEnabled())
        {
            if (dynamic_cast<Particle*>(entity.get()))
            {
                graphics.particle = *entity->command_buffers[image_index];
            }
            else
            {
                graphics.secondary = *entity->command_buffers[image_index];
                if (!entity->shadowmap_buffers.empty())
                    graphics.shadow = *entity->shadowmap_buffers[image_index];
            }
        }
    }

    void EntityRenderTask::updateUniformBuffer(WorkerThread* thread, int image_index, RenderableEntity* entity)
    {
        RenderableEntity::UniformBufferObject* ubo = reinterpret_cast<RenderableEntity::UniformBufferObject*>(entity->uniform_buffer_mapped + (image_index * thread->engine->renderer->uniform_buffer_align_up(sizeof(RenderableEntity::UniformBufferObject))));
        ubo->model = entity->getModelMatrix();
        ubo->modelIT = glm::transpose(glm::inverse(glm::mat3(ubo->model)));
    }

    void EntityRenderTask::updateAnimationVertices(WorkerThread* thread, int image_index, DeformableEntity* entity)
    {
        auto component = entity->animation_component;
        auto& skeleton = component->skeleton;

        vk::CommandBufferAllocateInfo alloc_info = {};
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandPool = *thread->compute_pool;
        alloc_info.commandBufferCount = 1;

        auto command_buffers = thread->engine->renderer->gpu->device->allocateCommandBuffersUnique<std::allocator<vk::UniqueHandle<vk::CommandBuffer, vk::DispatchLoaderDynamic>>>(alloc_info);
        command_buffer = std::move(command_buffers[0]);

        vk::CommandBufferBeginInfo begin_info = {};
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        command_buffer->begin(begin_info);

        command_buffer->bindPipeline(vk::PipelineBindPoint::eCompute, *thread->engine->renderer->animation_pipeline);

        vk::DescriptorBufferInfo skeleton_buffer_info;
        skeleton_buffer_info.buffer = entity->animation_component->skeleton_bone_buffer->buffer;
        skeleton_buffer_info.offset = sizeof(AnimationComponent::BufferBone) * skeleton->bones.size() * image_index;
        skeleton_buffer_info.range = sizeof(AnimationComponent::BufferBone) * skeleton->bones.size();

        vk::WriteDescriptorSet skeleton_descriptor_set = {};

        skeleton_descriptor_set.dstSet = nullptr;
        skeleton_descriptor_set.dstBinding = 1;
        skeleton_descriptor_set.dstArrayElement = 0;
        skeleton_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
        skeleton_descriptor_set.descriptorCount = 1;
        skeleton_descriptor_set.pBufferInfo = &skeleton_buffer_info;

        command_buffer->pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *thread->engine->renderer->animation_pipeline_layout, 0, skeleton_descriptor_set);

        //transform skeleton with current animation   
        for (size_t i = 0; i < entity->models.size(); ++i)
        {
            for (size_t j = 0; j < entity->models[i]->meshes.size(); ++j)
            {
                auto& mesh = entity->models[i]->meshes[j];
                auto& vertex_buffer = component->transformed_geometries[i].vertex_buffers[j][image_index];

                vk::DescriptorBufferInfo vertex_weights_buffer_info;
                vertex_weights_buffer_info.buffer = mesh->vertex_buffer->buffer;
                vertex_weights_buffer_info.offset = 0;
                vertex_weights_buffer_info.range = VK_WHOLE_SIZE;

                vk::DescriptorBufferInfo vertex_output_buffer_info;
                vertex_output_buffer_info.buffer = vertex_buffer->buffer;
                vertex_output_buffer_info.offset = 0;
                vertex_output_buffer_info.range = VK_WHOLE_SIZE;

                vk::WriteDescriptorSet weight_descriptor_set {};
                weight_descriptor_set.dstSet = nullptr;
                weight_descriptor_set.dstBinding = 0;
                weight_descriptor_set.dstArrayElement = 0;
                weight_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
                weight_descriptor_set.descriptorCount = 1;
                weight_descriptor_set.pBufferInfo = &vertex_weights_buffer_info;

                vk::WriteDescriptorSet output_descriptor_set {};
                output_descriptor_set.dstSet = nullptr;
                output_descriptor_set.dstBinding = 2;
                output_descriptor_set.dstArrayElement = 0;
                output_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
                output_descriptor_set.descriptorCount = 1;
                output_descriptor_set.pBufferInfo = &vertex_output_buffer_info;

                command_buffer->pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *thread->engine->renderer->animation_pipeline_layout, 0, {weight_descriptor_set, output_descriptor_set});

                command_buffer->dispatch(mesh->getVertexCount(), 1, 1);

                vk::BufferMemoryBarrier barrier;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.buffer = vertex_buffer->buffer;
                barrier.size = VK_WHOLE_SIZE;
                barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
                barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR | vk::AccessFlagBits::eAccelerationStructureReadKHR;

                command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, nullptr, barrier, nullptr);
            }
            if (thread->engine->config->renderer.RaytraceEnabled())
            {
                component->transformed_geometries[i].bottom_level_as[image_index]->Update(*command_buffer);
            }
        }
        command_buffer->end();

        compute.primary = *command_buffer;
    }
}
