#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <utility>
#include <cctype>

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

enum class InjurySeverity : int32_t {
    None = 0,
    Minor = 1,      // 3-5 days
    Moderate = 2,   // 7-14 days
    Major = 3,      // 30-60 days
    Serious = 4     // 120-180 days + one-time 5-10% market value drop
};

inline std::string injury_severity_to_string(InjurySeverity tier) {
    switch (tier) {
        case InjurySeverity::Minor:    return "Minor Injury";
        case InjurySeverity::Moderate: return "Moderate Injury";
        case InjurySeverity::Major:    return "Major Injury";
        case InjurySeverity::Serious:  return "Serious Injury";
        default:                       return "";
    }
}

inline std::pair<int, int> injury_tier_day_bounds(InjurySeverity tier) {
    switch (tier) {
        case InjurySeverity::Minor:    return { 3, 5 };
        case InjurySeverity::Moderate: return { 7, 14 };
        case InjurySeverity::Major:    return { 30, 60 };
        case InjurySeverity::Serious:  return { 120, 180 };
        default:                       return { 0, 0 };
    }
}

inline InjurySeverity injury_severity_from_int(int32_t value) {
    switch (value) {
        case 1:  return InjurySeverity::Minor;
        case 2:  return InjurySeverity::Moderate;
        case 3:  return InjurySeverity::Major;
        case 4:  return InjurySeverity::Serious;
        default: return InjurySeverity::None;
    }
}

inline InjurySeverity injury_severity_from_string(const std::string& raw) {
    std::string s;
    s.reserve(raw.size());
    for (unsigned char c : raw) {
        if (!std::isspace(c)) {
            s.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    if (s == "minor" || s == "minorinjury")       return InjurySeverity::Minor;
    if (s == "moderate" || s == "moderateinjury") return InjurySeverity::Moderate;
    if (s == "major" || s == "majorinjury")       return InjurySeverity::Major;
    if (s == "serious" || s == "seriousinjury")   return InjurySeverity::Serious;
    return InjurySeverity::None;
}

struct PlayerAttributes {
    int Pace = 60;
    int Shooting = 60;
    int Passing = 60;
    int Dribbling = 60;
    int Defending = 60;
    int Physical = 60;
    int Stamina = 75;
};

class Player {
public:
    Player(std::string name, Position pos, PlayerAttributes attrs, int age, int potential, int debutAge = 14, int retirementAge = 37) 
        : Name(name), Pos(pos), Attributes(attrs), Age(age), Potential(potential), DebutAge(debutAge), RetirementAge(retirementAge), CurrentXP(0), Retired(false) {
        CalculateOVR();
        CheckRetirement();
    }
    
    void AgeOneYear() {
        Age += 1;
        if (ContractYears > 0) {
            ContractYears -= 1;
        }
        CheckRetirement();
    }

    void CalculateOVR() {
        if (Pos == Position::GK) {
            OVR = Attributes.Defending;
        } else if (Pos == Position::ST || Pos == Position::CF || Pos == Position::LW || Pos == Position::RW) {
            OVR = static_cast<int>((Attributes.Shooting * 0.4) + (Attributes.Pace * 0.3) + (Attributes.Dribbling * 0.3));
        } else if (Pos == Position::CM || Pos == Position::CAM || Pos == Position::CDM || Pos == Position::LM || Pos == Position::RM) {
            OVR = static_cast<int>((Attributes.Passing * 0.5) + (Attributes.Dribbling * 0.3) + (Attributes.Defending * 0.2));
        } else if (Pos == Position::CB || Pos == Position::LB || Pos == Position::RB || Pos == Position::LWB || Pos == Position::RWB) {
            OVR = static_cast<int>((Attributes.Defending * 0.6) + (Attributes.Physical * 0.3) + (Attributes.Pace * 0.1));
        } else {
            OVR = static_cast<int>((Attributes.Passing * 0.5) + (Attributes.Dribbling * 0.3) + (Attributes.Defending * 0.2));
        }
        OVR = std::clamp(OVR, 1, 99);
    }

    bool ApplyTrainingXP(int xpAmount) {
        if (Retired || IsInjured() || OVR >= Potential) return false;

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

    const std::string& GetClubName() const { return ClubName; }
    const std::string& GetLeagueName() const { return LeagueName; }
    const std::string& GetNationality() const { return Nationality; }
    const std::vector<std::string>& GetPlaystyles() const { return Playstyles; }
    int GetStamina() const { return Stamina; }
    void SetStamina(int value) {
        Stamina = std::clamp(value, 1, 99);
        Attributes.Stamina = Stamina;
    }
    void SetClubName(const std::string& value) { ClubName = value.empty() ? "Free Agent" : value; }
    void SetLeagueName(const std::string& value) { LeagueName = value.empty() ? "Free Agent" : value; }
    void SetNationality(const std::string& value) { Nationality = value.empty() ? "Unknown" : value; }
    void SetPlaystyles(const std::vector<std::string>& value) { Playstyles = value; }
    
    int GetId() const { return Id; }
    void SetId(int p_id) { Id = p_id; }

    int GetClubId() const { return ClubId; }
    void SetClubId(int p_club_id) { ClubId = p_club_id; }


    int GetMorale() const { return Morale; }
    void SetMorale(int p_mor) { Morale = std::clamp(p_mor, 0, 100); }

    int64_t GetWage() const { return Wage; }
    void SetWage(int64_t p_wage) { Wage = p_wage; }

    int GetContractYears() const { return ContractYears; }
    void SetContractYears(int p_years) { ContractYears = p_years; }

    int GetAppearances() const { return Appearances; }
    void SetAppearances(int p_app) { Appearances = p_app; }
    void RecordAppearance() { Appearances++; }

    int GetGoals() const { return Goals; }
    void SetGoals(int p_goals) { Goals = p_goals; }
    void RecordGoals(int p_g) { Goals += p_g; }

    int GetAssists() const { return Assists; }
    void SetAssists(int p_assists) { Assists = p_assists; }
    void RecordAssists(int p_a) { Assists += p_a; }

    bool GetIsStartingXI() const { return IsStartingXI; }
    void SetIsStartingXI(bool p_val) { IsStartingXI = p_val; }

    void SetAttributes(const PlayerAttributes& p_attrs) { Attributes = p_attrs; CalculateOVR(); }
    void SetPosition(Position p_pos) { Pos = p_pos; CalculateOVR(); }
    void SetAge(int p_age) { Age = p_age; CheckRetirement(); }
    void SetPotential(int p_pot) { Potential = p_pot; }
    void SetMarketValue(double p_val) { CustomMarketValue = p_val; }
    void SetRetired(bool p_ret) { Retired = p_ret; }

    double GetMarketValue() const { 
        if (Retired) return 0.0; 
        if (CustomMarketValue > 0.0) return CustomMarketValue;
        return OVR * 1000000.0; 
    }

    void AdvanceAge() {
        SimulateYearlyGrowth();
    }
    // --- Post-Match Stamina Persistence & Daily Calendar Recovery ---
    double GetCurrentStamina() const { return CurrentStamina; }
    void SetCurrentStamina(double p_stamina) {
        CurrentStamina = std::clamp(p_stamina, 0.0, 100.0);
        Condition = std::clamp(static_cast<int>(std::round(CurrentStamina)), 0, 100);
    }

    int GetCondition() const { return Condition; }
    void SetCondition(int p_cond) {
        Condition = std::clamp(p_cond, 0, 100);
        CurrentStamina = static_cast<double>(Condition);
    }

    void RecoverDailyStamina(double recovery_rate_per_day) {
        const double rate = std::clamp(recovery_rate_per_day, 0.0, 1.0);
        SetCurrentStamina(CurrentStamina + (100.0 * rate));
    }

    // --- Injury Lifecycle & One-Time Serious Value Reduction ---
    bool IsInjured() const {
        return InjuryTier != InjurySeverity::None && InjuryDaysRemaining > 0;
    }
    bool IsAvailableForMatch() const {
        return !Retired && !IsInjured();
    }
    InjurySeverity GetInjurySeverity() const { return InjuryTier; }
    std::string GetInjurySeverityLabel() const { return injury_severity_to_string(InjuryTier); }
    int GetInjuryDaysRemaining() const { return InjuryDaysRemaining; }
    bool IsSeriousValuePenaltyApplied() const { return SeriousValuePenaltyApplied; }
    void SetSeriousValuePenaltyApplied(bool applied) { SeriousValuePenaltyApplied = applied; }

    void ApplyInjury(InjurySeverity tier, int days = -1, double serious_drop_fraction = 0.075) {
        if (tier == InjurySeverity::None) {
            ClearInjury();
            return;
        }
        const auto bounds = injury_tier_day_bounds(tier);
        int resolved_days = (days > 0)
            ? std::clamp(days, bounds.first, bounds.second)
            : bounds.first;

        InjuryTier = tier;
        InjuryDaysRemaining = resolved_days;

        // One-time 5-10% market value drop on Serious Injury (never compounds)
        if (tier == InjurySeverity::Serious && !SeriousValuePenaltyApplied) {
            const double current_val = GetMarketValue();
            const double drop = std::clamp(serious_drop_fraction, 0.05, 0.10);
            CustomMarketValue = std::max(10000.0, std::round(current_val * (1.0 - drop)));
            SeriousValuePenaltyApplied = true;
        }
    }

    // Restores persisted injury state directly during save/load without re-triggering value drops
    void RestoreInjuryState(InjurySeverity tier, int days_remaining, bool serious_penalty_applied) {
        SeriousValuePenaltyApplied = serious_penalty_applied;
        if (tier == InjurySeverity::None || days_remaining <= 0) {
            InjuryTier = InjurySeverity::None;
            InjuryDaysRemaining = 0;
        } else {
            InjuryTier = tier;
            InjuryDaysRemaining = days_remaining;
        }
    }

    // Returns true on the exact day the player completes recovery
    bool RecoverOneInjuryDay() {
        if (!IsInjured()) return false;
        InjuryDaysRemaining = std::max(0, InjuryDaysRemaining - 1);
        if (InjuryDaysRemaining == 0) {
            InjuryTier = InjurySeverity::None;
            return true;
        }
        return false;
    }

    void ClearInjury() {
        InjuryTier = InjurySeverity::None;
        InjuryDaysRemaining = 0;
    }
private:
    std::string Name;
    Position Pos;
    PlayerAttributes Attributes;
    int OVR, Age, Potential, DebutAge, RetirementAge, CurrentXP;
    bool Retired;
    std::string ClubName = "Free Agent";
    std::string LeagueName = "Free Agent";
    std::string Nationality = "Unknown";
    std::vector<std::string> Playstyles;
    int Stamina = 75;

    double CurrentStamina = 100.0;
    int Condition = 100;
    InjurySeverity InjuryTier = InjurySeverity::None;
    int InjuryDaysRemaining = 0;
    bool SeriousValuePenaltyApplied = false;

    int Id = 0;
    int ClubId = -1;
    int Morale = 100;
    int64_t Wage = 5000;
    int ContractYears = 3;
    int Appearances = 0;
    int Goals = 0;
    int Assists = 0;
    double CustomMarketValue = 0.0;
    bool IsStartingXI = false;

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
        const int decline = std::max(0, amount);
        Attributes.Pace = std::max(1, Attributes.Pace - decline * 2);
        Attributes.Physical = std::max(1, Attributes.Physical - decline);
        Attributes.Defending = std::max(1, Attributes.Defending - decline);
        if (Pos != Position::GK) {
            Attributes.Passing = std::max(1, Attributes.Passing - decline);
            Attributes.Dribbling = std::max(1, Attributes.Dribbling - decline);
        }
        CalculateOVR();
    }
};