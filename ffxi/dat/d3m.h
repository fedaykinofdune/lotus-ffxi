#pragma once

#include "dat_chunk.h"
#include <atomic>
#include <latch>
#include <lotus/renderer/model.h>

import glm;

namespace FFXI
{
class D3M : public DatChunk
{
public:
    struct Vertex
    {
        glm::vec3 pos;
        float _pad;
        glm::vec3 normal;
        float _pad2;
        glm::vec4 color;
        glm::vec2 uv;
        glm::vec2 _pad3;
    };
    D3M(char* name, uint8_t* buffer, size_t len);

    std::string texture_name;
    uint16_t num_triangles{0};
    std::vector<Vertex> vertex_buffer;
};

class D3A : public DatChunk
{
public:
    D3A(char* name, uint8_t* buffer, size_t len);

    std::string texture_name;
    uint16_t num_quads{0};
    std::vector<D3M::Vertex> vertex_buffer;
};

class D3MLoader
{
public:
    static lotus::Task<> LoadD3M(std::shared_ptr<lotus::Model>, lotus::Engine*, D3M* d3m);
    static lotus::Task<> LoadD3A(std::shared_ptr<lotus::Model>, lotus::Engine*, D3A* d3a);
    static lotus::Task<> LoadModelRing(std::shared_ptr<lotus::Model>, lotus::Engine*, std::vector<D3M::Vertex>&& vertices, std::vector<uint16_t>&& indices);

private:
    static lotus::Task<> InitPipeline(lotus::Engine*);
    static inline vk::Pipeline pipeline_add;
    static inline vk::Pipeline pipeline_blend;
    static inline vk::Pipeline pipeline_sub;
    static inline std::latch pipeline_latch{1};
    static inline std::atomic_flag pipeline_flag;
    static inline std::shared_ptr<lotus::Texture> blank_texture;

    static lotus::Task<> LoadModelAABB(std::shared_ptr<lotus::Model>, lotus::Engine*, std::vector<D3M::Vertex>& vertices, std::vector<uint16_t>& indices,
                                       std::shared_ptr<lotus::Texture>, uint16_t sprite_count);
    static lotus::Task<> LoadModelTriangle(std::shared_ptr<lotus::Model>, lotus::Engine*, std::vector<D3M::Vertex>& vertices, std::vector<uint16_t>& indices,
                                           std::shared_ptr<lotus::Texture>, uint16_t sprite_count);

    class BlankTextureLoader
    {
    public:
        static lotus::Task<> LoadTexture(std::shared_ptr<lotus::Texture>& texture, lotus::Engine* engine);
    };
};
} // namespace FFXI
