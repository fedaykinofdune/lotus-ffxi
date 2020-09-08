#pragma once

#include "engine/renderer/vulkan/renderer.h"
#include "engine/entity/third_person_boom_camera.h"

class ThirdPersonFFXIVCamera : public lotus::ThirdPersonBoomCamera
{
public:
    explicit ThirdPersonFFXIVCamera(lotus::Engine*);
    std::vector<lotus::UniqueWork> Init(const std::shared_ptr<ThirdPersonFFXIVCamera>& sp, std::weak_ptr<Entity>& focus);
};
