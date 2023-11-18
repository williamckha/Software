#pragma once

#include <set>

#include "software/ai/hl/stp/play/dynamic_plays/support_tactic_scorers/support_tactic_scorer.h"
#include "software/ai/hl/stp/tactic/pass_receiver/pass_receiver_tactic.h"
#include "software/ai/hl/stp/tactic/fake_pass_receiver/fake_pass_receiver_tactic.h"
#include "software/ai/hl/stp/tactic/cherry_picker/cherry_picker_tactic.h"
#include "software/ai/hl/stp/tactic/disrupter/disrupter_tactic.h"

/**
 * A SupportTacticCandidate is a "type wrapper" that makes a type of tactic
 * eligible for scoring and assignment as a support tactic in a DynamicPlay.
 *
 * It acts as the visitee for a SupportTacticScorer, which operates under
 * the visitor pattern.
 */
template<typename SupportTacticType>
class SupportTacticCandidate 
{
   public:
    explicit SupportTacticCandidate();

    /**
     * Accepts a SupportTacticScorer and calls the scorer's `score` function 
     * on itself. The scorer's returned score is applied to this candidate.
     *
     * @param scorer a SupportTacticScorer to use to score this candidate
     */
    void score(SupportTacticScorer &scorer);

    /**
     * Removes all scores applied to this candidate
     */
    void clearScores();

    /**
     * Gets the total score for this support tactic candidate.
     * 
     * The total score X(x_1, x_2, ..., x_n) summarizes all the individual 
     * scores x_i that have been applied to this candidate. It is calculated 
     * as a generalized mean of the form 
     *
     *                                  n
     * X(x_1, x_2, ..., x_n) = ((1/n) * Σ sign(x_i) * |x_i|^(1/p))^p
     *                                 i=1
     *
     * where the parameter p is > 0. As p gets larger, scores close to 0
     * and negative scores will influence the total score more than values 
     * far away from 0. 
     *
     * This parameter p is useful to us since negative scores are meant
     * to penalize and scores close to 0 indicate low viability, so they
     * should disproportionately impact the total score more so than any 
     * single large positive score.
     * 
     * @return the total score for this support tactic candidate
     */
    double getTotalScore() const;

    /**
     * Returns a shared pointer to a newly constructed instance of the
     * candidate's support tactic type
     *
     * @return a shared pointer to a newly constructed instance of the
     * candidate's support tactic type
     */
    std::shared_ptr<SupportTacticType> createSupportTactic() const;

   private:
    std::vector<int> scores_;

    // Parameter p in total score calculation
    static constexpr double SINGLE_SCORE_INFLUENCE = 2.5;
}

using SupportTacticCandidateVector = std::vector<std::shared_ptr<SupportTacticCandidate<Tactic>>>;

/**
 * Returns all support tactic candidates
 * 
 * @return a vector of shared pointers to all support tactic candidates
 */
static SupportTacticCandidateVector allSupportTacticCandidates() 
{
    return 
    {
        std::make_shared<SupportTacticCandidate<PassReceiverTactic>>(),
        std::make_shared<SupportTacticCandidate<FakePassReceiverTactic>>(),
        std::make_shared<SupportTacticCandidate<CherryPickerTactic>>(),
        std::make_shared<SupportTacticCandidate<DisrupterTactic>>()
    };
}

template<typename SupportTacticType>
SupportTacticCandidate<SupportTacticType>::SupportTacticCandidate() : scores_() 
{
}

template<typename SupportTacticType>
void SupportTacticCandidate<SupportTacticType>::score(SupportTacticScorer &scorer)
{
    scores_.push_back(scorer.score(*this));
}

template<typename SupportTacticType>
void SupportTacticCandidate<SupportTacticType>::resetTotalScore()
{
    scores_.clear();
}

template<typename SupportTacticType>
double SupportTacticCandidate<SupportTacticType>::getTotalScore() const
{
    double sum = std::accumulate(scores_.begin(), scores_.end(), 0.0,
        [](double current_sum, double score) {
            double sign = (0.0 < score) - (score < 0.0); 
            double term = sign * std::pow(std::abs(score), 1.0 / SINGLE_SCORE_INFLUENCE);
            return current_sum + term;
        });

    double num_scores = std::static_cast<double>(scores_.size());
    double total_score = std::pow(sum / num_scores, SINGLE_SCORE_INFLUENCE)
    return total_score;
}

template<typename SupportTacticType>
std::shared_ptr<SupportTacticType> SupportTacticCandidate<SupportTacticType>::create() const
{
    return std::make_shared<SupportTacticType>();
}
