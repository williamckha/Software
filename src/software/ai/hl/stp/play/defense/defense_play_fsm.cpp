#include "software/ai/hl/stp/play/defense/defense_play_fsm.h"

DefensePlayFSM::DefensePlayFSM(TbotsProto::AiConfig ai_config)
    : ai_config(ai_config), crease_defenders({}), pass_defenders({}), shadowers({}),defender_assignments_q(), highest_cov_rating_assignment(std::nullopt)
{
}

bool DefensePlayFSM::shouldDefendAggressively(const Update& event)
{
    TeamPossession possession = event.common.world.getTeamWithPossession();
    return (possession == TeamPossession::STAGNANT_ENEMY_TEAM);
}

void DefensePlayFSM::resetAssignments(const Update& event)
{
    crease_defender_assignments = std::vector<DefenderAssignment>();
    pass_defender_assignments = std::vector<DefenderAssignment>();
    shadowers = std::vector<std::shared_ptr<ShadowEnemyTactic>>();

    if (event.control_params.defender_assignments.size() > 0)
    {
        highest_cov_rating_assignment = std::make_optional<DefenderAssignment>(event.control_params.defender_assignments.front());
    }

    defender_assignments_q = std::queue(event.control_params.defender_assignments);

    LOG(DEBUG) << "We have " << defender_assignments_q.size() << " assignments";
}

void DefensePlayFSM::blockShots(const Update& event)
{
    resetAssignments(event);

    int num_tactics_left_to_assign = static_cast<int>(event.common.num_tactics);

    if (event.control_params.defender_assignments.size() > 0)
    {
        // assign two to defend the most dangerous enemy threat
        num_tactics_left_to_assign -= assignMostThreateningThreat(event, num_tactics_left_to_assign);
        num_tactics_left_to_assign -= assignMostThreateningThreat(event, num_tactics_left_to_assign);
    }

    if (num_tactics_left_to_assign != static_cast<int>(event.common.num_tactics))
    {
        defender_assignments_q.pop();
    }

    num_tactics_left_to_assign -= updateCreaseAndPassDefenders(event, num_tactics_left_to_assign);

    auto enemy_threats = getAllEnemyThreats(event.common.world.field(), event.common.world.friendlyTeam(),
            event.common.world.enemyTeam(), event.common.world.ball(), false);

    for (int i = 0; (i < static_cast<int>(enemy_threats.size()) && i < num_tactics_left_to_assign); ++i)
    {
        addShadower(enemy_threats.at(i+1));
    }

    setTactics(event);
}

int DefensePlayFSM::assignMostThreateningThreat(const Update &event, int num_tactics_available)
{
    if (defender_assignments_q.size() == 0)
    {
        return 0;
    }

    DefenderAssignment defender_assignment = defender_assignments_q.front();
    if (num_tactics_available >= 1 && defender_assignment.type == CREASE_DEFENDER)
    {
        crease_defender_assignments.emplace_back(defender_assignment);
        return 1;
    }

    return 0;
}

void DefensePlayFSM::shadowAndBlockShots(const Update& event)
{
    int num_tactics_left_to_assign = static_cast<int>(event.common.num_tactics);
    resetAssignments(event);
    const int total_defenders = static_cast<int>(event.common.num_tactics);
    bool assigned_shadower_to_most_dangerous = false;

    num_tactics_left_to_assign -= assignMostThreateningThreat(event, num_tactics_left_to_assign);
    auto enemy_threats = getAllEnemyThreats(event.common.world.field(), event.common.world.friendlyTeam(),
            event.common.world.enemyTeam(), event.common.world.ball(), false);

    if (num_tactics_left_to_assign >= 1 && enemy_threats.size() > 0 && num_tactics_left_to_assign != total_defenders)
    {
        addShadower(enemy_threats.at(0));
        num_tactics_left_to_assign -= 1;
        assigned_shadower_to_most_dangerous = true;
    }

    num_tactics_left_to_assign -= assignMostThreateningThreat(event, num_tactics_left_to_assign); 

    if (num_tactics_left_to_assign != total_defenders)
    {
        defender_assignments_q.pop();
    }

    num_tactics_left_to_assign -= updateCreaseAndPassDefenders(event, num_tactics_left_to_assign);

    // assign shadowers to left over tactics
    int num_unaddressed_enemy_threats = static_cast<int>(enemy_threats.size());
    if (assigned_shadower_to_most_dangerous)
    {
        num_unaddressed_enemy_threats -= 1;
    }
    for (int i = 0; ((i < num_unaddressed_enemy_threats) && i < num_tactics_left_to_assign); ++i)
    {
        if (assigned_shadower_to_most_dangerous)
        {
            addShadower(enemy_threats.at(i+1));
        }
        else
        {
            addShadower(enemy_threats.at(i));
        }
    }

    setTactics(event);
}

int DefensePlayFSM::updateCreaseAndPassDefenders(
    const Update& event, const int num_tactics_to_assign)
{
    int num_tactics_assigned = num_tactics_to_assign;

    if (defender_assignments_q.size() == 0)
    {
        return 0;
    }

    // Choose which defender assignments to assign defenders to based on number
    // of tactics available to set
    for (int i = 0; i < num_tactics_to_assign; i++)
    {
        std::unique_ptr<struct DefenderAssignment> defender_assignment;
        if (!defender_assignments_q.empty())
        {
            defender_assignment = std::make_unique<struct DefenderAssignment>(defender_assignments_q.front());
            defender_assignments_q.pop();
        }
        else if (highest_cov_rating_assignment.has_value())
        {
            // If we have more tactics to set than determined defender assignments,
            // assign remaining defenders to the defender assignment with the
            // highest coverage rating
            defender_assignment = std::make_unique<struct DefenderAssignment>(highest_cov_rating_assignment.value());
        }
        else
        {
            LOG(WARNING) << "[DefensePlayFSM] Trying to assign too many robots to defend, and too few enemy threats to work with";
            break;
        }

        if (defender_assignment->type == CREASE_DEFENDER &&
                crease_defender_assignments.size() < 3)
        {
            crease_defender_assignments.emplace_back(*defender_assignment);
        }
        else if (defender_assignment->type == PASS_DEFENDER)
        {
            pass_defender_assignments.emplace_back(*defender_assignment);
        }
        else if (defender_assignments_q.empty())
        {
            num_tactics_assigned = i+1;
            break;
        }

        defender_assignments_q.pop();
    }

    // Reset tactics if the number of crease defenders or pass defenders
    // we intend to assign has changed
    setUpCreaseDefenders(static_cast<unsigned int>(crease_defender_assignments.size()));
    setUpPassDefenders(static_cast<unsigned int>(pass_defender_assignments.size()));

    for (unsigned int i = 0; i < crease_defenders.size(); i++)
    {
        auto target = crease_defender_assignments.at(i).target;

        // Determine the number of crease defenders already assigned to the target
        auto defenders_with_target_count = std::count_if(
            crease_defender_assignments.begin(), crease_defender_assignments.begin() + i,
            [&target](const auto& assignment) { return assignment.target == target; });

        // Pick alignment based on how many crease defenders are already assigned to the
        // target
        auto alignment = TbotsProto::CreaseDefenderAlignment::CENTRE;
        if (defenders_with_target_count == 1)
        {
            if (event.common.world.ball().position().y() > 0)
            {
                alignment = TbotsProto::CreaseDefenderAlignment::LEFT;
            }
            else
            {
                alignment = TbotsProto::CreaseDefenderAlignment::RIGHT;
            }
        }
        else if (defenders_with_target_count == 2)
        {
            if (event.common.world.ball().position().y() > 0)
            {
                alignment = TbotsProto::CreaseDefenderAlignment::RIGHT;
            }
            else
            {
                alignment = TbotsProto::CreaseDefenderAlignment::LEFT;
            }
        }

        crease_defenders.at(i)->updateControlParams(
            target, alignment, event.control_params.max_allowed_speed_mode);
    }

    for (unsigned int i = 0; i < pass_defenders.size(); i++)
    {
        pass_defenders.at(i)->updateControlParams(pass_defender_assignments.at(i).target);
    }

    return num_tactics_assigned;
}

void DefensePlayFSM::updateShadowers(const Update& event, const std::vector<EnemyThreat> &threats_to_shadow, const int num_shadowers_to_assign)
{
    setUpShadowers(static_cast<unsigned int>(num_shadowers_to_assign));

    for (int i = 0; i < num_shadowers_to_assign; i++)
    {
        shadowers.at(i)->updateControlParams(threats_to_shadow.at(i),
                                             ROBOT_SHADOWING_DISTANCE_METERS);
    }
}

void DefensePlayFSM::addShadower(EnemyThreat &enemy_threat)
{
    std::shared_ptr<ShadowEnemyTactic> shadower = std::make_shared<ShadowEnemyTactic>();
    shadower->updateControlParams(enemy_threat, ROBOT_SHADOWING_DISTANCE_METERS);
    shadowers.push_back(shadower);
}

void DefensePlayFSM::setUpCreaseDefenders(unsigned int num_crease_defenders)
{
    if (num_crease_defenders == crease_defenders.size())
    {
        return;
    }

    crease_defenders =
        std::vector<std::shared_ptr<CreaseDefenderTactic>>(num_crease_defenders);
    std::generate(crease_defenders.begin(), crease_defenders.end(), [this]() {
        return std::make_shared<CreaseDefenderTactic>(
            ai_config.robot_navigation_obstacle_config());
    });
}

void DefensePlayFSM::setUpPassDefenders(unsigned int num_pass_defenders)
{
    if (num_pass_defenders == pass_defenders.size())
    {
        return;
    }

    pass_defenders = std::vector<std::shared_ptr<PassDefenderTactic>>(num_pass_defenders);
    std::generate(pass_defenders.begin(), pass_defenders.end(),
                  [this]() { return std::make_shared<PassDefenderTactic>(); });
}

void DefensePlayFSM::setUpShadowers(unsigned int num_shadowers)
{
    if (num_shadowers == shadowers.size())
    {
        return;
    }

    shadowers = std::vector<std::shared_ptr<ShadowEnemyTactic>>(num_shadowers);
    std::generate(shadowers.begin(), shadowers.end(),
                  [this]() { return std::make_shared<ShadowEnemyTactic>(); });
}

void DefensePlayFSM::setTactics(const Update& event)
{
    PriorityTacticVector tactics_to_return = {{}, {}, {}};

    tactics_to_return[0].insert(tactics_to_return[0].end(), crease_defenders.begin(),
                                crease_defenders.end());
    tactics_to_return[1].insert(tactics_to_return[1].end(), pass_defenders.begin(),
                                pass_defenders.end());
    tactics_to_return[2].insert(tactics_to_return[2].end(), shadowers.begin(),
                                shadowers.end());

    event.common.set_tactics(tactics_to_return);
}
