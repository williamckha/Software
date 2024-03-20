#pragma once

#include "software/ai/hl/stp/strategy/strategy.h"
#include "proto/primitive/primitive_msg_factory.h"
#include "proto/tbots_software_msgs.pb.h"
#include "software/ai/hl/stp/tactic/primitive.h"
#include "software/ai/hl/stp/tactic/stop_primitive.h"
#include "software/util/sml_fsm/sml_fsm.h"

using SetPrimitiveCallback = std::function<void(std::shared_ptr<Primitive>)>;

struct SkillUpdate
{
    SkillUpdate(const Robot &robot, const WorldPtr &world_ptr,
                std::shared_ptr<Strategy> strategy,
                const SetPrimitiveCallback &set_primitive_fun)
        : robot(robot),
          world_ptr(world_ptr),
          strategy(strategy),
          set_primitive(set_primitive_fun)
    {
    }
    Robot robot;
    WorldPtr world_ptr;
    std::shared_ptr<Strategy> strategy;
    SetPrimitiveCallback set_primitive;
};

/**
 * The Update struct is the only event that a Skill FSM should respond to and it is
 * composed of the following structs:
 *
 * ControlParams - uniquely defined by each Skill FSM to control the FSM
 * SkillUpdate - common struct that contains Robot, World, Strategy, and
 * SetPrimitiveCallback
 */
#define DEFINE_SKILL_UPDATE_STRUCT_WITH_CONTROL_AND_COMMON_PARAMS                        \
    struct Update                                                                        \
    {                                                                                    \
        Update(const ControlParams &control_params, const SkillUpdate &common)           \
            : control_params(control_params), common(common)                             \
        {                                                                                \
        }                                                                                \
        ControlParams control_params;                                                    \
        SkillUpdate common;                                                              \
    };

#define SET_STOP_PRIMITIVE_ACTION                                                        \
    [this](auto event) { event.common.set_primitive(std::make_unique<StopPrimitive>()); }