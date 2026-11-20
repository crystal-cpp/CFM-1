// player_database.cpp
// Crystal Football Manager 1 (CFM-1) — C++23 GDExtension (Godot 4.7.2)

#include "player_database.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_set>
#include <vector>

using namespace godot;

PlayerDatabase *PlayerDatabase::singleton = nullptr;

namespace {

int position_priority(const String &pos) {
    const String p = pos.strip_edges().to_upper();
    if (p == "GK") return 0;
    if (p == "CB" || p == "LB" || p == "RB" || p == "LWB" || p == "RWB" || p == "SW") return 1;
    if (p == "CDM" || p == "CM" || p == "CAM" || p == "LM" || p == "RM") return 2;
    if (p == "LW" || p == "RW" || p == "CF" || p == "ST" || p == "LF" || p == "RF") return 3;
    return 4;
}

int compute_derived_ovr(const PlayerRecord &p) {
    if (p.position == "GK") {
        const double raw = (p.diving * 0.22) +
                           (p.reflex * 0.24) +
                           (p.positioning * 0.22) +
                           (p.reaction * 0.18) +
                           (p.kicking * 0.07) +
                           (p.physical * 0.07);
        return std::clamp(static_cast<int>(std::llround(raw)), 1, 99);
    }

    const String pos = p.position;
    double raw = 0.0;
    if (pos == "CB" || pos == "SW") {
        raw = p.defending * 0.42 + p.physical * 0.26 + p.passing * 0.14 + p.pace * 0.10 + p.dribbling * 0.08;
    } else if (pos == "LB" || pos == "RB" || pos == "LWB" || pos == "RWB") {
        raw = p.pace * 0.22 + p.defending * 0.28 + p.passing * 0.20 + p.dribbling * 0.16 + p.physical * 0.14;
    } else if (pos == "CDM") {
        raw = p.defending * 0.30 + p.passing * 0.26 + p.physical * 0.20 + p.dribbling * 0.14 + p.pace * 0.10;
    } else if (pos == "CM") {
        raw = p.passing * 0.30 + p.dribbling * 0.24 + p.defending * 0.16 + p.physical * 0.12 + p.shooting * 0.10 + p.pace * 0.08;
    } else if (pos == "CAM") {
        raw = p.passing * 0.30 + p.dribbling * 0.30 + p.shooting * 0.20 + p.pace * 0.12 + p.physical * 0.08;
    } else if (pos == "LM" || pos == "RM" || pos == "LW" || pos == "RW" || pos == "LF" || pos == "RF") {
        raw = p.pace * 0.26 + p.dribbling * 0.28 + p.passing * 0.20 + p.shooting * 0.18 + p.physical * 0.08;
    } else { // ST, CF
        raw = p.shooting * 0.34 + p.pace * 0.22 + p.dribbling * 0.22 + p.physical * 0.14 + p.passing * 0.08;
    }
    return std::clamp(static_cast<int>(std::llround(raw)), 1, 99);
}

PackedStringArray parse_csv_line(const String &line) {
    PackedStringArray fields;
    String current;
    bool in_quotes = false;

    for (int i = 0; i < line.length(); ++i) {
        const char32_t c = line[i];
        if (c == U'"') {
            if (in_quotes && i + 1 < line.length() && line[i + 1] == U'"') {
                current += U'"';
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == U',' && !in_quotes) {
            fields.append(current.strip_edges());
            current = "";
        } else if (c != U'\r' && c != U'\n') {
            current += c;
        }
    }
    fields.append(current.strip_edges());
    return fields;
}

int find_col(const PackedStringArray &headers, const std::vector<const char *> &aliases, int fallback_idx = -1) {
    for (int i = 0; i < headers.size(); ++i) {
        const String h = headers[i].strip_edges().to_lower();
        for (const char *alias : aliases) {
            if (h == alias) {
                return i;
            }
        }
    }
    return fallback_idx;
}

String get_str(const PackedStringArray &row, int idx, const String &def_val = "") {
    if (idx >= 0 && idx < row.size() && !row[idx].is_empty()) {
        return row[idx];
    }
    return def_val;
}

int get_int(const PackedStringArray &row, int idx, int def_val = 0) {
    if (idx >= 0 && idx < row.size() && row[idx].is_valid_int()) {
        return row[idx].to_int();
    }
    if (idx >= 0 && idx < row.size() && row[idx].is_valid_float()) {
        return static_cast<int>(std::llround(row[idx].to_float()));
    }
    return def_val;
}

int64_t get_i64(const PackedStringArray &row, int idx, int64_t def_val = 0) {
    if (idx >= 0 && idx < row.size() && row[idx].is_valid_int()) {
        return row[idx].to_int();
    }
    if (idx >= 0 && idx < row.size() && row[idx].is_valid_float()) {
        return static_cast<int64_t>(std::llround(row[idx].to_float()));
    }
    return def_val;
}

} // namespace

PlayerDatabase::PlayerDatabase() {
    singleton = this;
}

PlayerDatabase::~PlayerDatabase() {
    if (singleton == this) {
        singleton = nullptr;
    }
}

PlayerDatabase *PlayerDatabase::get_singleton() {
    return singleton;
}

void PlayerDatabase::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_databases", "clubs_csv_path", "players_csv_path"), &PlayerDatabase::load_databases);
    ClassDB::bind_method(D_METHOD("load_clubs_from_csv", "csv_path"), &PlayerDatabase::load_clubs_from_csv);
    ClassDB::bind_method(D_METHOD("load_players_from_csv", "csv_path"), &PlayerDatabase::load_players_from_csv);
    ClassDB::bind_method(D_METHOD("load_from_csv", "filepath"), &PlayerDatabase::load_from_csv);
    ClassDB::bind_method(D_METHOD("sync_clubs_from_players"), &PlayerDatabase::sync_clubs_from_players);
    ClassDB::bind_method(D_METHOD("clear_database"), &PlayerDatabase::clear_database);

    ClassDB::bind_method(D_METHOD("get_all_leagues"), &PlayerDatabase::get_all_leagues);
    ClassDB::bind_method(D_METHOD("get_league_count"), &PlayerDatabase::get_league_count);
    ClassDB::bind_method(D_METHOD("get_clubs_by_league", "league_name"), &PlayerDatabase::get_clubs_by_league);
    ClassDB::bind_method(D_METHOD("get_club_record", "club_name"), &PlayerDatabase::get_club_record);
    ClassDB::bind_method(D_METHOD("get_club_by_id", "club_id"), &PlayerDatabase::get_club_by_id);
    ClassDB::bind_method(D_METHOD("get_club", "club_name"), &PlayerDatabase::get_club);
    ClassDB::bind_method(D_METHOD("get_all_clubs"), &PlayerDatabase::get_all_clubs);

    ClassDB::bind_method(D_METHOD("get_players_by_club", "club_name"), &PlayerDatabase::get_players_by_club);
    ClassDB::bind_method(D_METHOD("get_player_by_id", "player_id"), &PlayerDatabase::get_player_by_id);
    ClassDB::bind_method(D_METHOD("get_player", "player_name"), &PlayerDatabase::get_player);
    ClassDB::bind_method(D_METHOD("get_player_by_name", "player_name"), &PlayerDatabase::get_player_by_name);
    ClassDB::bind_method(D_METHOD("get_all_players"), &PlayerDatabase::get_all_players);

    ClassDB::bind_method(D_METHOD("get_total_club_count"), &PlayerDatabase::get_total_club_count);
    ClassDB::bind_method(D_METHOD("get_club_count"), &PlayerDatabase::get_club_count);
    ClassDB::bind_method(D_METHOD("get_total_player_count"), &PlayerDatabase::get_total_player_count);
    ClassDB::bind_method(D_METHOD("get_player_count"), &PlayerDatabase::get_player_count);
}

bool PlayerDatabase::load_databases(const String &clubs_csv_path, const String &players_csv_path) {
    const bool clubs_ok = load_clubs_from_csv(clubs_csv_path);
    const bool players_ok = load_players_from_csv(players_csv_path);
    sync_clubs_from_players();
    return clubs_ok && players_ok;
}

bool PlayerDatabase::load_from_csv(const String &p_filepath) {
    return load_players_from_csv(p_filepath);
}

void PlayerDatabase::clear_database() {
    clubs.clear();
    club_index_by_name.clear();
    club_index_by_id.clear();
    league_order.clear();

    players.clear();
    player_index_by_id.clear();
    players_by_club_map.clear();
}

bool PlayerDatabase::load_clubs_from_csv(const String &csv_path) {
    if (!FileAccess::file_exists(csv_path)) {
        UtilityFunctions::push_error("PlayerDatabase::load_clubs_from_csv — File not found: " + csv_path);
        return false;
    }

    Ref<FileAccess> file = FileAccess::open(csv_path, FileAccess::READ);
    if (file.is_null()) {
        UtilityFunctions::push_error("PlayerDatabase::load_clubs_from_csv — Unable to open: " + csv_path);
        return false;
    }

    clubs.clear();
    club_index_by_name.clear();
    club_index_by_id.clear();
    league_order.clear();

    if (file->eof_reached()) {
        return false;
    }

    const PackedStringArray headers = parse_csv_line(file->get_line());

    const int col_club_id         = find_col(headers, { "clubid", "club_id", "id" }, 0);
    const int col_club_name       = find_col(headers, { "club", "clubname", "club_name", "name" }, 1);
    const int col_league          = find_col(headers, { "league", "leaguename", "league_name" }, 2);
    const int col_short_code      = find_col(headers, { "shortcode", "short_code", "code", "abbr" }, 3);
    const int col_country         = find_col(headers, { "country", "nation" }, 4);
    const int col_reputation      = find_col(headers, { "reputation", "rep" }, 5);
    const int col_total_balance   = find_col(headers, { "totalbalance", "total_balance", "balance" }, 6);
    const int col_transfer_budget = find_col(headers, { "transferbudget", "transfer_budget" }, 7);
    const int col_wage_budget     = find_col(headers, { "wagebudget", "wage_budget" }, 8);
    const int col_formation       = find_col(headers, { "defaultformation", "default_formation", "formation" }, 9);
    const int col_primary_color   = find_col(headers, { "primarycolor", "primary_color", "kitprimary" }, 10);
    const int col_secondary_color = find_col(headers, { "secondarycolor", "secondary_color", "kitsecondary" }, 11);

    std::unordered_set<std::string> seen_leagues;
    int next_auto_id = 1001;

    while (!file->eof_reached()) {
        const String raw_line = file->get_line();
        if (raw_line.strip_edges().is_empty()) {
            continue;
        }

        const PackedStringArray row = parse_csv_line(raw_line);
        const String club_name = get_str(row, col_club_name);
        if (club_name.is_empty()) {
            continue;
        }

        ClubRecord rec;
        rec.club_id = get_int(row, col_club_id, next_auto_id);
        next_auto_id = std::max(next_auto_id + 1, rec.club_id + 1);
        rec.club_name = club_name;
        rec.league = get_str(row, col_league, "Independent League");
        rec.short_code = get_str(row, col_short_code, club_name.substr(0, 3).to_upper());
        rec.country = get_str(row, col_country, "Unknown");
        rec.reputation = std::clamp(get_int(row, col_reputation, 50), 1, 100);
        rec.total_balance = get_i64(row, col_total_balance, 5000000);
        rec.transfer_budget = get_i64(row, col_transfer_budget, 1500000);
        rec.wage_budget = get_i64(row, col_wage_budget, 50000);
        rec.default_formation = get_str(row, col_formation, "4-4-2");
        rec.primary_color = get_str(row, col_primary_color, "#1E3A8A");
        rec.secondary_color = get_str(row, col_secondary_color, "#FFFFFF");

        const int idx = static_cast<int>(clubs.size());
        clubs.push_back(rec);
        club_index_by_name[rec.club_name.to_lower().utf8().get_data()] = idx;
        club_index_by_id[rec.club_id] = idx;

        const std::string league_key = rec.league.utf8().get_data();
        if (seen_leagues.insert(league_key).second) {
            league_order.push_back(rec.league);
        }
    }

    UtilityFunctions::print(vformat("PlayerDatabase: Loaded %d clubs across %d leagues from %s",
            static_cast<int>(clubs.size()), static_cast<int>(league_order.size()), csv_path));
    return true;
}

bool PlayerDatabase::load_players_from_csv(const String &csv_path) {
    if (!FileAccess::file_exists(csv_path)) {
        UtilityFunctions::push_error("PlayerDatabase::load_players_from_csv — File not found: " + csv_path);
        return false;
    }

    Ref<FileAccess> file = FileAccess::open(csv_path, FileAccess::READ);
    if (file.is_null()) {
        UtilityFunctions::push_error("PlayerDatabase::load_players_from_csv — Unable to open: " + csv_path);
        return false;
    }

    players.clear();
    player_index_by_id.clear();
    players_by_club_map.clear();

    if (file->eof_reached()) {
        return false;
    }

    const PackedStringArray headers = parse_csv_line(file->get_line());

    // Use -1 fallback for optional columns (PlayerID, Overall, Playstyle 1..3, Wage)
    // so positional fallbacks never collide with Passing or Name columns.
    const int col_id         = find_col(headers, { "playerid", "player_id", "id" }, -1);
    const int col_name       = find_col(headers, { "name", "playername", "player_name" }, 0);
    const int col_age        = find_col(headers, { "age" }, 1);
    const int col_debut_age  = find_col(headers, { "debutage", "debut_age" }, -1);
    const int col_pot        = find_col(headers, { "potential", "pot" }, -1);
    const int col_pos        = find_col(headers, { "position", "pos" }, -1);
    const int col_ovr        = find_col(headers, { "overall", "ovr" }, -1);
    const int col_nation     = find_col(headers, { "nationality", "country", "nation" }, -1);
    const int col_club       = find_col(headers, { "club", "clubname", "club_name" }, -1);
    const int col_league     = find_col(headers, { "league", "leaguename", "league_name" }, -1);
    const int col_playstyle  = find_col(headers, { "playstyle", "play_style", "style" }, -1);
    const int col_ps1        = find_col(headers, { "playstyle 1", "playstyle1", "playstyle_1" }, -1);
    const int col_ps2        = find_col(headers, { "playstyle 2", "playstyle2", "playstyle_2" }, -1);
    const int col_ps3        = find_col(headers, { "playstyle 3", "playstyle3", "playstyle_3" }, -1);
    const int col_price      = find_col(headers, { "price", "value", "marketvalue" }, -1);
    const int col_wage       = find_col(headers, { "wage", "salary" }, -1);
    const int col_stamina    = find_col(headers, { "stamina", "sta" }, -1);

    // Outfield attributes
    const int col_pace       = find_col(headers, { "pace", "pac" }, -1);
    const int col_shooting   = find_col(headers, { "shooting", "sho" }, -1);
    const int col_passing    = find_col(headers, { "passing", "pas" }, -1);
    const int col_dribbling  = find_col(headers, { "dribbling", "dri" }, -1);
    const int col_defending  = find_col(headers, { "defending", "def" }, -1);
    const int col_physical   = find_col(headers, { "physical", "phy" }, -1);

    // Goalkeeper attributes
    const int col_diving      = find_col(headers, { "diving", "gk_diving", "div" }, -1);
    const int col_reflex      = find_col(headers, { "reflex", "reflexes", "gk_reflex", "ref" }, -1);
    const int col_positioning = find_col(headers, { "positioning", "gk_positioning", "pos_gk" }, -1);
    const int col_reaction    = find_col(headers, { "reaction", "reactions", "gk_reaction", "rea" }, -1);
    const int col_kicking     = find_col(headers, { "kicking", "gk_kicking", "kic" }, -1);

    int auto_player_id = 1;

    while (!file->eof_reached()) {
        const String raw_line = file->get_line();
        if (raw_line.strip_edges().is_empty()) {
            continue;
        }

        const PackedStringArray row = parse_csv_line(raw_line);
        const String player_name = get_str(row, col_name);
        if (player_name.is_empty()) {
            continue;
        }

        PlayerRecord p;
        p.player_id = get_int(row, col_id, auto_player_id);
        auto_player_id = std::max(auto_player_id + 1, p.player_id + 1);
        p.name = player_name;
        p.age = get_int(row, col_age, 22);
        p.debut_age = get_int(row, col_debut_age, std::clamp(p.age - 3, 15, 22));
        p.nationality = get_str(row, col_nation, "Unknown");
        p.club = get_str(row, col_club, "Free Agent");
        p.league = get_str(row, col_league, "Free Agents");
        p.position = get_str(row, col_pos, "CM").to_upper();
        p.price = get_i64(row, col_price, 500000);
        p.wage = get_i64(row, col_wage, 5000);
        p.stamina = std::clamp(get_int(row, col_stamina, 75), 1, 99);

        // Parse Playstyles (single column or Playstyle 1..3)
        p.playstyles.clear();
        const int ps_cols[4] = { col_playstyle, col_ps1, col_ps2, col_ps3 };
        for (int c_idx : ps_cols) {
            if (c_idx >= 0) {
                const String ps_val = get_str(row, c_idx).strip_edges();
                if (!ps_val.is_empty() && ps_val.nocasecmp_to("None") != 0 && ps_val.nocasecmp_to("Balanced") != 0) {
                    p.playstyles.push_back(ps_val);
                }
            }
        }
        p.playstyle = p.playstyles.empty() ? String("Balanced") : p.playstyles[0];

        const int raw_ovr = get_int(row, col_ovr, 0);
        const int fallback_stat = raw_ovr > 0 ? raw_ovr : 65;

        const bool is_gk = (p.position == "GK");
        if (is_gk) {
            p.diving = std::clamp(get_int(row, col_diving, fallback_stat), 1, 99);
            p.reflex = std::clamp(get_int(row, col_reflex, fallback_stat), 1, 99);
            p.positioning = std::clamp(get_int(row, col_positioning, fallback_stat), 1, 99);
            p.reaction = std::clamp(get_int(row, col_reaction, fallback_stat), 1, 99);
            p.kicking = std::clamp(get_int(row, col_kicking, std::max(45, fallback_stat - 4)), 1, 99);
            p.physical = std::clamp(get_int(row, col_physical, fallback_stat), 1, 99);

            p.pace = 0;
            p.shooting = 0;
            p.passing = 0;
            p.dribbling = 0;
            p.defending = 0;
        } else {
            p.pace = std::clamp(get_int(row, col_pace, fallback_stat), 1, 99);
            p.shooting = std::clamp(get_int(row, col_shooting, fallback_stat), 1, 99);
            p.passing = std::clamp(get_int(row, col_passing, fallback_stat), 1, 99);
            p.dribbling = std::clamp(get_int(row, col_dribbling, fallback_stat), 1, 99);
            p.defending = std::clamp(get_int(row, col_defending, fallback_stat), 1, 99);
            p.physical = std::clamp(get_int(row, col_physical, fallback_stat), 1, 99);

            p.diving = 0;
            p.reflex = 0;
            p.positioning = 0;
            p.reaction = 0;
            p.kicking = 0;
        }

        p.overall = (raw_ovr > 0) ? std::clamp(raw_ovr, 1, 99) : compute_derived_ovr(p);
        p.potential = std::clamp(get_int(row, col_pot, p.overall), p.overall, 99);

        const int idx = static_cast<int>(players.size());
        players.push_back(p);
        player_index_by_id[p.player_id] = idx;
        players_by_club_map[p.club.to_lower().utf8().get_data()].push_back(idx);
    }

    for (auto &kv : players_by_club_map) {
        auto &indices = kv.second;
        std::sort(indices.begin(), indices.end(), [this](int a, int b) {
            const PlayerRecord &pa = players[a];
            const PlayerRecord &pb = players[b];
            const int rank_a = position_priority(pa.position);
            const int rank_b = position_priority(pb.position);
            if (rank_a != rank_b) {
                return rank_a < rank_b;
            }
            if (pa.overall != pb.overall) {
                return pa.overall > pb.overall;
            }
            return pa.name < pb.name;
        });
    }

    UtilityFunctions::print(vformat("PlayerDatabase: Loaded %d players from %s",
            static_cast<int>(players.size()), csv_path));
    return true;
}

void PlayerDatabase::sync_clubs_from_players() {
    int max_id = 1000;
    for (const auto &c : clubs) {
        max_id = std::max(max_id, c.club_id);
    }

    std::unordered_set<std::string> existing_leagues;
    for (const auto &l : league_order) {
        existing_leagues.insert(l.utf8().get_data());
    }

    for (const auto &p : players) {
        if (p.club.is_empty() || p.club.nocasecmp_to("Free Agent") == 0) {
            continue;
        }
        const std::string key = p.club.to_lower().utf8().get_data();
        if (club_index_by_name.find(key) == club_index_by_name.end()) {
            ClubRecord fallback;
            fallback.club_id = ++max_id;
            fallback.club_name = p.club;
            fallback.league = p.league.is_empty() ? String("Independent League") : p.league;
            fallback.short_code = p.club.substr(0, 3).to_upper();
            fallback.country = p.nationality;
            fallback.reputation = 55;
            fallback.total_balance = 5000000;
            fallback.transfer_budget = 1500000;
            fallback.wage_budget = 50000;
            fallback.default_formation = "4-4-2";
            fallback.primary_color = "#1F2937";
            fallback.secondary_color = "#FFFFFF";

            const int idx = static_cast<int>(clubs.size());
            clubs.push_back(fallback);
            club_index_by_name[key] = idx;
            club_index_by_id[fallback.club_id] = idx;

            const std::string l_key = fallback.league.utf8().get_data();
            if (existing_leagues.insert(l_key).second) {
                league_order.push_back(fallback.league);
            }
        }
    }
}

Dictionary PlayerDatabase::club_to_dict(const ClubRecord &rec) const {
    Dictionary d;
    d["club_id"] = rec.club_id;
    d["club"] = rec.club_name;
    d["league"] = rec.league;
    d["short_code"] = rec.short_code;
    d["country"] = rec.country;
    d["reputation"] = rec.reputation;
    d["total_balance"] = rec.total_balance;
    d["transfer_budget"] = rec.transfer_budget;
    d["wage_budget"] = rec.wage_budget;
    d["default_formation"] = rec.default_formation;
    d["primary_color"] = rec.primary_color;
    d["secondary_color"] = rec.secondary_color;
    return d;
}

Dictionary PlayerDatabase::player_to_dict(const PlayerRecord &p) const {
    Dictionary d;
    d["player_id"] = p.player_id;
    d["name"] = p.name;
    d["age"] = p.age;
    d["debut_age"] = p.debut_age;
    d["nationality"] = p.nationality;
    d["club"] = p.club;
    d["league"] = p.league;
    d["position"] = p.position;
    d["overall"] = p.overall;
    d["potential"] = p.potential;
    d["playstyle"] = p.playstyle;
    Array ps_arr;
    for (const auto &ps : p.playstyles) {
        ps_arr.append(ps);
    }
    d["playstyles"] = ps_arr;
    d["price"] = p.price;
    d["wage"] = p.wage;
    d["stamina"] = p.stamina;

    const bool is_gk = (p.position == "GK");
    d["is_gk"] = is_gk;
    if (is_gk) {
        d["diving"] = p.diving;
        d["reflex"] = p.reflex;
        d["positioning"] = p.positioning;
        d["reaction"] = p.reaction;
        d["kicking"] = p.kicking;
        d["physical"] = p.physical;
    } else {
        d["pace"] = p.pace;
        d["shooting"] = p.shooting;
        d["passing"] = p.passing;
        d["dribbling"] = p.dribbling;
        d["defending"] = p.defending;
        d["physical"] = p.physical;
    }
    return d;
}

Array PlayerDatabase::get_all_leagues() const {
    Array out;
    for (const auto &league : league_order) {
        out.append(league);
    }
    return out;
}

int PlayerDatabase::get_league_count() const {
    return static_cast<int>(league_order.size());
}

Array PlayerDatabase::get_clubs_by_league(const String &league_name) const {
    std::vector<ClubRecord> matching;
    for (const auto &c : clubs) {
        if (c.league.nocasecmp_to(league_name.strip_edges()) == 0) {
            matching.push_back(c);
        }
    }

    std::sort(matching.begin(), matching.end(), [](const ClubRecord &a, const ClubRecord &b) {
        if (a.reputation != b.reputation) {
            return a.reputation > b.reputation;
        }
        return a.club_name < b.club_name;
    });

    Array out;
    for (const auto &c : matching) {
        out.append(club_to_dict(c));
    }
    return out;
}

Dictionary PlayerDatabase::get_club_record(const String &club_name) const {
    const std::string key = club_name.strip_edges().to_lower().utf8().get_data();
    auto it = club_index_by_name.find(key);
    if (it == club_index_by_name.end()) {
        return Dictionary();
    }
    return club_to_dict(clubs[it->second]);
}

Dictionary PlayerDatabase::get_club_by_id(int club_id) const {
    auto it = club_index_by_id.find(club_id);
    if (it == club_index_by_id.end()) {
        return Dictionary();
    }
    return club_to_dict(clubs[it->second]);
}

Dictionary PlayerDatabase::get_club(const String &p_name) const {
    return get_club_record(p_name);
}

Array PlayerDatabase::get_all_clubs() const {
    Array out;
    for (const auto &c : clubs) {
        out.append(club_to_dict(c));
    }
    return out;
}

Array PlayerDatabase::get_players_by_club(const String &club_name) const {
    const std::string key = club_name.strip_edges().to_lower().utf8().get_data();
    auto it = players_by_club_map.find(key);
    if (it == players_by_club_map.end()) {
        return Array();
    }

    Array out;
    for (int idx : it->second) {
        out.append(player_to_dict(players[idx]));
    }
    return out;
}

Dictionary PlayerDatabase::get_player_by_id(int player_id) const {
    auto it = player_index_by_id.find(player_id);
    if (it == player_index_by_id.end()) {
        return Dictionary();
    }
    return player_to_dict(players[it->second]);
}

Dictionary PlayerDatabase::get_player(const String &p_name) const {
    const String target = p_name.strip_edges();
    for (const auto &p : players) {
        if (p.name.nocasecmp_to(target) == 0) {
            return player_to_dict(p);
        }
    }
    return Dictionary();
}

Dictionary PlayerDatabase::get_player_by_name(const String &p_name) const {
    return get_player(p_name);
}

Array PlayerDatabase::get_all_players() const {
    Array out;
    for (const auto &p : players) {
        out.append(player_to_dict(p));
    }
    return out;
}

int PlayerDatabase::get_total_club_count() const {
    return static_cast<int>(clubs.size());
}

int PlayerDatabase::get_club_count() const {
    return get_total_club_count();
}

int PlayerDatabase::get_total_player_count() const {
    return static_cast<int>(players.size());
}

int PlayerDatabase::get_player_count() const {
    return get_total_player_count();
}