#include "software/ai/play_selection_fsm.h"

#include "software/ai/hl/stp/play/ball_placement_play.h"
#include "software/ai/hl/stp/play/corner_kick_play.h"
#include "software/ai/hl/stp/play/enemy_ball_placement_play.h"
#include "software/ai/hl/stp/play/enemy_free_kick_play.h"
#include "software/ai/hl/stp/play/free_kick_play.h"
#include "software/ai/hl/stp/play/halt_play.h"
#include "software/ai/hl/stp/play/kickoff_enemy_play.h"
#include "software/ai/hl/stp/play/kickoff_friendly_play.h"
#include "software/ai/hl/stp/play/offense/offense_play.h"
#include "software/ai/hl/stp/play/penalty_kick/penalty_kick_play.h"
#include "software/ai/hl/stp/play/penalty_kick_enemy/penalty_kick_enemy_play.h"
#include "software/ai/hl/stp/play/stop_play.h"


PlaySelectionFSM::PlaySelectionFSM(TbotsProto::AiConfig ai_config)
    : ai_config(ai_config),
      ball_placement_play(std::make_shared<BallPlacementPlay>()),
      enemy_ball_placement_play(std::make_shared<EnemyBallPlacementPlay>()),
      enemy_free_kick_play(std::make_shared<EnemyFreeKickPlay>()),
      free_kick_play(std::make_shared<FreeKickPlay>()),
      halt_play(std::make_shared<HaltPlay>()),
      kickoff_enemy_play(std::make_shared<KickoffEnemyPlay>()),
      kickoff_friendly_play(std::make_shared<KickoffFriendlyPlay>()),
      offense_play(std::make_shared<OffensePlay>()),
      penalty_kick_enemy_play(std::make_shared<PenaltyKickEnemyPlay>()),
      penalty_kick_play(std::make_shared<PenaltyKickPlay>()),
      stop_play(std::make_shared<StopPlay>())
{
}

bool PlaySelectionFSM::gameStateStopped(const Update& event)
{
    return event.game_state.isStopped();
}

bool PlaySelectionFSM::gameStateHalted(const Update& event)
{
    return event.game_state.isHalted();
}

bool PlaySelectionFSM::gameStatePlaying(const Update& event)
{
    return event.game_state.isPlaying();
}

bool PlaySelectionFSM::gameStateSetupRestart(const Update& event)
{
    return event.game_state.isSetupRestart();
}

void PlaySelectionFSM::setupSetPlay(const Update& event)
{
    current_play.reset();
    if (event.game_state.isOurBallPlacement())
    {
        event.set_current_play(std::move(ball_placement_play));
    }

    if (event.game_state.isTheirBallPlacement())
    {
        event.set_current_play(std::make_unique<EnemyBallPlacementPlay>(ai_config));
    }

    if (event.game_state.isOurKickoff())
    {
        event.set_current_play(std::make_unique<KickoffFriendlyPlay>(ai_config));
    }

    if (event.game_state.isTheirKickoff())
    {
        event.set_current_play(std::make_unique<KickoffEnemyPlay>(ai_config));
    }

    if (event.game_state.isOurPenalty())
    {
        event.set_current_play(std::make_unique<PenaltyKickPlay>(ai_config));
    }

    if (event.game_state.isTheirPenalty())
    {
        event.set_current_play(std::make_unique<PenaltyKickEnemyPlay>(ai_config));
    }

    if (event.game_state.isOurDirectFree() || event.game_state.isOurIndirectFree())
    {
        event.set_current_play(std::make_unique<FreeKickPlay>(ai_config));
    }

    if (event.game_state.isTheirDirectFree() || event.game_state.isTheirIndirectFree())
    {
        event.set_current_play(std::make_unique<EnemyFreekickPlay>(ai_config));
    }
}

void PlaySelectionFSM::setupStopPlay(const Update& event)
{
    event.set_current_play(std::make_unique<StopPlay>(ai_config));
}

void PlaySelectionFSM::setupHaltPlay(const Update& event)
{
    event.set_current_play(std::make_unique<HaltPlay>(ai_config));
}

void PlaySelectionFSM::setupOffensePlay(const Update& event)
{
    event.set_current_play(std::make_unique<OffensePlay>(ai_config));
}
