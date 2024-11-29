#pragma once

#include "dat_chunk.h"
import lotus;
#include <optional>

namespace FFXI
{
class Audio;
class Sep : public DatChunk
{
public:
    Sep(char* name, uint8_t* buffer, size_t len);
    std::optional<lotus::AudioEngine::AudioInstance> playSound(FFXI::Audio* audio);
};
} // namespace FFXI
