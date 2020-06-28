#include "landscape_entity_init.h"

#include "engine/worker_thread.h"
#include "engine/core.h"
#include "engine/renderer/vulkan/renderer.h"
#include "engine/entity/camera.h"

namespace lotus
{
    LandscapeEntityInitTask::LandscapeEntityInitTask(const std::shared_ptr<LandscapeEntity>& _entity, std::vector<LandscapeEntity::InstanceInfo>&& _instance_info) :
        WorkItem(), entity(_entity), instance_info(_instance_info)
    {
    }

    void LandscapeEntityInitTask::Process(WorkerThread* thread)
    {
        entity->uniform_buffer = thread->engine->renderer.gpu->memory_manager->GetBuffer(thread->engine->renderer.uniform_buffer_align_up(sizeof(RenderableEntity::UniformBufferObject)) * thread->engine->renderer.getImageCount(), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        entity->mesh_index_buffer = thread->engine->renderer.gpu->memory_manager->GetBuffer(thread->engine->renderer.uniform_buffer_align_up(sizeof(uint32_t)) * thread->engine->renderer.getImageCount(), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        entity->uniform_buffer_mapped = static_cast<uint8_t*>(entity->uniform_buffer->map(0, thread->engine->renderer.uniform_buffer_align_up(sizeof(RenderableEntity::UniformBufferObject)) * thread->engine->renderer.getImageCount(), {}));
        entity->mesh_index_buffer_mapped = static_cast<uint8_t*>(entity->mesh_index_buffer->map(0, thread->engine->renderer.uniform_buffer_align_up(sizeof(uint32_t)) * thread->engine->renderer.getImageCount(), {}));

        populateInstanceBuffer(thread);
        createCommandBuffers(thread);
    }

    void LandscapeEntityInitTask::createCommandBuffers(WorkerThread* thread)
    {
        vk::CommandBufferAllocateInfo alloc_info;
        alloc_info.level = vk::CommandBufferLevel::eSecondary;
        alloc_info.commandPool = *thread->graphics_pool;
        alloc_info.commandBufferCount = static_cast<uint32_t>(thread->engine->renderer.getImageCount());

        entity->command_buffers = thread->engine->renderer.gpu->device->allocateCommandBuffersUnique<std::allocator<vk::UniqueHandle<vk::CommandBuffer, vk::DispatchLoaderDynamic>>>(alloc_info);
        entity->shadowmap_buffers = thread->engine->renderer.gpu->device->allocateCommandBuffersUnique<std::allocator<vk::UniqueHandle<vk::CommandBuffer, vk::DispatchLoaderDynamic>>>(alloc_info);

        if (thread->engine->config->renderer.RasterizationEnabled())
        {
            for (int i = 0; i < entity->command_buffers.size(); ++i)
            {
                auto& command_buffer = entity->command_buffers[i];
                vk::CommandBufferInheritanceInfo inheritInfo = {};
                inheritInfo.renderPass = *thread->engine->renderer.gbuffer_render_pass;
                inheritInfo.framebuffer = *thread->engine->renderer.gbuffer.frame_buffer;

                vk::CommandBufferBeginInfo beginInfo = {};
                beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse | vk::CommandBufferUsageFlagBits::eRenderPassContinue;
                beginInfo.pInheritanceInfo = &inheritInfo;

                command_buffer->begin(beginInfo);

                command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *thread->engine->renderer.landscape_pipeline_group.graphics_pipeline);

                vk::DescriptorBufferInfo buffer_info;
                buffer_info.buffer = thread->engine->renderer.camera_buffers.view_proj_ubo->buffer;
                buffer_info.offset = i * thread->engine->renderer.uniform_buffer_align_up(sizeof(Camera::CameraData));
                buffer_info.range = sizeof(Camera::CameraData);

                vk::DescriptorBufferInfo mesh_info;
                mesh_info.buffer = thread->engine->renderer.mesh_info_buffer->buffer;
                mesh_info.offset = sizeof(Renderer::MeshInfo) * Renderer::max_acceleration_binding_index * i;
                mesh_info.range = sizeof(Renderer::MeshInfo) * Renderer::max_acceleration_binding_index;

                std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {};

                descriptorWrites[0].dstSet = nullptr;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = &buffer_info;

                descriptorWrites[1].dstSet = nullptr;
                descriptorWrites[1].dstBinding = 3;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = vk::DescriptorType::eUniformBuffer;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pBufferInfo = &mesh_info;

                command_buffer->pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *thread->engine->renderer.pipeline_layout, 0, descriptorWrites);

                drawModel(thread, *command_buffer, false, *thread->engine->renderer.pipeline_layout);

                command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *thread->engine->renderer.landscape_pipeline_group.blended_graphics_pipeline);

                drawModel(thread, *command_buffer, true, *thread->engine->renderer.pipeline_layout);

                command_buffer->end();
            }
        }

        if (thread->engine->config->renderer.render_mode == Config::Renderer::RenderMode::Rasterization)
        {
            for (size_t i = 0; i < entity->shadowmap_buffers.size(); ++i)
            {
                auto& command_buffer = entity->shadowmap_buffers[i];
                vk::CommandBufferInheritanceInfo inheritInfo = {};
                inheritInfo.renderPass = *thread->engine->renderer.shadowmap_render_pass;
                //used in multiple framebuffers so we can't give the hint
                //inheritInfo.framebuffer = *thread->engine->renderer.shadowmap_frame_buffer;

                vk::CommandBufferBeginInfo beginInfo = {};
                beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse | vk::CommandBufferUsageFlagBits::eRenderPassContinue;
                beginInfo.pInheritanceInfo = &inheritInfo;

                command_buffer->begin(beginInfo);

                vk::DescriptorBufferInfo buffer_info;
                buffer_info.buffer = entity->uniform_buffer->buffer;
                buffer_info.offset = i * thread->engine->renderer.uniform_buffer_align_up(sizeof(RenderableEntity::UniformBufferObject));
                buffer_info.range = sizeof(RenderableEntity::UniformBufferObject);

                vk::DescriptorBufferInfo cascade_buffer_info;
                cascade_buffer_info.buffer = thread->engine->renderer.camera_buffers.cascade_data_ubo->buffer;
                cascade_buffer_info.offset = i * thread->engine->renderer.uniform_buffer_align_up(sizeof(thread->engine->camera->cascade_data));
                cascade_buffer_info.range = sizeof(thread->engine->camera->cascade_data);

                std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {};

                descriptorWrites[0].dstSet = nullptr;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = &buffer_info;

                descriptorWrites[1].dstSet = nullptr;
                descriptorWrites[1].dstBinding = 3;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = vk::DescriptorType::eUniformBuffer;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pBufferInfo = &cascade_buffer_info;

                command_buffer->pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, *thread->engine->renderer.shadowmap_pipeline_layout, 0, descriptorWrites);

                command_buffer->setDepthBias(1.25f, 0, 1.75f);

                command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *thread->engine->renderer.landscape_pipeline_group.shadowmap_pipeline);
                drawModel(thread, *command_buffer, false, *thread->engine->renderer.shadowmap_pipeline_layout);
                command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *thread->engine->renderer.landscape_pipeline_group.blended_shadowmap_pipeline);
                drawModel(thread, *command_buffer, true, *thread->engine->renderer.shadowmap_pipeline_layout);

                command_buffer->end();
            }
        }
    }

    void LandscapeEntityInitTask::drawModel(WorkerThread* thread, vk::CommandBuffer command_buffer, bool transparency, vk::PipelineLayout layout)
    {
        for (const auto& model : entity->models)
        {
            auto [offset, count] = entity->instance_offsets[model->name];
            if (count > 0 && !model->meshes.empty())
            {
                command_buffer.bindVertexBuffers(1, entity->instance_buffer->buffer, offset * sizeof(LandscapeEntity::InstanceInfo));
                uint32_t material_index = 1;
                for (size_t i = 0; i < model->meshes.size(); ++i)
                {
                    auto& mesh = model->meshes[i];
                    if (mesh->has_transparency == transparency)
                    {
                        if (model->bottom_level_as)
                        {
                            material_index = model->bottom_level_as->resource_index + i;
                        }
                        drawMesh(thread, command_buffer, *mesh, count, layout, material_index);
                    }
                }
            }
        }
    }

    void LandscapeEntityInitTask::drawMesh(WorkerThread* thread, vk::CommandBuffer command_buffer, const Mesh& mesh, uint32_t count, vk::PipelineLayout layout, uint32_t material_index)
    {
        vk::DescriptorImageInfo image_info;
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        //TODO: debug texture? probably AYAYA
        if (mesh.texture)
        {
            image_info.imageView = *mesh.texture->image_view;
            image_info.sampler = *mesh.texture->sampler;
        }

        std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {};

        descriptorWrites[0].dstSet = nullptr;
        descriptorWrites[0].dstBinding = 1;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &image_info;

        command_buffer.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, layout, 0, descriptorWrites);

        command_buffer.pushConstants<uint32_t>(layout, vk::ShaderStageFlagBits::eFragment, 0, material_index);

        command_buffer.bindVertexBuffers(0, mesh.vertex_buffer->buffer, {0});
        command_buffer.bindIndexBuffer(mesh.index_buffer->buffer, {0}, vk::IndexType::eUint16);

        command_buffer.drawIndexed(mesh.getIndexCount(), count, 0, 0, 0);
    }

    void LandscapeEntityInitTask::populateInstanceBuffer(WorkerThread* thread)
    {
        vk::DeviceSize buffer_size = sizeof(LandscapeEntity::InstanceInfo) * instance_info.size();

        staging_buffer = thread->engine->renderer.gpu->memory_manager->GetBuffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* data = staging_buffer->map(0, buffer_size, {});
        memcpy(data, instance_info.data(), buffer_size);
        staging_buffer->unmap();

        entity->instance_info = std::move(instance_info);

        vk::CommandBufferAllocateInfo alloc_info = {};
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandPool = *thread->graphics_pool;
        alloc_info.commandBufferCount = 1;

        auto command_buffers = thread->engine->renderer.gpu->device->allocateCommandBuffersUnique<std::allocator<vk::UniqueHandle<vk::CommandBuffer, vk::DispatchLoaderDynamic>>>(alloc_info);
        command_buffer = std::move(command_buffers[0]);

        vk::CommandBufferBeginInfo begin_info = {};
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        command_buffer->begin(begin_info);

        vk::BufferMemoryBarrier barrier;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = entity->instance_buffer->buffer;
        barrier.size = VK_WHOLE_SIZE;
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, barrier, nullptr);

        vk::BufferCopy copy_region = {};
        copy_region.size = buffer_size;
        command_buffer->copyBuffer(staging_buffer->buffer, entity->instance_buffer->buffer, copy_region);

        command_buffer->end();

        graphics.primary = *command_buffer;
    }

    LandscapeEntityReInitTask::LandscapeEntityReInitTask(const std::shared_ptr<LandscapeEntity>& entity) : LandscapeEntityInitTask(entity, {})
    {
    }

    void LandscapeEntityReInitTask::Process(WorkerThread* thread)
    {
        createCommandBuffers(thread);
    }
}
