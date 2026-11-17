#pragma once
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "mainplayer.h"

struct Coach {
    std::string name;
    int coachingSkill;
    double weeklySalary;
    std::string specialty;
};

class YouthAcademy {
public:
    YouthAcademy() = default;

    void HireCoach(const Coach& coach) {
        coaches.push_back(coach);
    }

    bool RemoveCoach(int index) {
        if (index < 0 || index >= static_cast<int>(coaches.size())) {
            return false;
        }
        coaches.erase(coaches.begin() + index);
        return true;
    }

    void AddYouthPlayer(std::shared_ptr<Player> player) {
        youthRoster.push_back(player);
    }

    // Weekly training tick: grants attribute XP scaled by coaches & facility level without aging
    void ProcessWeeklyTraining() {
        int avgCoachSkill = GetAverageCoachSkill();
        int statBoost = std::max(1, (avgCoachSkill / 25) + (FacilityLevel - 1));
        for (auto& player : youthRoster) {
            if (player) {
                player->ApplyTrainingXP(statBoost * 35);
            }
        }
    }

    // Annual season rollover: ages prospects by +1 year and applies end-of-season development
    void ProcessSeasonDevelopment() {
        int avgCoachSkill = GetAverageCoachSkill();
        for (auto& player : youthRoster) {
            if (!player) continue;
            player->AdvanceAge();
            int statBoost = std::max(1, avgCoachSkill / 20);
            player->ApplyTrainingXP(statBoost * 100);
        }
    }

    std::vector<std::shared_ptr<Player>> ExtractGraduatedPlayers() {
        std::vector<std::shared_ptr<Player>> graduated;
        auto it = youthRoster.begin();
        while (it != youthRoster.end()) {
            if ((*it)->IsAvailableForTransfer()) {
                graduated.push_back(*it);
                it = youthRoster.erase(it);
            } else {
                ++it;
            }
        }
        return graduated;
    }

    int GetAverageCoachSkill() const {
        if (coaches.empty()) return 10;
        int total = 0;
        for (const auto& c : coaches) total += c.coachingSkill;
        return total / static_cast<int>(coaches.size());
    }

    double GetTotalWeeklySalaries() const {
        double total = 0;
        for (const auto& c : coaches) total += c.weeklySalary;
        return total;
    }

    const std::vector<std::shared_ptr<Player>>& GetYouthRoster() const { return youthRoster; }
    std::vector<std::shared_ptr<Player>>& GetYouthRoster() { return youthRoster; }

    const std::vector<Coach>& GetCoaches() const { return coaches; }
    std::vector<Coach>& GetCoaches() { return coaches; }

    void Clear() {
        youthRoster.clear();
        coaches.clear();
        FacilityLevel = 1;
        AcademyLevel = 1;
    }

    bool RemoveYouthPlayer(int player_id) {
        auto it = std::remove_if(youthRoster.begin(), youthRoster.end(), [player_id](const std::shared_ptr<Player>& p) {
            return p && p->GetId() == player_id;
        });
        if (it != youthRoster.end()) {
            youthRoster.erase(it, youthRoster.end());
            return true;
        }
        return false;
    }

    std::shared_ptr<Player> FindYouthPlayer(int player_id) const {
        for (const auto& p : youthRoster) {
            if (p && p->GetId() == player_id) return p;
        }
        return nullptr;
    }

    int FacilityLevel = 1;
    int AcademyLevel = 1;

private:
    std::vector<Coach> coaches;
    std::vector<std::shared_ptr<Player>> youthRoster;
};