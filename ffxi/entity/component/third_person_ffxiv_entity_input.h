#pragma once
#include "engine/entity/component/third_person_entity_input_component.h"

class ThirdPersonEntityFFXIVInputComponent : public lotus::ThirdPersonEntityInputComponent
{
public:
    explicit ThirdPersonEntityFFXIVInputComponent(lotus::Entity*, lotus::Engine*, lotus::Input*);
    virtual lotus::Task<> tick(lotus::time_point time, lotus::duration delta) override;
protected:
    bool moving_prev {false};
};
