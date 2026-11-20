#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <unordered_set>

namespace godot {

struct ClubRecord {
    int club_id = 0;
    String club_name;
    String league = "Independent League";
    String short_code;
    String country = "Unknown";
    int reputation = 50;
    int64_t total_balance = 5000000;
    int64_t transfer_budget = 1500000;
    int64_t wage_budget = 50000;
    String default_formation = "4-4-2";
    String primary_color = "#1E3A8A";
    String secondary_color = "#FFFFFF";
};

struct PlayerRecord {
    int player_id = 0;
    String name;
    int age = 22;
    String nationality = "Unknown";
    String club = "Free Agent";
    String league = "Free Agents";
    String position = "CM";
    int overall = 65;
    int potential = 65;
    String playstyle = "Balanced";
    int64_t price = 500000;
    int64_t wage = 5000;
    int stamina = 75;

    // Outfield attributes
    int pace = 0;
    int shooting = 0;
    int passing = 0;
    int dribbling = 0;
    int defending = 0;
    int physical = 0;

    // Goalkeeper attributes
    int diving = 0;
    int reflex = 0;
    int positioning = 0;
    int reaction = 0;
    int kicking = 0;
};

class PlayerDatabase : public RefCounted {
    GDCLASS(PlayerDatabase, RefCounted)

public:
    PlayerDatabase();
    ~PlayerDatabase();

    static PlayerDatabase *get_singleton();

    // Database Loading & Sync
    bool load_databases(const String &clubs_csv_path, const String &players_csv_path);
    bool load_clubs_from_csv(const String &csv_path);
    bool load_players_from_csv(const String &csv_path);
    bool load_from_csv(const String &p_filepath); // Compatibility alias
    void sync_clubs_from_players();
    void clear_database();

    // League & Club Queries
    Array get_all_leagues() const;
    int get_league_count() const;
    Array get_clubs_by_league(const String &league_name) const;
    Dictionary get_club_record(const String &club_name) const;
    Dictionary get_club_by_id(int club_id) const;
    Dictionary get_club(const String &p_name) const; // Compatibility alias
    Array get_all_clubs() const;
    int get_total_club_count() const;
    int get_club_count() const; // Compatibility alias

    // Player Queries
    Array get_players_by_club(const String &club_name) const;
    Dictionary get_player_by_id(int player_id) const;
    Dictionary get_player(const String &p_name) const;
    Dictionary get_player_by_name(const String &p_name) const;
    Array get_all_players() const;
    int get_total_player_count() const;
    int get_player_count() const; // Compatibility alias

    // Raw C++ Access
    const std::vector<PlayerRecord> &get_raw_players() const { return players; }
    const std::vector<ClubRecord> &get_raw_clubs() const { return clubs; }

protected:
    static void _bind_methods();

private:
    static PlayerDatabase *singleton;

    std::vector<ClubRecord> clubs;
    std::unordered_map<std::string, int> club_index_by_name;
    std::unordered_map<int, int> club_index_by_id;
    std::vector<String> league_order;

    std::vector<PlayerRecord> players;
    std::unordered_map<int, int> player_index_by_id;
    std::unordered_map<std::string, std::vector<int>> players_by_club_map;

    Dictionary club_to_dict(const ClubRecord &rec) const;
    Dictionary player_to_dict(const PlayerRecord &p) const;
};

} // namespace godot