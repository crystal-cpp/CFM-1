#pragma once
#include "mainplayer.h"
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

class Team {
public:
    Team(std::string name, double budget) 
        : Name(name), TransferBudget(budget), TrainingPoints(0) {}
    
    void AddPlayer(std::shared_ptr<Player> player) { 
        Roster.push_back(player); 
        if (StartingXI.size() < 11) StartingXI.push_back(player);
    }
    
    int GetTeamRating() const {
        if (StartingXI.empty()) return 0;
        int totalOVR = 0;
        for (const auto& p : StartingXI) totalOVR += p->GetOVR();
        return totalOVR / StartingXI.size();
    }

    void AddTrainingPoints(int amount) { TrainingPoints += amount; }
    bool SpendTrainingPoints(int amount) {
        if (TrainingPoints >= amount) {
            TrainingPoints -= amount;
            return true;
        }
        return false;
    }
    
    void DeductBudget(double amount) { TransferBudget -= amount; }
    void AddBudget(double amount) { TransferBudget += amount; }
    double GetBudget() const { return TransferBudget; }
    void SetBudget(double amount) { TransferBudget = amount; }
    int GetTrainingPoints() const { return TrainingPoints; }
    std::string GetName() const { return Name; }
    void SetName(const std::string& name) { Name = name; }

    int GetId() const { return Id; }
    void SetId(int id) { Id = id; }

    const std::vector<std::shared_ptr<Player>>& GetRoster() const { return Roster; }
    std::vector<std::shared_ptr<Player>>& GetRoster() { return Roster; }

    const std::vector<std::shared_ptr<Player>>& GetStartingXI() const { return StartingXI; }
    std::vector<std::shared_ptr<Player>>& GetStartingXI() { return StartingXI; }
    void SetStartingXI(const std::vector<std::shared_ptr<Player>>& xi) { StartingXI = xi; }

    void ClearRoster() {
        Roster.clear();
        StartingXI.clear();
    }

    bool RemovePlayer(int player_id) {
        bool removed = false;
        auto it = std::remove_if(Roster.begin(), Roster.end(), [player_id](const std::shared_ptr<Player>& p) {
            return p && p->GetId() == player_id;
        });
        if (it != Roster.end()) {
            Roster.erase(it, Roster.end());
            removed = true;
        }

        auto it_xi = std::remove_if(StartingXI.begin(), StartingXI.end(), [player_id](const std::shared_ptr<Player>& p) {
            return p && p->GetId() == player_id;
        });
        if (it_xi != StartingXI.end()) {
            StartingXI.erase(it_xi, StartingXI.end());
        }
        return removed;
    }

    bool HasPlayer(int player_id) const {
        for (const auto& p : Roster) {
            if (p && p->GetId() == player_id) return true;
        }
        return false;
    }

    std::string League = "Premier Division";
    std::string Country = "England";
    std::string ShortCode;
    int Reputation = 75;
    int64_t TotalBalance = 0;
    int64_t WageBudget = 0;
    std::string KitColor = "#FFFFFF";
    std::string SecondaryColor = "#111111";

    // Tactical setup for match handoff
    std::string Formation = "4-3-3";
    int Mentality = 2;            // Balanced
    int Pressing = 1;             // Normal
    int DefensiveLine = 1;        // Normal
    int PassingStyle = 1;         // Mixed
    int Width = 1;                // Normal
    int Tempo = 1;                // Normal
    int DefensiveApproach = 1;    // Normal

    void ProcessEndOfWeekOrSeason() {
        for (auto& player : Roster) player->SimulateYearlyGrowth();
    }

private:
    int Id = 1;
    std::string Name;
    std::vector<std::shared_ptr<Player>> Roster;
    std::vector<std::shared_ptr<Player>> StartingXI;
    double TransferBudget;
    int TrainingPoints;
};
