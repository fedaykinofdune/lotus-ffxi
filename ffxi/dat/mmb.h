#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include "dat_chunk.h"
#include "engine/renderer/vulkan/vulkan_inc.h"
#include "engine/renderer/model.h"

namespace FFXI
{
    class MMB : public DatChunk
    {
    public:
        struct Vertex
        {
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec3 color;
            glm::vec2 tex_coord;
            float _pad;

            static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions();
            static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();
        };
        struct Mesh
        {
            char textureName[16];
            uint16_t blending;
            std::vector<Vertex> vertices;
            std::vector<uint16_t> indices;
            vk::PrimitiveTopology topology;
        };
        MMB(char* _name, uint8_t* _buffer, size_t _len, bool offset_vertices);

        static bool DecodeMMB(uint8_t* buffer, size_t max_len);

        char name[16];
        std::vector<Mesh> meshes;
    private:
        
    };

    class MMBLoader : public lotus::ModelLoader
    {
    public:
        explicit MMBLoader(MMB* mmb);
        virtual std::vector<std::unique_ptr<lotus::WorkItem>> LoadModel(std::shared_ptr<lotus::Model>&) override;
    private:
        MMB* mmb;
    };
}
