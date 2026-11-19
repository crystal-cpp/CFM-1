#include "player_database.h"
#include <godot_cpp/core/class_db.hpp>
#include <algorithm>
#include <cctype>
#include <unordered_map>

using namespace godot;

namespace {
std::string to_lower_ascii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

int find_first_column(const std::unordered_map<std::string, int>& cols, const std::vector<std::string>& candidates) {
    for (const auto& name : candidates) {
        auto it = cols.find(name);
        if (it != cols.end()) {
            return it->second;
        }
    }
    return -1;
}
} // namespace

void PlayerDatabase::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_from_csv", "filepath"), &PlayerDatabase::load_from_csv);
    ClassDB::bind_method(D_METHOD("load_players_from_csv", "filepath"), &PlayerDatabase::load_players_from_csv);
    ClassDB::bind_method(D_METHOD("clear_database"), &PlayerDatabase::clear_database);
    ClassDB::bind_method(D_METHOD("get_player_count"), &PlayerDatabase::get_player_count);
    ClassDB::bind_method(D_METHOD("get_player", "name"), &PlayerDatabase::get_player);
    ClassDB::bind_method(D_METHOD("get_player_by_name", "name"), &PlayerDatabase::get_player_by_name);
    ClassDB::bind_method(D_METHOD("get_all_players"), &PlayerDatabase::get_all_players);
    ClassDB::bind_method(D_METHOD("get_active_market_players"), &PlayerDatabase::get_active_market_players);

    ClassDB::bind_method(D_METHOD("load_clubs_from_csv", "filepath"), &PlayerDatabase::load_clubs_from_csv);
    ClassDB::bind_method(D_METHOD("clear_clubs"), &PlayerDatabase::clear_clubs);
    ClassDB::bind_method(D_METHOD("get_club_count"), &PlayerDatabase::get_club_count);
    ClassDB::bind_method(D_METHOD("get_club", "name"), &PlayerDatabase::get_club);
    ClassDB::bind_method(D_METHOD("get_all_clubs"), &PlayerDatabase::get_all_clubs);
    ClassDB::bind_method(D_METHOD("get_clubs_by_league", "league"), &PlayerDatabase::get_clubs_by_league);
    ClassDB::bind_method(D_METHOD("get_all_leagues"), &PlayerDatabase::get_all_leagues);
    ClassDB::bind_method(D_METHOD("sync_clubs_from_players"), &PlayerDatabase::sync_clubs_from_players);

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

void PlayerDatabase::clear_clubs() {
    club_objects.clear();
    club_index_by_name.clear();
}

int PlayerDatabase::get_club_count() const {
    return static_cast<int>(club_objects.size());
}

bool PlayerDatabase::load_clubs_from_csv(const String& p_filepath) {
    Ref<FileAccess> file = FileAccess::open(p_filepath, FileAccess::READ);
    if (file.is_null()) {
        UtilityFunctions::printerr("PlayerDatabase: Failed to open clubs CSV at: ", p_filepath);
        return false;
    }

    clear_clubs();
    if (file->eof_reached()) {
        UtilityFunctions::printerr("PlayerDatabase: Clubs CSV file is empty: ", p_filepath);
        return false;
    }

    PackedStringArray header = file->get_csv_line();
    std::unordered_map<std::string, int> column_indices;
    for (int i = 0; i < header.size(); ++i) {
        String column_name = header[i].strip_edges();
        if (i == 0) {
            column_name = column_name.replace(String::utf8("\xEF\xBB\xBF"), "");
        }
        column_indices[column_name.to_lower().utf8().get_data()] = i;
    }

    const int name_col = find_first_column(column_indices, {"club", "club_name", "name", "team"});
    const int league_col = find_first_column(column_indices, {"league", "league_name", "division", "competition"});
    if (name_col < 0 || league_col < 0) {
        UtilityFunctions::printerr("PlayerDatabase: Clubs CSV is missing required 'club'/'name' or 'league' column.");
        return false;
    }

    const int country_col = find_first_column(column_indices, {"country", "nation", "nationality"});
    const int rating_col = find_first_column(column_indices, {"rating", "ovr", "overall", "reputation", "prestige"});
    const int budget_col = find_first_column(column_indices, {"budget", "transfer_budget", "balance", "finances"});
    const int stadium_col = find_first_column(column_indices, {"stadium", "venue", "ground"});
    const int primary_color_col = find_first_column(column_indices, {"primary_color", "color", "kit_color"});
    const int secondary_color_col = find_first_column(column_indices, {"secondary_color", "accent_color"});

    const auto get_value = [](const PackedStringArray& row, int index, const String& fallback = String()) -> String {
        return (index >= 0 && index < row.size()) ? row[index].strip_edges() : fallback;
    };

    int loaded_count = 0;
    int row_index = 1;
    while (!file->eof_reached()) {
        PackedStringArray row = file->get_csv_line();
        ++row_index;
        if (row.size() <= std::max(name_col, league_col)) continue;

        String name = get_value(row, name_col);
        String league = get_value(row, league_col);
        if (name.is_empty() || league.is_empty()) continue;

        std::string key = to_lower_ascii(name.utf8().get_data());
        if (club_index_by_name.find(key) != club_index_by_name.end()) {
            UtilityFunctions::print("PlayerDatabase: Duplicate club skipped at row ", row_index, ": ", name);
            continue;
        }

        ClubRecord record;
        record.name = name.utf8().get_data();
        record.league = league.utf8().get_data();
        record.country = get_value(row, country_col, "Unknown").utf8().get_data();
        record.stadium = get_value(row, stadium_col, "").utf8().get_data();
        record.primary_color = get_value(row, primary_color_col, "#FFFFFF").utf8().get_data();
        record.secondary_color = get_value(row, secondary_color_col, "#111111").utf8().get_data();
        record.rating = std::clamp(static_cast<int>(get_value(row, rating_col, "70").to_int()), 1, 99);
        record.budget = std::max(0.0, static_cast<double>(get_value(row, budget_col, "0").to_float()));

        club_index_by_name[key] = club_objects.size();
        club_objects.push_back(std::move(record));
        ++loaded_count;
    }

    UtilityFunctions::print("PlayerDatabase: Successfully loaded ", loaded_count, " clubs from ", p_filepath, ".");
    return true;
}

int PlayerDatabase::sync_clubs_from_players() {
    struct Accumulator {
        std::string name;
        std::string league;
        int ovr_sum = 0;
        int count = 0;
    };

    std::unordered_map<std::string, Accumulator> discovered;
    for (const auto& player : player_objects) {
        const std::string& club_name = player.GetClubName();
        const std::string& league_name = player.GetLeagueName();
        if (club_name.empty() || league_name.empty()) continue;

        std::string club_lower = to_lower_ascii(club_name);
        std::string league_lower = to_lower_ascii(league_name);
        if (club_lower == "free agent" || league_lower == "free agent") continue;
        if (club_index_by_name.find(club_lower) != club_index_by_name.end()) continue;

        auto& acc = discovered[club_lower];
        if (acc.count == 0) {
            acc.name = club_name;
            acc.league = league_name;
        }
        acc.ovr_sum += player.GetOVR();
        acc.count += 1;
    }

    int added = 0;
    for (const auto& [key, acc] : discovered) {
        ClubRecord record;
        record.name = acc.name;
        record.league = acc.league;
        record.country = "Unknown";
        record.rating = acc.count > 0 ? std::clamp(acc.ovr_sum / acc.count, 1, 99) : 70;
        club_index_by_name[key] = club_objects.size();
        club_objects.push_back(std::move(record));
        ++added;
    }
    return added;
}

Dictionary PlayerDatabase::_club_to_dict(const ClubRecord& c) const {
    Dictionary d;
    d["name"] = String(c.name.c_str());
    d["league"] = String(c.league.c_str());
    d["country"] = String(c.country.c_str());
    d["stadium"] = String(c.stadium.c_str());
    d["primary_color"] = String(c.primary_color.c_str());
    d["secondary_color"] = String(c.secondary_color.c_str());
    d["rating"] = c.rating;
    d["budget"] = c.budget;
    return d;
}

Dictionary PlayerDatabase::get_club(const String& p_name) const {
    std::string key = to_lower_ascii(p_name.strip_edges().utf8().get_data());
    auto it = club_index_by_name.find(key);
    if (it != club_index_by_name.end()) {
        return _club_to_dict(club_objects[it->second]);
    }
    return Dictionary();
}

Array PlayerDatabase::get_all_clubs() const {
    Array list;
    for (const auto& club : club_objects) {
        list.append(_club_to_dict(club));
    }
    return list;
}

Array PlayerDatabase::get_clubs_by_league(const String& p_league) const {
    std::string target = to_lower_ascii(p_league.strip_edges().utf8().get_data());
    Array list;
    for (const auto& club : club_objects) {
        if (to_lower_ascii(club.league) == target) {
            list.append(_club_to_dict(club));
        }
    }
    return list;
}

Array PlayerDatabase::get_all_leagues() const {
    Array leagues;
    std::unordered_set<std::string> seen;
    for (const auto& club : club_objects) {
        std::string key = to_lower_ascii(club.league);
        if (seen.insert(key).second) {
            leagues.append(String(club.league.c_str()));
        }
    }
    return leagues;
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