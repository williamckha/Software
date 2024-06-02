#pragma once

#include "software/ai/evaluation/q_learning/attacker_mdp_action.h"
#include "software/ai/evaluation/q_learning/attacker_mdp_feature_extractor.h"
#include "software/ai/evaluation/q_learning/bandits/epsilon_greedy_strategy.hpp"
#include "software/ai/evaluation/q_learning/bandits/softmax_strategy.hpp"
#include "software/ai/evaluation/q_learning/gameplay_monitor.h"
#include "software/ai/evaluation/q_learning/linear_q_function.hpp"
#include "software/ai/evaluation/q_learning/q_policy.hpp"
#include "software/ai/hl/stp/tactic/tactic.h"

/**
 * The Attacker is the main ball handler during offensive gameplay. It executes
 * sequences of Skills (e.g. dribble, pass, kick, chip) to create chances and score goals.
 *
 * The Attacker selects which Skills to execute according to a learned policy, which
 * gives the probability of taking a given action (Skill) in a given state (the World).
 * We attempt to find the optimal policy using reinforcement learning algorithms.
 */
class AttackerTactic : public Tactic
{
   public:
    /**
     * Creates a AttackerTactic.
     *
     * @param strategy the shared Strategy used by all of AI
     */
    explicit AttackerTactic(std::shared_ptr<Strategy> strategy);

    AttackerTactic() = delete;

    void accept(TacticVisitor& visitor) const override;

    bool done() const override;

    std::string getFSMState() const override;

   private:
    // The shared Strategy used by all of AI
    std::shared_ptr<Strategy> strategy_;

    // The Q-function of the attacker agent's policy
    std::shared_ptr<LinearQFunction<AttackerMdpState, AttackerMdpAction>> q_function_;

    // The action selection strategy of attacker agent's policy
    std::shared_ptr<ActionSelectionStrategy<AttackerMdpState, AttackerMdpAction>>
        action_selection_strategy_;

    // The policy that the attacker agent will follow
    QPolicy<AttackerMdpState, AttackerMdpAction> policy_;

    // The reward function for the attacker agent
    GameplayMonitor gameplay_monitor_;

    // The current skill that the attacker is executing
    std::unique_ptr<Skill> current_skill_;

    void updatePrimitive(const TacticUpdate& tactic_update, bool reset_fsm) override;
};
