#pragma once

#include "engine/work_item.h"
#include "engine/entity/landscape_entity.h"

class LandscapeDatLoad : public lotus::WorkItem
{
public:
    LandscapeDatLoad(const std::shared_ptr<lotus::LandscapeEntity>& entity, const std::string& dat);
    virtual ~LandscapeDatLoad() override = default;
    virtual void Process(lotus::WorkerThread*) override;

private:
    std::shared_ptr<lotus::LandscapeEntity> entity;
    std::string dat;
};
