#include "software/ai/motion_constraint/motion_constraint_visitor.h"

void MotionConstraintVisitor::visit(GoalieTactic &tactic)
{
    current_motion_constraints.erase(TbotsProto::MotionConstraint::FRIENDLY_DEFENSE_AREA);
    current_motion_constraints.erase(
        TbotsProto::MotionConstraint::HALF_METER_AROUND_BALL);
    current_motion_constraints.erase(TbotsProto::MotionConstraint::FRIENDLY_HALF);
}

void MotionConstraintVisitor::visit(CreaseDefenderTactic &tactic)
{
    current_motion_constraints.erase(
        TbotsProto::MotionConstraint::HALF_METER_AROUND_BALL);
}

void MotionConstraintVisitor::visit(ShadowEnemyTactic &tactic) {}

void MotionConstraintVisitor::visit(MoveTactic &tactic) {}

void MotionConstraintVisitor::visit(ChipTactic &tactic) {}

void MotionConstraintVisitor::visit(KickTactic &tactic) {}

void MotionConstraintVisitor::visit(PivotKickTactic &tactic) {}

void MotionConstraintVisitor::visit(KickoffChipTactic &tactic)
{
    current_motion_constraints.erase(TbotsProto::MotionConstraint::CENTER_CIRCLE);
    current_motion_constraints.erase(TbotsProto::MotionConstraint::ENEMY_HALF);
    current_motion_constraints.erase(
        TbotsProto::MotionConstraint::HALF_METER_AROUND_BALL);
    current_motion_constraints.insert(
        TbotsProto::MotionConstraint::ENEMY_HALF_WITHOUT_CENTRE_CIRCLE);
}

void MotionConstraintVisitor::visit(PrepareKickoffMoveTactic &tactic)
{
    current_motion_constraints.erase(TbotsProto::MotionConstraint::CENTER_CIRCLE);
    current_motion_constraints.erase(
        TbotsProto::MotionConstraint::HALF_METER_AROUND_BALL);
    current_motion_constraints.erase(TbotsProto::MotionConstraint::ENEMY_HALF);
    current_motion_constraints.insert(
        TbotsProto::MotionConstraint::ENEMY_HALF_WITHOUT_CENTRE_CIRCLE);
}

void MotionConstraintVisitor::visit(StopTactic &tactic) {}

void MotionConstraintVisitor::visit(PenaltyKickTactic &tactic)
{
    current_motion_constraints.erase(
        TbotsProto::MotionConstraint::HALF_METER_AROUND_BALL);
    current_motion_constraints.erase(TbotsProto::MotionConstraint::ENEMY_DEFENSE_AREA);
    current_motion_constraints.erase(TbotsProto::MotionConstraint::ENEMY_HALF);
}

void MotionConstraintVisitor::visit(PenaltySetupTactic &tactic)
{
    current_motion_constraints.erase(TbotsProto::MotionConstraint::ENEMY_HALF);
    current_motion_constraints.erase(TbotsProto::MotionConstraint::ENEMY_DEFENSE_AREA);
    current_motion_constraints.erase(TbotsProto::MotionConstraint::FRIENDLY_HALF);
    current_motion_constraints.erase(
        TbotsProto::MotionConstraint::HALF_METER_AROUND_BALL);
}

void MotionConstraintVisitor::visit(ReceiverTactic &tactic) {}

void MotionConstraintVisitor::visit(AttackerTactic &tactic) {}

void MotionConstraintVisitor::visit(DribbleTactic &tactic) {}

void MotionConstraintVisitor::visit(GetBehindBallTactic &tactic) {}

void MotionConstraintVisitor::visit(MoveGoalieToGoalLineTactic &tactic)
{
    current_motion_constraints.erase(TbotsProto::MotionConstraint::FRIENDLY_HALF);
    current_motion_constraints.erase(TbotsProto::MotionConstraint::FRIENDLY_DEFENSE_AREA);
}

void MotionConstraintVisitor::visit(PlaceBallTactic &tactic)
{
    current_motion_constraints.clear();
}

void MotionConstraintVisitor::visit(PlaceBallMoveTactic &tactic)
{
    current_motion_constraints.clear();
}

void MotionConstraintVisitor::visit(WallKickoffTactic &tactic)
{
    current_motion_constraints.clear();
}

void MotionConstraintVisitor::visit(AvoidInterferenceTactic &tactic)
{
    current_motion_constraints.clear();
}

void MotionConstraintVisitor::visit(PassDefenderTactic &tactic) {}

std::set<TbotsProto::MotionConstraint>
MotionConstraintVisitor::getUpdatedMotionConstraints(
    Tactic &tactic,
    const std::set<TbotsProto::MotionConstraint> &existing_motion_constraints)
{
    current_motion_constraints = existing_motion_constraints;
    tactic.accept(*this);
    return current_motion_constraints;
}
