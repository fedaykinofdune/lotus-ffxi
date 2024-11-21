#pragma once
#include "dat_chunk.h"
#include <filesystem>
#include <lotus/types.h>
#include <memory>
#include <string>

namespace FFXI
{
class DatLoader;
struct WeatherData
{
    float unk[3]; // always 0?
    // entity light colors
    uint32_t sunlight_diffuse1;
    uint32_t moonlight_diffuse1;
    uint32_t ambient1;
    uint32_t fog1;
    float max_fog_dist1;
    float min_fog_dist1;
    float brightness1;
    float unk2;
    // landscape light colors
    uint32_t sunlight_diffuse2;
    uint32_t moonlight_diffuse2;
    uint32_t ambient2;
    uint32_t fog2;
    float max_fog_dist2;
    float min_fog_dist2;
    float brightness2;
    float unk3;
    uint32_t fog_color;
    float fog_offset;
    float unk4; // always 0?
    float max_far_clip;
    uint32_t unk5; // some type of flags
    float unk6[3];
    uint32_t skybox_colors[8];
    float skybox_values[8];
    float unk7; // always 0?
};

class Weather : public DatChunk
{
public:
    Weather(char* _name, uint8_t* _buffer, size_t _len) : DatChunk(_name, _buffer, _len)
    {
        data = reinterpret_cast<WeatherData*>(buffer);
    }
    WeatherData* data;
};

class Dat
{
public:
    Dat(const Dat&) = delete;
    Dat(Dat&&) = default;
    Dat& operator=(const Dat&) = delete;
    Dat& operator=(Dat&&) = default;

    std::unique_ptr<DatChunk> root;

private:
    friend class DatLoader;
    Dat(const std::filesystem::path& filepath);
    std::vector<uint8_t> buffer;
};
} // namespace FFXI
