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

    // Resolve columns by header name so optional metadata and future column
    // reordering do not silently corrupt player attributes.
    PackedStringArray header = file->get_csv_line();
    std::unordered_map<std::string, int> column_indices;
    for (int i = 0; i < header.size(); ++i) {
        String column_name = header[i].strip_edges();
        if (i == 0) {
            column_name = column_name.replace(String::utf8("\xEF\xBB\xBF"), "");
        }
        column_indices[column_name.to_lower().utf8().get_data()] = i;
    }

    const auto find_column = [&column_indices](const std::string& name) -> int {
        auto it = column_indices.find(name);
        return it == column_indices.end() ? -1 : it->second;
    };
    const int name_col = find_column("name");
    const int age_col = find_column("age");
    const int debut_col = find_column("debutage");
    const int potential_col = find_column("potential");
    const int position_col = find_column("position");
    const int pace_col = find_column("pace");
    const int shooting_col = find_column("shooting");
    const int passing_col = find_column("passing");
    const int dribbling_col = find_column("dribbling");
    const int defending_col = find_column("defending");
    const int physical_col = find_column("physical");

    if (name_col < 0 || age_col < 0 || debut_col < 0 || potential_col < 0 ||
        position_col < 0 || pace_col < 0 || shooting_col < 0 || passing_col < 0 ||
        dribbling_col < 0 || defending_col < 0 || physical_col < 0) {
        UtilityFunctions::printerr("PlayerDatabase: CSV is missing one or more required core columns.");
        return false;
    }

    const auto get_value = [](const PackedStringArray& row, int index, const String& fallback = String()) -> String {
        return (index >= 0 && index < row.size()) ? row[index].strip_edges() : fallback;
    };

    const int price_col = find_column("price");
    const int club_col = find_column("club");
    const int league_col = find_column("league");
    const int nationality_col = find_column("nationality");
    const int playstyle1_col = find_column("playstyle 1");
    const int playstyle2_col = find_column("playstyle 2");
    const int playstyle3_col = find_column("playstyle 3");

    int loaded_count = 0;
    int row_index = 1;
    while (!file->eof_reached()) {
        PackedStringArray row = file->get_csv_line();
        ++row_index;
        if (row.size() <= name_col) continue;

        String name = get_value(row, name_col);
        if (name.is_empty()) continue;

        std::string std_name = name.utf8().get_data();
        if (loaded_player_names.find(std_name) != loaded_player_names.end()) {
            UtilityFunctions::print("PlayerDatabase: Duplicate player skipped at row ", row_index, ": ", name);
            continue;
        }

        String pos_str = get_value(row, position_col).to_upper();
        if (!is_valid_position(pos_str)) {
            UtilityFunctions::printerr("PlayerDatabase: Invalid position '", pos_str, "' for player '", name, "' at row ", row_index, ". Skipping row.");
            continue;
        }

        Position pos_enum = string_to_position(pos_str.utf8().get_data());
        PlayerAttributes attrs;
        attrs.Pace      = std::clamp(static_cast<int>(get_value(row, pace_col, "60").to_int()), 1, 99);
        attrs.Shooting  = std::clamp(static_cast<int>(get_value(row, shooting_col, "60").to_int()), 1, 99);
        attrs.Passing   = std::clamp(static_cast<int>(get_value(row, passing_col, "60").to_int()), 1, 99);
        attrs.Dribbling = std::clamp(static_cast<int>(get_value(row, dribbling_col, "60").to_int()), 1, 99);
        attrs.Defending = std::clamp(static_cast<int>(get_value(row, defending_col, "60").to_int()), 1, 99);
        attrs.Physical  = std::clamp(static_cast<int>(get_value(row, physical_col, "60").to_int()), 1, 99);

        const int age = get_value(row, age_col, "24").to_int();
        const int debut_age = get_value(row, debut_col, "16").to_int();
        const int potential = std::clamp(static_cast<int>(get_value(row, potential_col, "75").to_int()), 1, 99);

        // All imported players use the project's fixed age-37 retirement rule.
        Player player(name.utf8().get_data(), pos_enum, attrs, age, potential, debut_age, 37);
        player.SetClubName(get_value(row, club_col, "Free Agent").utf8().get_data());
        player.SetLeagueName(get_value(row, league_col, "Free Agent").utf8().get_data());
        player.SetNationality(get_value(row, nationality_col, "Unknown").utf8().get_data());

        std::vector<std::string> playstyles;
        for (int index : {playstyle1_col, playstyle2_col, playstyle3_col}) {
            String style = get_value(row, index);
            if (!style.is_empty()) {
                std::string style_name = style.utf8().get_data();
                if (std::find(playstyles.begin(), playstyles.end(), style_name) == playstyles.end()) {
                    playstyles.push_back(style_name);
                }
            }
        }
        player.SetPlaystyles(playstyles);

        if (price_col >= 0) {
            const double price = get_value(row, price_col, "0").to_float();
            if (price > 0.0) player.SetMarketValue(price);
        }

        player_objects.push_back(player);
        loaded_player_names.insert(std_name);
        ++loaded_count;
    }

    UtilityFunctions::print("PlayerDatabase: Successfully loaded ", loaded_count, " players from ", p_filepath, " into Godot DB.");
    return true;
}

Dictionary PlayerDatabase::_player_to_dict(const Player& p) const {
    Dictionary d;
    d["name"] = String(p.GetName().c_str());
    d["age"] = p.GetAge();
    d["debut_age"] = p.GetDebutAge();
    d["potential"] = p.GetPotential();
    d["retirement_age"] = p.GetRetirementAge();
    d["club_name"] = String(p.GetClubName().c_str());
    d["league"] = String(p.GetLeagueName().c_str());
    d["nationality"] = String(p.GetNationality().c_str());
    Array playstyles;
    for (const auto& style : p.GetPlaystyles()) playstyles.append(String(style.c_str()));
    d["playstyles"] = playstyles;
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