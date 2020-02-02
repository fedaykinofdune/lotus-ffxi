#include "model_init.h"
#include <utility>
#include "../worker_thread.h"
#include "../core.h"
#include "../../../ffxi/dat/mmb.h"

namespace lotus
{
    ModelInitTask::ModelInitTask(int _image_index, std::shared_ptr<Model> _model, std::vector<std::vector<uint8_t>>&& _vertex_buffers,
        std::vector<std::vector<uint8_t>>&& _index_buffers) : WorkItem(), image_index(_image_index), model(std::move(_model)), vertex_buffers(std::move(_vertex_buffers)), index_buffers(std::move(_index_buffers))
    {
        priority = -1;
    }

    void ModelInitTask::Process(WorkerThread* thread)
    {
        if (!vertex_buffers.empty())
        {
            std::vector<vk::GeometryNV> raytrace_geometry;
            vk::CommandBufferAllocateInfo alloc_info = {};
            alloc_info.level = vk::CommandBufferLevel::ePrimary;
            alloc_info.commandPool = *thread->graphics_pool;
            alloc_info.commandBufferCount = 1;

            auto command_buffers = thread->engine->renderer.device->allocateCommandBuffersUnique<std::allocator<vk::UniqueHandle<vk::CommandBuffer, vk::DispatchLoaderDynamic>>>(alloc_info, thread->engine->renderer.dispatch);

            vk::DeviceSize staging_buffer_size = 0;
            for (int i = 0; i < vertex_buffers.size(); ++i)
            {
                auto& vertex_buffer = vertex_buffers[i];
                auto& index_buffer = index_buffers[i];
                staging_buffer_size += vertex_buffer.size() + index_buffer.size();
            }

            staging_buffer = thread->engine->renderer.memory_manager->GetBuffer(staging_buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            uint8_t* staging_buffer_data = static_cast<uint8_t*>(staging_buffer->map(0, staging_buffer_size, {}));

            vk::DeviceSize staging_buffer_offset = 0;

            command_buffer = std::move(command_buffers[0]);

            vk::CommandBufferBeginInfo begin_info = {};
            begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

            command_buffer->begin(begin_info, thread->engine->renderer.dispatch);

            for (int i = 0; i < vertex_buffers.size(); ++i)
            {
                auto& vertex_buffer = vertex_buffers[i];
                auto& index_buffer = index_buffers[i];
                auto& mesh = model->meshes[i];

                memcpy(staging_buffer_data + staging_buffer_offset, vertex_buffer.data(), vertex_buffer.size());
                memcpy(staging_buffer_data + staging_buffer_offset + vertex_buffer.size(), index_buffer.data(), index_buffer.size());

                std::array<vk::BufferMemoryBarrier, 2> barriers;
                barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[0].buffer = mesh->vertex_buffer->buffer;
                barriers[0].size = VK_WHOLE_SIZE;
                barriers[0].srcAccessMask = {};
                barriers[0].dstAccessMask = vk::AccessFlagBits::eTransferWrite;

                barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[1].buffer = mesh->index_buffer->buffer;
                barriers[1].size = VK_WHOLE_SIZE;
                barriers[1].srcAccessMask = {};
                barriers[1].dstAccessMask = vk::AccessFlagBits::eTransferWrite;

                command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, barriers, nullptr, thread->engine->renderer.dispatch);

                vk::BufferCopy copy_region;
                copy_region.srcOffset = staging_buffer_offset;
                copy_region.size = vertex_buffer.size();
                command_buffer->copyBuffer(staging_buffer->buffer, mesh->vertex_buffer->buffer, copy_region, thread->engine->renderer.dispatch);
                copy_region.size = index_buffer.size();
                copy_region.srcOffset = vertex_buffer.size() + staging_buffer_offset;
                command_buffer->copyBuffer(staging_buffer->buffer, mesh->index_buffer->buffer, copy_region, thread->engine->renderer.dispatch);

                if (thread->engine->renderer.RTXEnabled() && !model->weighted)
                {
                    auto& geo = raytrace_geometry.emplace_back();
                    geo.geometryType = vk::GeometryTypeNV::eTriangles;
                    geo.geometry.triangles.vertexData = mesh->vertex_buffer->buffer;
                    geo.geometry.triangles.vertexOffset = 0;
                    geo.geometry.triangles.vertexCount = static_cast<uint32_t>(vertex_buffer.size() / sizeof(FFXI::MMB::Vertex));
                    geo.geometry.triangles.vertexStride = sizeof(FFXI::MMB::Vertex);
                    geo.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;

                    geo.geometry.triangles.indexData = mesh->index_buffer->buffer;
                    geo.geometry.triangles.indexOffset = 0;
                    geo.geometry.triangles.indexCount = static_cast<uint32_t>(index_buffer.size() / sizeof(uint16_t));
                    geo.geometry.triangles.indexType = vk::IndexType::eUint16;
                    if (!mesh->has_transparency)
                    {
                        geo.flags = vk::GeometryFlagBitsNV::eOpaque;
                    }
                }

                staging_buffer_offset += vertex_buffer.size() + index_buffer.size();
            }
            staging_buffer->unmap();

            if (thread->engine->renderer.RTXEnabled() && !model->weighted)
            {
                vk::MemoryBarrier barrier;
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV;
                command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildNV, {}, barrier, nullptr, nullptr, thread->engine->renderer.dispatch);
                model->bottom_level_as = std::make_unique<BottomLevelAccelerationStructure>(thread->engine, *command_buffer, raytrace_geometry, false, model->lifetime == Lifetime::Long, BottomLevelAccelerationStructure::Performance::FastTrace);

                if (model->lifetime == Lifetime::Long)
                {
                    std::vector<vk::DescriptorBufferInfo> descriptor_vertex_info;
                    std::vector<vk::DescriptorBufferInfo> descriptor_index_info;
                    std::vector<vk::DescriptorImageInfo> descriptor_texture_info;
                    //TODO: move these into some kind of thread-safe implementation
                    std::lock_guard lg{ thread->engine->renderer.acceleration_binding_mutex };
                    uint16_t index = thread->engine->renderer.static_acceleration_bindings_offset;
                    for (size_t i = 0; i < model->meshes.size(); ++i)
                    {
                        auto& mesh = model->meshes[i];
                        descriptor_vertex_info.emplace_back(mesh->vertex_buffer->buffer, 0, VK_WHOLE_SIZE);
                        descriptor_index_info.emplace_back(mesh->index_buffer->buffer, 0, VK_WHOLE_SIZE);
                        descriptor_texture_info.emplace_back(*mesh->texture->sampler, *mesh->texture->image_view, vk::ImageLayout::eShaderReadOnlyOptimal);
                        for (int image = 0; image < thread->engine->renderer.getImageCount(); ++image)
                        {
                            thread->engine->renderer.mesh_info_buffer_mapped[image * Renderer::max_acceleration_binding_index + index + i] = { index + (uint32_t)i, index + (uint32_t)i, mesh->specular_exponent, mesh->specular_intensity };
                        }
                    }
                    model->bottom_level_as->resource_index = index;

                    thread->engine->renderer.static_acceleration_bindings_offset = index + model->meshes.size();
                    vk::WriteDescriptorSet write_info_vertex;
                    write_info_vertex.descriptorType = vk::DescriptorType::eStorageBuffer;
                    write_info_vertex.dstArrayElement = index;
                    write_info_vertex.dstBinding = 1;
                    write_info_vertex.descriptorCount = static_cast<uint32_t>(descriptor_vertex_info.size());
                    write_info_vertex.pBufferInfo = descriptor_vertex_info.data();

                    vk::WriteDescriptorSet write_info_index;
                    write_info_index.descriptorType = vk::DescriptorType::eStorageBuffer;
                    write_info_index.dstArrayElement = index;
                    write_info_index.dstBinding = 2;
                    write_info_index.descriptorCount = static_cast<uint32_t>(descriptor_index_info.size());
                    write_info_index.pBufferInfo = descriptor_index_info.data();

                    vk::WriteDescriptorSet write_info_texture;
                    write_info_texture.descriptorType = vk::DescriptorType::eCombinedImageSampler;
                    write_info_texture.dstArrayElement = index;
                    write_info_texture.dstBinding = 3;
                    write_info_texture.descriptorCount = static_cast<uint32_t>(descriptor_texture_info.size());
                    write_info_texture.pImageInfo = descriptor_texture_info.data();

                    std::vector<vk::WriteDescriptorSet> writes;
                    for (size_t i = 0; i < thread->engine->renderer.getImageCount(); ++i)
                    {
                        write_info_vertex.dstSet = *thread->engine->renderer.rtx_descriptor_sets_const[i];
                        write_info_index.dstSet = *thread->engine->renderer.rtx_descriptor_sets_const[i];
                        write_info_texture.dstSet = *thread->engine->renderer.rtx_descriptor_sets_const[i];
                        writes.push_back(write_info_vertex);
                        writes.push_back(write_info_index);
                        writes.push_back(write_info_texture);
                    }
                    thread->engine->renderer.device->updateDescriptorSets(writes, nullptr, thread->engine->renderer.dispatch);
                }
            }
            command_buffer->end(thread->engine->renderer.dispatch);

            graphics.primary = *command_buffer;
        }
    }
}
