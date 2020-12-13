#pragma once

#include <memory>
#include <engine/renderer/vulkan/vulkan_inc.h>
#include "memory.h"
#include "material.h"

namespace lotus
{
    class Engine;

    class Mesh
    {
    public:
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;
        ~Mesh() = default;

        std::vector<vk::VertexInputBindingDescription>& getVertexInputBindingDescription()
        {
            return vertex_bindings;
        }

        void setVertexInputBindingDescription(std::vector<vk::VertexInputBindingDescription>&& desc)
        {
            vertex_bindings = desc;
        }

        std::vector<vk::VertexInputAttributeDescription>& getVertexInputAttributeDescription()
        {
            return vertex_attributes;
        }

        void setVertexInputAttributeDescription(std::vector<vk::VertexInputAttributeDescription>&& attrs)
        {
            vertex_attributes = std::move(attrs);
        }

        int getIndexCount() const { return indices; }
        int getVertexCount() const { return vertices; }
        void setIndexCount(int _indices) { indices = _indices; }
        void setVertexCount(int _vertices) { vertices = _vertices; }

        void setVertexBuffer(uint8_t* buffer, size_t len);

        std::unique_ptr<Buffer> vertex_buffer;
        std::unique_ptr<Buffer> index_buffer;
        std::unique_ptr<Buffer> aabbs_buffer;

        std::shared_ptr<Material> material;

        bool has_transparency{ false };
        uint16_t blending{ 0 };

        Mesh() = default;

        vk::Pipeline pipeline;
        vk::Pipeline pipeline_shadow;
    protected:

        std::vector<vk::VertexInputBindingDescription> vertex_bindings;
        std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
        int indices{ 0 };
        int vertices{ 0 };
    };

}
