#pragma once
#include "entity/actor.h"
#include <atomic>
#include <latch>

namespace FFXI
{
class OS2;
}

class FFXIActorLoader
{
public:
    static lotus::Task<> LoadModel(std::shared_ptr<lotus::Model>, lotus::Engine* engine,
                                   std::span<FFXI::OS2* const> os2s);

private:
    static void InitPipeline(lotus::Engine*);
    static inline vk::Pipeline pipeline;
    static inline vk::Pipeline pipeline_shadowmap;
    static inline std::latch pipeline_latch{1};
    static inline std::atomic_flag pipeline_flag;
};
