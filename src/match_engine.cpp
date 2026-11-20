#include "match_engine.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <unordered_set>
#include <algorithm>
#include <cmath>

using namespace godot;

void MatchEngine::_bind_methods() {
    // Control Methods
    ClassDB::bind_method(D_METHOD("start_match"), &MatchEngine::start_match);
    ClassDB::bind_method(D_METHOD("pause_match", "pause"), &MatchEngine::pause_match);
    ClassDB::bind_method(D_METHOD("resolve_manual_scenario", "success", "is_home_team"), &MatchEngine::resolve_manual_scenario, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("begin_chance", "is_home_attacking", "attacker_idx", "xg"), &MatchEngine::begin_chance, DEFVAL(-1), DEFVAL(0.25f));

    // Post-Goal Kickoff Resume Methods
    ClassDB::bind_method(D_METHOD("is_waiting_for_kickoff_resume"), &MatchEngine::is_waiting_for_kickoff_resume);
    ClassDB::bind_method(D_METHOD("get_kickoff_team"), &MatchEngine::get_kickoff_team);
    ClassDB::bind_method(D_METHOD("resume_from_goal_kickoff"), &MatchEngine::resume_from_goal_kickoff);

    // Direct Player Control & Defensive Chance Pacing/Camera Methods
    ClassDB::bind_method(D_METHOD("get_chance_state"), &MatchEngine::get_chance_state);
    ClassDB::bind_method(D_METHOD("is_chance_active"), &MatchEngine::is_chance_active);
    ClassDB::bind_method(D_METHOD("is_manual_control_active"), &MatchEngine::is_manual_control_active);
    ClassDB::bind_method(D_METHOD("is_manual_dribble_active"), &MatchEngine::is_manual_dribble_active);
    ClassDB::bind_method(D_METHOD("get_controlled_player_index"), &MatchEngine::get_controlled_player_index);
    ClassDB::bind_method(D_METHOD("set_controlled_player", "player_idx"), &MatchEngine::set_controlled_player);
    ClassDB::bind_method(D_METHOD("select_nearest_defender"), &MatchEngine::select_nearest_defender);
    ClassDB::bind_method(D_METHOD("get_valid_pass_targets"), &MatchEngine::get_valid_pass_targets);

    ClassDB::bind_method(D_METHOD("set_defensive_chance_time_scale", "scale"), &MatchEngine::set_defensive_chance_time_scale);
    ClassDB::bind_method(D_METHOD("get_defensive_chance_time_scale"), &MatchEngine::get_defensive_chance_time_scale);
    ClassDB::bind_method(D_METHOD("get_recommended_camera_zoom"), &MatchEngine::get_recommended_camera_zoom);
    ClassDB::bind_method(D_METHOD("get_chance_camera_focus"), &MatchEngine::get_chance_camera_focus);
    ClassDB::bind_method(D_METHOD("get_chance_framing_rect"), &MatchEngine::get_chance_framing_rect);

    ClassDB::bind_method(D_METHOD("command_move_player", "target_pos"), &MatchEngine::command_move_player);
    ClassDB::bind_method(D_METHOD("command_pass", "target_player_idx"), &MatchEngine::command_pass);
    ClassDB::bind_method(D_METHOD("command_through_ball", "direction_or_target"), &MatchEngine::command_through_ball);
    ClassDB::bind_method(D_METHOD("command_shot", "aim_target", "power_ratio"), &MatchEngine::command_shot, DEFVAL(0.75f));
    ClassDB::bind_method(D_METHOD("command_tackle"), &MatchEngine::command_tackle);

    // Skip Match API Methods
    ClassDB::bind_method(D_METHOD("can_skip_match"), &MatchEngine::can_skip_match);
    ClassDB::bind_method(D_METHOD("skip_match"), &MatchEngine::skip_match);
    ClassDB::bind_method(D_METHOD("get_skip_tokens"), &MatchEngine::get_skip_tokens);
    ClassDB::bind_method(D_METHOD("set_skip_tokens", "tokens"), &MatchEngine::set_skip_tokens);
    ClassDB::bind_method(D_METHOD("get_manual_matches_played"), &MatchEngine::get_manual_matches_played);

    // Formation & Field Methods
    ClassDB::bind_method(D_METHOD("load_formations_from_csv", "filepath"), &MatchEngine::load_formations_from_csv);
    ClassDB::bind_method(D_METHOD("load_default_formations"), &MatchEngine::load_default_formations);
    ClassDB::bind_method(D_METHOD("is_formations_loaded"), &MatchEngine::is_formations_loaded);
    ClassDB::bind_method(D_METHOD("get_loaded_formations_path"), &MatchEngine::get_loaded_formations_path);
    ClassDB::bind_method(D_METHOD("has_formation", "formation_name"), &MatchEngine::has_formation);
    ClassDB::bind_method(D_METHOD("get_available_formations"), &MatchEngine::get_available_formations);
    ClassDB::bind_method(D_METHOD("set_home_formation", "formation_name"), &MatchEngine::set_home_formation);
    ClassDB::bind_method(D_METHOD("set_away_formation", "formation_name"), &MatchEngine::set_away_formation);
    ClassDB::bind_method(D_METHOD("get_home_formation"), &MatchEngine::get_home_formation);
    ClassDB::bind_method(D_METHOD("get_away_formation"), &MatchEngine::get_away_formation);
    ClassDB::bind_method(D_METHOD("set_field_size", "size"), &MatchEngine::set_field_size);
    ClassDB::bind_method(D_METHOD("get_field_size"), &MatchEngine::get_field_size);

    // Team OVR & Squad Lineup Methods
    ClassDB::bind_method(D_METHOD("set_home_team_ovr", "ovr"), &MatchEngine::set_home_team_ovr);
    ClassDB::bind_method(D_METHOD("get_home_team_ovr"), &MatchEngine::get_home_team_ovr);
    ClassDB::bind_method(D_METHOD("set_away_team_ovr", "ovr"), &MatchEngine::set_away_team_ovr);
    ClassDB::bind_method(D_METHOD("get_away_team_ovr"), &MatchEngine::get_away_team_ovr);
    ClassDB::bind_method(D_METHOD("set_home_lineup", "players"), &MatchEngine::set_home_lineup);
    ClassDB::bind_method(D_METHOD("set_away_lineup", "players"), &MatchEngine::set_away_lineup);

    // Tactical Instructions Methods
    ClassDB::bind_method(D_METHOD("set_home_mentality", "mentality"), &MatchEngine::set_home_mentality);
    ClassDB::bind_method(D_METHOD("get_home_mentality"), &MatchEngine::get_home_mentality);
    ClassDB::bind_method(D_METHOD("set_away_mentality", "mentality"), &MatchEngine::set_away_mentality);
    ClassDB::bind_method(D_METHOD("get_away_mentality"), &MatchEngine::get_away_mentality);

    ClassDB::bind_method(D_METHOD("set_home_pressing", "pressing"), &MatchEngine::set_home_pressing);
    ClassDB::bind_method(D_METHOD("get_home_pressing"), &MatchEngine::get_home_pressing);
    ClassDB::bind_method(D_METHOD("set_away_pressing", "pressing"), &MatchEngine::set_away_pressing);
    ClassDB::bind_method(D_METHOD("get_away_pressing"), &MatchEngine::get_away_pressing);

    ClassDB::bind_method(D_METHOD("set_home_defensive_line", "line"), &MatchEngine::set_home_defensive_line);
    ClassDB::bind_method(D_METHOD("get_home_defensive_line"), &MatchEngine::get_home_defensive_line);
    ClassDB::bind_method(D_METHOD("set_away_defensive_line", "line"), &MatchEngine::set_away_defensive_line);
    ClassDB::bind_method(D_METHOD("get_away_defensive_line"), &MatchEngine::get_away_defensive_line);

    ClassDB::bind_method(D_METHOD("set_home_passing_style", "style"), &MatchEngine::set_home_passing_style);
    ClassDB::bind_method(D_METHOD("get_home_passing_style"), &MatchEngine::get_home_passing_style);
    ClassDB::bind_method(D_METHOD("set_away_passing_style", "style"), &MatchEngine::set_away_passing_style);
    ClassDB::bind_method(D_METHOD("get_away_passing_style"), &MatchEngine::get_away_passing_style);

    ClassDB::bind_method(D_METHOD("set_home_attacking_width", "width"), &MatchEngine::set_home_attacking_width);
    ClassDB::bind_method(D_METHOD("get_home_attacking_width"), &MatchEngine::get_home_attacking_width);
    ClassDB::bind_method(D_METHOD("set_away_attacking_width", "width"), &MatchEngine::set_away_attacking_width);
    ClassDB::bind_method(D_METHOD("get_away_attacking_width"), &MatchEngine::get_away_attacking_width);

    ClassDB::bind_method(D_METHOD("set_home_tempo", "tempo"), &MatchEngine::set_home_tempo);
    ClassDB::bind_method(D_METHOD("get_home_tempo"), &MatchEngine::get_home_tempo);
    ClassDB::bind_method(D_METHOD("set_away_tempo", "tempo"), &MatchEngine::set_away_tempo);
    ClassDB::bind_method(D_METHOD("get_away_tempo"), &MatchEngine::get_away_tempo);

    ClassDB::bind_method(D_METHOD("set_home_defensive_approach", "approach"), &MatchEngine::set_home_defensive_approach);
    ClassDB::bind_method(D_METHOD("get_home_defensive_approach"), &MatchEngine::get_home_defensive_approach);
    ClassDB::bind_method(D_METHOD("set_away_defensive_approach", "approach"), &MatchEngine::set_away_defensive_approach);
    ClassDB::bind_method(D_METHOD("get_away_defensive_approach"), &MatchEngine::get_away_defensive_approach);

    // Getters & Telemetry
    ClassDB::bind_method(D_METHOD("get_home_score"), &MatchEngine::get_home_score);
    ClassDB::bind_method(D_METHOD("get_away_score"), &MatchEngine::get_away_score);
    ClassDB::bind_method(D_METHOD("get_current_minute"), &MatchEngine::get_current_minute);
    ClassDB::bind_method(D_METHOD("is_match_paused"), &MatchEngine::is_match_paused);
    ClassDB::bind_method(D_METHOD("is_match_active"), &MatchEngine::is_match_active);
    ClassDB::bind_method(D_METHOD("get_possession_team"), &MatchEngine::get_possession_team);
    ClassDB::bind_method(D_METHOD("get_ball_carrier_index"), &MatchEngine::get_ball_carrier_index);
    ClassDB::bind_method(D_METHOD("get_pass_target_index"), &MatchEngine::get_pass_target_index);
    ClassDB::bind_method(D_METHOD("get_player_positions"), &MatchEngine::get_player_positions);
    ClassDB::bind_method(D_METHOD("get_player_roles"), &MatchEngine::get_player_roles);
    ClassDB::bind_method(D_METHOD("get_player_names"), &MatchEngine::get_player_names);
    ClassDB::bind_method(D_METHOD("get_player_stamina"), &MatchEngine::get_player_stamina);
    ClassDB::bind_method(D_METHOD("get_ball_position"), &MatchEngine::get_ball_position);
    ClassDB::bind_method(D_METHOD("get_match_stats"), &MatchEngine::get_match_stats);

    // Live-Position Chance & Manual Shooting Controls
    ClassDB::bind_method(D_METHOD("set_home_auto_shoot_enabled", "enabled"), &MatchEngine::set_home_auto_shoot_enabled);
    ClassDB::bind_method(D_METHOD("is_home_auto_shoot_enabled"), &MatchEngine::is_home_auto_shoot_enabled);
    ClassDB::bind_method(D_METHOD("trigger_wide_range_chance", "is_home_attacking"), &MatchEngine::trigger_wide_range_chance);

    // Inspector Properties
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "home_formation"), "set_home_formation", "get_home_formation");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "away_formation"), "set_away_formation", "get_away_formation");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "field_size"), "set_field_size", "get_field_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "home_team_ovr"), "set_home_team_ovr", "get_home_team_ovr");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "away_team_ovr"), "set_away_team_ovr", "get_away_team_ovr");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "skip_tokens"), "set_skip_tokens", "get_skip_tokens");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "defensive_chance_time_scale"), "set_defensive_chance_time_scale", "get_defensive_chance_time_scale");

    // Signals
    ADD_SIGNAL(MethodInfo("interactive_event_triggered",
        PropertyInfo(Variant::INT, "scenario_type"),
        PropertyInfo(Variant::BOOL, "is_home_team"),
        PropertyInfo(Variant::VECTOR3, "ball_spawn_3d")));

    ADD_SIGNAL(MethodInfo("chance_state_changed",
        PropertyInfo(Variant::INT, "chance_state"),
        PropertyInfo(Variant::INT, "controlled_player_index")));
    ADD_SIGNAL(MethodInfo("controlled_player_changed",
        PropertyInfo(Variant::INT, "controlled_player_index")));

    ADD_SIGNAL(MethodInfo("goal_scored",
        PropertyInfo(Variant::INT, "home_score"),
        PropertyInfo(Variant::INT, "away_score"),
        PropertyInfo(Variant::BOOL, "is_home_team")));

    ADD_SIGNAL(MethodInfo("shot_hit_crossbar",
        PropertyInfo(Variant::BOOL, "is_home_team"),
        PropertyInfo(Variant::VECTOR2, "impact_pos")));

    ADD_SIGNAL(MethodInfo("goal_kickoff_ready",
        PropertyInfo(Variant::INT, "kickoff_team"),
        PropertyInfo(Variant::STRING, "prompt_text")));

    ADD_SIGNAL(MethodInfo("goal_kickoff_resumed",
        PropertyInfo(Variant::INT, "kickoff_team")));

    ADD_SIGNAL(MethodInfo("minute_changed",
        PropertyInfo(Variant::INT, "minute")));

    ADD_SIGNAL(MethodInfo("match_ended",
        PropertyInfo(Variant::INT, "final_home_score"),
        PropertyInfo(Variant::INT, "final_away_score")));

    ADD_SIGNAL(MethodInfo("match_event_logged",
        PropertyInfo(Variant::INT, "minute"),
        PropertyInfo(Variant::STRING, "event_type"),
        PropertyInfo(Variant::BOOL, "is_home_team"),
        PropertyInfo(Variant::STRING, "player_name"),
        PropertyInfo(Variant::STRING, "description")));
}

MatchEngine::MatchEngine() {
    _initialize_default_squads();
}

MatchEngine::~MatchEngine() {}

void MatchEngine::_ready() {
    set_physics_process(false);
    load_default_formations();
    _reset_field_positions();
}

float MatchEngine::_get_fatigue_multiplier(const SimPlayer& p_player) const {
    float stamina_ratio = std::clamp(p_player.stamina / 100.0f, 0.0f, 1.0f);
    return 0.62f + (0.38f * stamina_ratio);
}

bool MatchEngine::_is_backline_role(const String& p_role) const {
    return (p_role == "CB" || p_role == "LB" || p_role == "RB" || p_role == "LWB" || p_role == "RWB");
}

void MatchEngine::_initialize_default_squads() {
    sim_players.resize(22);
    for (int i = 0; i < 22; i++) {
        bool is_home = (i < 11);
        int local_idx = is_home ? i : (i - 11);
        SimPlayer& p = sim_players[i];
        p.is_home = is_home;
        p.slot_index = i;
        p.name = (is_home ? String("Home #") : String("Away #")) + String::num_int64(local_idx + 1);
        p.role = (local_idx == 0) ? String("GK") : String("CM");
        p.stamina = 100.0f;
        p.recovery_timer = 0.0f;
        p.sprint_boost_timer = 0.0f;
        p.dribble_boost_timer = 0.0f;
        p.velocity = Vector2(0.0f, 0.0f);
        _apply_role_profile(p, is_home ? home_team_ovr : away_team_ovr);
    }
}

void MatchEngine::_apply_role_profile(SimPlayer& p_player, int p_base_ovr) const {
    int base = std::clamp(p_base_ovr, 1, 99);
    String r = p_player.role.to_upper();

    if (r == "GK") {
        p_player.pace = std::clamp(base - 18, 1, 99);
        p_player.shooting = std::clamp(base - 25, 1, 99);
        p_player.passing = std::clamp(base - 6, 1, 99);
        p_player.dribbling = std::clamp(base - 15, 1, 99);
        p_player.defending = std::clamp(base + 4, 1, 99);
        p_player.physical = std::clamp(base + 2, 1, 99);
    } else if (r == "CB") {
        p_player.pace = std::clamp(base - 5, 1, 99);
        p_player.shooting = std::clamp(base - 20, 1, 99);
        p_player.passing = std::clamp(base - 4, 1, 99);
        p_player.dribbling = std::clamp(base - 8, 1, 99);
        p_player.defending = std::clamp(base + 5, 1, 99);
        p_player.physical = std::clamp(base + 4, 1, 99);
    } else if (r == "LB" || r == "RB" || r == "LWB" || r == "RWB") {
        p_player.pace = std::clamp(base + 4, 1, 99);
        p_player.shooting = std::clamp(base - 12, 1, 99);
        p_player.passing = std::clamp(base, 1, 99);
        p_player.dribbling = std::clamp(base, 1, 99);
        p_player.defending = std::clamp(base + 2, 1, 99);
        p_player.physical = std::clamp(base, 1, 99);
    } else if (r == "CDM" || r == "CM" || r == "CAM") {
        p_player.pace = std::clamp(base - 2, 1, 99);
        p_player.shooting = std::clamp(base - 3, 1, 99);
        p_player.passing = std::clamp(base + 5, 1, 99);
        p_player.dribbling = std::clamp(base + 3, 1, 99);
        p_player.defending = (r == "CDM") ? std::clamp(base + 3, 1, 99) : std::clamp(base - 5, 1, 99);
        p_player.physical = std::clamp(base, 1, 99);
    } else if (r == "LW" || r == "RW" || r == "LM" || r == "RM") {
        p_player.pace = std::clamp(base + 6, 1, 99);
        p_player.shooting = std::clamp(base + 1, 1, 99);
        p_player.passing = std::clamp(base + 2, 1, 99);
        p_player.dribbling = std::clamp(base + 5, 1, 99);
        p_player.defending = std::clamp(base - 18, 1, 99);
        p_player.physical = std::clamp(base - 4, 1, 99);
    } else {
        p_player.pace = std::clamp(base + 3, 1, 99);
        p_player.shooting = std::clamp(base + 6, 1, 99);
        p_player.passing = std::clamp(base - 4, 1, 99);
        p_player.dribbling = std::clamp(base + 2, 1, 99);
        p_player.defending = std::clamp(base - 25, 1, 99);
        p_player.physical = std::clamp(base + 3, 1, 99);
    }
}

void MatchEngine::set_defensive_chance_time_scale(float p_scale) {
    defensive_chance_time_scale = std::clamp(p_scale, 0.45f, 1.0f);
}

void MatchEngine::set_skip_tokens(int p_tokens) {
    skip_tokens = std::max(0, p_tokens);
}

void MatchEngine::set_field_size(Vector2 p_size) {
    field_size.x = std::max(200.0f, p_size.x);
    field_size.y = std::max(150.0f, p_size.y);
    _apply_formation_positions(!match_active);
}

void MatchEngine::set_home_team_ovr(int p_ovr) {
    home_team_ovr = std::clamp(p_ovr, 1, 99);
    for (int i = 0; i < 11 && i < static_cast<int>(sim_players.size()); i++) {
        _apply_role_profile(sim_players[i], home_team_ovr);
    }
}

void MatchEngine::set_away_team_ovr(int p_ovr) {
    away_team_ovr = std::clamp(p_ovr, 1, 99);
    for (int i = 11; i < 22 && i < static_cast<int>(sim_players.size()); i++) {
        _apply_role_profile(sim_players[i], away_team_ovr);
    }
}

void MatchEngine::set_home_lineup(const Array& p_players) {
    if (sim_players.size() != 22) {
        _initialize_default_squads();
    }
    int limit = std::min(11, static_cast<int>(p_players.size()));
    for (int i = 0; i < limit; i++) {
        if (p_players[i].get_type() != Variant::DICTIONARY) {
            continue;
        }
        Dictionary d = p_players[i];
        SimPlayer& p = sim_players[i];
        if (d.has("name")) {
            String player_name = String(d["name"]).strip_edges();
            if (!player_name.is_empty()) {
                p.name = player_name;
            }
        }
        if (d.has("position")) p.role = String(d["position"]).strip_edges().to_upper();
        if (d.has("pace")) p.pace = std::clamp(static_cast<int>(d["pace"]), 1, 99);
        if (d.has("shooting")) p.shooting = std::clamp(static_cast<int>(d["shooting"]), 1, 99);
        if (d.has("passing")) p.passing = std::clamp(static_cast<int>(d["passing"]), 1, 99);
        if (d.has("dribbling")) p.dribbling = std::clamp(static_cast<int>(d["dribbling"]), 1, 99);
        if (d.has("defending")) p.defending = std::clamp(static_cast<int>(d["defending"]), 1, 99);
        if (d.has("physical")) p.physical = std::clamp(static_cast<int>(d["physical"]), 1, 99);
        if (d.has("stamina")) {
            p.starting_stamina = std::clamp(static_cast<float>(d["stamina"]), 10.0f, 100.0f);
            p.stamina = p.starting_stamina;
        }
    }
}

void MatchEngine::set_away_lineup(const Array& p_players) {
    if (sim_players.size() != 22) {
        _initialize_default_squads();
    }
    int limit = std::min(11, static_cast<int>(p_players.size()));
    for (int i = 0; i < limit; i++) {
        if (p_players[i].get_type() != Variant::DICTIONARY) {
            continue;
        }
        Dictionary d = p_players[i];
        SimPlayer& p = sim_players[11 + i];
        if (d.has("name")) {
            String player_name = String(d["name"]).strip_edges();
            if (!player_name.is_empty()) {
                p.name = player_name;
            }
        }
        if (d.has("position")) p.role = String(d["position"]).strip_edges().to_upper();
        if (d.has("pace")) p.pace = std::clamp(static_cast<int>(d["pace"]), 1, 99);
        if (d.has("shooting")) p.shooting = std::clamp(static_cast<int>(d["shooting"]), 1, 99);
        if (d.has("passing")) p.passing = std::clamp(static_cast<int>(d["passing"]), 1, 99);
        if (d.has("dribbling")) p.dribbling = std::clamp(static_cast<int>(d["dribbling"]), 1, 99);
        if (d.has("defending")) p.defending = std::clamp(static_cast<int>(d["defending"]), 1, 99);
        if (d.has("physical")) p.physical = std::clamp(static_cast<int>(d["physical"]), 1, 99);
        if (d.has("stamina")) {
            p.starting_stamina = std::clamp(static_cast<float>(d["stamina"]), 10.0f, 100.0f);
            p.stamina = p.starting_stamina;
        }
    }
}

void MatchEngine::set_home_mentality(int p_mentality) {
    home_mentality = std::clamp(p_mentality, 0, 4);
}

void MatchEngine::set_away_mentality(int p_mentality) {
    away_mentality = std::clamp(p_mentality, 0, 4);
}

void MatchEngine::set_home_pressing(int p_pressing) {
    home_pressing = std::clamp(p_pressing, 0, 2);
}

void MatchEngine::set_away_pressing(int p_pressing) {
    away_pressing = std::clamp(p_pressing, 0, 2);
}

void MatchEngine::set_home_defensive_line(int p_line) {
    home_defensive_line = std::clamp(p_line, 0, 2);
}

void MatchEngine::set_away_defensive_line(int p_line) {
    away_defensive_line = std::clamp(p_line, 0, 2);
}

void MatchEngine::set_home_passing_style(int p_style) {
    home_passing_style = std::clamp(p_style, 0, 2);
}

void MatchEngine::set_away_passing_style(int p_style) {
    away_passing_style = std::clamp(p_style, 0, 2);
}

void MatchEngine::set_home_attacking_width(int p_width) {
    home_attacking_width = std::clamp(p_width, 0, 2);
}

void MatchEngine::set_away_attacking_width(int p_width) {
    away_attacking_width = std::clamp(p_width, 0, 2);
}

void MatchEngine::set_home_tempo(int p_tempo) {
    home_tempo = std::clamp(p_tempo, 0, 2);
}

void MatchEngine::set_away_tempo(int p_tempo) {
    away_tempo = std::clamp(p_tempo, 0, 2);
}

void MatchEngine::set_home_defensive_approach(int p_approach) {
    home_defensive_approach = std::clamp(p_approach, 0, 2);
}

void MatchEngine::set_away_defensive_approach(int p_approach) {
    away_defensive_approach = std::clamp(p_approach, 0, 2);
}

bool MatchEngine::can_skip_match() const {
    return skip_tokens > 0 && match_active && !is_paused_for_manual && !waiting_for_kickoff_resume && chance_state == CHANCE_NONE;
}

bool MatchEngine::skip_match() {
    if (!can_skip_match()) {
        UtilityFunctions::printerr("MatchEngine: Cannot skip match — insufficient tokens or invalid match state.");
        return false;
    }

    skip_tokens--;

    float ovr_diff = static_cast<float>(home_team_ovr - away_team_ovr);
    float home_exp = 1.35f + (ovr_diff * 0.05f);
    float away_exp = 1.10f - (ovr_diff * 0.05f);

    float home_luck = static_cast<float>(UtilityFunctions::randf_range(-1.25, 1.25));
    float away_luck = static_cast<float>(UtilityFunctions::randf_range(-1.25, 1.25));

    home_score = std::max(0, static_cast<int>(std::round(home_exp + home_luck)));
    away_score = std::max(0, static_cast<int>(std::round(away_exp + away_luck)));

    float poss_jitter = static_cast<float>(UtilityFunctions::randf_range(-0.05, 0.05));
    float home_poss_share = std::clamp(0.50f + (ovr_diff * 0.006f) + poss_jitter, 0.30f, 0.70f);
    stats.home_possession_time = 90.0f * home_poss_share;
    stats.away_possession_time = 90.0f * (1.0f - home_poss_share);

    stats.home_shots = std::max(home_score + 2, static_cast<int>(std::round(9.0f + ovr_diff * 0.25f + static_cast<float>(UtilityFunctions::randf_range(-2.0, 4.0)))));
    stats.away_shots = std::max(away_score + 2, static_cast<int>(std::round(8.0f - ovr_diff * 0.25f + static_cast<float>(UtilityFunctions::randf_range(-2.0, 4.0)))));
    stats.home_shots_on_target = std::clamp(home_score + static_cast<int>(stats.home_shots * 0.35f), home_score, stats.home_shots);
    stats.away_shots_on_target = std::clamp(away_score + static_cast<int>(stats.away_shots * 0.35f), away_score, stats.away_shots);

    float home_xg_jitter = static_cast<float>(UtilityFunctions::randf_range(0.0, 0.45));
    float away_xg_jitter = static_cast<float>(UtilityFunctions::randf_range(0.0, 0.45));
    stats.home_xg = std::max(home_score * 0.45f, stats.home_shots * 0.11f + home_xg_jitter);
    stats.away_xg = std::max(away_score * 0.45f, stats.away_shots * 0.11f + away_xg_jitter);

    stats.home_passes = static_cast<int>(420.0f * home_poss_share * 2.0f);
    stats.away_passes = static_cast<int>(420.0f * (1.0f - home_poss_share) * 2.0f);
    stats.home_passes_completed = static_cast<int>(stats.home_passes * std::clamp(0.78f + home_team_ovr * 0.0012f, 0.65f, 0.92f));
    stats.away_passes_completed = static_cast<int>(stats.away_passes * std::clamp(0.78f + away_team_ovr * 0.0012f, 0.65f, 0.92f));

    stats.away_saves = std::max(0, stats.home_shots_on_target - home_score);
    stats.home_saves = std::max(0, stats.away_shots_on_target - away_score);
    stats.home_tackles = UtilityFunctions::randi_range(10, 22);
    stats.away_tackles = UtilityFunctions::randi_range(10, 22);
    stats.home_interceptions = UtilityFunctions::randi_range(7, 16);
    stats.away_interceptions = UtilityFunctions::randi_range(7, 16);
    stats.home_corners = UtilityFunctions::randi_range(2, 8);
    stats.away_corners = UtilityFunctions::randi_range(2, 8);

    chance_state = CHANCE_NONE;
    manual_control_active = false;
    user_controlled_index = -1;
    has_user_move_target = false;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    waiting_for_kickoff_resume = false;
    kickoff_team = -1;
    shot_hits_crossbar = false;
    current_minute = 90;
    match_active = false;
    is_paused_for_manual = false;
    set_physics_process(false);

    UtilityFunctions::print("MatchEngine: Match Skipped! Score: ", home_score, " - ", away_score, " | Remaining Tokens: ", skip_tokens);

    emit_signal("minute_changed", 90);
    emit_signal("match_ended", home_score, away_score);
    return true;
}

void MatchEngine::record_manual_match_completed() {
    manual_matches_played++;
    if (manual_matches_played % 5 == 0) {
        skip_tokens++;
        UtilityFunctions::print("MatchEngine: Reward earned (+1 Skip Token). Total tokens: ", skip_tokens);
    }
}

bool MatchEngine::load_default_formations() {
    if (FileAccess::file_exists(PRIMARY_FORMATIONS_PATH)) {
        if (load_formations_from_csv(PRIMARY_FORMATIONS_PATH)) {
            return true;
        }
    }

    if (FileAccess::file_exists(FALLBACK_FORMATIONS_PATH)) {
        UtilityFunctions::push_warning("MatchEngine: Primary formation file missing at '", PRIMARY_FORMATIONS_PATH,
            "'. Using workspace file at '", FALLBACK_FORMATIONS_PATH, "'. Copy it to demo/data/formations.csv for exports.");
        if (load_formations_from_csv(FALLBACK_FORMATIONS_PATH)) {
            return true;
        }
    }

    UtilityFunctions::printerr("MatchEngine: CRITICAL — Failed to load formations.csv from '",
        PRIMARY_FORMATIONS_PATH, "' or '", FALLBACK_FORMATIONS_PATH, "'.");
    return false;
}

bool MatchEngine::load_formations_from_csv(const String& p_filepath) {
    if (!p_filepath.begins_with("res://") && !p_filepath.begins_with("user://")) {
        UtilityFunctions::printerr("MatchEngine: Formation path must use res:// or user:// — rejected: ", p_filepath);
        return false;
    }

    Ref<FileAccess> file = FileAccess::open(p_filepath, FileAccess::READ);
    if (file.is_null()) {
        UtilityFunctions::printerr("MatchEngine: Failed to open formations CSV at: ", p_filepath);
        return false;
    }

    formation_db.clear();
    loaded_formations_path = "";

    if (file->eof_reached()) {
        UtilityFunctions::printerr("MatchEngine: Formations CSV is empty (0 bytes): ", p_filepath);
        return false;
    }

    PackedStringArray header = file->get_csv_line();
    if (header.size() < 4) {
        UtilityFunctions::printerr("MatchEngine: Invalid formations CSV header in ", p_filepath, " (expected 4 columns, got ", header.size(), ").");
        return false;
    }

    String h2 = header[2].strip_edges();
    String h3 = header[3].strip_edges();
    if (h2.is_valid_float() || h3.is_valid_float()) {
        UtilityFunctions::printerr("MatchEngine: Missing header row in ", p_filepath, " (expected column names like Formation,Position,NormX,NormY).");
        return false;
    }

    std::unordered_set<std::string> corrupted_formations;
    int line_number = 1;

    while (!file->eof_reached()) {
        line_number++;
        PackedStringArray row = file->get_csv_line();

        if (row.size() == 1 && row[0].strip_edges().is_empty()) {
            continue;
        }

        if (row.size() < 4) {
            UtilityFunctions::printerr("MatchEngine: Malformed row at line ", line_number, " in ", p_filepath, " (expected 4 columns, got ", row.size(), ").");
            if (row.size() >= 1 && !row[0].strip_edges().is_empty()) {
                corrupted_formations.insert(row[0].strip_edges().utf8().get_data());
            }
            continue;
        }

        String fmt_str = row[0].strip_edges();
        String role_str = row[1].strip_edges().to_upper();
        String x_str = row[2].strip_edges();
        String y_str = row[3].strip_edges();

        if (fmt_str.is_empty()) {
            UtilityFunctions::printerr("MatchEngine: Empty formation name at line ", line_number, " in ", p_filepath);
            continue;
        }

        std::string fmt_key = fmt_str.utf8().get_data();

        if (role_str.is_empty() || role_str.is_valid_float()) {
            UtilityFunctions::printerr("MatchEngine: Invalid role '", role_str, "' at line ", line_number, " for formation ", fmt_str);
            corrupted_formations.insert(fmt_key);
            continue;
        }

        if (!x_str.is_valid_float() || !y_str.is_valid_float()) {
            UtilityFunctions::printerr("MatchEngine: Non-numeric coordinate at line ", line_number, " for formation ", fmt_str);
            corrupted_formations.insert(fmt_key);
            continue;
        }

        float nx = x_str.to_float();
        float ny = y_str.to_float();

        if (!std::isfinite(nx) || !std::isfinite(ny) || nx < 0.0f || nx > 1.0f || ny < 0.0f || ny > 1.0f) {
            UtilityFunctions::printerr("MatchEngine: Out-of-bounds coordinate (", nx, ", ", ny, ") at line ", line_number, " for formation ", fmt_str);
            corrupted_formations.insert(fmt_key);
            continue;
        }

        TacticalPosition pos;
        pos.role = role_str;
        pos.norm_x = nx;
        pos.norm_y = ny;

        formation_db[fmt_key].push_back(pos);
    }

    for (auto it = formation_db.begin(); it != formation_db.end(); ) {
        if (corrupted_formations.find(it->first) != corrupted_formations.end()) {
            UtilityFunctions::printerr("MatchEngine: Discarding formation '", String(it->first.c_str()), "' due to malformed row data.");
            it = formation_db.erase(it);
        } else if (it->second.size() != 11) {
            UtilityFunctions::printerr("MatchEngine: Formation '", String(it->first.c_str()), "' has ", static_cast<int64_t>(it->second.size()), " positions (expected 11). Discarding.");
            it = formation_db.erase(it);
        } else {
            ++it;
        }
    }

    if (formation_db.empty()) {
        UtilityFunctions::printerr("MatchEngine: No valid 11-player formations found in ", p_filepath);
        return false;
    }

    loaded_formations_path = p_filepath;
    UtilityFunctions::print("MatchEngine: Loaded ", static_cast<int64_t>(formation_db.size()), " validated tactical formations from ", p_filepath);
    return true;
}

bool MatchEngine::_has_valid_formation(const std::string& p_name) const {
    auto it = formation_db.find(p_name);
    return it != formation_db.end() && it->second.size() == 11;
}

bool MatchEngine::has_formation(const String& p_formation_name) const {
    String clean_name = p_formation_name.strip_edges();
    if (clean_name.is_empty()) {
        return false;
    }
    return _has_valid_formation(clean_name.utf8().get_data());
}

PackedStringArray MatchEngine::get_available_formations() const {
    PackedStringArray names;
    for (const auto& pair : formation_db) {
        if (pair.second.size() == 11) {
            names.append(String(pair.first.c_str()));
        }
    }
    names.sort();
    return names;
}

bool MatchEngine::set_home_formation(const String& p_formation_name) {
    String clean_name = p_formation_name.strip_edges();
    if (clean_name.is_empty()) {
        UtilityFunctions::printerr("MatchEngine: Empty home formation name rejected.");
        return false;
    }

    if (!formation_db.empty() && !_has_valid_formation(clean_name.utf8().get_data())) {
        UtilityFunctions::printerr("MatchEngine: Unknown home formation '", clean_name, "'. Keeping '", home_formation, "'.");
        return false;
    }

    home_formation = clean_name;
    _apply_formation_positions(!match_active);
    return true;
}

bool MatchEngine::set_away_formation(const String& p_formation_name) {
    String clean_name = p_formation_name.strip_edges();
    if (clean_name.is_empty()) {
        UtilityFunctions::printerr("MatchEngine: Empty away formation name rejected.");
        return false;
    }

    if (!formation_db.empty() && !_has_valid_formation(clean_name.utf8().get_data())) {
        UtilityFunctions::printerr("MatchEngine: Unknown away formation '", clean_name, "'. Keeping '", away_formation, "'.");
        return false;
    }

    away_formation = clean_name;
    _apply_formation_positions(!match_active);
    return true;
}

void MatchEngine::_sync_player_positions_array() {
    player_positions.clear();
    for (size_t i = 0; i < sim_players.size(); i++) {
        player_positions.append(sim_players[i].position);
    }
}

void MatchEngine::_reset_field_positions() {
    _apply_formation_positions(true, true);
}

void MatchEngine::_apply_formation_positions(bool p_reset_ball, bool p_force_snap_all) {
    if (sim_players.size() != 22) {
        _initialize_default_squads();
    }

    float half_width = field_size.x * 0.5f;
    float field_height = field_size.y;
    const float margin = 14.0f;
    bool snap_positions = p_reset_ball || p_force_snap_all || !match_active;

    std::string home_str = home_formation.utf8().get_data();
    std::string away_str = away_formation.utf8().get_data();

    // 1. Home Team Anchors (Left Half around halfway line)
    if (_has_valid_formation(home_str)) {
        const auto& nodes = formation_db[home_str];
        for (size_t i = 0; i < 11; i++) {
            float posX = std::clamp(nodes[i].norm_x * half_width, margin, half_width - margin);
            float posY = std::clamp(nodes[i].norm_y * field_height, margin, field_height - margin);
            sim_players[i].role = nodes[i].role;
            sim_players[i].anchor = Vector2(posX, posY);
            if (snap_positions) {
                sim_players[i].position = sim_players[i].anchor;
                sim_players[i].velocity = Vector2(0.0f, 0.0f);
                sim_players[i].recovery_timer = 0.0f;
                sim_players[i].sprint_boost_timer = 0.0f;
                sim_players[i].dribble_boost_timer = 0.0f;
            }
        }
    } else {
        for (int i = 0; i < 11; i++) {
            float norm_x = 0.12f + (static_cast<float>(i % 4) * 0.22f);
            float norm_y = 0.15f + (static_cast<float>(i) / 10.0f) * 0.70f;
            sim_players[i].anchor = Vector2(norm_x * half_width, norm_y * field_height);
            if (snap_positions) {
                sim_players[i].position = sim_players[i].anchor;
                sim_players[i].velocity = Vector2(0.0f, 0.0f);
                sim_players[i].recovery_timer = 0.0f;
                sim_players[i].sprint_boost_timer = 0.0f;
                sim_players[i].dribble_boost_timer = 0.0f;
            }
        }
    }

    // 2. Away Team Anchors (Mirrored Right Half around halfway line)
    if (_has_valid_formation(away_str)) {
        const auto& nodes = formation_db[away_str];
        for (size_t i = 0; i < 11; i++) {
            float posX = std::clamp(field_size.x - (nodes[i].norm_x * half_width), half_width + margin, field_size.x - margin);
            float posY = std::clamp((1.0f - nodes[i].norm_y) * field_height, margin, field_height - margin);
            sim_players[11 + i].role = nodes[i].role;
            sim_players[11 + i].anchor = Vector2(posX, posY);
            if (snap_positions) {
                sim_players[11 + i].position = sim_players[11 + i].anchor;
                sim_players[11 + i].velocity = Vector2(0.0f, 0.0f);
                sim_players[11 + i].recovery_timer = 0.0f;
                sim_players[11 + i].sprint_boost_timer = 0.0f;
                sim_players[11 + i].dribble_boost_timer = 0.0f;
            }
        }
    } else {
        for (int i = 0; i < 11; i++) {
            float norm_x = 0.12f + (static_cast<float>(i % 4) * 0.22f);
            float norm_y = 0.15f + (static_cast<float>(i) / 10.0f) * 0.70f;
            sim_players[11 + i].anchor = Vector2(field_size.x - (norm_x * half_width), (1.0f - norm_y) * field_height);
            if (snap_positions) {
                sim_players[11 + i].position = sim_players[11 + i].anchor;
                sim_players[11 + i].velocity = Vector2(0.0f, 0.0f);
                sim_players[11 + i].recovery_timer = 0.0f;
                sim_players[11 + i].sprint_boost_timer = 0.0f;
                sim_players[11 + i].dribble_boost_timer = 0.0f;
            }
        }
    }

    if (p_reset_ball) {
        _give_kickoff(true);
    }

    _sync_player_positions_array();
}

void MatchEngine::_reset_after_goal(bool p_conceding_is_home) {
    chance_state = CHANCE_NONE;
    manual_control_active = false;
    user_controlled_index = -1;
    has_user_move_target = false;
    user_move_target = Vector2(-1.0f, -1.0f);
    active_chance_timer = 0.0f;
    pending_chance_xg = 0.0f;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    shot_hits_crossbar = false;
    ball_velocity = Vector2(0.0f, 0.0f);
    ball_target = field_size * 0.5f;
    event_cooldown = 8.0f;

    _apply_formation_positions(false, true);
    _give_kickoff(p_conceding_is_home);
    _sync_player_positions_array();

    is_paused_for_manual = false;
    waiting_for_kickoff_resume = true;
    kickoff_team = p_conceding_is_home ? 0 : 1;

    emit_signal("chance_state_changed", chance_state, -1);
    emit_signal("goal_kickoff_ready", kickoff_team, String("CLICK TO RESUME"));
}

void MatchEngine::resume_from_goal_kickoff() {
    if (!match_active || !waiting_for_kickoff_resume) {
        return;
    }

    int resuming_team = kickoff_team;
    waiting_for_kickoff_resume = false;
    kickoff_team = -1;

    chance_state = CHANCE_NONE;
    manual_control_active = false;
    user_controlled_index = -1;
    has_user_move_target = false;
    user_move_target = Vector2(-1.0f, -1.0f);
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    shot_hits_crossbar = false;
    ball_state = BALL_CARRIED;
    ball_position = field_size * 0.5f;
    ball_velocity = Vector2(0.0f, 0.0f);
    ball_target = ball_position;
    action_decision_timer = 0.35f;
    event_cooldown = 6.0f;

    if (resuming_team == 0 || resuming_team == 1) {
        possession_team = resuming_team;
    }

    emit_signal("goal_kickoff_resumed", possession_team);
}

void MatchEngine::_give_kickoff(bool p_home_kickoff) {
    Vector2 center = field_size * 0.5f;
    float half_width = center.x;
    const float margin = 14.0f;
    const float center_circle_radius = 92.0f;

    for (int i = 0; i < 22; i++) {
        SimPlayer& p = sim_players[i];
        p.velocity = Vector2(0.0f, 0.0f);
        p.recovery_timer = 0.0f;
        p.sprint_boost_timer = 0.0f;
        p.dribble_boost_timer = 0.0f;
        p.position = p.anchor;

        if (p.is_home) {
            p.position.x = std::clamp(p.position.x, margin, half_width - 14.0f);
        } else {
            p.position.x = std::clamp(p.position.x, half_width + 14.0f, field_size.x - margin);
        }
        p.position.y = std::clamp(p.position.y, margin, field_size.y - margin);
    }

    int defending_start = p_home_kickoff ? 11 : 0;
    for (int i = defending_start; i < defending_start + 11; i++) {
        SimPlayer& opp = sim_players[i];
        Vector2 from_center = opp.position - center;
        float dist = from_center.length();
        if (dist < center_circle_radius) {
            Vector2 push_dir = (dist > 0.01f)
                ? (from_center / dist)
                : Vector2(p_home_kickoff ? 1.0f : -1.0f, 0.0f);
            if (p_home_kickoff && push_dir.x < 0.25f) push_dir.x = 0.45f;
            if (!p_home_kickoff && push_dir.x > -0.25f) push_dir.x = -0.45f;
            push_dir = push_dir.normalized();

            opp.position = center + push_dir * (center_circle_radius + 6.0f);
            if (opp.is_home) {
                opp.position.x = std::clamp(opp.position.x, margin, half_width - 14.0f);
            } else {
                opp.position.x = std::clamp(opp.position.x, half_width + 14.0f, field_size.x - margin);
            }
            opp.position.y = std::clamp(opp.position.y, margin, field_size.y - margin);
        }
    }

    int kicking_start = p_home_kickoff ? 0 : 11;
    int taker_idx = -1;
    float best_taker_score = 1e9f;

    for (int i = kicking_start + 1; i < kicking_start + 11; i++) {
        const SimPlayer& p = sim_players[i];
        float dist_center = p.position.distance_to(center);
        float role_bonus = 0.0f;
        if (p.role == "ST" || p.role == "CF") role_bonus = -85.0f;
        else if (p.role == "CAM") role_bonus = -55.0f;
        else if (p.role == "CM" || p.role == "LW" || p.role == "RW") role_bonus = -20.0f;

        float score = dist_center + role_bonus;
        if (score < best_taker_score) {
            best_taker_score = score;
            taker_idx = i;
        }
    }
    if (taker_idx == -1) {
        taker_idx = kicking_start + 9;
    }

    int partner_idx = -1;
    float best_partner_dist = 1e9f;
    for (int i = kicking_start + 1; i < kicking_start + 11; i++) {
        if (i == taker_idx) continue;
        float dist_center = sim_players[i].position.distance_to(center);
        if (dist_center < best_partner_dist) {
            best_partner_dist = dist_center;
            partner_idx = i;
        }
    }

    float own_side_sign = p_home_kickoff ? -1.0f : 1.0f;
    sim_players[taker_idx].position = center + Vector2(own_side_sign * 6.0f, 0.0f);
    sim_players[taker_idx].velocity = Vector2(0.0f, 0.0f);

    if (partner_idx != -1) {
        sim_players[partner_idx].position = center + Vector2(own_side_sign * 24.0f, 34.0f);
        sim_players[partner_idx].velocity = Vector2(0.0f, 0.0f);
    }

    ball_position = center;
    ball_velocity = Vector2(0.0f, 0.0f);
    ball_target = center;
    ball_state = BALL_CARRIED;
    possession_team = p_home_kickoff ? 0 : 1;
    ball_carrier_index = taker_idx;
    last_touch_index = taker_idx;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    shot_hits_crossbar = false;
    action_decision_timer = 0.35f;
}

void MatchEngine::_give_gk_possession(bool p_home_gk) {
    int gk_idx = p_home_gk ? 0 : 11;
    float gk_x = p_home_gk ? 42.0f : (field_size.x - 42.0f);
    float gk_y = field_size.y * 0.5f;

    sim_players[gk_idx].position = Vector2(gk_x, gk_y);
    sim_players[gk_idx].velocity = Vector2(0.0f, 0.0f);

    possession_team = p_home_gk ? 0 : 1;
    ball_carrier_index = gk_idx;
    last_touch_index = gk_idx;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    shot_hits_crossbar = false;
    ball_state = BALL_CARRIED;
    ball_position = sim_players[gk_idx].position + Vector2(p_home_gk ? 8.0f : -8.0f, 0.0f);
    ball_velocity = Vector2(0.0f, 0.0f);
    action_decision_timer = 0.55f;
}

// =========================================================================
// DIRECT PLAYER CONTROL & CAMERA FRAMING API
// =========================================================================

bool MatchEngine::is_manual_dribble_active() const {
    if (!match_active || !manual_control_active || chance_state != CHANCE_HOME_ATTACK) {
        return false;
    }
    if (user_controlled_index < 1 || user_controlled_index >= 11) {
        return false;
    }
    return has_user_move_target;
}


bool MatchEngine::set_controlled_player(int p_player_idx) {
    if (p_player_idx < 0 || p_player_idx >= 11) {
        return false;
    }
    if (user_controlled_index == p_player_idx) {
        return true;
    }
    user_controlled_index = p_player_idx;
    has_user_move_target = false;
    emit_signal("controlled_player_changed", user_controlled_index);
    return true;
}

int MatchEngine::select_nearest_defender() {
    int best_idx = 1;
    float best_score = 1e9f;
    Vector2 focus_point = (ball_state == BALL_IN_PASS)
        ? ball_position.lerp(ball_target, 0.45f)
        : ball_position;

    for (int i = 1; i < 11; i++) {
        float d = sim_players[i].position.distance_to(focus_point);
        float goal_side_bonus = (sim_players[i].position.x <= focus_point.x + 12.0f) ? -14.0f : 12.0f;
        float score = d + goal_side_bonus;
        if (score < best_score) {
            best_score = score;
            best_idx = i;
        }
    }
    if (user_controlled_index != best_idx) {
        user_controlled_index = best_idx;
        has_user_move_target = false;
        emit_signal("controlled_player_changed", user_controlled_index);
    }
    return best_idx;
}

Rect2 MatchEngine::get_chance_framing_rect() const {
    Vector2 min_pt = ball_position;
    Vector2 max_pt = ball_position;

    auto expand_pt = [&](const Vector2& pt) {
        min_pt.x = std::min(min_pt.x, pt.x);
        min_pt.y = std::min(min_pt.y, pt.y);
        max_pt.x = std::max(max_pt.x, pt.x);
        max_pt.y = std::max(max_pt.y, pt.y);
    };

    if (user_controlled_index >= 0 && user_controlled_index < 22) {
        expand_pt(sim_players[user_controlled_index].position);
    }
    if (ball_carrier_index >= 0 && ball_carrier_index < 22) {
        expand_pt(sim_players[ball_carrier_index].position);
    }
    if (ball_state == BALL_IN_PASS && pass_target_index >= 0 && pass_target_index < 22) {
        expand_pt(sim_players[pass_target_index].position);
        expand_pt(ball_target);
    }

    std::vector<std::pair<float, int>> home_near;
    std::vector<std::pair<float, int>> away_near;
    for (int i = 0; i < 11; i++) {
        home_near.push_back({ sim_players[i].position.distance_squared_to(ball_position), i });
    }
    for (int j = 11; j < 22; j++) {
        away_near.push_back({ sim_players[j].position.distance_squared_to(ball_position), j });
    }
    std::sort(home_near.begin(), home_near.end());
    std::sort(away_near.begin(), away_near.end());

    int home_count = std::min(4, static_cast<int>(home_near.size()));
    int away_count = std::min(4, static_cast<int>(away_near.size()));
    for (int k = 0; k < home_count; k++) {
        if (home_near[k].first <= 280.0f * 280.0f) {
            expand_pt(sim_players[home_near[k].second].position);
        }
    }
    for (int k = 0; k < away_count; k++) {
        if (away_near[k].first <= 280.0f * 280.0f) {
            expand_pt(sim_players[away_near[k].second].position);
        }
    }

    const float pad_x = 95.0f;
    const float pad_y = 75.0f;
    min_pt.x = std::max(0.0f, min_pt.x - pad_x);
    min_pt.y = std::max(0.0f, min_pt.y - pad_y);
    max_pt.x = std::min(field_size.x, max_pt.x + pad_x);
    max_pt.y = std::min(field_size.y, max_pt.y + pad_y);

    return Rect2(min_pt, max_pt - min_pt);
}

Vector2 MatchEngine::get_chance_camera_focus() const {
    if (waiting_for_kickoff_resume || chance_state == CHANCE_NONE) {
        return field_size * 0.5f;
    }

    Vector2 weighted_sum = ball_position * 0.45f;
    float total_weight = 0.45f;

    if (user_controlled_index >= 0 && user_controlled_index < 22) {
        weighted_sum += sim_players[user_controlled_index].position * 0.30f;
        total_weight += 0.30f;
    }

    Rect2 framing = get_chance_framing_rect();
    Vector2 rect_center = framing.position + (framing.size * 0.5f);
    weighted_sum += rect_center * 0.25f;
    total_weight += 0.25f;

    return weighted_sum / total_weight;
}

float MatchEngine::get_recommended_camera_zoom() const {
    if (waiting_for_kickoff_resume || chance_state == CHANCE_NONE) {
        return 1.0f;
    }

    float desired_zoom = (chance_state == CHANCE_AWAY_ATTACK) ? 1.20f : 1.12f;

    Rect2 framing = get_chance_framing_rect();
    float fit_zoom_x = field_size.x / std::max(340.0f, framing.size.x);
    float fit_zoom_y = field_size.y / std::max(240.0f, framing.size.y);
    float safe_max_zoom = std::min(fit_zoom_x, fit_zoom_y);

    return std::clamp(std::min(desired_zoom, safe_max_zoom), 1.0f, 1.24f);
}

int MatchEngine::_resolve_home_action_player() const {
    if (ball_state == BALL_CARRIED && ball_carrier_index >= 0 && ball_carrier_index < 11) {
        return ball_carrier_index;
    }
    if (user_controlled_index >= 1 && user_controlled_index < 11 &&
        ball_position.distance_to(sim_players[user_controlled_index].position) <= 48.0f) {
        return user_controlled_index;
    }
    if (ball_state == BALL_IN_PASS && pass_target_index >= 1 && pass_target_index < 11 &&
        ball_position.distance_to(sim_players[pass_target_index].position) <= 48.0f) {
        return pass_target_index;
    }
    int best_idx = -1;
    float best_dist = 48.0f;
    for (int i = 1; i < 11; i++) {
        float d = ball_position.distance_to(sim_players[i].position);
        if (d < best_dist) {
            best_dist = d;
            best_idx = i;
        }
    }
    return best_idx;
}

Array MatchEngine::get_valid_pass_targets() const {
    Array targets;
    int passer_idx = _resolve_home_action_player();
    if (passer_idx < 0 || passer_idx >= 11) {
        passer_idx = (user_controlled_index >= 0 && user_controlled_index < 11) ? user_controlled_index : ball_carrier_index;
    }
    if (passer_idx < 0 || passer_idx >= 11) {
        return targets;
    }

    const SimPlayer& passer = sim_players[passer_idx];
    float fatigue = _get_fatigue_multiplier(passer);
    float max_range = (240.0f + (passer.passing * 2.6f)) * fatigue;

    for (int t = 1; t < 11; t++) {
        if (t == passer_idx) continue;
        Vector2 pass_vec = sim_players[t].position - passer.position;
        float dist = pass_vec.length();
        if (dist < 16.0f || dist > max_range) continue;
        targets.append(t);
    }
    return targets;
}

bool MatchEngine::command_move_player(Vector2 p_target_pos) {
    if (!match_active || waiting_for_kickoff_resume) {
        return false;
    }
    if (!manual_control_active && chance_state != CHANCE_NONE) {
        manual_control_active = true;
    }
    if (!manual_control_active) {
        return false;
    }

    if (chance_state == CHANCE_HOME_ATTACK && ball_state == BALL_CARRIED &&
        ball_carrier_index >= 0 && ball_carrier_index < 11 &&
        user_controlled_index != ball_carrier_index) {
        user_controlled_index = ball_carrier_index;
        emit_signal("controlled_player_changed", user_controlled_index);
    }

    if (user_controlled_index < 0 || user_controlled_index >= 11) {
        return false;
    }

    SimPlayer& player = sim_players[user_controlled_index];
    const float margin = 14.0f;

    Vector2 resolved_target = p_target_pos;
    Vector2 raw_delta = resolved_target - player.position;

    if (chance_state == CHANCE_HOME_ATTACK && raw_delta.length() < 16.0f) {
        Vector2 forward_dir(1.0f, 0.0f);
        if (player.velocity.length() > 10.0f) {
            Vector2 vel_dir = player.velocity.normalized();
            forward_dir = Vector2(std::max(0.55f, vel_dir.x), vel_dir.y * 0.65f).normalized();
        }
        resolved_target = player.position + forward_dir * 76.0f;
    }

    user_move_target.x = std::clamp(resolved_target.x, margin, field_size.x - margin);
    user_move_target.y = std::clamp(resolved_target.y, margin, field_size.y - margin);
    has_user_move_target = true;
    is_paused_for_manual = false;

    if (chance_state == CHANCE_HOME_ATTACK && ball_state == BALL_CARRIED && ball_carrier_index == user_controlled_index) {
        Vector2 move_vec = user_move_target - player.position;
        float move_dist = move_vec.length();
        if (move_dist > 1.0f) {
            Vector2 dir = move_vec / move_dist;
            float fatigue_mult = _get_fatigue_multiplier(player);
            float burst_speed = (128.0f + player.pace * 0.72f + player.dribbling * 0.45f) * fatigue_mult;
            player.dribble_boost_timer = 0.55f;
            player.velocity = player.velocity.lerp(dir * burst_speed, 0.75f);
        }
    }

    return true;
}

bool MatchEngine::command_pass(int p_target_player_idx) {
    if (!match_active || waiting_for_kickoff_resume || chance_state != CHANCE_HOME_ATTACK) {
        return false;
    }
    if (p_target_player_idx < 0 || p_target_player_idx >= 11) {
        return false;
    }

    int passer_idx = _resolve_home_action_player();
    if (passer_idx < 0 || passer_idx >= 11 || passer_idx == p_target_player_idx) {
        return false;
    }

    Vector2 to_rec = sim_players[p_target_player_idx].position - sim_players[passer_idx].position;
    float dist = to_rec.length();
    float fatigue = _get_fatigue_multiplier(sim_players[passer_idx]);
    float max_range = (260.0f + sim_players[passer_idx].passing * 2.8f) * fatigue;
    if (dist < 12.0f || dist > max_range) {
        return false;
    }

    Vector2 pass_dir = (dist > 1.0f) ? (to_rec / dist) : Vector2(1.0f, 0.0f);

    sim_players[passer_idx].dribble_boost_timer = 0.0f;
    ball_position = sim_players[passer_idx].position + pass_dir * 10.0f;
    ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
    ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);

    has_user_move_target = false;
    is_paused_for_manual = false;
    _execute_pass(passer_idx, p_target_player_idx, false);
    _log_event("pass", true, sim_players[passer_idx].name,
        sim_players[passer_idx].name + String(" passes to ") + sim_players[p_target_player_idx].name);

    if (user_controlled_index != p_target_player_idx) {
        user_controlled_index = p_target_player_idx;
        emit_signal("controlled_player_changed", user_controlled_index);
    }
    return true;
}

bool MatchEngine::command_through_ball(Vector2 p_direction_or_target) {
    if (!match_active || waiting_for_kickoff_resume || chance_state != CHANCE_HOME_ATTACK) {
        return false;
    }

    int passer_idx = _resolve_home_action_player();
    if (passer_idx < 0 || passer_idx >= 11) {
        return false;
    }

    SimPlayer& passer = sim_players[passer_idx];
    bool is_unit_dir = (p_direction_or_target.length() > 0.01f && p_direction_or_target.length() <= 1.5f);
    Vector2 raw_vec = is_unit_dir
        ? (p_direction_or_target.normalized() * 130.0f)
        : (p_direction_or_target - passer.position);

    bool is_explicit_target_point = (!is_unit_dir &&
                                     p_direction_or_target.x >= 0.0f && p_direction_or_target.x <= field_size.x &&
                                     p_direction_or_target.y >= 0.0f && p_direction_or_target.y <= field_size.y &&
                                     raw_vec.length() >= 12.0f);

    Vector2 aim_vec = (raw_vec.length() >= 8.0f) ? raw_vec : Vector2(120.0f, 0.0f);
    Vector2 aim_dir = aim_vec.normalized();

    passer.dribble_boost_timer = 0.0f;
    ball_position = passer.position + aim_dir * 10.0f;
    ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
    ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);

    Vector2 desired_space = is_explicit_target_point
        ? p_direction_or_target
        : (passer.position + aim_dir * std::clamp(aim_vec.length(), 70.0f, 185.0f));
    desired_space.x = std::clamp(desired_space.x, 22.0f, field_size.x - 22.0f);
    desired_space.y = std::clamp(desired_space.y, 22.0f, field_size.y - 22.0f);

    int best_runner = -1;
    float best_runner_score = -1e9f;
    for (int t = 1; t < 11; t++) {
        if (t == passer_idx) continue;
        Vector2 to_tm = sim_players[t].position - passer.position;
        float dist_from_passer = to_tm.length();
        if (dist_from_passer < 18.0f || dist_from_passer > 460.0f) continue;

        float dist_to_space = sim_players[t].position.distance_to(desired_space);
        float alignment = (to_tm / dist_from_passer).dot(aim_dir);
        float forward_bonus = (sim_players[t].position.x - passer.position.x) * 0.35f;

        float score = (alignment * 120.0f) + forward_bonus - (dist_to_space * 0.85f);
        if (score > best_runner_score) {
            best_runner_score = score;
            best_runner = t;
        }
    }

    if (best_runner == -1) {
        float max_x = -1.0f;
        for (int t = 1; t < 11; t++) {
            if (t != passer_idx && sim_players[t].position.x > max_x) {
                max_x = sim_players[t].position.x;
                best_runner = t;
            }
        }
    }

    if (best_runner == -1) {
        return false;
    }

    SimPlayer& runner = sim_players[best_runner];
    Vector2 space_target = desired_space;
    if (!is_explicit_target_point) {
        float lead_dist = std::clamp(64.0f + aim_vec.length() * 0.25f, 60.0f, 130.0f);
        space_target = runner.position + Vector2(std::max(0.35f, aim_dir.x), aim_dir.y * 0.75f).normalized() * lead_dist;
    } else if (runner.position.distance_to(desired_space) < 95.0f) {
        Vector2 lead_point = runner.position + aim_dir * 42.0f;
        space_target = desired_space.lerp(lead_point, 0.35f);
    }
    space_target.x = std::clamp(space_target.x, 18.0f, field_size.x - 18.0f);
    space_target.y = std::clamp(space_target.y, 18.0f, field_size.y - 18.0f);

    float fatigue = _get_fatigue_multiplier(passer);
    passer.stamina = std::max(10.0f, passer.stamina - 0.40f);
    float inaccuracy = (100.0f - passer.passing * fatigue) * 0.12f;
    space_target.x = std::clamp(space_target.x + static_cast<float>(UtilityFunctions::randf_range(-inaccuracy, inaccuracy)), 18.0f, field_size.x - 18.0f);
    space_target.y = std::clamp(space_target.y + static_cast<float>(UtilityFunctions::randf_range(-inaccuracy, inaccuracy)), 18.0f, field_size.y - 18.0f);

    Vector2 delta = space_target - ball_position;
    float dist = std::max(1.0f, delta.length());
    float pass_speed = std::clamp((295.0f + passer.passing * 1.50f) * fatigue, 285.0f, 485.0f);

    ball_target = space_target;
    ball_velocity = (delta / dist) * pass_speed;
    ball_state = BALL_IN_PASS;
    ball_carrier_index = -1;
    pass_target_index = best_runner;
    is_through_ball_in_flight = true;
    last_touch_index = passer_idx;
    stats.home_passes++;

    passer.recovery_timer = 0.25f;
    runner.sprint_boost_timer = 1.85f;
    bool ctrl_changed = (user_controlled_index != best_runner);
    user_controlled_index = best_runner;
    user_move_target = space_target;
    has_user_move_target = true;
    is_paused_for_manual = false;

    _log_event("through_ball", true, passer.name,
        passer.name + String(" threads a through ball into space for ") + runner.name);

    if (ctrl_changed) {
        emit_signal("controlled_player_changed", user_controlled_index);
    }
    return true;
}

bool MatchEngine::command_shot(Vector2 p_aim_target, float p_power_ratio) {
    if (!match_active || waiting_for_kickoff_resume || chance_state != CHANCE_HOME_ATTACK) {
        return false;
    }

    int shooter_idx = _resolve_home_action_player();
    if (shooter_idx < 0 || shooter_idx >= 11) {
        return false;
    }

    const SimPlayer& shooter = sim_players[shooter_idx];
    Vector2 shot_vec = p_aim_target - shooter.position;
    Vector2 shot_dir = (shot_vec.length() > 1.0f) ? shot_vec.normalized() : Vector2(1.0f, 0.0f);

    sim_players[shooter_idx].dribble_boost_timer = 0.0f;
    ball_position = shooter.position + shot_dir * 10.0f;
    ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
    ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);

    float clamped_power = std::clamp(p_power_ratio, 0.25f, 1.35f);

    Vector2 opp_goal_center(field_size.x, field_size.y * 0.5f);
    float dist_to_goal = shooter.position.distance_to(opp_goal_center);
    float xg = std::clamp((1.0f - dist_to_goal / 320.0f) * (0.48f + shooter.shooting * 0.0042f), 0.08f, 0.75f);

    has_user_move_target = false;
    is_paused_for_manual = false;
    _execute_shot(shooter_idx, xg, p_aim_target, clamped_power);
    return true;
}

bool MatchEngine::command_tackle() {
    if (!match_active || waiting_for_kickoff_resume || chance_state != CHANCE_AWAY_ATTACK) {
        return false;
    }
    if (user_controlled_index < 1 || user_controlled_index >= 11) {
        select_nearest_defender();
    }

    SimPlayer& defender = sim_players[user_controlled_index];
    if (defender.recovery_timer > 0.0f) {
        return false;
    }

    float dist_to_ball = defender.position.distance_to(ball_position);
    if (dist_to_ball > 42.0f) {
        defender.sprint_boost_timer = 0.65f;
        command_move_player(ball_position);
        return false;
    }

    defender.stamina = std::max(10.0f, defender.stamina - 0.85f);
    float def_fatigue = _get_fatigue_multiplier(defender);
    float approach_bonus = (home_defensive_approach == TIER_HIGH) ? 6.0f : ((home_defensive_approach == TIER_LOW) ? -4.0f : 0.0f);

    if (ball_state == BALL_CARRIED && ball_carrier_index >= 11 && ball_carrier_index < 22) {
        const SimPlayer& attacker = sim_players[ball_carrier_index];
        float atk_fatigue = _get_fatigue_multiplier(attacker);

        float def_score = ((defender.defending * 0.65f + defender.physical * 0.35f) * def_fatigue)
                        + approach_bonus + static_cast<float>(UtilityFunctions::randf_range(-10.0, 12.0));
        float atk_score = ((attacker.dribbling * 0.65f + attacker.pace * 0.35f) * atk_fatigue)
                        + static_cast<float>(UtilityFunctions::randf_range(-10.0, 10.0));

        if (def_score >= atk_score) {
            stats.home_tackles++;
            _log_event("tackle", true, defender.name,
                defender.name + String(" wins the ball with a decisive tackle!"));
            possession_team = 0;
            ball_carrier_index = user_controlled_index;
            last_touch_index = user_controlled_index;
            pass_target_index = -1;
            is_through_ball_in_flight = false;
            ball_state = BALL_CARRIED;
            ball_velocity = Vector2(0.0f, 0.0f);
            action_decision_timer = 0.35f;
            _end_active_chance(false);
            return true;
        } else {
            defender.recovery_timer = 0.55f;
            _log_event("tackle_missed", true, defender.name,
                defender.name + String(" misses the challenge — attacker breaks free!"));
            return false;
        }
    } else if (ball_state == BALL_LOOSE && dist_to_ball <= 26.0f) {
        stats.home_tackles++;
        possession_team = 0;
        ball_carrier_index = user_controlled_index;
        last_touch_index = user_controlled_index;
        pass_target_index = -1;
        is_through_ball_in_flight = false;
        ball_state = BALL_CARRIED;
        ball_velocity = Vector2(0.0f, 0.0f);
        _end_active_chance(false);
        return true;
    }

    return false;
}

bool MatchEngine::begin_chance(bool p_is_home_attacking, int p_attacker_idx, float p_xg) {
    if (!match_active || waiting_for_kickoff_resume) {
        return false;
    }

    int chosen_idx = p_attacker_idx;
    if (p_is_home_attacking) {
        if (chosen_idx < 1 || chosen_idx >= 11) {
            if (ball_carrier_index >= 1 && ball_carrier_index < 11) {
                chosen_idx = ball_carrier_index;
            } else {
                float best_d = 1e9f;
                chosen_idx = 9;
                for (int i = 1; i < 11; i++) {
                    float d = sim_players[i].position.distance_squared_to(ball_position);
                    if (d < best_d) {
                        best_d = d;
                        chosen_idx = i;
                    }
                }
            }
        }
        possession_team = 0;
        ball_carrier_index = chosen_idx;
        last_touch_index = chosen_idx;
        pass_target_index = -1;
        is_through_ball_in_flight = false;
        shot_hits_crossbar = false;
        ball_state = BALL_CARRIED;
        ball_position = sim_players[chosen_idx].position + Vector2(7.0f, 0.0f);
        ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
        ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);
        ball_velocity = Vector2(0.0f, 0.0f);
    } else {
        if (chosen_idx < 12 || chosen_idx >= 22) {
            if (ball_carrier_index >= 12 && ball_carrier_index < 22) {
                chosen_idx = ball_carrier_index;
            } else {
                float best_d = 1e9f;
                chosen_idx = 20;
                for (int j = 12; j < 22; j++) {
                    float d = sim_players[j].position.distance_squared_to(ball_position);
                    if (d < best_d) {
                        best_d = d;
                        chosen_idx = j;
                    }
                }
            }
        }
        possession_team = 1;
        ball_carrier_index = chosen_idx;
        last_touch_index = chosen_idx;
        pass_target_index = -1;
        is_through_ball_in_flight = false;
        shot_hits_crossbar = false;
        ball_state = BALL_CARRIED;
        ball_position = sim_players[chosen_idx].position + Vector2(-7.0f, 0.0f);
        ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
        ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);
        ball_velocity = Vector2(0.0f, 0.0f);
        action_decision_timer = 0.65f;
    }

    _trigger_key_chance(p_is_home_attacking ? 1 : 4, p_is_home_attacking, chosen_idx, p_xg);
    return true;
}

void MatchEngine::set_home_auto_shoot_enabled(bool p_enabled) {
    home_auto_shoot_enabled = p_enabled;
}

bool MatchEngine::is_home_auto_shoot_enabled() const {
    return home_auto_shoot_enabled;
}

void MatchEngine::trigger_wide_range_chance(bool p_is_home_attacking) {
    if (!match_active || waiting_for_kickoff_resume || chance_state != CHANCE_NONE) {
        return;
    }
    _spawn_wide_range_chance(p_is_home_attacking);
}

void MatchEngine::_spawn_wide_range_chance(bool p_is_home_attacking) {
    if (!match_active || waiting_for_kickoff_resume || chance_state != CHANCE_NONE) {
        return;
    }

    int carrier_idx = -1;
    if (p_is_home_attacking) {
        if (ball_carrier_index >= 1 && ball_carrier_index < 11) {
            carrier_idx = ball_carrier_index;
        } else if (last_touch_index >= 1 && last_touch_index < 11) {
            carrier_idx = last_touch_index;
        } else {
            float best_dist_sq = 1e12f;
            for (int i = 1; i < 11; i++) {
                float d_sq = sim_players[i].position.distance_squared_to(ball_position);
                if (d_sq < best_dist_sq) {
                    best_dist_sq = d_sq;
                    carrier_idx = i;
                }
            }
        }
    } else {
        if (ball_carrier_index >= 12 && ball_carrier_index < 22) {
            carrier_idx = ball_carrier_index;
        } else if (last_touch_index >= 12 && last_touch_index < 22) {
            carrier_idx = last_touch_index;
        } else {
            float best_dist_sq = 1e12f;
            for (int j = 12; j < 22; j++) {
                float d_sq = sim_players[j].position.distance_squared_to(ball_position);
                if (d_sq < best_dist_sq) {
                    best_dist_sq = d_sq;
                    carrier_idx = j;
                }
            }
        }
    }

    if (carrier_idx < 0 || carrier_idx >= 22) {
        return;
    }

    const SimPlayer& carrier = sim_players[carrier_idx];
    float default_dir = p_is_home_attacking ? 1.0f : -1.0f;
    Vector2 facing = (carrier.velocity.length() > 8.0f)
        ? carrier.velocity.normalized()
        : Vector2(default_dir, 0.0f);

    possession_team = p_is_home_attacking ? 0 : 1;
    ball_carrier_index = carrier_idx;
    last_touch_index = carrier_idx;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    shot_hits_crossbar = false;
    ball_state = BALL_CARRIED;
    ball_position = carrier.position + facing * 7.0f;
    ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
    ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);
    ball_velocity = Vector2(0.0f, 0.0f);
    action_decision_timer = p_is_home_attacking ? 0.35f : 0.65f;

    Vector2 target_goal(p_is_home_attacking ? field_size.x : 0.0f, field_size.y * 0.5f);
    float dist_to_goal = carrier.position.distance_to(target_goal);
    float lateral_offset = std::abs(carrier.position.y - (field_size.y * 0.5f));
    float dist_factor = std::clamp(1.0f - (dist_to_goal / 380.0f), 0.06f, 0.85f);
    float angle_factor = std::clamp(1.0f - (lateral_offset / 280.0f), 0.30f, 1.0f);
    float est_xg = std::clamp(dist_factor * angle_factor * 0.55f, 0.06f, 0.62f);

    _sync_player_positions_array();
    _trigger_key_chance(p_is_home_attacking ? 1 : 4, p_is_home_attacking, carrier_idx, est_xg);
}

void MatchEngine::_end_active_chance(bool p_goal_scored) {
    if (chance_state == CHANCE_NONE && !manual_control_active) {
        return;
    }

    if (user_controlled_index >= 0 && user_controlled_index < 22) {
        sim_players[user_controlled_index].dribble_boost_timer = 0.0f;
    }

    chance_state = CHANCE_NONE;
    manual_control_active = false;
    user_controlled_index = -1;
    has_user_move_target = false;
    user_move_target = Vector2(-1.0f, -1.0f);
    active_chance_timer = 0.0f;
    pending_chance_xg = 0.0f;
    is_through_ball_in_flight = false;
    shot_hits_crossbar = false;
    event_cooldown = p_goal_scored ? 12.0f : 9.0f;

    emit_signal("chance_state_changed", chance_state, -1);
}

// =========================================================================
// ORGANIZED DEFENSIVE LINE, PRESSING, LANE COVERAGE & SMART ATTACKING RUNS
// =========================================================================

int MatchEngine::_select_smart_presser(bool p_is_home, int p_exclude_idx) const {
    int start_idx = p_is_home ? 1 : 12;
    int end_idx = p_is_home ? 11 : 22;
    float own_goal_x = p_is_home ? 0.0f : field_size.x;
    float ball_dist_to_goal = std::abs(ball_position.x - own_goal_x);

    int best_idx = -1;
    float best_score = 1e9f;

    for (int i = start_idx; i < end_idx; i++) {
        if (i == p_exclude_idx) continue;
        const SimPlayer& p = sim_players[i];
        float d = p.position.distance_to(ball_position);

        float role_bias = 0.0f;
        if (p.role == "CDM" || p.role == "CM") {
            role_bias = -18.0f;
        } else if (p.role == "CB") {
            role_bias = (ball_dist_to_goal > 195.0f) ? 36.0f : -6.0f;
        } else if (p.role == "LB" || p.role == "RB" || p.role == "LWB" || p.role == "RWB") {
            bool ball_wide = std::abs(ball_position.y - (field_size.y * 0.5f)) > 115.0f;
            role_bias = (ball_wide && ball_dist_to_goal < 260.0f) ? -14.0f : 16.0f;
        }

        float score = d + role_bias + (p.recovery_timer * 60.0f);
        if (score < best_score) {
            best_score = score;
            best_idx = i;
        }
    }
    return best_idx;
}

float MatchEngine::_compute_organized_backline_x(bool p_is_home) const {
    float attack_dir = p_is_home ? 1.0f : -1.0f;
    float own_goal_x = p_is_home ? 0.0f : field_size.x;

    int team_mentality = p_is_home ? home_mentality : away_mentality;
    int team_pressing = p_is_home ? home_pressing : away_pressing;
    int team_def_line = p_is_home ? home_defensive_line : away_defensive_line;
    int team_approach = p_is_home ? home_defensive_approach : away_defensive_approach;

    static const float MENTALITY_LINE_OFFSETS[5] = { -26.0f, -12.0f, 0.0f, 18.0f, 36.0f };
    float mentality_offset = MENTALITY_LINE_OFFSETS[std::clamp(team_mentality, 0, 4)];
    float line_instruction_offset = (team_def_line == TIER_HIGH) ? 38.0f : ((team_def_line == TIER_LOW) ? -26.0f : 0.0f);
    float press_offset = (team_pressing == PRESSING_HIGH) ? 28.0f : ((team_pressing == PRESSING_LOW) ? -16.0f : 0.0f);
    float approach_offset = (team_approach == TIER_HIGH) ? 14.0f : ((team_approach == TIER_LOW) ? -12.0f : 0.0f);

    float ball_depth_from_own_goal = std::abs(ball_position.x - own_goal_x);

    float desired_depth;
    if (ball_depth_from_own_goal >= 230.0f) {
        desired_depth = 195.0f + ((ball_depth_from_own_goal - 230.0f) * 0.44f);
    } else {
        desired_depth = std::max(118.0f, ball_depth_from_own_goal - 12.0f);
    }

    desired_depth += line_instruction_offset + press_offset + mentality_offset + approach_offset;

    float hard_min_depth = (team_def_line == TIER_HIGH) ? 142.0f : ((team_def_line == TIER_LOW) ? 106.0f : 124.0f);
    if (ball_depth_from_own_goal < 115.0f) {
        hard_min_depth = std::max(72.0f, ball_depth_from_own_goal - 14.0f);
    }

    float clamped_depth = std::clamp(desired_depth, hard_min_depth, field_size.x * 0.54f);
    return own_goal_x + (attack_dir * clamped_depth);
}

Vector2 MatchEngine::_compute_lane_cover_target(bool p_is_home, const SimPlayer& p_defender) const {
    float attack_dir = p_is_home ? 1.0f : -1.0f;
    float own_goal_x = p_is_home ? 0.0f : field_size.x;
    Vector2 own_goal_center(own_goal_x, field_size.y * 0.5f);

    int opp_start = p_is_home ? 11 : 0;
    int best_receiver = -1;
    float best_threat = -1e9f;

    for (int j = opp_start + 1; j < opp_start + 11; j++) {
        if (j == ball_carrier_index) continue;
        float dist_ball = sim_players[j].position.distance_to(ball_position);
        if (dist_ball < 35.0f || dist_ball > 260.0f) continue;

        float dist_goal = sim_players[j].position.distance_to(own_goal_center);
        float threat = (420.0f - dist_goal) - (dist_ball * 0.35f);
        if (threat > best_threat) {
            best_threat = threat;
            best_receiver = j;
        }
    }

    Vector2 danger_point = (best_receiver != -1)
        ? sim_players[best_receiver].position
        : own_goal_center;

    Vector2 lane_dir = danger_point - ball_position;
    float lane_len = lane_dir.length();
    Vector2 screen_pos = (lane_len > 1.0f)
        ? (ball_position + (lane_dir / lane_len) * std::min(46.0f, lane_len * 0.42f))
        : (ball_position - Vector2(attack_dir * 38.0f, 0.0f));

    screen_pos.y = (screen_pos.y * 0.78f) + (p_defender.position.y * 0.22f);
    return screen_pos;
}

Vector2 MatchEngine::_compute_runner_tracking_offset(bool p_is_home, const SimPlayer& p_defender, float p_line_x, float p_base_y) const {
    float attack_dir = p_is_home ? 1.0f : -1.0f;
    int opp_start = p_is_home ? 11 : 0;

    // Nerf defender runner-tracking IQ when defending a player's Attack Chance
    bool is_nerfed_ai_defense = (!p_is_home && chance_state == CHANCE_HOME_ATTACK);
    float best_scan_dist = is_nerfed_ai_defense ? 68.0f : 125.0f;
    float track_weight = is_nerfed_ai_defense ? 0.20f : 0.48f;
    Vector2 offset(0.0f, 0.0f);

    for (int j = opp_start + 1; j < opp_start + 11; j++) {
        if (j == ball_carrier_index) continue;
        const SimPlayer& opp = sim_players[j];

        float opp_forward_vel = opp.velocity.x * (-attack_dir);
        float dist = Vector2(p_line_x, p_base_y).distance_to(opp.position);
        float velocity_anticipation = is_nerfed_ai_defense ? 0.0f : (opp_forward_vel > 35.0f ? 22.0f : 0.0f);
        float effective_dist = dist - velocity_anticipation;

        if (effective_dist < best_scan_dist) {
            best_scan_dist = effective_dist;
            Vector2 goal_side_mark = opp.position - Vector2(attack_dir * 16.0f, 0.0f);
            offset = (goal_side_mark - Vector2(p_line_x, p_base_y)) * track_weight;
        }
    }

    if (p_is_home) {
        offset.x = std::max(offset.x, -24.0f);
    } else {
        offset.x = std::min(offset.x, 24.0f);
    }
    return offset;
}

Vector2 MatchEngine::_compute_smart_attacking_run_target(
    const SimPlayer& p_player,
    int p_index,
    float p_widened_anchor_y,
    float p_ball_prog
) const {
    float center_y = field_size.y * 0.5f;
    float ball_center_offset_y = ball_position.y - center_y;
    String r = p_player.role;

    int carrier_idx = (ball_carrier_index >= 0 && ball_carrier_index < 11)
        ? ball_carrier_index
        : ((user_controlled_index >= 0 && user_controlled_index < 11) ? user_controlled_index : -1);
    Vector2 carrier_pos = (carrier_idx != -1) ? sim_players[carrier_idx].position : ball_position;

    // Locate the deepest outfield away defender to time line-breaking runs
    float deepest_away_def_x = field_size.x - 105.0f;
    for (int j = 12; j < 22; j++) {
        if (sim_players[j].position.x > deepest_away_def_x) {
            deepest_away_def_x = sim_players[j].position.x;
        }
    }

    float base_x = p_player.anchor.x + (p_ball_prog * 365.0f) + 145.0f;
    float base_y = p_widened_anchor_y + (ball_center_offset_y * 0.24f);

    if (r == "ST" || r == "CF") {
        // Dart into the channel behind the last defender or peel away from carrier's side for a cutback/through-ball
        float channel_y = (carrier_pos.y < center_y)
            ? std::clamp(center_y + 52.0f, 95.0f, field_size.y - 95.0f)
            : std::clamp(center_y - 52.0f, 95.0f, field_size.y - 95.0f);
        base_y = (base_y * 0.45f) + (channel_y * 0.55f);
        base_x = std::max(base_x, std::max(carrier_pos.x + 56.0f, deepest_away_def_x + 26.0f));
    } else if (r == "LW" || r == "RW" || r == "LM" || r == "RM") {
        // Diagonal inside half-space run behind the fullback when ball is on opposite side, or wide overlap when ball is near
        bool is_top_wing = (p_player.anchor.y < center_y);
        bool ball_on_same_side = ((carrier_pos.y < center_y) == is_top_wing);
        if (!ball_on_same_side) {
            float inside_channel_y = is_top_wing ? (center_y - 76.0f) : (center_y + 76.0f);
            base_y = (base_y * 0.35f) + (inside_channel_y * 0.65f);
            base_x = std::max(base_x, std::max(carrier_pos.x + 64.0f, deepest_away_def_x + 22.0f));
        } else {
            float wide_y = is_top_wing ? std::max(54.0f, p_widened_anchor_y - 18.0f)
                                       : std::min(field_size.y - 54.0f, p_widened_anchor_y + 18.0f);
            base_y = wide_y;
            base_x = std::max(base_x, carrier_pos.x + 48.0f);
        }
    } else if (r == "CAM" || r == "CM") {
        // Find the open pocket in Zone 14 / edge of the box for a pass or late box crash
        float pocket_y = center_y - (ball_center_offset_y * 0.45f);
        base_y = std::clamp((base_y * 0.40f) + (pocket_y * 0.60f), 85.0f, field_size.y - 85.0f);
        base_x = std::max(base_x, carrier_pos.x + ((r == "CAM") ? 42.0f : 24.0f));
    } else if (r == "LB" || r == "RB" || r == "LWB" || r == "RWB") {
        base_x = std::max(base_x, carrier_pos.x + 12.0f);
    }

    base_x = std::clamp(base_x, 70.0f, field_size.x - 42.0f);
    base_y = std::clamp(base_y, 36.0f, field_size.y - 36.0f);
    Vector2 candidate(base_x, base_y);

    // Passing-lane shadow avoidance: if a defender sits directly between ball carrier and candidate, step sideways into open sightline
    Vector2 to_cand = candidate - carrier_pos;
    float cand_dist = to_cand.length();
    if (cand_dist > 28.0f) {
        Vector2 lane_dir = to_cand / cand_dist;
        Vector2 perp_dir(-lane_dir.y, lane_dir.x);
        for (int j = 12; j < 22; j++) {
            Vector2 to_def = sim_players[j].position - carrier_pos;
            float proj = to_def.dot(lane_dir);
            if (proj > 12.0f && proj < cand_dist - 8.0f) {
                float signed_perp = to_def.cross(lane_dir);
                if (std::abs(signed_perp) < 26.0f) {
                    float dodge_sign = (signed_perp >= 0.0f) ? 1.0f : -1.0f;
                    candidate += perp_dir * (dodge_sign * 34.0f);
                    break;
                }
            }
        }
    }

    candidate.x = std::clamp(candidate.x, 60.0f, field_size.x - 38.0f);
    candidate.y = std::clamp(candidate.y, 32.0f, field_size.y - 32.0f);
    return candidate;
}

Vector2 MatchEngine::_compute_tactical_target(
    const SimPlayer& p_player,
    int p_index,
    int p_primary_presser,
    int p_secondary_presser,
    float p_team_backline_x
) const {
    bool is_home = p_player.is_home;
    float attack_dir = is_home ? 1.0f : -1.0f;
    float own_goal_x = is_home ? 0.0f : field_size.x;
    float opp_goal_x = is_home ? field_size.x : 0.0f;

    int team_mentality = is_home ? home_mentality : away_mentality;
    int team_pressing = is_home ? home_pressing : away_pressing;
    int team_def_line = is_home ? home_defensive_line : away_defensive_line;
    int team_width = is_home ? home_attacking_width : away_attacking_width;

    static const float MENTALITY_SHIFTS[5] = { -60.0f, -30.0f, 0.0f, 40.0f, 80.0f };
    float mentality_shift = MENTALITY_SHIFTS[std::clamp(team_mentality, 0, 4)];
    float def_line_shift = (team_def_line == TIER_HIGH) ? 45.0f : ((team_def_line == TIER_LOW) ? -38.0f : 0.0f);
    float width_scale = (team_width == TIER_HIGH) ? 1.14f : ((team_width == TIER_LOW) ? 0.84f : 1.0f);

    float ball_prog = is_home ? (ball_position.x / field_size.x) : ((field_size.x - ball_position.x) / field_size.x);
    ball_prog = std::clamp(ball_prog, 0.0f, 1.0f);

    float center_y = field_size.y * 0.5f;
    float ball_center_offset_y = ball_position.y - center_y;
    String r = p_player.role;

    // 1. Goalkeeper Positioning
    if (r == "GK") {
        if (ball_carrier_index == p_index) {
            return p_player.position;
        }
        float gk_depth = 18.0f + (ball_prog * 14.0f);
        float target_x = own_goal_x + (attack_dir * gk_depth);
        float target_y = center_y + (ball_center_offset_y * 0.14f);
        return Vector2(target_x, std::clamp(target_y, field_size.y * 0.40f, field_size.y * 0.60f));
    }

    // 2. User-Controlled Player Target during Manual Chance
    if (manual_control_active && p_index == user_controlled_index) {
        if (has_user_move_target) {
            return user_move_target;
        }
        return p_player.position;
    }

    // 3. AI Ball Carrier Dribbling
    if (ball_carrier_index == p_index && ball_state == BALL_CARRIED) {
        Vector2 goal_center(opp_goal_x - (attack_dir * 75.0f), center_y);
        Vector2 to_goal = (goal_center - p_player.position);
        float dist_to_goal = to_goal.length();
        Vector2 dir = (dist_to_goal > 1.0f) ? (to_goal / dist_to_goal) : Vector2(attack_dir, 0.0f);

        Vector2 evade(0.0f, 0.0f);
        int opp_start = is_home ? 11 : 0;
        for (int j = opp_start; j < opp_start + 11; j++) {
            Vector2 diff = p_player.position - sim_players[j].position;
            float d = diff.length();
            if (d < 65.0f && d > 1.0f) {
                evade += (diff / d) * ((65.0f - d) / 65.0f);
            }
        }
        Vector2 move_dir = (dir + evade * 0.65f).normalized();
        return p_player.position + move_dir * 45.0f;
    }

    // 4. Pass Receiver Moving Into Ball Path / Space Target
    if (ball_state == BALL_IN_PASS && pass_target_index == p_index) {
        return ball_target;
    }

    // 5. Loose Ball Contest
    if (ball_state == BALL_LOOSE && p_index == p_primary_presser) {
        return ball_position;
    }

    bool team_in_possession = (possession_team == (is_home ? 0 : 1));
    float widened_anchor_y = center_y + ((p_player.anchor.y - center_y) * width_scale);

    // 6. ATTACKING PHASE
    if (team_in_possession) {
        if (is_home && chance_state == CHANCE_HOME_ATTACK && r != "CB") {
            return _compute_smart_attacking_run_target(p_player, p_index, widened_anchor_y, ball_prog);
        }

        float push_x = 0.0f;
        float lateral_shift = ball_center_offset_y * 0.28f;

        if (r == "CB") {
            push_x = (ball_prog * 210.0f) + (mentality_shift * 0.45f) + (def_line_shift * 0.5f);
            lateral_shift = ball_center_offset_y * 0.22f;
        } else if (r == "LB" || r == "RB" || r == "LWB" || r == "RWB") {
            float overlap_bonus = (team_mentality >= MENTALITY_ATTACKING) ? 145.0f :
                                  ((team_mentality <= MENTALITY_DEFENSIVE) ? 45.0f : 95.0f);
            push_x = (ball_prog * 285.0f) + overlap_bonus;
            lateral_shift = ball_center_offset_y * 0.16f;
        } else if (r == "CDM") {
            push_x = (ball_prog * 270.0f) + (mentality_shift * 0.6f);
            lateral_shift = ball_center_offset_y * 0.35f;
        } else if (r == "CM" || r == "CAM") {
            float cam_bonus = (r == "CAM") ? 95.0f : 55.0f;
            push_x = (ball_prog * 335.0f) + cam_bonus + mentality_shift;
            lateral_shift = ball_center_offset_y * 0.32f;
        } else if (r == "LW" || r == "RW" || r == "LM" || r == "RM") {
            push_x = (ball_prog * 355.0f) + 125.0f + mentality_shift;
            lateral_shift = ball_center_offset_y * 0.15f;
        } else {
            push_x = (ball_prog * 365.0f) + 155.0f + mentality_shift;
            lateral_shift = ball_center_offset_y * 0.26f;
        }

        float target_x = p_player.anchor.x + (attack_dir * push_x);
        float max_adv_x = is_home ? (field_size.x - 48.0f) : 48.0f;
        target_x = is_home ? std::min(target_x, max_adv_x) : std::max(target_x, max_adv_x);

        float target_y = widened_anchor_y + lateral_shift;
        return Vector2(target_x, target_y);
    }

    // 7. ORGANIZED DEFENDING PHASE
    float ball_dist_from_own_goal = std::abs(ball_position.x - own_goal_x);
    float press_trigger_dist = (team_pressing == PRESSING_HIGH) ? 420.0f :
                               ((team_pressing == PRESSING_LOW) ? 235.0f : 315.0f);

    if (!is_home && chance_state == CHANCE_HOME_ATTACK) {
        press_trigger_dist = 185.0f;
    }

    if (p_index == p_primary_presser && !(manual_control_active && is_home)) {
        if (ball_dist_from_own_goal <= press_trigger_dist) {
            return ball_position - Vector2(attack_dir * 8.0f, 0.0f);
        }
        return ball_position - Vector2(attack_dir * 28.0f, (ball_position.y - center_y) * 0.12f);
    }

    bool allow_smart_lane_cover = !(!is_home && chance_state == CHANCE_HOME_ATTACK);
    if (allow_smart_lane_cover && p_index == p_secondary_presser && team_pressing >= PRESSING_NORMAL && ball_dist_from_own_goal <= press_trigger_dist + 40.0f) {
        return _compute_lane_cover_target(is_home, p_player);
    }

    float line_x = p_team_backline_x;
    if (r == "CB") {
        line_x = p_team_backline_x;
    } else if (r == "LB" || r == "RB" || r == "LWB" || r == "RWB") {
        bool is_ball_side = ((p_player.anchor.y - center_y) * ball_center_offset_y) >= 0.0f;
        line_x = p_team_backline_x + (attack_dir * (is_ball_side ? 14.0f : 4.0f));
    } else if (r == "CDM") {
        line_x = p_team_backline_x + (attack_dir * 66.0f);
    } else if (r == "CM" || r == "CAM") {
        float mid_gap = (r == "CAM") ? 132.0f : 104.0f;
        line_x = p_team_backline_x + (attack_dir * mid_gap);
    } else if (r == "LW" || r == "RW" || r == "LM" || r == "RM") {
        float wing_gap = (team_mentality >= MENTALITY_ATTACKING) ? 148.0f : 114.0f;
        line_x = p_team_backline_x + (attack_dir * wing_gap);
    } else {
        float st_gap = (team_mentality <= MENTALITY_DEFENSIVE) ? 175.0f : 215.0f;
        line_x = p_team_backline_x + (attack_dir * st_gap);
    }

    line_x = std::clamp(line_x, 86.0f, field_size.x - 86.0f);

    float depth_compactness = std::clamp(0.68f + (ball_dist_from_own_goal / field_size.x) * 0.24f, 0.68f, 0.92f);
    float compact_anchor_y = center_y + ((p_player.anchor.y - center_y) * depth_compactness);
    float ball_shift_y = ball_center_offset_y * 0.46f;
    float compact_y = std::clamp(compact_anchor_y + ball_shift_y, 38.0f, field_size.y - 38.0f);

    if (_is_backline_role(r) || r == "CDM") {
        Vector2 track_offset = _compute_runner_tracking_offset(is_home, p_player, line_x, compact_y);
        line_x += track_offset.x;
        compact_y += track_offset.y;

        float min_goal_clearance = (ball_dist_from_own_goal < 115.0f) ? 74.0f : 104.0f;
        line_x = is_home
            ? std::max(line_x, own_goal_x + min_goal_clearance)
            : std::min(line_x, own_goal_x - min_goal_clearance);
    }

    return Vector2(line_x, compact_y);
}

void MatchEngine::_update_player_movement(float dt) {
    const float margin = 14.0f;
    const float min_separation = 26.0f;

    int home_p1 = _select_smart_presser(true, -1);
    int home_p2 = _select_smart_presser(true, home_p1);
    int away_p1 = _select_smart_presser(false, -1);
    int away_p2 = _select_smart_presser(false, away_p1);

    float home_backline_x = _compute_organized_backline_x(true);
    float away_backline_x = _compute_organized_backline_x(false);

    std::vector<Vector2> desired_targets(22);
    for (int i = 0; i < 22; i++) {
        bool is_home = (i < 11);
        int p1 = is_home ? home_p1 : away_p1;
        int p2 = is_home ? home_p2 : away_p2;
        float line_x = is_home ? home_backline_x : away_backline_x;
        desired_targets[i] = _compute_tactical_target(sim_players[i], i, p1, p2, line_x);
    }

    float default_smooth_factor = 1.0f - std::exp(-7.5f * dt);
    float dribble_smooth_factor = 1.0f - std::exp(-18.5f * dt);

    for (int i = 0; i < 22; i++) {
        SimPlayer& p = sim_players[i];
        if (p.recovery_timer > 0.0f) {
            p.recovery_timer = std::max(0.0f, p.recovery_timer - dt);
        }
        if (p.sprint_boost_timer > 0.0f) {
            p.sprint_boost_timer = std::max(0.0f, p.sprint_boost_timer - dt);
        }
        if (p.dribble_boost_timer > 0.0f) {
            p.dribble_boost_timer = std::max(0.0f, p.dribble_boost_timer - dt);
        }

        Vector2 to_target = desired_targets[i] - p.position;
        float dist = to_target.length();

        bool is_user_controlled = (manual_control_active && i == user_controlled_index);
        bool is_user_ball_carrier = (is_user_controlled && ball_carrier_index == i && ball_state == BALL_CARRIED);

        if (is_user_controlled && has_user_move_target && dist < 8.0f) {
            has_user_move_target = false;
        }

        float fatigue_mult = _get_fatigue_multiplier(p);
        int team_tempo = p.is_home ? home_tempo : away_tempo;
        float tempo_speed_mult = (team_tempo == TIER_HIGH) ? 1.07f : ((team_tempo == TIER_LOW) ? 0.93f : 1.0f);

        float max_speed = (95.0f + p.pace * 0.85f) * fatigue_mult * tempo_speed_mult;
        if (p.role == "GK") {
            max_speed *= 0.68f;
        } else if (!p.is_home && chance_state == CHANCE_HOME_ATTACK) {
            max_speed *= 0.84f;
        } else if (p.is_home && chance_state == CHANCE_HOME_ATTACK && i != user_controlled_index && p.role != "CB") {
            max_speed *= 1.12f;
        }

        if (p.recovery_timer > 0.0f) {
            max_speed *= 0.35f;
        } else if (is_user_ball_carrier) {
            // Responsive football dribble speed scaled by pace and dribbling attributes
            max_speed = (114.0f + p.pace * 0.72f + p.dribbling * 0.42f) * fatigue_mult * tempo_speed_mult;
            if (p.dribble_boost_timer > 0.0f) {
                max_speed *= 1.18f;
            }
        } else if (p.sprint_boost_timer > 0.0f) {
            max_speed *= 1.18f;
        } else if (ball_carrier_index == i && ball_state == BALL_CARRIED) {
            max_speed *= 0.92f;
        }

        Vector2 desired_vel(0.0f, 0.0f);
        if (dist > 2.0f) {
            float arrive_gain = is_user_ball_carrier ? 8.5f : 3.5f;
            float arrive_speed = std::min(max_speed, dist * arrive_gain);
            desired_vel = (to_target / dist) * arrive_speed;
        }

        Vector2 sep_force(0.0f, 0.0f);
        bool is_active_presser = (i == home_p1 || i == away_p1);
        float effective_sep = (is_user_controlled || is_active_presser)
            ? 14.5f
            : min_separation;
        for (int j = 0; j < 22; j++) {
            if (i == j) continue;
            Vector2 diff = p.position - sim_players[j].position;
            float d = diff.length();
            if (d < effective_sep) {
                if (d < 0.01f) {
                    sep_force += Vector2((i < j) ? -1.0f : 1.0f, 0.0f) * 120.0f;
                } else {
                    sep_force += (diff / d) * ((effective_sep - d) / effective_sep) * 140.0f;
                }
            }
        }

        desired_vel += sep_force;
        if (desired_vel.length() > max_speed * 1.15f) {
            desired_vel = desired_vel.normalized() * (max_speed * 1.15f);
        }

        float smooth_factor = is_user_ball_carrier ? dribble_smooth_factor : default_smooth_factor;
        p.velocity = p.velocity.lerp(desired_vel, smooth_factor);
        p.position += p.velocity * dt;

        p.position.x = std::clamp(p.position.x, margin, field_size.x - margin);
        p.position.y = std::clamp(p.position.y, margin, field_size.y - margin);

        int team_pressing = p.is_home ? home_pressing : away_pressing;
        int team_mentality = p.is_home ? home_mentality : away_mentality;

        float press_mult = (team_pressing == PRESSING_HIGH) ? 1.46f :
                           ((team_pressing == PRESSING_LOW) ? 0.72f : 1.00f);
        static const float MENTALITY_DRAIN[5] = { 0.82f, 0.90f, 1.00f, 1.15f, 1.30f };
        float mentality_mult = MENTALITY_DRAIN[std::clamp(team_mentality, 0, 4)];
        float tempo_drain = (team_tempo == TIER_HIGH) ? 1.15f : ((team_tempo == TIER_LOW) ? 0.88f : 1.00f);
        float endurance_factor = 1.18f - (static_cast<float>(p.physical) / 100.0f) * 0.36f;
        float sprint_mult = (p.sprint_boost_timer > 0.0f || p.dribble_boost_timer > 0.0f) ? 1.32f : 1.0f;

        float speed_ratio = p.velocity.length() / std::max(1.0f, max_speed);
        float base_drain = 0.43f * press_mult * mentality_mult * tempo_drain * endurance_factor * sprint_mult;
        p.stamina = std::max(10.0f, p.stamina - (base_drain * (0.45f + speed_ratio) * dt));
    }
}

void MatchEngine::_execute_pass(int p_passer_idx, int p_receiver_idx, bool p_is_through_ball) {
    SimPlayer& passer = sim_players[p_passer_idx];
    SimPlayer& receiver = sim_players[p_receiver_idx];

    float fatigue = _get_fatigue_multiplier(passer);
    passer.stamina = std::max(10.0f, passer.stamina - (p_is_through_ball ? 0.45f : 0.22f));

    float attack_dir = passer.is_home ? 1.0f : -1.0f;
    Vector2 lead_offset = p_is_through_ball
        ? Vector2(attack_dir * 52.0f, receiver.velocity.y * 0.25f)
        : (receiver.velocity * 0.18f);

    if (p_is_through_ball) {
        receiver.sprint_boost_timer = 1.6f;
    }

    Vector2 target = receiver.position + lead_offset;
    target.x = std::clamp(target.x, 18.0f, field_size.x - 18.0f);
    target.y = std::clamp(target.y, 18.0f, field_size.y - 18.0f);

    float effective_passing = passer.passing * fatigue;
    float inaccuracy = (100.0f - effective_passing) * (p_is_through_ball ? 0.20f : 0.15f);
    target.x += static_cast<float>(UtilityFunctions::randf_range(-inaccuracy, inaccuracy));
    target.y += static_cast<float>(UtilityFunctions::randf_range(-inaccuracy, inaccuracy));

    Vector2 delta = target - ball_position;
    float dist = std::max(1.0f, delta.length());
    float pass_speed = std::clamp((260.0f + effective_passing * 1.38f + dist * 0.25f), 260.0f, 450.0f);

    if (chance_state == CHANCE_AWAY_ATTACK && !passer.is_home) {
        pass_speed *= 0.86f;
    }

    ball_target = target;
    ball_velocity = (delta / dist) * pass_speed;
    ball_state = BALL_IN_PASS;
    ball_carrier_index = -1;
    pass_target_index = p_receiver_idx;
    is_through_ball_in_flight = p_is_through_ball;
    last_touch_index = p_passer_idx;

    if (passer.is_home) {
        stats.home_passes++;
    } else {
        stats.away_passes++;
    }

    if (p_is_through_ball) {
        _log_event("through_ball", passer.is_home, passer.name,
            passer.name + String(" threads a through ball toward ") + receiver.name);
    }
}

void MatchEngine::_execute_shot(int p_shooter_idx, float p_xg, Vector2 p_custom_target, float p_power_ratio) {
    SimPlayer& shooter = sim_players[p_shooter_idx];
    bool is_home = shooter.is_home;
    float fatigue = _get_fatigue_multiplier(shooter);
    shooter.stamina = std::max(10.0f, shooter.stamina - 0.75f);

    float opp_goal_x = is_home ? field_size.x : 0.0f;
    float center_y = field_size.y * 0.5f;
    Vector2 opp_goal_center(opp_goal_x, center_y);
    float dist_to_goal = shooter.position.distance_to(opp_goal_center);

    float goal_y;
    if (p_custom_target.x >= 0.0f && p_custom_target.y >= 0.0f) {
        Vector2 aim_vec = (p_custom_target - ball_position);
        float aim_len = aim_vec.length();
        Vector2 aim_dir = (aim_len > 1.0f) ? (aim_vec / aim_len) : Vector2(is_home ? 1.0f : -1.0f, 0.0f);
        bool forward_toward_goal = is_home ? (aim_dir.x > 0.08f) : (aim_dir.x < -0.08f);

        if (forward_toward_goal) {
            float t = std::abs(opp_goal_x - ball_position.x) / std::abs(aim_dir.x);
            goal_y = ball_position.y + (aim_dir.y * t);
        } else {
            goal_y = p_custom_target.y;
        }
        float spread = ((100.0f - shooter.shooting * fatigue) * 0.24f) + (std::max(0.0f, p_power_ratio - 0.92f) * 34.0f);
        goal_y += static_cast<float>(UtilityFunctions::randf_range(-spread, spread));
    } else {
        float spread = 32.0f + ((1.0f - fatigue) * 22.0f);
        goal_y = center_y + static_cast<float>(UtilityFunctions::randf_range(-spread, spread));
    }

    goal_y = std::clamp(goal_y, 24.0f, field_size.y - 24.0f);

    shot_hits_crossbar = false;
    last_shot_power_ratio = p_power_ratio;
    if (p_power_ratio > 1.04f && dist_to_goal > 175.0f) {
        float overpower = std::clamp((p_power_ratio - 1.04f) / 0.31f, 0.0f, 1.0f);
        float long_range_factor = std::clamp((dist_to_goal - 175.0f) / 160.0f, 0.0f, 1.0f);
        float high_ball_prob = std::clamp((0.28f + overpower * 0.42f) * (0.45f + long_range_factor * 0.55f), 0.0f, 0.85f);

        if (UtilityFunctions::randf() < high_ball_prob) {
            if (UtilityFunctions::randf() < 0.68f) {
                shot_hits_crossbar = true;
                goal_y = std::clamp(goal_y, center_y - 38.0f, center_y + 38.0f);
            } else {
                float over_sign = (goal_y >= center_y) ? 1.0f : -1.0f;
                goal_y = std::clamp(center_y + over_sign * static_cast<float>(UtilityFunctions::randf_range(50.0, 68.0)), 24.0f, field_size.y - 24.0f);
            }
        }
    }

    Vector2 target(opp_goal_x, goal_y);
    Vector2 delta = target - ball_position;
    float dist = std::max(1.0f, delta.length());
    float shot_speed = (375.0f + shooter.shooting * 1.45f) * fatigue * std::clamp(p_power_ratio, 0.65f, 1.35f);

    ball_target = target;
    ball_velocity = (delta / dist) * shot_speed;
    ball_state = BALL_IN_SHOT;
    ball_carrier_index = -1;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    last_touch_index = p_shooter_idx;

    if (is_home) {
        stats.home_shots++;
        stats.home_xg += p_xg;
    } else {
        stats.away_shots++;
        stats.away_xg += p_xg;
    }

    _log_event("shot", is_home, shooter.name,
        shooter.name + String(" unleashes a shot toward goal!"));
}

Vector3 MatchEngine::_to_3d_coords(Vector2 p_pos_2d) const {
    return Vector3(
        (p_pos_2d.x - (field_size.x * 0.5f)) * 0.1f,
        0.0f,
        (p_pos_2d.y - (field_size.y * 0.5f)) * 0.1f
    );
}

void MatchEngine::_trigger_key_chance(int p_scenario_type, bool p_is_home_attacking, int p_attacker_idx, float p_xg) {
    chance_state = p_is_home_attacking ? CHANCE_HOME_ATTACK : CHANCE_AWAY_ATTACK;
    manual_control_active = true;
    active_chance_timer = p_is_home_attacking ? 14.0f : 16.5f;
    event_cooldown = 12.0f;
    pending_chance_xg = p_xg;
    has_user_move_target = false;

    if (p_is_home_attacking) {
        user_controlled_index = (p_attacker_idx >= 1 && p_attacker_idx < 11) ? p_attacker_idx : 9;
    } else {
        select_nearest_defender();
    }

    last_scenario_3d.ball_pos_3d = _to_3d_coords(ball_position);
    last_scenario_3d.attacker_pos_3d = _to_3d_coords(sim_players[p_attacker_idx].position);
    last_scenario_3d.defender_positions_3d.clear();
    last_scenario_3d.teammate_positions_3d.clear();

    for (int i = 0; i < 22; i++) {
        if (i == p_attacker_idx) continue;
        if (sim_players[i].is_home == p_is_home_attacking) {
            last_scenario_3d.teammate_positions_3d.append(_to_3d_coords(sim_players[i].position));
        } else {
            last_scenario_3d.defender_positions_3d.append(_to_3d_coords(sim_players[i].position));
        }
    }

    String attacker_name = sim_players[p_attacker_idx].name;
    String desc = p_is_home_attacking
        ? (attacker_name + String(" breaks into an ATTACK CHANCE!"))
        : (attacker_name + String(" threatens the box — DEFEND CHANCE!"));
    _log_event(p_is_home_attacking ? "attack_chance" : "defend_chance", p_is_home_attacking, attacker_name, desc);

    emit_signal("chance_state_changed", chance_state, user_controlled_index);
    emit_signal("interactive_event_triggered", p_scenario_type, p_is_home_attacking, last_scenario_3d.ball_pos_3d);
}

void MatchEngine::_evaluate_carrier_action(int p_carrier_idx) {
    if (manual_control_active && p_carrier_idx == user_controlled_index) {
        return;
    }

    SimPlayer& carrier = sim_players[p_carrier_idx];
    bool is_home = carrier.is_home;
    float attack_dir = is_home ? 1.0f : -1.0f;
    float opp_goal_x = is_home ? field_size.x : 0.0f;
    Vector2 opp_goal_center(opp_goal_x, field_size.y * 0.5f);

    float fatigue = _get_fatigue_multiplier(carrier);
    int team_mentality = is_home ? home_mentality : away_mentality;
    int opp_mentality = is_home ? away_mentality : home_mentality;
    int team_passing_style = is_home ? home_passing_style : away_passing_style;

    static const float MENTALITY_CHANCE_MULT[5] = { 0.65f, 0.82f, 1.00f, 1.24f, 1.48f };
    static const float OPP_EXPOSURE_MULT[5]     = { 0.80f, 0.90f, 1.00f, 1.15f, 1.32f };
    float chance_creation_mult = MENTALITY_CHANCE_MULT[std::clamp(team_mentality, 0, 4)]
                               * OPP_EXPOSURE_MULT[std::clamp(opp_mentality, 0, 4)]
                               * fatigue;

    float dist_to_goal = carrier.position.distance_to(opp_goal_center);
    float lateral_offset = std::abs(carrier.position.y - (field_size.y * 0.5f));

    int opp_start = is_home ? 11 : 0;
    float nearest_opp_dist = 1e9f;
    int nearest_opp_idx = opp_start + 1;
    for (int j = opp_start; j < opp_start + 11; j++) {
        float d = carrier.position.distance_to(sim_players[j].position);
        if (d < nearest_opp_dist) {
            nearest_opp_dist = d;
            nearest_opp_idx = j;
        }
    }

    // 0. LIVE-POSITION CHANCE TRIGGER
    if (chance_state == CHANCE_NONE && event_cooldown <= 0.0f && carrier.role != "GK") {
        bool in_attacking_half = is_home ? (carrier.position.x >= field_size.x * 0.48f)
                                         : (carrier.position.x <= field_size.x * 0.52f);
        if (in_attacking_half) {
            float trigger_prob = std::clamp(0.45f * chance_creation_mult, 0.25f, 0.90f);
            if (is_home || UtilityFunctions::randf() < trigger_prob) {
                _spawn_wide_range_chance(is_home);
                return;
            }
        }
    }

    // 1. SHOOTING EVALUATION
    if ((!is_home || home_auto_shoot_enabled) && carrier.role != "GK" && dist_to_goal < 320.0f && lateral_offset < 235.0f) {
        float dist_factor = std::clamp(1.0f - (dist_to_goal / 340.0f), 0.06f, 0.90f);
        float angle_factor = std::clamp(1.0f - (lateral_offset / 250.0f), 0.25f, 1.0f);
        float pressure_factor = std::clamp(nearest_opp_dist / 55.0f, 0.45f, 1.15f);
        float xg = std::clamp(dist_factor * angle_factor * pressure_factor * (0.45f + carrier.shooting * fatigue * 0.004f), 0.04f, 0.65f);

        if (chance_state == CHANCE_AWAY_ATTACK && !is_home) {
            float long_shot_bias = (dist_to_goal > 180.0f) ? 0.22f : 0.44f;
            if (dist_to_goal < 145.0f || UtilityFunctions::randf() < (long_shot_bias + xg)) {
                _execute_shot(p_carrier_idx, xg);
                return;
            }
        }
    }

    // 2. PASSING EVALUATION
    int team_start = is_home ? 0 : 11;
    float prog_weight = (team_mentality >= MENTALITY_ATTACKING) ? 1.45f :
                        ((team_mentality <= MENTALITY_DEFENSIVE) ? 0.70f : 1.05f);
    if (team_passing_style == TIER_HIGH) prog_weight *= 1.25f;

    int best_receiver = -1;
    float best_pass_score = -1e9f;
    bool best_is_through = false;

    for (int t = team_start; t < team_start + 11; t++) {
        if (t == p_carrier_idx) continue;
        if (sim_players[t].role == "GK" && dist_to_goal < field_size.x * 0.55f) continue;

        Vector2 pass_vec = sim_players[t].position - carrier.position;
        float pass_dist = pass_vec.length();
        float max_range = (170.0f + (carrier.passing * 2.1f)) * fatigue;
        if (pass_dist < 32.0f || pass_dist > max_range) continue;

        float forward_gain = (sim_players[t].position.x - carrier.position.x) * attack_dir;

        float receiver_space = 150.0f;
        for (int j = opp_start; j < opp_start + 11; j++) {
            receiver_space = std::min(receiver_space, sim_players[t].position.distance_to(sim_players[j].position));
        }

        float min_lane_clearance = 100.0f;
        Vector2 seg_dir = pass_vec / pass_dist;
        for (int j = opp_start; j < opp_start + 11; j++) {
            Vector2 to_opp = sim_players[j].position - carrier.position;
            float proj = to_opp.dot(seg_dir);
            if (proj > 12.0f && proj < pass_dist - 10.0f) {
                float perp = std::abs(to_opp.cross(seg_dir));
                min_lane_clearance = std::min(min_lane_clearance, perp);
            }
        }

        if (min_lane_clearance < 15.0f) continue;

        bool is_through = (forward_gain > 75.0f && receiver_space > 36.0f &&
                           ((is_home && sim_players[t].position.x > field_size.x * 0.68f) ||
                            (!is_home && sim_players[t].position.x < field_size.x * 0.32f)));

        float ideal_dist = (team_passing_style == TIER_LOW) ? 85.0f : ((team_passing_style == TIER_HIGH) ? 175.0f : 125.0f);
        float score = (forward_gain * prog_weight)
                    + (receiver_space * 1.15f)
                    + (min_lane_clearance * 0.90f)
                    - (std::abs(pass_dist - ideal_dist) * 0.25f)
                    + (is_through ? 45.0f : 0.0f)
                    + static_cast<float>(UtilityFunctions::randf_range(-18.0, 18.0));

        if (score > best_pass_score) {
            best_pass_score = score;
            best_receiver = t;
            best_is_through = is_through;
        }
    }

    bool under_pressure = (nearest_opp_dist < 38.0f);
    bool should_pass = (best_receiver != -1) &&
                       (under_pressure || carrier.role == "GK" || best_pass_score > 75.0f || UtilityFunctions::randf() < 0.62f);

    if (should_pass) {
        _execute_pass(p_carrier_idx, best_receiver, best_is_through);
        return;
    }

    // 3. DRIBBLE / 1v1 DUEL EVALUATION
    float duel_range = (is_home && chance_state == CHANCE_HOME_ATTACK) ? 16.0f : 24.0f;
    if (nearest_opp_dist < duel_range && carrier.role != "GK") {
        SimPlayer& defender = sim_players[nearest_opp_idx];
        float def_fatigue = _get_fatigue_multiplier(defender);
        int def_pressing = defender.is_home ? home_pressing : away_pressing;
        float press_tackle_bonus = (def_pressing == PRESSING_HIGH) ? 6.0f : ((def_pressing == PRESSING_LOW) ? -4.0f : 0.0f);
        float attack_chance_nerf = (is_home && chance_state == CHANCE_HOME_ATTACK) ? -18.0f : 0.0f;

        float atk_rating = ((carrier.dribbling * 0.65f + carrier.pace * 0.35f) * fatigue)
                         + static_cast<float>(UtilityFunctions::randf_range(-12.0, 12.0));
        float def_rating = ((defender.defending * 0.65f + defender.physical * 0.35f) * def_fatigue)
                         + press_tackle_bonus + attack_chance_nerf
                         + static_cast<float>(UtilityFunctions::randf_range(-12.0, 12.0));

        if (atk_rating >= def_rating) {
            bool in_final_third = is_home ? (carrier.position.x > field_size.x * 0.72f) : (carrier.position.x < field_size.x * 0.28f);
            float duel_chance_prob = std::clamp(0.42f * chance_creation_mult, 0.15f, 0.75f);
            if (chance_state == CHANCE_NONE && in_final_third && event_cooldown <= 0.0f && UtilityFunctions::randf() < duel_chance_prob) {
                _trigger_key_chance(is_home ? 3 : 5, is_home, p_carrier_idx, 0.28f);
                return;
            }
        } else {
            if (defender.is_home) {
                stats.home_tackles++;
            } else {
                stats.away_tackles++;
            }
            _log_event("tackle", defender.is_home, defender.name,
                defender.name + String(" wins the ball with a crisp tackle!"));

            if ((chance_state == CHANCE_HOME_ATTACK && !defender.is_home) ||
                (chance_state == CHANCE_AWAY_ATTACK && defender.is_home)) {
                _end_active_chance(false);
            }

            if (UtilityFunctions::randf() < 0.75f) {
                possession_team = defender.is_home ? 0 : 1;
                ball_carrier_index = nearest_opp_idx;
                last_touch_index = nearest_opp_idx;
                pass_target_index = -1;
                is_through_ball_in_flight = false;
                ball_state = BALL_CARRIED;
                action_decision_timer = 0.35f;
            } else {
                possession_team = -1;
                ball_carrier_index = -1;
                pass_target_index = -1;
                is_through_ball_in_flight = false;
                ball_state = BALL_LOOSE;
                Vector2 deflect_dir = (carrier.position - defender.position).normalized();
                ball_velocity = Vector2(-deflect_dir.y, deflect_dir.x) * 140.0f;
            }
        }
    }
}

void MatchEngine::_update_ball_and_actions(float dt) {
    if (possession_team == 0) {
        stats.home_possession_time += dt;
    } else if (possession_team == 1) {
        stats.away_possession_time += dt;
    }

    // STATE 1: Ball Carried at Player's Feet
    if (ball_state == BALL_CARRIED && ball_carrier_index >= 0 && ball_carrier_index < 22) {
        const SimPlayer& carrier = sim_players[ball_carrier_index];
        float default_dir = carrier.is_home ? 1.0f : -1.0f;
        Vector2 facing = (carrier.velocity.length() > 8.0f)
            ? carrier.velocity.normalized()
            : Vector2(default_dir, 0.0f);

        ball_position = carrier.position + facing * 7.0f;
        ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
        ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);

        if ((chance_state == CHANCE_HOME_ATTACK && !carrier.is_home) ||
            (chance_state == CHANCE_AWAY_ATTACK && carrier.is_home)) {
            _end_active_chance(false);
        }

        int opp_start = carrier.is_home ? 11 : 0;
        int def_pressing = carrier.is_home ? away_pressing : home_pressing;
        float press_bonus = (def_pressing == PRESSING_HIGH) ? 6.0f : ((def_pressing == PRESSING_LOW) ? -4.0f : 0.0f);
        bool nerf_ai_defending = (carrier.is_home && chance_state == CHANCE_HOME_ATTACK);

        for (int j = opp_start; j < opp_start + 11; j++) {
            SimPlayer& defender = sim_players[j];
            if (defender.recovery_timer > 0.0f) {
                continue;
            }

            float dist_to_carrier = defender.position.distance_to(carrier.position);
            float dist_to_ball = defender.position.distance_to(ball_position);
            float engage_dist = std::min(dist_to_carrier, dist_to_ball);
            float tackle_radius = (defender.role == "GK")
                ? 14.5f
                : (nerf_ai_defending ? 14.5f : 21.0f);

            if (engage_dist <= tackle_radius) {
                float atk_fatigue = _get_fatigue_multiplier(carrier);
                float def_fatigue = _get_fatigue_multiplier(defender);
                float collision_bonus = nerf_ai_defending ? 0.0f : ((engage_dist <= 15.5f) ? 16.0f : 4.0f);
                float ai_def_nerf = nerf_ai_defending ? -18.0f : 0.0f;

                float atk_rating = ((carrier.dribbling * 0.65f + carrier.physical * 0.35f) * atk_fatigue)
                                 + static_cast<float>(UtilityFunctions::randf_range(-10.0, 10.0));
                float def_rating = ((defender.defending * 0.65f + defender.physical * 0.35f) * def_fatigue)
                                 + press_bonus + collision_bonus + ai_def_nerf
                                 + static_cast<float>(UtilityFunctions::randf_range(-10.0, 10.0));

                if (def_rating >= atk_rating) {
                    if (defender.is_home) {
                        stats.home_tackles++;
                    } else {
                        stats.away_tackles++;
                    }
                    _log_event("tackle", defender.is_home, defender.name,
                        defender.name + String(" dispossesses ") + carrier.name + String(" with a clean tackle!"));

                    if ((chance_state == CHANCE_HOME_ATTACK && !defender.is_home) ||
                        (chance_state == CHANCE_AWAY_ATTACK && defender.is_home)) {
                        _end_active_chance(false);
                    }

                    possession_team = defender.is_home ? 0 : 1;
                    ball_carrier_index = j;
                    last_touch_index = j;
                    pass_target_index = -1;
                    is_through_ball_in_flight = false;
                    ball_state = BALL_CARRIED;
                    ball_velocity = Vector2(0.0f, 0.0f);
                    action_decision_timer = 0.35f;
                    return;
                } else {
                    defender.recovery_timer = nerf_ai_defending ? 1.15f : 0.65f;
                }
            }
        }

        action_decision_timer -= dt;
        if (action_decision_timer <= 0.0f) {
            float fatigue = _get_fatigue_multiplier(carrier);
            float base_timer = std::clamp(0.68f - (carrier.passing * fatigue * 0.003f), 0.28f, 0.72f);
            if (chance_state == CHANCE_AWAY_ATTACK && !carrier.is_home) {
                base_timer *= 1.28f;
            }
            action_decision_timer = base_timer;
            _evaluate_carrier_action(ball_carrier_index);
        }
        return;
    }

    // STATE 2: Ball In Pass Flight / Through-Ball Space Roll
    if (ball_state == BALL_IN_PASS) {
        ball_position += ball_velocity * dt;

        bool passer_is_home = (last_touch_index >= 0 && last_touch_index < 11);
        int opp_start = passer_is_home ? 11 : 0;
        int def_pressing = passer_is_home ? away_pressing : home_pressing;
        bool nerf_ai_interception = (passer_is_home && chance_state == CHANCE_HOME_ATTACK);

        float base_intercept_radius = nerf_ai_interception
            ? 9.0f
            : ((def_pressing == PRESSING_HIGH) ? 16.5f : ((def_pressing == PRESSING_LOW) ? 11.5f : 13.5f));
        float press_intercept_bonus = (def_pressing == PRESSING_HIGH) ? 0.12f :
                                      ((def_pressing == PRESSING_LOW) ? -0.08f : 0.0f);

        for (int j = opp_start; j < opp_start + 11; j++) {
            float def_fatigue = _get_fatigue_multiplier(sim_players[j]);
            float effective_radius = base_intercept_radius * def_fatigue;
            if (sim_players[j].position.distance_to(ball_position) < effective_radius) {
                float intercept_prob = std::clamp(
                    (0.32f + (sim_players[j].defending * def_fatigue - 65.0f) * 0.006f + press_intercept_bonus),
                    0.12f, 0.78f
                );
                if (nerf_ai_interception) {
                    intercept_prob = std::clamp(intercept_prob * 0.38f, 0.06f, 0.24f);
                }
                if (UtilityFunctions::randf() < intercept_prob) {
                    if (sim_players[j].is_home) {
                        stats.home_interceptions++;
                    } else {
                        stats.away_interceptions++;
                    }
                    possession_team = sim_players[j].is_home ? 0 : 1;
                    ball_carrier_index = j;
                    last_touch_index = j;
                    pass_target_index = -1;
                    is_through_ball_in_flight = false;
                    ball_state = BALL_CARRIED;
                    ball_velocity = Vector2(0.0f, 0.0f);
                    action_decision_timer = 0.35f;
                    _log_event("interception", sim_players[j].is_home, sim_players[j].name,
                        sim_players[j].name + String(" intercepts the pass!"));

                    if ((chance_state == CHANCE_HOME_ATTACK && !sim_players[j].is_home) ||
                        (chance_state == CHANCE_AWAY_ATTACK && sim_players[j].is_home)) {
                        _end_active_chance(false);
                    }
                    return;
                }
            }
        }

        if (pass_target_index >= 0 && pass_target_index < 22) {
            float dist_to_receiver = ball_position.distance_to(sim_players[pass_target_index].position);
            float dist_to_target = ball_position.distance_to(ball_target);

            bool arrived = is_through_ball_in_flight
                ? (dist_to_receiver < 22.0f)
                : (dist_to_receiver < 16.0f || dist_to_target < 12.0f);

            if (!arrived && is_through_ball_in_flight && dist_to_target < 24.0f) {
                ball_velocity *= std::exp(-3.6f * dt);
                if (ball_velocity.length() < 18.0f) {
                    ball_state = BALL_LOOSE;
                    possession_team = -1;
                    pass_target_index = -1;
                    is_through_ball_in_flight = false;
                    return;
                }
            }

            if (arrived) {
                if (passer_is_home) {
                    stats.home_passes_completed++;
                } else {
                    stats.away_passes_completed++;
                }
                int receiver_idx = pass_target_index;
                bool rec_is_home = sim_players[receiver_idx].is_home;
                possession_team = rec_is_home ? 0 : 1;
                ball_carrier_index = receiver_idx;
                last_touch_index = receiver_idx;
                pass_target_index = -1;
                is_through_ball_in_flight = false;
                ball_state = BALL_CARRIED;
                ball_velocity = Vector2(0.0f, 0.0f);
                action_decision_timer = (chance_state == CHANCE_AWAY_ATTACK && !rec_is_home) ? 0.45f : 0.30f;

                if (chance_state == CHANCE_NONE && event_cooldown <= 0.0f && sim_players[receiver_idx].role != "GK") {
                    bool in_atk_half = rec_is_home ? (sim_players[receiver_idx].position.x >= field_size.x * 0.48f)
                                                   : (sim_players[receiver_idx].position.x <= field_size.x * 0.52f);
                    if (in_atk_half && (rec_is_home || UtilityFunctions::randf() < 0.45f)) {
                        _spawn_wide_range_chance(rec_is_home);
                    }
                }
                return;
            }
        }

        if (ball_position.x <= 10.0f || ball_position.x >= field_size.x - 10.0f ||
            ball_position.y <= 10.0f || ball_position.y >= field_size.y - 10.0f) {
            ball_position.x = std::clamp(ball_position.x, 12.0f, field_size.x - 12.0f);
            ball_position.y = std::clamp(ball_position.y, 12.0f, field_size.y - 12.0f);
            ball_velocity *= 0.25f;
            ball_state = BALL_LOOSE;
            possession_team = -1;
            pass_target_index = -1;
            is_through_ball_in_flight = false;
            if (chance_state != CHANCE_NONE) {
                _end_active_chance(false);
            }
        }
        return;
    }

    // STATE 3: Live 2D Shot In Flight
    if (ball_state == BALL_IN_SHOT) {
        ball_position += ball_velocity * dt;

        int shooter_idx = std::clamp(last_touch_index, 0, 21);
        bool shooter_is_home = (shooter_idx < 11);
        int def_gk_idx = shooter_is_home ? 11 : 0;
        const SimPlayer& shooter = sim_players[shooter_idx];
        const SimPlayer& gk = sim_players[def_gk_idx];

        int opp_start = shooter_is_home ? 11 : 0;
        bool nerf_ai_blocks = (shooter_is_home && chance_state == CHANCE_HOME_ATTACK);
        float block_radius = nerf_ai_blocks ? 8.5f : 13.0f;
        for (int j = opp_start + 1; j < opp_start + 11; j++) {
            float d_ball = sim_players[j].position.distance_to(ball_position);
            float d_shooter = sim_players[j].position.distance_to(shooter.position);
            if (d_ball < block_radius && d_shooter > 14.0f) {
                float def_fatigue = _get_fatigue_multiplier(sim_players[j]);
                float block_prob = std::clamp(0.30f + (sim_players[j].defending * def_fatigue - 65.0f) * 0.006f, 0.15f, 0.72f);
                if (nerf_ai_blocks) {
                    block_prob *= 0.35f;
                }
                if (UtilityFunctions::randf() < block_prob) {
                    if (sim_players[j].is_home) {
                        stats.home_tackles++;
                    } else {
                        stats.away_tackles++;
                    }
                    _log_event("shot_blocked", sim_players[j].is_home, sim_players[j].name,
                        sim_players[j].name + String(" throws in a vital block!"));

                    shot_hits_crossbar = false;
                    ball_state = BALL_LOOSE;
                    possession_team = -1;
                    ball_velocity = Vector2(-ball_velocity.x * 0.35f, static_cast<float>(UtilityFunctions::randf_range(-90.0, 90.0)));
                    _end_active_chance(false);
                    return;
                }
            }
        }

        bool reached_goal_line = shooter_is_home ? (ball_position.x >= field_size.x - 14.0f) : (ball_position.x <= 14.0f);
        bool near_gk = !shot_hits_crossbar && (ball_position.distance_to(gk.position) < 16.0f);

        if (reached_goal_line || near_gk) {
            if (shot_hits_crossbar && reached_goal_line) {
                Vector2 impact_pos(shooter_is_home ? (field_size.x - 14.0f) : 14.0f, ball_position.y);
                float rebound_dir = shooter_is_home ? -1.0f : 1.0f;

                _log_event("crossbar", shooter_is_home, shooter.name,
                    shooter.name + String("'s rocket from distance rattles the crossbar!"));
                emit_signal("shot_hit_crossbar", shooter_is_home, impact_pos);

                shot_hits_crossbar = false;
                ball_position = impact_pos + Vector2(rebound_dir * 8.0f, 0.0f);
                ball_state = BALL_LOOSE;
                possession_team = -1;
                pass_target_index = -1;
                is_through_ball_in_flight = false;
                ball_velocity = Vector2(rebound_dir * 165.0f, static_cast<float>(UtilityFunctions::randf_range(-105.0, 105.0)));
                _end_active_chance(false);
                return;
            }

            float goal_half_span = 44.0f;
            float offset_from_center = std::abs(ball_position.y - (field_size.y * 0.5f));
            bool on_target = offset_from_center <= goal_half_span;

            if (!on_target) {
                shot_hits_crossbar = false;
                if (offset_from_center > goal_half_span + 20.0f) {
                    _log_event("shot_over", shooter_is_home, shooter.name,
                        shooter.name + String(" fires high over the crossbar!"));
                } else {
                    _log_event("shot_wide", shooter_is_home, shooter.name,
                        shooter.name + String("'s effort drifts wide of the post."));
                }
                _end_active_chance(false);
                _give_gk_possession(!shooter_is_home);
                return;
            }

            if (shooter_is_home) {
                stats.home_shots_on_target++;
            } else {
                stats.away_shots_on_target++;
            }

            bool can_score_now = match_active && (
                (shooter_is_home && chance_state == CHANCE_HOME_ATTACK) ||
                (!shooter_is_home && chance_state == CHANCE_AWAY_ATTACK)
            );

            float gk_fatigue = _get_fatigue_multiplier(gk);
            float shooter_fatigue = _get_fatigue_multiplier(shooter);
            float gk_reach_offset = std::abs(ball_position.y - gk.position.y);
            float corner_penalty = std::clamp(gk_reach_offset / 44.0f, 0.0f, 1.0f) * 0.22f;
            float power_penalty = std::clamp((last_shot_power_ratio - 0.65f) * 0.16f, 0.0f, 0.11f);

            float raw_save_prob = 0.34f
                + (((gk.defending * gk_fatigue) - (shooter.shooting * shooter_fatigue)) * 0.0032f)
                - corner_penalty
                - power_penalty;

            float save_prob = (shooter_is_home && chance_state == CHANCE_HOME_ATTACK)
                ? std::clamp(raw_save_prob * 0.82f, 0.06f, 0.42f)
                : std::clamp(raw_save_prob, 0.08f, 0.52f);

            if (!can_score_now || UtilityFunctions::randf() < save_prob) {
                shot_hits_crossbar = false;
                if (!shooter_is_home) {
                    stats.home_saves++;
                } else {
                    stats.away_saves++;
                }
                _log_event("save", !shooter_is_home, gk.name,
                    gk.name + String(" makes a clean save!"));
                _end_active_chance(false);
                _give_gk_possession(!shooter_is_home);
            } else {
                shot_hits_crossbar = false;
                if (shooter_is_home) {
                    home_score++;
                } else {
                    away_score++;
                }
                _log_event("goal", shooter_is_home, shooter.name,
                    String("GOAL! ") + shooter.name + String(" finds the back of the net!"));
                _end_active_chance(true);
                _reset_after_goal(!shooter_is_home);
                emit_signal("goal_scored", home_score, away_score, shooter_is_home);
            }
        }
        return;
    }

    // STATE 4: Loose Ball Recovery
    ball_position += ball_velocity * dt;
    ball_velocity *= std::exp(-2.8f * dt);
    ball_position.x = std::clamp(ball_position.x, 12.0f, field_size.x - 12.0f);
    ball_position.y = std::clamp(ball_position.y, 12.0f, field_size.y - 12.0f);

    int closest_idx = -1;
    float closest_dist = 16.0f;
    for (int i = 0; i < 22; i++) {
        float d = sim_players[i].position.distance_to(ball_position);
        if (d < closest_dist) {
            closest_dist = d;
            closest_idx = i;
        }
    }

    if (closest_idx != -1) {
        possession_team = sim_players[closest_idx].is_home ? 0 : 1;
        ball_carrier_index = closest_idx;
        last_touch_index = closest_idx;
        ball_state = BALL_CARRIED;
        ball_velocity = Vector2(0.0f, 0.0f);
        action_decision_timer = 0.30f;
    }
}

void MatchEngine::_step_simulation(float dt) {
    if (chance_state != CHANCE_NONE && active_chance_timer > 0.0f) {
        active_chance_timer -= dt;
        if (active_chance_timer <= 0.0f) {
            _log_event("chance_expired", chance_state == CHANCE_HOME_ATTACK, "Referee",
                "The attacking move breaks down and play resets.");
            _end_active_chance(false);
        }
    }

    _update_player_movement(dt);
    _update_ball_and_actions(dt);
    _sync_player_positions_array();
}

void MatchEngine::_physics_process(double delta) {
    if (!match_active || is_paused_for_manual || waiting_for_kickoff_resume) {
        return;
    }

    float raw_dt = static_cast<float>(delta);
    if (raw_dt <= 0.0f) {
        return;
    }

    float dt = std::min(raw_dt, 0.05f);

    if (chance_state == CHANCE_AWAY_ATTACK) {
        dt *= defensive_chance_time_scale;
    }

    match_timer += dt;
    int new_minute = std::min(90, static_cast<int>(match_timer / 1.0f));

    if (new_minute != current_minute) {
        current_minute = new_minute;
        emit_signal("minute_changed", current_minute);
    }

    if (current_minute >= 90) {
        _end_active_chance(false);
        waiting_for_kickoff_resume = false;
        kickoff_team = -1;
        match_active = false;
        is_paused_for_manual = false;
        set_physics_process(false);
        record_manual_match_completed();
        emit_signal("match_ended", home_score, away_score);
        return;
    }

    if (event_cooldown > 0.0f) {
        event_cooldown = std::max(0.0f, event_cooldown - dt);
    }

    _step_simulation(dt);
}

void MatchEngine::start_match() {
    if (formation_db.empty()) {
        load_default_formations();
    }

    match_timer = 0.0f;
    current_minute = 0;
    home_score = 0;
    away_score = 0;
    is_paused_for_manual = false;
    waiting_for_kickoff_resume = false;
    kickoff_team = -1;
    shot_hits_crossbar = false;
    match_active = true;
    event_cooldown = 4.0f;
    pending_chance_xg = 0.0f;

    chance_state = CHANCE_NONE;
    manual_control_active = false;
    user_controlled_index = -1;
    has_user_move_target = false;
    user_move_target = Vector2(-1.0f, -1.0f);
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    active_chance_timer = 0.0f;

    stats = MatchStats();

    for (auto& p : sim_players) {
        p.stamina = std::clamp(p.starting_stamina, 10.0f, 100.0f);
        p.recovery_timer = 0.0f;
        p.sprint_boost_timer = 0.0f;
        p.dribble_boost_timer = 0.0f;
    }

    _reset_field_positions();
    set_physics_process(true);
}

void MatchEngine::pause_match(bool p_pause) {
    if (!match_active) {
        return;
    }
    is_paused_for_manual = p_pause;
}

void MatchEngine::resolve_manual_scenario(bool p_success, bool p_is_home_team) {
    if (!match_active) {
        UtilityFunctions::printerr("MatchEngine: Ignored resolve_manual_scenario() while match is inactive.");
        return;
    }

    bool valid_attack_chance = (p_is_home_team && chance_state == CHANCE_HOME_ATTACK) ||
                               (!p_is_home_team && chance_state == CHANCE_AWAY_ATTACK);

    if (!valid_attack_chance && !is_paused_for_manual) {
        UtilityFunctions::printerr("MatchEngine: Blocked resolve_manual_scenario() outside active ATTACK CHANCE.");
        return;
    }

    float xg_value = (pending_chance_xg > 0.0f) ? pending_chance_xg : 0.25f;
    if (p_is_home_team) {
        stats.home_shots++;
        stats.home_xg += xg_value;
    } else {
        stats.away_shots++;
        stats.away_xg += xg_value;
    }

    if (p_success && valid_attack_chance) {
        if (p_is_home_team) {
            home_score++;
            stats.home_shots_on_target++;
        } else {
            away_score++;
            stats.away_shots_on_target++;
        }
        int scorer_idx = (ball_carrier_index >= 0 && ball_carrier_index < 22)
            ? ball_carrier_index
            : (p_is_home_team ? 9 : 20);
        String scorer_name = (scorer_idx >= 0 && scorer_idx < static_cast<int>(sim_players.size()))
            ? sim_players[scorer_idx].name
            : (p_is_home_team ? String("Home Attack") : String("Away Attack"));
        _log_event("goal", p_is_home_team, scorer_name, scorer_name + String(" scores from key chance!"));
        _end_active_chance(true);
        _reset_after_goal(!p_is_home_team);
        emit_signal("goal_scored", home_score, away_score, p_is_home_team);
    } else {
        if (p_is_home_team) {
            stats.away_saves++;
        } else {
            stats.home_saves++;
        }
        _log_event("chance_saved", !p_is_home_team, !p_is_home_team ? "Home GK" : "Away GK", "Key chance denied by the defense!");
        _end_active_chance(false);
        _give_gk_possession(!p_is_home_team);
    }

    _sync_player_positions_array();
    is_paused_for_manual = false;
}

void MatchEngine::_log_event(const String& p_type, bool p_is_home, const String& p_player_name, const String& p_desc) {
    emit_signal("match_event_logged", current_minute, p_type, p_is_home, p_player_name, p_desc);
}

Array MatchEngine::get_player_positions() const {
    return player_positions;
}

PackedStringArray MatchEngine::get_player_roles() const {
    PackedStringArray roles;
    for (const auto& p : sim_players) {
        roles.append(p.role);
    }
    return roles;
}

PackedStringArray MatchEngine::get_player_names() const {
    PackedStringArray names;
    for (const auto& p : sim_players) {
        names.append(p.name);
    }
    return names;
}

PackedFloat32Array MatchEngine::get_player_stamina() const {
    PackedFloat32Array staminas;
    for (const auto& p : sim_players) {
        staminas.append(p.stamina);
    }
    return staminas;
}

Vector2 MatchEngine::get_ball_position() const {
    return ball_position;
}

Dictionary MatchEngine::get_match_stats() const {
    Dictionary d;
    float total_poss = stats.home_possession_time + stats.away_possession_time;
    int home_poss_pct = (total_poss > 0.001f) ? static_cast<int>(std::round((stats.home_possession_time / total_poss) * 100.0f)) : 50;
    int away_poss_pct = 100 - home_poss_pct;

    d["home_possession"] = home_poss_pct;
    d["away_possession"] = away_poss_pct;
    d["home_shots"] = stats.home_shots;
    d["away_shots"] = stats.away_shots;
    d["home_shots_on_target"] = stats.home_shots_on_target;
    d["away_shots_on_target"] = stats.away_shots_on_target;
    d["home_xg"] = stats.home_xg;
    d["away_xg"] = stats.away_xg;
    d["home_passes"] = stats.home_passes;
    d["away_passes"] = stats.away_passes;
    d["home_passes_completed"] = stats.home_passes_completed;
    d["away_passes_completed"] = stats.away_passes_completed;
    d["home_tackles"] = stats.home_tackles;
    d["away_tackles"] = stats.away_tackles;
    d["home_interceptions"] = stats.home_interceptions;
    d["away_interceptions"] = stats.away_interceptions;
    d["home_saves"] = stats.home_saves;
    d["away_saves"] = stats.away_saves;
    d["home_corners"] = stats.home_corners;
    d["away_corners"] = stats.away_corners;
    return d;
}