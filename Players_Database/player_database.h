#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

#include "mainplayer.h"

namespace godot {

struct ClubRecord {
    std::string name;
    std::string league;
    std::string country;
    std::string stadium;
    std::string primary_color;
    std::string secondary_color;
    int rating = 70;
    double budget = 0.0;
};

class PlayerDatabase : public RefCounted {
    GDCLASS(PlayerDatabase, RefCounted)

public:
    PlayerDatabase();
    ~PlayerDatabase();

    // Core Player Database Operations
    bool load_from_csv(const String& p_filepath);
    bool load_players_from_csv(const String& p_filepath); // Compatibility alias
    void clear_database();
    int get_player_count() const;

    // Player Queries
    Dictionary get_player(const String& p_name) const;
    Dictionary get_player_by_name(const String& p_name) const;
    Array get_all_players() const;
    Array get_active_market_players() const;

    // Club & League Database Operations (res://data/clubs.csv)
    bool load_clubs_from_csv(const String& p_filepath);
    void clear_clubs();
    int get_club_count() const;
    Dictionary get_club(const String& p_name) const;
    Array get_all_clubs() const;
    Array get_clubs_by_league(const String& p_league) const;
    Array get_all_leagues() const;
    int sync_clubs_from_players();

    // Position Validator
    static bool is_valid_position(const String& p_pos);

    // Raw Access
    const std::vector<Player>& get_raw_players() const { return player_objects; }
    const std::vector<ClubRecord>& get_raw_clubs() const { return club_objects; }

protected:
    static void _bind_methods();

private:
    std::vector<Player> player_objects;
    std::unordered_set<std::string> loaded_player_names;

    std::vector<ClubRecord> club_objects;
    std::unordered_map<std::string, size_t> club_index_by_name;

    Dictionary _player_to_dict(const Player& p) const;
    Dictionary _club_to_dict(const ClubRecord& c) const;
};

} // namespace godot