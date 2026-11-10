#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <unordered_set>
#include <vector>
#include <string>

#include "mainplayer.h"

namespace godot {

class PlayerDatabase : public RefCounted {
    GDCLASS(PlayerDatabase, RefCounted)

public:
    PlayerDatabase();
    ~PlayerDatabase();

    // Core Database Operations
    bool load_from_csv(const String& p_filepath);
    bool load_players_from_csv(const String& p_filepath); // Compatibility alias
    void clear_database();
    int get_player_count() const;

    // Queries
    Dictionary get_player(const String& p_name) const;
    Dictionary get_player_by_name(const String& p_name) const;
    Array get_all_players() const;
    Array get_active_market_players() const;

    // Position Validator
    static bool is_valid_position(const String& p_pos);

protected:
    static void _bind_methods();

private:
    std::vector<Player> player_objects;
    std::unordered_set<std::string> loaded_player_names;

    Dictionary _player_to_dict(const Player& p) const;
};

} // namespace godot