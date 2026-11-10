#pragma once
#include <string>
#include <vector>
#include <algorithm>

enum class Position { 
    GK, CB, LB, RB, LWB, RWB, 
    CM, CDM, CAM, LM, RM, 
    LW, RW, ST, CF, UNKNOWN 
};

inline std::string position_to_string(Position pos) {
    switch (pos) {
        case Position::GK:  return "GK";
        case Position::CB:  return "CB";
        case Position::LB:  return "LB";
        case Position::RB:  return "RB";
        case Position::LWB: return "LWB";
        case Position::RWB: return "RWB";
        case Position::CM:  return "CM";
        case Position::CDM: return "CDM";
        case Position::CAM: return "CAM";
        case Position::LM:  return "LM";
        case Position::RM:  return "RM";
        case Position::LW:  return "LW";
        case Position::RW:  return "RW";
        case Position::ST:  return "ST";
        case Position::CF:  return "CF";
        default:            return "UNKNOWN";
    }
}

inline Position string_to_position(const std::string& str) {
    if (str == "GK")  return Position::GK;
    if (str == "CB")  return Position::CB;
    if (str == "LB")  return Position::LB;
    if (str == "RB")  return Position::RB;
    if (str == "LWB") return Position::LWB;
    if (str == "RWB") return Position::RWB;
    if (str == "CM")  return Position::CM;
    if (str == "CDM") return Position::CDM;
    if (str == "CAM") return Position::CAM;
    if (str == "LM")  return Position::LM;
    if (str == "RM")  return Position::RM;
    if (str == "LW")  return Position::LW;
    if (str == "RW")  return Position::RW;
    if (str == "ST")  return Position::ST;
    if (str == "CF")  return Position::CF;
    return Position::UNKNOWN;
}

enum class PlayerStatus {
    UnbornProspect, 
    YouthAcademy,   
    Professional,   
    Retired         
};

struct PlayerAttributes {
    int Pace = 60;
    int Shooting = 60;
    int Passing = 60;
    int Dribbling = 60;
    int Defending = 60;
    int Physical = 60;
};

class Player {
public:
    Player(std::string name, Position pos, PlayerAttributes attrs, int age, int potential, int debutAge = 14, int retirementAge = 37) 
        : Name(name), Pos(pos), Attributes(attrs), Age(age), Potential(potential), DebutAge(debutAge), RetirementAge(retirementAge), CurrentXP(0), Retired(false) {
        CalculateOVR();
        CheckRetirement();
    }
    
    void CalculateOVR() {
        if (Pos == Position::ST || Pos == Position::CF || Pos == Position::LW || Pos == Position::RW) {
            OVR = static_cast<int>((Attributes.Shooting * 0.4) + (Attributes.Pace * 0.3) + (Attributes.Dribbling * 0.3));
        } else if (Pos == Position::CM || Pos == Position::CAM || Pos == Position::CDM || Pos == Position::LM || Pos == Position::RM) {
            OVR = static_cast<int>((Attributes.Passing * 0.5) + (Attributes.Dribbling * 0.3) + (Attributes.Defending * 0.2));
        } else if (Pos == Position::CB || Pos == Position::LB || Pos == Position::RB || Pos == Position::LWB || Pos == Position::RWB) {
            OVR = static_cast<int>((Attributes.Defending * 0.6) + (Attributes.Physical * 0.3) + (Attributes.Pace * 0.1));
        } else {
            OVR = Attributes.Defending; 
        }
        OVR = std::clamp(OVR, 1, 99);
    }

    bool ApplyTrainingXP(int xpAmount) {
        if (Retired || OVR >= Potential) return false;

        CurrentXP += xpAmount;
        int xpNeeded = OVR * 100; 

        if (CurrentXP >= xpNeeded) {
            CurrentXP -= xpNeeded;
            LevelUpAttributes(1);
            return true; 
        }
        return false;
    }

    void SimulateYearlyGrowth() {
        if (Retired) return;

        Age++;

        if (Age >= RetirementAge) {
            Retired = true;
            return;
        }

        if (Age >= 0 && Age < DebutAge) {
            LevelUpAttributes(1);
        } else if (Age >= DebutAge && Age < 24 && OVR < Potential) {
            LevelUpAttributes(2); 
        } else if (Age >= 32) {
            DeclineAttributes(1);
        }
    }

    bool IsAvailableForTransfer() const {
        return !Retired && (Age >= DebutAge);
    }

    PlayerStatus GetStatus() const {
        if (Retired) return PlayerStatus::Retired;
        if (Age < 0) return PlayerStatus::UnbornProspect;
        if (Age < DebutAge) return PlayerStatus::YouthAcademy;
        return PlayerStatus::Professional;
    }

    bool IsRetired() const { return Retired; }
    std::string GetName() const { return Name; }
    Position GetPosition() const { return Pos; }
    PlayerAttributes GetAttributes() const { return Attributes; }
    int GetOVR() const { return OVR; }
    int GetAge() const { return Age; }
    int GetDebutAge() const { return DebutAge; }
    int GetRetirementAge() const { return RetirementAge; }
    int GetPotential() const { return Potential; }
    
    double GetMarketValue() const { 
        if (Retired) return 0.0; 
        return OVR * 1000000.0; 
    }

    void AdvanceAge() {
        SimulateYearlyGrowth();
    }

private:
    std::string Name;
    Position Pos;
    PlayerAttributes Attributes;
    int OVR, Age, Potential, DebutAge, RetirementAge, CurrentXP;
    bool Retired;

    void CheckRetirement() {
        if (Age >= RetirementAge) {
            Retired = true;
        }
    }

    void LevelUpAttributes(int amount) {
        Attributes.Pace = std::min(99, Attributes.Pace + amount);
        Attributes.Shooting = std::min(99, Attributes.Shooting + amount);
        Attributes.Passing = std::min(99, Attributes.Passing + amount);
        Attributes.Dribbling = std::min(99, Attributes.Dribbling + amount);
        Attributes.Defending = std::min(99, Attributes.Defending + amount);
        Attributes.Physical = std::min(99, Attributes.Physical + amount);
        CalculateOVR();
    }

    void DeclineAttributes(int amount) {
        Attributes.Pace = std::max(1, Attributes.Pace - amount * 2); 
        Attributes.Physical = std::max(1, Attributes.Physical - amount);
        CalculateOVR();
    }
};