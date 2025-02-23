module;

#include <chrono>
#include <coroutine>
#include <memory>
#include <span>

module ffxi:entity.component.actor;

import glm;
import lotus;

namespace FFXI
{
class Scheduler;
class Generator;
class ActorComponent : public lotus::Component::Component<ActorComponent, lotus::Component::Before<lotus::Component::RenderBaseComponent>>
{
public:
    explicit ActorComponent(lotus::Entity*, lotus::Engine* engine, lotus::Component::RenderBaseComponent& physics,
                            lotus::Component::AnimationComponent& animation);

    lotus::Task<> tick(lotus::time_point time, lotus::duration delta);

    glm::vec3 getPos() const { return pos; }
    glm::quat getRot() const { return rot; }
    float getSpeed() const { return speed; }

    void setPos(glm::vec3 pos, bool interpolate = true);
    void setRot(glm::quat rot, bool interpolate = true);
    void setModelOffsetRot(glm::quat rot);
    void setSpeed(float _speed) { speed = _speed; }

    lotus::Component::AnimationComponent& getAnimationComponent() const { return animation; }

protected:
    lotus::Component::RenderBaseComponent& base_component;
    glm::vec3 pos{};
    glm::quat rot{1.f, 0.f, 0.f, 0.f};
    glm::quat model_offset_rot{1.f, 0.f, 0.f, 0.f};
    float speed{4.f};
    lotus::Component::AnimationComponent& animation;
};
} // namespace FFXI
