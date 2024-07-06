#pragma once

#include "software/ai/hl/stp/tactic/all_tactics.h"
#include "software/util/visitor/visitor.hpp"

/**
 * Base class for a visitor that can visit every Tactic.
 * Refer to the docs about why we use the Visitor Design Pattern.
 *
 * @tparam R the return type of the visit operation
 */
template <typename R = void>
class TacticVisitor : public BaseVisitor,
                      public Visitor<ChipTactic, R>,
                      public Visitor<CreaseDefenderTactic, R>,
                      public Visitor<DribbleTactic, R>,
                      public Visitor<GetBehindBallTactic, R>,
                      public Visitor<GoalieTactic, R>,
                      public Visitor<KickTactic, R>,
                      public Visitor<KickoffChipTactic, R>,
                      public Visitor<MoveTactic, R>,
                      public Visitor<AttackerTactic, R>,
                      public Visitor<PassDefenderTactic, R>,
                      public Visitor<PenaltyKickTactic, R>,
                      public Visitor<PenaltySetupTactic, R>,
                      public Visitor<PivotKickTactic, R>,
                      public Visitor<ReceiverTactic, R>,
                      public Visitor<ShadowEnemyTactic, R>,
                      public Visitor<StopTactic, R>,
                      public Visitor<MoveGoalieToGoalLineTactic, R>,
                      public Visitor<PrepareKickoffMoveTactic, R>,
                      public Visitor<PlaceBallTactic, R>,
                      public Visitor<PlaceBallMoveTactic, R>,
                      public Visitor<WallKickoffTactic, R>,
                      public Visitor<AvoidInterferenceTactic, R>
{
   public:
    virtual ~TacticVisitor() = default;
};
