module;

#include <memory>
#include <unordered_map>
#include <vector>

export module ffxi:system_dat;

import lotus;

export class FFXIGame;
namespace FFXI
{
class Dat;
class Generator;
class Keyframe;
class Scheduler;
class DatChunk;
} // namespace FFXI

export class SystemDat
{
    struct _private_tag
    {
        explicit _private_tag() = default;
    };

public:
    static lotus::Task<std::unique_ptr<SystemDat>> Load(FFXIGame* game);

    std::unordered_map<std::string, FFXI::Generator*> generators;
    std::unordered_map<std::string, FFXI::Scheduler*> schedulers;

    SystemDat(FFXIGame* game, _private_tag);

private:
    FFXIGame* game{nullptr};

    SystemDat(const SystemDat&) = delete;
    SystemDat(SystemDat&&) = default;
    SystemDat& operator=(const SystemDat&) = delete;
    SystemDat& operator=(SystemDat&&) = default;

    lotus::Task<> ParseDir(FFXI::DatChunk*);

    std::vector<std::shared_ptr<lotus::Model>> generator_models;
    std::unordered_map<std::string, FFXI::Keyframe*> keyframes;
};
