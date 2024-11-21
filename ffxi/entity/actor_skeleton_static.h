#pragma once
#include "actor_data.h"
#include "dat/sk2.h"
#include <array>
#include <lotus/renderer/animation.h>
#include <lotus/renderer/skeleton.h>
#include <lotus/task.h>
#include <lotus/worker_task.h>
#include <span>
#include <unordered_map>

namespace lotus
{
class Engine;
}

namespace FFXI
{
class Scheduler;
class Generator;
class ActorSkeletonStatic
{
public:
    static constexpr size_t skeleton_size = 4;
    static constexpr size_t battle_animation_size = 8;
    static constexpr size_t dw_animation_size = 4;
    static lotus::Task<std::shared_ptr<const ActorSkeletonStatic>> getSkeleton(lotus::Engine*, size_t id);
    static lotus::Task<std::shared_ptr<const ActorSkeletonStatic>> getSkeleton(lotus::Engine*, ActorData::PCDatIDs);
    const lotus::Skeleton::BoneData& getBoneData() const { return static_bone_data; }
    const std::unordered_map<std::string, FFXI::Scheduler*> getSchedulers() const;
    const std::unordered_map<std::string, FFXI::Generator*> getGenerators() const;
    const std::unordered_map<std::string, std::unique_ptr<lotus::Animation>>& getAnimations() const;
    const std::unordered_map<std::string, std::unique_ptr<lotus::Animation>>& getBattleAnimations(size_t index) const;
    std::tuple<const std::unordered_map<std::string, std::unique_ptr<lotus::Animation>>&,
               const std::unordered_map<std::string, std::unique_ptr<lotus::Animation>>&>
    getBattleAnimationsDualWield(size_t index) const;
    std::span<const FFXI::SK2::GeneratorPoint, FFXI::SK2::GeneratorPointMax> getGeneratorPoints() const;

protected:
    lotus::Skeleton::BoneData static_bone_data;
    std::unordered_map<std::string, FFXI::Scheduler*> scheduler_map;
    std::unordered_map<std::string, FFXI::Generator*> generator_map;
    std::unordered_map<std::string, std::unique_ptr<lotus::Animation>> animations;
    std::array<std::unordered_map<std::string, std::unique_ptr<lotus::Animation>>, battle_animation_size>
        battle_animations;
    std::array<std::unordered_map<std::string, std::unique_ptr<lotus::Animation>>, dw_animation_size> dw_animations_l;
    std::array<std::unordered_map<std::string, std::unique_ptr<lotus::Animation>>, dw_animation_size> dw_animations_r;
    std::array<FFXI::SK2::GeneratorPoint, FFXI::SK2::GeneratorPointMax> generator_points{};
    std::unordered_map<std::string, std::unique_ptr<lotus::Animation>> loadAnimationDat(lotus::Engine*, size_t id);
    inline static std::unordered_map<size_t, std::weak_ptr<ActorSkeletonStatic>> skeleton_map;
    static lotus::WorkerTask<std::shared_ptr<ActorSkeletonStatic>>
    loadSkeleton(lotus::Engine*, std::span<size_t> skeleton_ids, std::span<size_t> motions = {},
                 std::span<size_t> dw_motions_l = {}, std::span<size_t> dw_motions_r = {});
    ActorSkeletonStatic();
};
} // namespace FFXI
