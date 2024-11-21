#pragma once

#include "dat_chunk.h"
#include <map>
#include <string>
#include <vector>

import glm;

namespace FFXI
{
class MO2 : public DatChunk
{
public:
    struct Frame
    {
        glm::quat rot;
        glm::vec3 trans;
        glm::vec3 scale;
    };

    MO2(char* _name, uint8_t* _buffer, size_t _len);

    std::string name;
    uint32_t frames;
    float speed;
    std::map<uint32_t, std::vector<Frame>> animation_data;
};
} // namespace FFXI
