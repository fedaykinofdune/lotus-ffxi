#pragma once
#include "engine/core.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/texture.h"
#include "engine/task/model_init.h"
#include "engine/task/texture_init.h"
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class TestTextureLoader : public lotus::TextureLoader
{
public:
    virtual std::vector<lotus::UniqueWork> LoadTexture(std::shared_ptr<lotus::Texture>& texture) override
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = static_cast<uint64_t>(texWidth) * static_cast<uint64_t>(texHeight) * 4;

        texture->setWidth(texWidth);
        texture->setHeight(texHeight);

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        std::vector<uint8_t> texture_data;
        texture_data.resize(imageSize);
        memcpy(texture_data.data(), pixels, imageSize);
        stbi_image_free(pixels);

        texture->image = engine->renderer->gpu->memory_manager->GetImage(texture->getWidth(), texture->getHeight(), vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

        vk::ImageViewCreateInfo image_view_info;
        image_view_info.image = texture->image->image;
        image_view_info.viewType = vk::ImageViewType::e2D;
        image_view_info.format = vk::Format::eR8G8B8A8Unorm;
        image_view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount = 1;

        texture->image_view = engine->renderer->gpu->device->createImageViewUnique(image_view_info, nullptr);

        vk::SamplerCreateInfo sampler_info = {};
        sampler_info.magFilter = vk::Filter::eLinear;
        sampler_info.minFilter = vk::Filter::eLinear;
        sampler_info.addressModeU = vk::SamplerAddressMode::eRepeat;
        sampler_info.addressModeV = vk::SamplerAddressMode::eRepeat;
        sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
        sampler_info.anisotropyEnable = true;
        sampler_info.maxAnisotropy = 16;
        sampler_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
        sampler_info.unnormalizedCoordinates = false;
        sampler_info.compareEnable = false;
        sampler_info.compareOp = vk::CompareOp::eAlways;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;

        texture->sampler = engine->renderer->gpu->device->createSamplerUnique(sampler_info, nullptr);

        std::vector<lotus::UniqueWork> ret;
        ret.push_back(std::make_unique<lotus::TextureInitTask>(engine->renderer->getCurrentImage(), texture, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, std::move(texture_data)));
        return ret;
    }
};
