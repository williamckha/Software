#pragma once

#include "software/util/visitor/visitor.hpp"
#include "software/ai/hl/stp/tactic/all_tactics.h"

/**
 * Refer to the docs about why we use the Visitor Design Pattern
 */
class TacticVisitor : public Visitor<ChipTactic>,
                      public Visitor<CreaseDefenderTactic>,
                      public Visitor<DribbleTactic>,
                      public Visitor<GetBehindBallTactic>,
                      public Visitor<GoalieTactic>,
                      public Visitor<KickTactic>,
                      public Visitor<KickoffChipTactic>,
                      public Visitor<MoveTactic>,
                      public Visitor<AttackerTactic>,
                      public Visitor<PassDefenderTactic>,
                      public Visitor<PenaltyKickTactic>,
                      public Visitor<PenaltySetupTactic>,
                      public Visitor<PivotKickTactic>,
                      public Visitor<ReceiverTactic>,
                      public Visitor<ShadowEnemyTactic>,
                      public Visitor<StopTactic>,
                      public Visitor<MoveGoalieToGoalLineTactic>,
                      public Visitor<PrepareKickoffMoveTactic>,
                      public Visitor<PlaceBallTactic>,
                      public Visitor<PlaceBallMoveTactic>,
                      public Visitor<WallKickoffTactic>,
                      public Visitor<AvoidInterferenceTactic>
{
   public:
    virtual ~TacticVisitor() = default;
};
