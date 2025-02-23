module;

#include <coroutine>
#include <cstdint>
#include <string>

module ffxi:entity.component.scheduler;

import :entity.component.actor_skeleton;
import lotus;

class SchedulerResources;

namespace FFXI
{
class Scheduler;
class SchedulerComponent : public lotus::Component::Component<SchedulerComponent, lotus::Component::Before<lotus::Component::AnimationComponent>>
{
public:
    explicit SchedulerComponent(lotus::Entity*, lotus::Engine* engine, ActorSkeletonComponent& actor, FFXI::Scheduler* scheduler, SchedulerResources* resources,
                                FFXI::SchedulerComponent* parent);

    lotus::Task<> tick(lotus::time_point time, lotus::duration delta);

    FFXI::SchedulerComponent* getParent() { return parent; }
    void cancel();

protected:
    ActorSkeletonComponent& actor;
    FFXI::Scheduler* scheduler{};
    SchedulerResources* resources{};
    FFXI::SchedulerComponent* parent{};
    lotus::time_point start_time;
    uint32_t stage{0};
    bool finished{false};
};
} // namespace FFXI
