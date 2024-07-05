#pragma once

#include <set>

#include "proto/primitive.pb.h"
#include "software/ai/hl/stp/tactic/tactic_visitor.h"
#include "software/world/game_state.h"

class MotionConstraintVisitor : public TacticVisitor
{
   public:
    explicit MotionConstraintVisitor() = default;

    /**
     * Visits a tactic to register the associated motion constraint
     *
     * @param tactic the tactic to register
     *
     * @modifies current_motion_constraints
     */
    void visit(GoalieTactic &tactic) override;
    void visit(CreaseDefenderTactic &tactic) override;
    void visit(ShadowEnemyTactic &tactic) override;
    void visit(MoveTactic &tactic) override;
    void visit(ChipTactic &tactic) override;
    void visit(KickTactic &tactic) override;
    void visit(KickoffChipTactic &tactic) override;
    void visit(StopTactic &tactic) override;
    void visit(PenaltyKickTactic &tactic) override;
    void visit(PenaltySetupTactic &tactic) override;
    void visit(ReceiverTactic &tactic) override;
    void visit(AttackerTactic &tactic) override;
    void visit(DribbleTactic &tactic) override;
    void visit(GetBehindBallTactic &tactic) override;
    void visit(PivotKickTactic &tactic) override;
    void visit(MoveGoalieToGoalLineTactic &tactic) override;
    void visit(PrepareKickoffMoveTactic &tactic) override;
    void visit(PlaceBallTactic &tactic) override;
    void visit(PlaceBallMoveTactic &tactic) override;
    void visit(WallKickoffTactic &tactic) override;
    void visit(AvoidInterferenceTactic &tactic) override;
    void visit(PassDefenderTactic &tactic) override;

    /**
     * Gets the motion constraints updated with the requirements of the tactics
     *
     * @param tactic the tactic to use to update the motion constraints
     * @param existing_motion_constraints the existing motion constraints from 
     * other sources
     *
     * @modifies current_motion_constraints
     * @return set of MotionConstraints
     */
    std::set<TbotsProto::MotionConstraint> getUpdatedMotionConstraints(
        Tactic &tactic,
        const std::set<TbotsProto::MotionConstraint> &existing_motion_constraints);

   private:
    std::set<TbotsProto::MotionConstraint> current_motion_constraints;
};
