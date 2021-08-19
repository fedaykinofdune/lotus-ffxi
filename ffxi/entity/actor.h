#pragma once
#include "engine/entity/deformable_entity.h"
#include "engine/task.h"
#include "dat/sk2.h"
#include "dat/scheduler.h"
#include "dat/generator.h"

namespace FFXI {
    class OS2;
    class Dat;
}

union LookData
{
    struct 
    {
        uint8_t race = 1;
        uint8_t face = 1;
        uint16_t head = 0x1000;
        uint16_t body = 0x2000;
        uint16_t hands = 0x3000;
        uint16_t legs = 0x4000;
        uint16_t feet = 0x5000;
        uint16_t main = 0x6000;
        uint16_t sub = 0x7000;
        uint16_t ranged = 0x8000;
    } look;
    uint16_t slots[9];
};

//main FFXI entity class
class Actor : public lotus::DeformableEntity
{
public:
    explicit Actor(lotus::Engine* engine);
    static lotus::Task<std::shared_ptr<Actor>> Init(lotus::Engine* engine, size_t modelid);
    static lotus::Task<std::shared_ptr<Actor>> Init(lotus::Engine* engine, LookData look);

    void updateEquipLook(uint16_t modelid);

    float speed{ 4.f };
    std::array<FFXI::SK2::GeneratorPoint, FFXI::SK2::GeneratorPointMax> generator_points{};
    std::map<std::string, FFXI::Scheduler*> scheduler_map;
    std::map<std::string, FFXI::Generator*> generator_map;
protected:
    size_t GetModelDat(uint16_t modelid);
    lotus::WorkerTask<> Load(std::initializer_list<std::reference_wrapper<const FFXI::Dat>> dats);
    lotus::Task<> updateEquipLookTask(uint16_t modelid);

    enum class ModelType
    {
        PC,
        NPC,
        NPCFixed,
        Environment
    } model_type{ ModelType::PC };

    LookData look{};
};
