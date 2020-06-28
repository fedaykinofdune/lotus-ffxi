#pragma once
#include "entity/actor.h"
#include "engine/work_item.h"

class ActorDatLoad : public lotus::WorkItem
{
public:
    ActorDatLoad(const std::shared_ptr<Actor>& entity, const std::filesystem::path& dat);
    virtual ~ActorDatLoad() override = default;
    virtual void Process(lotus::WorkerThread*) override;

private:
    std::shared_ptr<Actor> entity;
    std::filesystem::path dat;
};

