#pragma once

#include <lotus/config.h>
#include <filesystem>

class FFXIConfig : public lotus::Config
{
public:
    struct FFXIInfo
    {
        std::filesystem::path ffxi_install_path;
    } ffxi {};

    FFXIConfig();
};
