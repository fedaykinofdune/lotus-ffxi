#pragma once
#include "engine/entity/component/third_person_entity_input_component.h"

class ThirdPersonEntityFFXIVInputComponent : public lotus::ThirdPersonEntityInputComponent
{
public:
    explicit ThirdPersonEntityFFXIVInputComponent(lotus::Entity*, lotus::Engine*, lotus::Input*);
    virtual lotus::Task<> tick(lotus::time_point time, lotus::duration delta) override;
protected:
    glm::vec3 moving_prev {};
    constexpr static glm::vec3 step_height { 0.f, -0.3f, 0.f };
};
