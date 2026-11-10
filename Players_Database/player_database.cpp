#include "player_database.h"
#include <godot_cpp/core/class_db.hpp>
#include <algorithm>

using namespace godot;

void PlayerDatabase::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_from_csv", "filepath"), &PlayerDatabase::load_from_csv);
    ClassDB::bind_method(D_METHOD("load_players_from_csv", "filepath"), &PlayerDatabase::load_players_from_csv);
    ClassDB::bind_method(D_METHOD("clear_database"), &PlayerDatabase::clear_database);
    ClassDB::bind_method(D_METHOD("get_player_count"), &PlayerDatabase::get_player_count);
    ClassDB::bind_method(D_METHOD("get_player", "name"), &PlayerDatabase::get_player);
    ClassDB::bind_method(D_METHOD("get_player_by_name", "name"), &PlayerDatabase::get_player_by_name);
    ClassDB::bind_method(D_METHOD("get_all_players"), &PlayerDatabase::get_all_players);
    ClassDB::bind_method(D_METHOD("get_active_market_players"), &PlayerDatabase::get_active_market_players);
    ClassDB::bind_static_method("PlayerDatabase", D_METHOD("is_valid_position", "pos"), &PlayerDatabase::is_valid_position);
}

PlayerDatabase::PlayerDatabase() {}
PlayerDatabase::~PlayerDatabase() {}

bool PlayerDatabase::is_valid_position(const String& p_pos) {
    String pos = p_pos.strip_edges().to_upper();
    return (pos == "GK"  || pos == "CB"  || pos == "LB"  || pos == "RB"  || 
            pos == "LWB" || pos == "RWB" || pos == "CDM" || pos == "CM"  || 
            pos == "CAM" || pos == "LM"  || pos == "RM"  || pos == "LW"  || 
            pos == "RW"  || pos == "ST"  || pos == "CF");
}

void PlayerDatabase::clear_database() {
    player_objects.clear();
    loaded_player_names.clear();
}

int PlayerDatabase::get_player_count() const {
    return static_cast<int>(player_objects.size());
}

bool PlayerDatabase::load_players_from_csv(const String& p_filepath) {
    return load_from_csv(p_filepath);
}

bool PlayerDatabase::load_from_csv(const String& p_filepath) {
    Ref<FileAccess> file = FileAccess::open(p_filepath, FileAccess::READ);
    if (file.is_null()) {
        UtilityFunctions::printerr("PlayerDatabase: Failed to open CSV at: ", p_filepath);
        return false;
    }

    clear_database();

    if (file->eof_reached()) {
        UtilityFunctions::printerr("PlayerDatabase: CSV file is empty: ", p_filepath);
        return false;
    }

    PackedStringArray header = file->get_csv_line();
    if (header.size() < 11) {
        UtilityFunctions::printerr("PlayerDatabase: Invalid CSV header size. Expected at least 11 columns, got ", header.size());
        return false;
    }

    int loaded_count = 0;
    int row_index = 1;

    while (!file->eof_reached()) {
        row_index++;
        PackedStringArray row = file->get_csv_line();
        if (row.size() < 11) continue;

        String name = row[0].strip_edges();
        if (name.is_empty()) continue;

        std::string std_name = name.utf8().get_data();
        if (loaded_player_names.find(std_name) != loaded_player_names.end()) {
            UtilityFunctions::print("PlayerDatabase: Duplicate player skipped at row ", row_index, ": ", name);
            continue;
        }

        String pos_str = row[4].strip_edges().to_upper();
        if (!is_valid_position(pos_str)) {
            UtilityFunctions::printerr("PlayerDatabase: Invalid position '", pos_str, "' for player '", name, "' at row ", row_index, ". Skipping row.");
            continue;
        }

        Position pos_enum = string_to_position(pos_str.utf8().get_data());

        PlayerAttributes attrs;
        attrs.Pace     = std::clamp(static_cast<int>(row[5].to_int()), 1, 99);
        attrs.Shooting = std::clamp(static_cast<int>(row[6].to_int()), 1, 99);
        attrs.Passing  = std::clamp(static_cast<int>(row[7].to_int()), 1, 99);
        attrs.Dribbling= std::clamp(static_cast<int>(row[8].to_int()), 1, 99);
        attrs.Defending= std::clamp(static_cast<int>(row[9].to_int()), 1, 99);
        attrs.Physical = std::clamp(static_cast<int>(row[10].to_int()), 1, 99);

        int age = row[1].to_int();
        int debut_age = row[2].to_int();
        int potential = std::clamp(static_cast<int>(row[3].to_int()), 1, 99);

        Player player(name.utf8().get_data(), pos_enum, attrs, age, potential, debut_age);

        player_objects.push_back(player);
        loaded_player_names.insert(std_name);
        loaded_count++;
    }

    UtilityFunctions::print("PlayerDatabase: Successfully loaded ", loaded_count, " players into Godot DB.");
    return true;
}

Dictionary PlayerDatabase::_player_to_dict(const Player& p) const {
    Dictionary d;
    d["name"] = String(p.GetName().c_str());
    d["age"] = p.GetAge();
    d["debut_age"] = p.GetDebutAge();
    d["potential"] = p.GetPotential();
    d["position"] = String(position_to_string(p.GetPosition()).c_str());
    
    PlayerAttributes attrs = p.GetAttributes();
    d["pace"] = attrs.Pace;
    d["shooting"] = attrs.Shooting;
    d["passing"] = attrs.Passing;
    d["dribbling"] = attrs.Dribbling;
    d["defending"] = attrs.Defending;
    d["physical"] = attrs.Physical;
    
    d["ovr"] = p.GetOVR();
    d["market_value"] = p.GetMarketValue();
    d["is_retired"] = p.IsRetired();
    d["is_available_transfer"] = p.IsAvailableForTransfer();
    d["status"] = static_cast<int>(p.GetStatus());
    return d;
}

Dictionary PlayerDatabase::get_player(const String& p_name) const {
    return get_player_by_name(p_name);
}

Dictionary PlayerDatabase::get_player_by_name(const String& p_name) const {
    std::string search_name = p_name.strip_edges().to_lower().utf8().get_data();
    for (const auto& player : player_objects) {
        std::string p_name_lower = player.GetName();
        std::transform(p_name_lower.begin(), p_name_lower.end(), p_name_lower.begin(), ::tolower);
        if (p_name_lower == search_name) {
            return _player_to_dict(player);
        }
    }
    return Dictionary();
}

Array PlayerDatabase::get_all_players() const {
    Array list;
    for (const auto& player : player_objects) {
        list.append(_player_to_dict(player));
    }
    return list;
}

Array PlayerDatabase::get_active_market_players() const {
    Array list;
    for (const auto& player : player_objects) {
        if (player.IsAvailableForTransfer()) {
            list.append(_player_to_dict(player));
        }
    }
    return list;
}