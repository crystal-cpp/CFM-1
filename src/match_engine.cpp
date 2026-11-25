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
    ClassDB::bind_method(D_METHOD("reset_standalone_match_setup"), &MatchEngine::reset_standalone_match_setup);
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

    ClassDB::bind_method(D_METHOD("command_gk_rush"), &MatchEngine::command_gk_rush);
    ClassDB::bind_method(D_METHOD("cancel_gk_rush"), &MatchEngine::cancel_gk_rush);
    ClassDB::bind_method(D_METHOD("is_gk_rush_active"), &MatchEngine::is_gk_rush_active);

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
    ClassDB::bind_method(D_METHOD("set_home_bench", "players"), &MatchEngine::set_home_bench);
    ClassDB::bind_method(D_METHOD("set_away_bench", "players"), &MatchEngine::set_away_bench);
    ClassDB::bind_method(D_METHOD("get_home_squad_snapshot"), &MatchEngine::get_home_squad_snapshot);
    ClassDB::bind_method(D_METHOD("get_home_bench_snapshot"), &MatchEngine::get_home_bench_snapshot);
    ClassDB::bind_method(D_METHOD("apply_substitution", "is_home", "pitch_slot", "bench_slot"), &MatchEngine::apply_substitution);
    ClassDB::bind_method(D_METHOD("can_make_home_substitution"), &MatchEngine::can_make_home_substitution);
    ClassDB::bind_method(D_METHOD("get_home_subs_used"), &MatchEngine::get_home_subs_used);
    ClassDB::bind_method(D_METHOD("get_home_sub_windows_used"), &MatchEngine::get_home_sub_windows_used);
    ClassDB::bind_method(D_METHOD("get_max_subs"), &MatchEngine::get_max_subs);
    ClassDB::bind_method(D_METHOD("get_max_sub_windows"), &MatchEngine::get_max_sub_windows);
    ClassDB::bind_method(D_METHOD("is_halftime_break"), &MatchEngine::is_halftime_break);

    // Match Identity & User Side Methods
    ClassDB::bind_method(D_METHOD("set_user_is_home", "is_home"), &MatchEngine::set_user_is_home);
    ClassDB::bind_method(D_METHOD("is_user_home"), &MatchEngine::is_user_home);
    ClassDB::bind_method(D_METHOD("get_user_attack_chance_state"), &MatchEngine::get_user_attack_chance_state);
    ClassDB::bind_method(D_METHOD("get_user_defend_chance_state"), &MatchEngine::get_user_defend_chance_state);
    ClassDB::bind_method(D_METHOD("get_user_start_index"), &MatchEngine::get_user_start_index);
    ClassDB::bind_method(D_METHOD("get_user_end_index"), &MatchEngine::get_user_end_index);
    ClassDB::bind_method(D_METHOD("get_user_gk_index"), &MatchEngine::get_user_gk_index);
    ClassDB::bind_method(D_METHOD("is_user_player_index", "player_idx"), &MatchEngine::is_user_player_index);
    ClassDB::bind_method(D_METHOD("set_home_club_id", "club_id"), &MatchEngine::set_home_club_id);
    ClassDB::bind_method(D_METHOD("get_home_club_id"), &MatchEngine::get_home_club_id);
    ClassDB::bind_method(D_METHOD("set_away_club_id", "club_id"), &MatchEngine::set_away_club_id);
    ClassDB::bind_method(D_METHOD("get_away_club_id"), &MatchEngine::get_away_club_id);
    ClassDB::bind_method(D_METHOD("set_home_club_name", "name"), &MatchEngine::set_home_club_name);
    ClassDB::bind_method(D_METHOD("get_home_club_name"), &MatchEngine::get_home_club_name);
    ClassDB::bind_method(D_METHOD("set_away_club_name", "name"), &MatchEngine::set_away_club_name);
    ClassDB::bind_method(D_METHOD("get_away_club_name"), &MatchEngine::get_away_club_name);
    ClassDB::bind_method(D_METHOD("set_home_kit_color", "color"), &MatchEngine::set_home_kit_color);
    ClassDB::bind_method(D_METHOD("get_home_kit_color"), &MatchEngine::get_home_kit_color);
    ClassDB::bind_method(D_METHOD("set_away_kit_color", "color"), &MatchEngine::set_away_kit_color);
    ClassDB::bind_method(D_METHOD("get_away_kit_color"), &MatchEngine::get_away_kit_color);

    ClassDB::bind_method(D_METHOD("get_away_squad_snapshot"), &MatchEngine::get_away_squad_snapshot);
    ClassDB::bind_method(D_METHOD("get_away_bench_snapshot"), &MatchEngine::get_away_bench_snapshot);
    ClassDB::bind_method(D_METHOD("get_user_squad_snapshot"), &MatchEngine::get_user_squad_snapshot);
    ClassDB::bind_method(D_METHOD("get_user_bench_snapshot"), &MatchEngine::get_user_bench_snapshot);
    ClassDB::bind_method(D_METHOD("can_make_user_substitution"), &MatchEngine::can_make_user_substitution);
    ClassDB::bind_method(D_METHOD("get_user_subs_used"), &MatchEngine::get_user_subs_used);
    ClassDB::bind_method(D_METHOD("get_user_sub_windows_used"), &MatchEngine::get_user_sub_windows_used);

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
    ClassDB::bind_method(D_METHOD("get_restart_state"), &MatchEngine::get_restart_state);
    ClassDB::bind_method(D_METHOD("is_restart_active"), &MatchEngine::is_restart_active);
    ClassDB::bind_method(D_METHOD("get_sent_off_flags"), &MatchEngine::get_sent_off_flags);
    ClassDB::bind_method(D_METHOD("get_yellow_card_counts"), &MatchEngine::get_yellow_card_counts);
    ClassDB::bind_method(D_METHOD("is_pass_offside", "passer_idx", "receiver_idx"), &MatchEngine::is_pass_offside);

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
    ADD_SIGNAL(MethodInfo("gk_rush_changed",
        PropertyInfo(Variant::BOOL, "active"),
        PropertyInfo(Variant::INT, "gk_index")));
    ADD_SIGNAL(MethodInfo("restart_state_changed",
        PropertyInfo(Variant::INT, "restart_type"),
        PropertyInfo(Variant::BOOL, "to_home"),
        PropertyInfo(Variant::VECTOR2, "spot")));
    ADD_SIGNAL(MethodInfo("squad_updated"));
    ADD_SIGNAL(MethodInfo("substitution_applied",
        PropertyInfo(Variant::BOOL, "is_home"),
        PropertyInfo(Variant::INT, "pitch_slot"),
        PropertyInfo(Variant::INT, "bench_slot")));

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
    _initialize_default_home_bench();
    _initialize_default_away_bench();
}

MatchEngine::~MatchEngine() {}

void MatchEngine::_ready() {
    set_physics_process(false);
    load_default_formations();
    _reset_field_positions();
}

void MatchEngine::set_user_is_home(bool p_is_home) {
    const bool side_changed = (user_is_home != p_is_home);
    user_is_home = p_is_home;

    // Control indices are absolute simulation slots. Always seed control from
    // the managed side; never retain a HOME slot when the user is AWAY.
    if (side_changed || !_is_user_player_idx(user_controlled_index)) {
        user_controlled_index = user_is_home ? 9 : 20;
        manual_control_active = false;
        has_user_move_target = false;
        user_move_target = Vector2(-1.0f, -1.0f);
        if (match_active) {
            emit_signal("controlled_player_changed", user_controlled_index);
        }
    }
}

float MatchEngine::_get_fatigue_multiplier(const SimPlayer& p_player) const {
    float stamina_ratio = std::clamp(p_player.stamina / 100.0f, 0.0f, 1.0f);
    return 0.62f + (0.38f * stamina_ratio);
}

bool MatchEngine::_is_backline_role(const String& p_role) const {
    return (p_role == "CB" || p_role == "LB" || p_role == "RB" || p_role == "LWB" || p_role == "RWB");
}

float MatchEngine::_get_defender_awareness_factor(const SimPlayer& p_defender) const {
    float def_stat = static_cast<float>(p_defender.defending);
    float phy_stat = static_cast<float>(p_defender.physical);
    float role_bonus = (p_defender.role == "CB") ? 0.14f :
                       ((p_defender.role == "CDM") ? 0.10f :
                       (_is_backline_role(p_defender.role) ? 0.06f : -0.08f));
    float base = 0.70f + ((def_stat * 0.72f + phy_stat * 0.28f - 50.0f) / 100.0f) * 0.58f + role_bonus;
    return std::clamp(base * _get_fatigue_multiplier(p_defender), 0.52f, 1.35f);
}

float MatchEngine::_get_defensive_positioning_iq(const SimPlayer& p_defender) const {
    float def_stat = static_cast<float>(p_defender.defending);
    float vis_stat = static_cast<float>(p_defender.passing); // Tactical read / anticipation proxy
    float phy_stat = static_cast<float>(p_defender.physical);
    float role_iq = (p_defender.role == "CB" || p_defender.role == "CDM") ? 0.12f :
                    (_is_backline_role(p_defender.role) ? 0.06f :
                    ((p_defender.role == "CM") ? 0.02f : -0.10f));
    float raw = 0.68f + (((def_stat * 0.74f + vis_stat * 0.16f + phy_stat * 0.10f) - 50.0f) / 100.0f) * 0.62f + role_iq;
    return std::clamp(raw * _get_fatigue_multiplier(p_defender), 0.48f, 1.36f);
}

float MatchEngine::_get_role_defensive_bonus(const SimPlayer& p_defender, bool p_in_box, bool p_is_aerial_or_block) const {
    const String& r = p_defender.role;
    if (r == "CB") {
        return p_in_box ? (p_is_aerial_or_block ? 10.5f : 7.5f) : 4.0f;
    }
    if (r == "CDM") {
        return p_in_box ? 5.0f : 6.5f;
    }
    if (r == "LB" || r == "RB" || r == "LWB" || r == "RWB") {
        return p_in_box ? 3.5f : 4.5f;
    }
    if (r == "CM") {
        return 1.5f;
    }
    if (r == "GK") {
        return p_in_box ? 8.0f : -12.0f;
    }
    return -5.5f; // Attackers/wingers are less disciplined in defensive challenges
}

bool MatchEngine::_is_player_recovering_position(const SimPlayer& p_player, float p_team_backline_x) const {
    if (p_player.role == "GK" || possession_team == (p_player.is_home ? 0 : 1)) {
        return false;
    }
    bool is_home = p_player.is_home;
    // Recovering if ball is behind the player toward their own goal, or player is stranded far ahead of their defensive line
    bool ball_behind = is_home ? (ball_position.x < p_player.position.x - 24.0f)
                               : (ball_position.x > p_player.position.x + 24.0f);
    bool stranded_high = is_home ? (p_player.position.x > p_team_backline_x + 78.0f && (_is_backline_role(p_player.role) || p_player.role == "CDM"))
                                 : (p_player.position.x < p_team_backline_x - 78.0f && (_is_backline_role(p_player.role) || p_player.role == "CDM"));
    return ball_behind || stranded_high;
}


void MatchEngine::_execute_clearance(int p_defender_idx) {
    SimPlayer& defender = sim_players[p_defender_idx];
    bool is_home = defender.is_home;
    float clear_dir = is_home ? 1.0f : -1.0f;
    float fatigue = _get_fatigue_multiplier(defender);
    float def_norm = std::clamp(defender.defending / 100.0f, 0.25f, 1.0f);
    float phy_norm = std::clamp(defender.physical / 100.0f, 0.25f, 1.0f);

    defender.stamina = std::max(10.0f, defender.stamina - 0.55f);
    if (is_home) {
        stats.home_clearances++;
    } else {
        stats.away_clearances++;
    }

    // High-Defending defenders aim cleanly toward wide touchline channels; low-Defending defenders have wider/riskier scatter
    float center_y = field_size.y * 0.5f;
    float flank_sign = (defender.position.y >= center_y) ? 1.0f : -1.0f;
    float clear_dist = std::clamp((225.0f + phy_norm * 155.0f + def_norm * 65.0f) * fatigue, 200.0f, 445.0f);
    float ideal_flank = flank_sign * static_cast<float>(UtilityFunctions::randf_range(95.0, 185.0));
    float miscue_spread = (1.0f - def_norm) * static_cast<float>(UtilityFunctions::randf_range(-95.0, 95.0));

    Vector2 target(
        std::clamp(defender.position.x + clear_dir * clear_dist, 36.0f, field_size.x - 36.0f),
        std::clamp(defender.position.y + ideal_flank + miscue_spread, 24.0f, field_size.y - 24.0f)
    );

    Vector2 delta = target - ball_position;
    float dist = std::max(1.0f, delta.length());
    float clear_speed = std::clamp((310.0f + phy_norm * 145.0f) * fatigue, 285.0f, 475.0f);

    ball_position = defender.position + Vector2(clear_dir * 9.0f, 0.0f);
    ball_position.x = std::clamp(ball_position.x, 12.0f, field_size.x - 12.0f);
    ball_position.y = std::clamp(ball_position.y, 12.0f, field_size.y - 12.0f);

    ball_target = target;
    ball_velocity = (delta / dist) * clear_speed;
    ball_state = BALL_LOOSE;
    possession_team = -1;
    ball_carrier_index = -1;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    last_touch_index = p_defender_idx;

    _log_event("clearance", is_home, defender.name,
        defender.name + String(" hooks the danger clear upfield!"));

    if (((is_home != user_is_home) && chance_state == _user_attack_chance_state()) ||
        ((is_home == user_is_home) && chance_state == _user_defend_chance_state())) {
        _end_active_chance(false);
    }
}


bool MatchEngine::_try_defensive_clearance(int p_defender_idx, float p_danger_score) {
    if (p_defender_idx < 0 || p_defender_idx >= 22) return false;
    if (manual_control_active && p_defender_idx == user_controlled_index) return false;

    const SimPlayer& defender = sim_players[p_defender_idx];
    if (defender.role == "GK") return false;

    bool is_home = defender.is_home;
    float own_goal_x = is_home ? 0.0f : field_size.x;
    float dist_to_own_goal = std::abs(defender.position.x - own_goal_x);

    if (dist_to_own_goal > 205.0f) {
        return false;
    }

    int opp_start = is_home ? 11 : 0;
    float nearest_opp = 1e9f;
    for (int j = opp_start; j < opp_start + 11; j++) {
        nearest_opp = std::min(nearest_opp, defender.position.distance_to(sim_players[j].position));
    }

    bool inside_box = dist_to_own_goal < 138.0f && std::abs(defender.position.y - field_size.y * 0.5f) < 168.0f;
    if (!inside_box && nearest_opp > 50.0f && p_danger_score < 0.52f) {
        return false;
    }

    float awareness = _get_defender_awareness_factor(defender);
    float role_clear_bias = (defender.role == "CB") ? 0.14f : (_is_backline_role(defender.role) ? 0.08f : 0.04f);
    float clear_prob = std::clamp((0.44f + p_danger_score * 0.36f + role_clear_bias) * awareness, 0.28f, 0.94f);
    if (inside_box && nearest_opp < 46.0f) {
        clear_prob = std::max(clear_prob, 0.80f);
    }

    if (UtilityFunctions::randf() < clear_prob) {
        _execute_clearance(p_defender_idx);
        return true;
    }
    return false;
}

void MatchEngine::_initialize_default_squads() {
    sim_players.resize(22);
    for (int i = 0; i < 22; i++) {
        bool is_home = (i < 11);
        int local_idx = is_home ? i : (i - 11);
        SimPlayer& p = sim_players[i];
        p.is_home = is_home;
        p.slot_index = i;
        p.player_id = "";
        p.name = (is_home ? String("Home #") : String("Away #")) + String::num_int64(local_idx + 1);
        p.role = (local_idx == 0) ? String("GK") : String("CM");
        p.ovr = is_home ? home_team_ovr : away_team_ovr;
        p.stamina = 100.0f;
        p.recovery_timer = 0.0f;
        p.tackle_cooldown = 0.0f;
        p.sprint_boost_timer = 0.0f;
        p.dribble_boost_timer = 0.0f;
        p.velocity = Vector2(0.0f, 0.0f);
        _apply_role_profile(p, is_home ? home_team_ovr : away_team_ovr);
    }
}

void MatchEngine::_initialize_default_home_bench() {
    static const char* BENCH_ROLES[7] = { "GK", "CB", "LB", "CDM", "CM", "LW", "ST" };
    static const int BENCH_OFFSETS[7] = { -2, -1, -2, 0, 1, 0, 1 };
    home_bench.clear();
    for (int b = 0; b < 7; b++) {
        BenchPlayer hb;
        hb.name = String("Home #") + String::num_int64(12 + b);
        hb.player_id = "";
        hb.role = String(BENCH_ROLES[b]);
        hb.ovr = std::clamp(home_team_ovr + BENCH_OFFSETS[b], 50, 99);
        hb.pace = hb.ovr;
        hb.shooting = hb.ovr;
        hb.passing = hb.ovr;
        hb.dribbling = hb.ovr;
        hb.defending = hb.ovr;
        hb.physical = hb.ovr;
        hb.stamina = 100.0f;
        hb.used = false;
        home_bench.push_back(hb);
    }
}

void MatchEngine::_initialize_default_away_bench() {
    static const char* BENCH_ROLES[7] = { "GK", "CB", "LB", "CDM", "CM", "LW", "ST" };
    static const int BENCH_OFFSETS[7] = { -2, -1, -2, 0, 1, 0, 1 };
    away_bench.clear();
    for (int b = 0; b < 7; b++) {
        BenchPlayer ab;
        ab.name = String("Away #") + String::num_int64(12 + b);
        ab.player_id = "";
        ab.role = String(BENCH_ROLES[b]);
        ab.ovr = std::clamp(away_team_ovr + BENCH_OFFSETS[b], 50, 99);
        ab.pace = ab.ovr;
        ab.shooting = ab.ovr;
        ab.passing = ab.ovr;
        ab.dribbling = ab.ovr;
        ab.defending = ab.ovr;
        ab.physical = ab.ovr;
        ab.stamina = 100.0f;
        ab.used = false;
        away_bench.push_back(ab);
    }
}

void MatchEngine::_apply_role_profile(SimPlayer& p_player, int p_base_ovr) const {
    int base = std::clamp(p_base_ovr, 1, 99);
    p_player.ovr = base;
    String r = p_player.role.to_upper();

    p_player.starting_stamina = static_cast<float>(std::clamp(base, 40, 95));
    p_player.stamina = p_player.starting_stamina;

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
    defensive_chance_time_scale = std::clamp(p_scale, 0.55f, 1.0f);
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

void MatchEngine::_populate_sim_player_from_dict(SimPlayer& p_player, const Dictionary& p_dict, int p_fallback_ovr) const {
    int base_ovr = p_dict.has("ovr")
        ? std::clamp(static_cast<int>(p_dict["ovr"]), 1, 99)
        : std::clamp(p_fallback_ovr, 1, 99);

    if (p_dict.has("position")) {
        String pos_str = String(p_dict["position"]).strip_edges().to_upper();
        if (!pos_str.is_empty()) {
            p_player.role = pos_str;
        }
    }
    _apply_role_profile(p_player, base_ovr);

    if (p_dict.has("id") || p_dict.has("player_id")) {
        Variant id_var = p_dict.has("id") ? p_dict["id"] : p_dict["player_id"];
        String id_str = String(id_var).strip_edges();
        if (!id_str.is_empty() && id_str != "-1" && id_str != "0") {
            p_player.player_id = id_str;
        }
    }
    if (p_dict.has("name")) {
        String player_name = String(p_dict["name"]).strip_edges();
        if (!player_name.is_empty()) {
            p_player.name = player_name;
        }
    }
    if (p_dict.has("pace")) p_player.pace = std::clamp(static_cast<int>(p_dict["pace"]), 1, 99);
    if (p_dict.has("shooting")) p_player.shooting = std::clamp(static_cast<int>(p_dict["shooting"]), 1, 99);
    if (p_dict.has("passing")) p_player.passing = std::clamp(static_cast<int>(p_dict["passing"]), 1, 99);
    if (p_dict.has("dribbling")) p_player.dribbling = std::clamp(static_cast<int>(p_dict["dribbling"]), 1, 99);
    if (p_dict.has("defending")) p_player.defending = std::clamp(static_cast<int>(p_dict["defending"]), 1, 99);
    if (p_dict.has("physical")) p_player.physical = std::clamp(static_cast<int>(p_dict["physical"]), 1, 99);

    if (p_dict.has("condition")) {
        float cond = std::clamp(static_cast<float>(p_dict["condition"]), 10.0f, 100.0f);
        p_player.starting_stamina = cond;
        p_player.stamina = cond;
    } else if (p_dict.has("stamina")) {
        float stam = std::clamp(static_cast<float>(p_dict["stamina"]), 10.0f, 100.0f);
        p_player.starting_stamina = stam;
        p_player.stamina = stam;
    }
}

BenchPlayer MatchEngine::_parse_bench_player_from_dict(const Dictionary& p_dict, const String& p_fallback_name, const String& p_fallback_role, int p_fallback_ovr) const {
    BenchPlayer bp;
    bp.name = p_fallback_name;
    bp.role = p_fallback_role;
    bp.ovr = std::clamp(p_fallback_ovr, 40, 99);

    if (p_dict.has("id") || p_dict.has("player_id")) {
        Variant id_var = p_dict.has("id") ? p_dict["id"] : p_dict["player_id"];
        String id_str = String(id_var).strip_edges();
        if (!id_str.is_empty() && id_str != "-1" && id_str != "0") {
            bp.player_id = id_str;
        }
    }
    if (p_dict.has("name")) {
        String n = String(p_dict["name"]).strip_edges();
        if (!n.is_empty()) bp.name = n;
    }
    if (p_dict.has("position")) {
        String r = String(p_dict["position"]).strip_edges().to_upper();
        if (!r.is_empty()) bp.role = r;
    }
    if (p_dict.has("ovr")) {
        bp.ovr = std::clamp(static_cast<int>(p_dict["ovr"]), 40, 99);
    }

    bp.pace = p_dict.has("pace") ? std::clamp(static_cast<int>(p_dict["pace"]), 1, 99) : bp.ovr;
    bp.shooting = p_dict.has("shooting") ? std::clamp(static_cast<int>(p_dict["shooting"]), 1, 99) : bp.ovr;
    bp.passing = p_dict.has("passing") ? std::clamp(static_cast<int>(p_dict["passing"]), 1, 99) : bp.ovr;
    bp.dribbling = p_dict.has("dribbling") ? std::clamp(static_cast<int>(p_dict["dribbling"]), 1, 99) : bp.ovr;
    bp.defending = p_dict.has("defending") ? std::clamp(static_cast<int>(p_dict["defending"]), 1, 99) : bp.ovr;
    bp.physical = p_dict.has("physical") ? std::clamp(static_cast<int>(p_dict["physical"]), 1, 99) : bp.ovr;

    if (p_dict.has("condition")) {
        bp.stamina = std::clamp(static_cast<float>(p_dict["condition"]), 10.0f, 100.0f);
    } else if (p_dict.has("stamina")) {
        bp.stamina = std::clamp(static_cast<float>(p_dict["stamina"]), 10.0f, 100.0f);
    } else {
        bp.stamina = 100.0f;
    }
    bp.used = bool(p_dict.get("used", false));
    return bp;
}

void MatchEngine::_recompute_team_ovrs_from_lineups() {
    if (sim_players.size() < 22) return;
    int home_sum = 0;
    int away_sum = 0;
    for (int i = 0; i < 11; i++) {
        home_sum += sim_players[i].ovr;
        away_sum += sim_players[11 + i].ovr;
    }
    home_team_ovr = std::clamp(static_cast<int>(std::round(home_sum / 11.0f)), 40, 99);
    away_team_ovr = std::clamp(static_cast<int>(std::round(away_sum / 11.0f)), 40, 99);
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
        sim_players[i].is_home = true;
        sim_players[i].slot_index = i;
        _populate_sim_player_from_dict(sim_players[i], p_players[i], home_team_ovr);
    }
    _recompute_team_ovrs_from_lineups();
    emit_signal("squad_updated");
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
        sim_players[11 + i].is_home = false;
        sim_players[11 + i].slot_index = 11 + i;
        _populate_sim_player_from_dict(sim_players[11 + i], p_players[i], away_team_ovr);
    }
    _recompute_team_ovrs_from_lineups();
    emit_signal("squad_updated");
}

void MatchEngine::set_home_bench(const Array& p_players) {
    home_bench.clear();
    if (p_players.is_empty()) {
        _initialize_default_home_bench();
        emit_signal("squad_updated");
        return;
    }
    int limit = std::min(12, static_cast<int>(p_players.size()));
    for (int i = 0; i < limit; i++) {
        if (p_players[i].get_type() != Variant::DICTIONARY) continue;
        String fb_name = String("Home #") + String::num_int64(12 + i);
        home_bench.push_back(_parse_bench_player_from_dict(p_players[i], fb_name, "CM", home_team_ovr));
    }
    emit_signal("squad_updated");
}

void MatchEngine::set_away_bench(const Array& p_players) {
    away_bench.clear();
    if (p_players.is_empty()) {
        _initialize_default_away_bench();
        emit_signal("squad_updated");
        return;
    }
    int limit = std::min(12, static_cast<int>(p_players.size()));
    for (int i = 0; i < limit; i++) {
        if (p_players[i].get_type() != Variant::DICTIONARY) continue;
        String fb_name = String("Away #") + String::num_int64(12 + i);
        away_bench.push_back(_parse_bench_player_from_dict(p_players[i], fb_name, "CM", away_team_ovr));
    }
    emit_signal("squad_updated");
}

Array MatchEngine::get_home_squad_snapshot() const {
    Array arr;
    for (int i = 0; i < 11 && i < static_cast<int>(sim_players.size()); i++) {
        const SimPlayer& p = sim_players[i];
        Dictionary d;
        d["slot"] = i;
        d["id"] = p.player_id;
        d["name"] = p.name;
        d["position"] = p.role;
        d["ovr"] = p.ovr;
        d["stamina"] = p.stamina;
        d["pace"] = p.pace;
        d["shooting"] = p.shooting;
        d["passing"] = p.passing;
        d["dribbling"] = p.dribbling;
        d["defending"] = p.defending;
        d["physical"] = p.physical;
        arr.append(d);
    }
    return arr;
}

Array MatchEngine::get_home_bench_snapshot() const {
    Array arr;
    for (size_t i = 0; i < home_bench.size(); i++) {
        const BenchPlayer& b = home_bench[i];
        Dictionary d;
        d["bench_slot"] = static_cast<int>(i);
        d["id"] = b.player_id;
        d["name"] = b.name;
        d["position"] = b.role;
        d["ovr"] = b.ovr;
        d["stamina"] = b.stamina;
        d["used"] = b.used;
        d["pace"] = b.pace;
        d["shooting"] = b.shooting;
        d["passing"] = b.passing;
        d["dribbling"] = b.dribbling;
        d["defending"] = b.defending;
        d["physical"] = b.physical;
        arr.append(d);
    }
    return arr;
}

bool MatchEngine::can_make_home_substitution() const {
    if (home_subs_used >= MAX_MATCH_SUBS) {
        return false;
    }
    // Half-time break (45' kickoff pause) does not consume one of the 3 stoppage windows
    if (is_halftime_break()) {
        return true;
    }
    // Multiple subs in the exact same match stoppage minute share 1 stoppage window
    if (home_subs_used > 0 && !last_home_sub_was_halftime && current_minute == last_home_sub_minute) {
        return true;
    }
    return home_sub_windows_used < MAX_SUB_WINDOWS;
}

bool MatchEngine::apply_substitution(bool p_is_home, int p_pitch_slot, int p_bench_slot) {
    if (p_pitch_slot < 0 || p_pitch_slot >= 11) {
        return false;
    }
    std::vector<BenchPlayer>& bench_vec = p_is_home ? home_bench : away_bench;
    int& subs_used = p_is_home ? home_subs_used : away_subs_used;
    int& windows_used = p_is_home ? home_sub_windows_used : away_sub_windows_used;
    int& last_sub_min = p_is_home ? last_home_sub_minute : last_away_sub_minute;
    bool& last_was_ht = p_is_home ? last_home_sub_was_halftime : last_away_sub_was_halftime;

    if (p_bench_slot < 0 || p_bench_slot >= static_cast<int>(bench_vec.size())) {
        return false;
    }
    if (subs_used >= MAX_MATCH_SUBS || bench_vec[p_bench_slot].used) {
        return false;
    }

    bool at_ht = is_halftime_break();
    bool same_stoppage = (subs_used > 0 && !last_was_ht && current_minute == last_sub_min);
    if (!at_ht && !same_stoppage && windows_used >= MAX_SUB_WINDOWS) {
        UtilityFunctions::push_warning("MatchEngine: Substitution rejected — all 3 stoppage windows used.");
        return false;
    }

    int sim_idx = p_is_home ? p_pitch_slot : (11 + p_pitch_slot);
    SimPlayer& outgoing = sim_players[sim_idx];
    BenchPlayer& incoming = bench_vec[p_bench_slot];

    // Record subbed-off player's exact fatigue & ID so CareerState::commit_match_result() syncs them too
    Dictionary off_rec;
    off_rec["id"] = outgoing.player_id;
    off_rec["name"] = outgoing.name;
    off_rec["stamina"] = static_cast<int>(std::round(std::clamp(outgoing.stamina, 1.0f, 99.0f)));
    off_rec["minute_off"] = current_minute;

    if (p_is_home) {
        home_subbed_off_records.append(off_rec);
        if (!incoming.player_id.is_empty()) {
            home_subbed_on_ids.append(incoming.player_id);
        }
    } else {
        away_subbed_off_records.append(off_rec);
        if (!incoming.player_id.is_empty()) {
            away_subbed_on_ids.append(incoming.player_id);
        }
    }

    String out_name = outgoing.name;
    String in_name = incoming.name;
    String slot_role = outgoing.role; // Preserve tactical formation slot role

    outgoing.name = incoming.name;
    outgoing.player_id = incoming.player_id;
    outgoing.role = slot_role;
    outgoing.ovr = incoming.ovr;
    outgoing.pace = incoming.pace;
    outgoing.shooting = incoming.shooting;
    outgoing.passing = incoming.passing;
    outgoing.dribbling = incoming.dribbling;
    outgoing.defending = incoming.defending;
    outgoing.physical = incoming.physical;
    outgoing.starting_stamina = std::clamp(incoming.stamina, 15.0f, 100.0f);
    outgoing.stamina = outgoing.starting_stamina;
    outgoing.recovery_timer = 0.0f;
    outgoing.sprint_boost_timer = 0.0f;
    outgoing.dribble_boost_timer = 0.0f;

    incoming.used = true;
    subs_used++;
    if (!at_ht && !same_stoppage) {
        windows_used++;
    }
    last_sub_min = current_minute;
    last_was_ht = at_ht;

    _recompute_team_ovrs_from_lineups();

    String win_note = at_ht
        ? String(" [HT]")
        : (String(" [Window ") + String::num_int64(windows_used) + String("/") + String::num_int64(MAX_SUB_WINDOWS) + String("]"));
    _log_event("substitution", p_is_home, in_name,
        String("SUB (") + String::num_int64(subs_used) + String("/") + String::num_int64(MAX_MATCH_SUBS) + win_note + String("): ")
        + in_name + String(" ON for ") + out_name);

    emit_signal("substitution_applied", p_is_home, p_pitch_slot, p_bench_slot);
    emit_signal("squad_updated");
    return true;
}

Array MatchEngine::get_away_squad_snapshot() const {
    Array arr;
    for (int i = 11; i < 22 && i < static_cast<int>(sim_players.size()); i++) {
        const SimPlayer& p = sim_players[i];
        Dictionary d;
        d["slot"] = i - 11;
        d["sim_index"] = i;
        d["id"] = p.player_id;
        d["name"] = p.name;
        d["position"] = p.role;
        d["ovr"] = p.ovr;
        d["stamina"] = p.stamina;
        d["pace"] = p.pace;
        d["shooting"] = p.shooting;
        d["passing"] = p.passing;
        d["dribbling"] = p.dribbling;
        d["defending"] = p.defending;
        d["physical"] = p.physical;
        arr.append(d);
    }
    return arr;
}

Array MatchEngine::get_away_bench_snapshot() const {
    Array arr;
    for (size_t i = 0; i < away_bench.size(); i++) {
        const BenchPlayer& b = away_bench[i];
        Dictionary d;
        d["bench_slot"] = static_cast<int>(i);
        d["id"] = b.player_id;
        d["name"] = b.name;
        d["position"] = b.role;
        d["ovr"] = b.ovr;
        d["stamina"] = b.stamina;
        d["used"] = b.used;
        d["pace"] = b.pace;
        d["shooting"] = b.shooting;
        d["passing"] = b.passing;
        d["dribbling"] = b.dribbling;
        d["defending"] = b.defending;
        d["physical"] = b.physical;
        arr.append(d);
    }
    return arr;
}

Array MatchEngine::get_user_squad_snapshot() const {
    return user_is_home ? get_home_squad_snapshot() : get_away_squad_snapshot();
}

Array MatchEngine::get_user_bench_snapshot() const {
    return user_is_home ? get_home_bench_snapshot() : get_away_bench_snapshot();
}

bool MatchEngine::can_make_user_substitution() const {
    int subs_used = user_is_home ? home_subs_used : away_subs_used;
    int windows_used = user_is_home ? home_sub_windows_used : away_sub_windows_used;
    int last_sub_min = user_is_home ? last_home_sub_minute : last_away_sub_minute;
    bool last_was_ht = user_is_home ? last_home_sub_was_halftime : last_away_sub_was_halftime;

    if (subs_used >= MAX_MATCH_SUBS) {
        return false;
    }
    if (is_halftime_break()) {
        return true;
    }
    if (subs_used > 0 && !last_was_ht && current_minute == last_sub_min) {
        return true;
    }
    return windows_used < MAX_SUB_WINDOWS;
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

    float home_luck = std::clamp(static_cast<float>(UtilityFunctions::randfn(0.0, 0.70)), -1.75f, 1.75f);
    float away_luck = std::clamp(static_cast<float>(UtilityFunctions::randfn(0.0, 0.70)), -1.75f, 1.75f);

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
    pending_chance_signal = false;
    pending_chance_end_signal = false;
    pending_interactive_signal = false;
    pending_match_ended_signal = false;
    pending_chance_state_val = CHANCE_NONE;
    pending_chance_ctrl_val = user_controlled_index;
    pending_interactive_type = 0;
    pending_interactive_home_atk = true;
    pending_interactive_ball_3d = Vector3();
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

bool MatchEngine::_has_valid_simulation_state() const {
    if (sim_players.size() != 22) {
        return false;
    }
    if (ball_state == BALL_CARRIED) {
        return ball_carrier_index >= 0 && ball_carrier_index < 22;
    }
    if (ball_state == BALL_IN_PASS) {
        return pass_target_index >= 0 && pass_target_index < 22;
    }
    return true;
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
    gk_rush_active = false;
    gk_rush_timer = 0.0f;
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
    gk_rush_active = false;
    gk_rush_timer = 0.0f;
    Vector2 center = field_size * 0.5f;
    float half_width = center.x;
    const float margin = 14.0f;
    const float center_circle_radius = 92.0f;

    for (int i = 0; i < 22; i++) {
        SimPlayer& p = sim_players[i];
        p.velocity = Vector2(0.0f, 0.0f);
        p.recovery_timer = 0.0f;
        p.tackle_cooldown = 0.0f;
        p.sprint_boost_timer = 0.0f;
        p.dribble_boost_timer = 0.0f;
        p.dribble_carry_distance = 0.0f;
        p.is_sliding = false;
        p.slide_timer = 0.0f;
        p.slide_dir = Vector2(0.0f, 0.0f);
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
    if (!match_active || !manual_control_active || chance_state != _user_attack_chance_state()) {
        return false;
    }
    if (!_is_user_outfield_idx(user_controlled_index)) {
        return false;
    }
    return has_user_move_target;
}


bool MatchEngine::set_controlled_player(int p_player_idx) {
    if (!match_active || sim_players.size() != 22 || !_is_user_player_idx(p_player_idx)) {
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
    if (sim_players.size() != 22) {
        return -1;
    }
    int start_idx = _user_gk_idx() + 1;
    int end_idx = _user_end_idx();
    int best_idx = start_idx;
    float best_score = 1e9f;
    Vector2 focus_point = (ball_state == BALL_IN_PASS)
        ? ball_position.lerp(ball_target, 0.45f)
        : ball_position;

    for (int i = start_idx; i < end_idx; i++) {
        if (sim_players[i].is_sent_off) continue;
        float d = sim_players[i].position.distance_to(focus_point);
        bool on_goal_side = user_is_home
            ? (sim_players[i].position.x <= focus_point.x + 12.0f)
            : (sim_players[i].position.x >= focus_point.x - 12.0f);
        float goal_side_bonus = on_goal_side ? -14.0f : 12.0f;
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

    float desired_zoom = (chance_state == _user_defend_chance_state()) ? 1.20f : 1.12f;

    Rect2 framing = get_chance_framing_rect();
    float fit_zoom_x = field_size.x / std::max(340.0f, framing.size.x);
    float fit_zoom_y = field_size.y / std::max(240.0f, framing.size.y);
    float safe_max_zoom = std::min(fit_zoom_x, fit_zoom_y);

    return std::clamp(std::min(desired_zoom, safe_max_zoom), 1.0f, 1.24f);
}

int MatchEngine::_resolve_user_action_player() const {
    if (!match_active || sim_players.size() != 22) {
        return -1;
    }
    int start_idx = _user_start_idx();
    int outfield_start = _user_gk_idx() + 1;
    int end_idx = _user_end_idx();

    if (ball_state == BALL_CARRIED && ball_carrier_index >= start_idx && ball_carrier_index < end_idx) {
        return ball_carrier_index;
    }
    if (_is_user_outfield_idx(user_controlled_index) &&
        ball_position.distance_to(sim_players[user_controlled_index].position) <= 48.0f) {
        return user_controlled_index;
    }
    if (ball_state == BALL_IN_PASS && pass_target_index >= outfield_start && pass_target_index < end_idx &&
        ball_position.distance_to(sim_players[pass_target_index].position) <= 48.0f) {
        return pass_target_index;
    }
    int best_idx = -1;
    float best_dist = 48.0f;
    for (int i = outfield_start; i < end_idx; i++) {
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
    int passer_idx = _resolve_user_action_player();
    if (!_is_user_player_idx(passer_idx)) {
        passer_idx = _is_user_player_idx(user_controlled_index) ? user_controlled_index : ball_carrier_index;
    }
    if (!_is_user_player_idx(passer_idx)) {
        return targets;
    }

    const SimPlayer& passer = sim_players[passer_idx];
    float fatigue = _get_fatigue_multiplier(passer);
    float max_range = (240.0f + (passer.passing * 2.6f)) * fatigue;

    for (int t = _user_gk_idx() + 1; t < _user_end_idx(); t++) {
        if (t == passer_idx) continue;
        Vector2 pass_vec = sim_players[t].position - passer.position;
        float dist = pass_vec.length();
        if (dist < 16.0f || dist > max_range) continue;
        targets.append(t);
    }
    return targets;
}

bool MatchEngine::command_move_player(Vector2 p_target_pos) {
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume) {
        return false;
    }
    if (!manual_control_active && chance_state != CHANCE_NONE) {
        manual_control_active = true;
    }
    if (!manual_control_active) {
        return false;
    }

    int user_atk_state = _user_attack_chance_state();
    if (chance_state == user_atk_state && ball_state == BALL_CARRIED &&
        _is_user_player_idx(ball_carrier_index) &&
        user_controlled_index != ball_carrier_index) {
        user_controlled_index = ball_carrier_index;
        emit_signal("controlled_player_changed", user_controlled_index);
    }

    if (!_is_user_player_idx(user_controlled_index)) {
        return false;
    }

    SimPlayer& player = sim_players[user_controlled_index];
    const float margin = 14.0f;
    float atk_dir = _user_attack_dir();

    Vector2 resolved_target = p_target_pos;
    Vector2 raw_delta = resolved_target - player.position;

    if (chance_state == user_atk_state && raw_delta.length() < 16.0f) {
        Vector2 forward_dir(atk_dir, 0.0f);
        if (player.velocity.length() > 10.0f) {
            Vector2 vel_dir = player.velocity.normalized();
            float clamped_x = user_is_home ? std::max(0.55f, vel_dir.x) : std::min(-0.55f, vel_dir.x);
            forward_dir = Vector2(clamped_x, vel_dir.y * 0.65f).normalized();
        }
        resolved_target = player.position + forward_dir * 76.0f;
    }

    user_move_target.x = std::clamp(resolved_target.x, margin, field_size.x - margin);
    user_move_target.y = std::clamp(resolved_target.y, margin, field_size.y - margin);
    has_user_move_target = true;
    is_paused_for_manual = false;

    if (chance_state == user_atk_state && ball_state == BALL_CARRIED && ball_carrier_index == user_controlled_index) {
        if (player.dribble_carry_distance < 65.0f) {
            player.dribble_boost_timer = 0.24f;
        }
    } else if (chance_state == _user_defend_chance_state()) {
        player.sprint_boost_timer = std::max(player.sprint_boost_timer, 0.35f);
    }

    return true;
}

bool MatchEngine::command_pass(int p_target_player_idx) {
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume || chance_state != _user_attack_chance_state()) {
        return false;
    }
    const int resolved_target = p_target_player_idx;
    if (!_is_user_player_idx(resolved_target) || sim_players[resolved_target].is_sent_off) {
        return false;
    }

    int passer_idx = _resolve_user_action_player();
    if (!_is_user_player_idx(passer_idx) || passer_idx == resolved_target) {
        return false;
    }

    Vector2 to_rec = sim_players[resolved_target].position - sim_players[passer_idx].position;
    float dist = to_rec.length();
    float fatigue = _get_fatigue_multiplier(sim_players[passer_idx]);
    float max_range = (260.0f + sim_players[passer_idx].passing * 2.8f) * fatigue;
    if (dist < 12.0f || dist > max_range) {
        return false;
    }

    Vector2 pass_dir = (dist > 1.0f) ? (to_rec / dist) : Vector2(_user_attack_dir(), 0.0f);

    sim_players[passer_idx].dribble_boost_timer = 0.0f;
    ball_position = sim_players[passer_idx].position + pass_dir * 10.0f;
    ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
    ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);

    has_user_move_target = false;
    is_paused_for_manual = false;
    _execute_pass(passer_idx, resolved_target, false);
    _log_event("pass", user_is_home, sim_players[passer_idx].name,
        sim_players[passer_idx].name + String(" passes to ") + sim_players[resolved_target].name);

    if (user_controlled_index != resolved_target) {
        user_controlled_index = resolved_target;
        emit_signal("controlled_player_changed", user_controlled_index);
    }
    return true;
}

bool MatchEngine::command_through_ball(Vector2 p_direction_or_target) {
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume || chance_state != _user_attack_chance_state()) {
        return false;
    }

    int passer_idx = _resolve_user_action_player();
    if (!_is_user_player_idx(passer_idx)) {
        return false;
    }

    float atk_dir = _user_attack_dir();
    SimPlayer& passer = sim_players[passer_idx];
    bool is_unit_dir = (p_direction_or_target.length() > 0.01f && p_direction_or_target.length() <= 1.5f);
    Vector2 raw_vec = is_unit_dir
        ? (p_direction_or_target.normalized() * 130.0f)
        : (p_direction_or_target - passer.position);

    bool is_explicit_target_point = (!is_unit_dir &&
                                     p_direction_or_target.x >= 0.0f && p_direction_or_target.x <= field_size.x &&
                                     p_direction_or_target.y >= 0.0f && p_direction_or_target.y <= field_size.y &&
                                     raw_vec.length() >= 12.0f);

    Vector2 aim_vec = (raw_vec.length() >= 8.0f) ? raw_vec : Vector2(120.0f * atk_dir, 0.0f);
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

    int outfield_start = _user_gk_idx() + 1;
    int end_idx = _user_end_idx();
    int best_runner = -1;
    float best_runner_score = -1e9f;

    for (int t = outfield_start; t < end_idx; t++) {
        if (t == passer_idx || sim_players[t].is_sent_off) continue;
        Vector2 to_tm = sim_players[t].position - passer.position;
        float dist_from_passer = to_tm.length();
        if (dist_from_passer < 18.0f || dist_from_passer > 460.0f) continue;

        float dist_to_space = sim_players[t].position.distance_to(desired_space);
        float alignment = (to_tm / dist_from_passer).dot(aim_dir);
        float forward_bonus = (sim_players[t].position.x - passer.position.x) * atk_dir * 0.35f;

        float score = (alignment * 120.0f) + forward_bonus - (dist_to_space * 0.85f);
        if (score > best_runner_score) {
            best_runner_score = score;
            best_runner = t;
        }
    }

    if (best_runner == -1) {
        float best_prog_x = user_is_home ? -1.0f : (field_size.x + 1.0f);
        for (int t = outfield_start; t < end_idx; t++) {
            if (t == passer_idx || sim_players[t].is_sent_off) continue;
            bool further = user_is_home ? (sim_players[t].position.x > best_prog_x) : (sim_players[t].position.x < best_prog_x);
            if (further) {
                best_prog_x = sim_players[t].position.x;
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
        float lead_x = user_is_home ? std::max(0.35f, aim_dir.x) : std::min(-0.35f, aim_dir.x);
        space_target = runner.position + Vector2(lead_x, aim_dir.y * 0.75f).normalized() * lead_dist;
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
    _snapshot_pass_offside_flags(passer_idx);
    pending_pass_offside = _evaluate_pass_offside(passer_idx, best_runner);
    offside_spot = runner.position;
    last_touch_index = passer_idx;
    if (user_is_home) stats.home_passes++;
    else stats.away_passes++;

    passer.recovery_timer = 0.25f;
    runner.sprint_boost_timer = 1.85f;
    bool ctrl_changed = (user_controlled_index != best_runner);
    user_controlled_index = best_runner;
    user_move_target = space_target;
    has_user_move_target = true;
    is_paused_for_manual = false;

    _log_event("through_ball", user_is_home, passer.name,
        passer.name + String(" threads a through ball into space for ") + runner.name);

    if (ctrl_changed) {
        emit_signal("controlled_player_changed", user_controlled_index);
    }
    return true;
}

bool MatchEngine::command_shot(Vector2 p_aim_target, float p_power_ratio) {
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume || chance_state != _user_attack_chance_state()) {
        return false;
    }

    int shooter_idx = _resolve_user_action_player();
    if (!_is_user_player_idx(shooter_idx)) {
        return false;
    }

    const SimPlayer& shooter = sim_players[shooter_idx];
    Vector2 shot_vec = p_aim_target - shooter.position;
    Vector2 shot_dir = (shot_vec.length() > 1.0f) ? shot_vec.normalized() : Vector2(_user_attack_dir(), 0.0f);

    sim_players[shooter_idx].dribble_boost_timer = 0.0f;
    ball_position = shooter.position + shot_dir * 10.0f;
    ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
    ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);

    float clamped_power = std::clamp(p_power_ratio, 0.25f, 1.35f);

    Vector2 opp_goal_center(_user_opp_goal_x(), field_size.y * 0.5f);
    float dist_to_goal = shooter.position.distance_to(opp_goal_center);
    float xg = std::clamp((1.0f - dist_to_goal / 320.0f) * (0.48f + shooter.shooting * 0.0042f), 0.08f, 0.75f);

    has_user_move_target = false;
    is_paused_for_manual = false;
    _execute_shot(shooter_idx, xg, p_aim_target, clamped_power);
    return true;
}

bool MatchEngine::command_tackle() {
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume || chance_state != _user_defend_chance_state()) {
        return false;
    }
    if (!_is_user_outfield_idx(user_controlled_index)) {
        select_nearest_defender();
    }
    if (!_is_user_outfield_idx(user_controlled_index)) {
        return false;
    }

    SimPlayer& defender = sim_players[user_controlled_index];
    if (defender.recovery_timer > 0.0f || defender.tackle_cooldown > 0.0f || defender.is_sliding) {
        return false;
    }

    Vector2 intercept_point = ball_position + (ball_velocity * 0.14f);
    if (ball_state == BALL_CARRIED && ball_carrier_index >= 0 && ball_carrier_index < 22) {
        intercept_point = sim_players[ball_carrier_index].position + (sim_players[ball_carrier_index].velocity * 0.14f);
    }

    float dist_to_point = defender.position.distance_to(intercept_point);
    if (dist_to_point > 84.0f) {
        defender.sprint_boost_timer = 0.60f;
        command_move_player(intercept_point);
        return false;
    }

    Vector2 delta = intercept_point - defender.position;
    defender.slide_dir = (delta.length() > 1.0f)
        ? delta.normalized()
        : Vector2(user_is_home ? -1.0f : 1.0f, 0.0f);
    defender.is_sliding = true;
    defender.slide_timer = 0.34f;
    defender.tackle_cooldown = 0.95f;
    defender.stamina = std::max(10.0f, defender.stamina - 0.95f);
    has_user_move_target = false;
    is_paused_for_manual = false;

    if (defender.position.distance_to(ball_position) <= 19.0f) {
        _resolve_slide_tackle_contact(user_controlled_index);
    }
    return true;
}

void MatchEngine::_award_defensive_free_kick(bool p_to_home, Vector2 p_spot, const String& p_fouler_name) {
    if (p_to_home) stats.away_fouls++;
    else stats.home_fouls++;

    _log_event("foul", !p_to_home, p_fouler_name,
        p_fouler_name + String(" commits a foul — free kick awarded!"));
    _begin_restart(RESTART_FREE_KICK, p_to_home, p_spot);
}

Array MatchEngine::get_sent_off_flags() const {
    Array arr;
    for (const SimPlayer& p : sim_players) {
        arr.append(p.is_sent_off);
    }
    return arr;
}

Array MatchEngine::get_yellow_card_counts() const {
    Array arr;
    for (const SimPlayer& p : sim_players) {
        arr.append(p.yellow_cards);
    }
    return arr;
}

bool MatchEngine::_is_last_defender_dogso(const SimPlayer& p_defender, const SimPlayer& p_attacker) const {
    float atk_goal_x = p_attacker.is_home ? field_size.x : 0.0f;
    float atk_dist_to_goal = std::abs(p_attacker.position.x - atk_goal_x);
    if (atk_dist_to_goal > field_size.x * 0.42f) {
        return false;
    }
    int def_start = p_defender.is_home ? 1 : 12;
    int outfield_ahead = 0;
    for (int i = def_start; i < def_start + 10; i++) {
        if (sim_players[i].is_sent_off) continue;
        float d = std::abs(sim_players[i].position.x - atk_goal_x);
        if (d < atk_dist_to_goal - 6.0f) {
            outfield_ahead++;
        }
    }
    return outfield_ahead == 0;
}

bool MatchEngine::_try_apply_tackle_injury(SimPlayer& p_attacker, float p_challenge_force, bool p_is_slide) {
    if (p_attacker.is_home != user_is_home || p_attacker.player_id.is_empty() || p_attacker.role == "GK") {
        return false;
    }
    for (int i = 0; i < match_injuries.size(); i++) {
        Dictionary existing = match_injuries[i];
        if (String(existing.get("player_id", "")) == p_attacker.player_id) {
            return false;
        }
    }

    float fatigue_risk = std::clamp((100.0f - p_attacker.stamina) / 100.0f, 0.0f, 1.0f);
    float contact_mult = (p_is_slide ? 1.45f : 0.75f) * std::clamp(p_challenge_force, 0.7f, 1.4f);
    float injury_prob = std::clamp((0.004f + fatigue_risk * 0.011f) * contact_mult, 0.002f, 0.024f);
    if (UtilityFunctions::randf() >= injury_prob) {
        return false;
    }

    float r = UtilityFunctions::randf();
    int sev = (r < 0.72f) ? 1 : ((r < 0.95f) ? 2 : 3);
    Dictionary inj;
    inj["player_id"] = p_attacker.player_id;
    inj["severity"] = sev;
    inj["days"] = -1;
    match_injuries.append(inj);

    _log_event("injury", p_attacker.is_home, p_attacker.name,
        p_attacker.name + String(" picks up a knock from the challenge."));
    return true;
}

void MatchEngine::_evaluate_foul_and_discipline(int p_defender_idx, int p_attacker_idx, float p_challenge_force, float p_from_behind, bool p_is_slide) {
    if (p_defender_idx < 0 || p_defender_idx >= 22 || p_attacker_idx < 0 || p_attacker_idx >= 22) {
        return;
    }
    SimPlayer& defender = sim_players[p_defender_idx];
    SimPlayer& attacker = sim_players[p_attacker_idx];
    if (defender.is_sent_off) return;

    int def_mentality = defender.is_home ? home_mentality : away_mentality;
    int def_pressing = defender.is_home ? home_pressing : away_pressing;
    float tactic_aggression = 0.72f + (static_cast<float>(def_mentality) * 0.10f) + (static_cast<float>(def_pressing) * 0.14f);

    bool injured_on_play = _try_apply_tackle_injury(attacker, p_challenge_force, p_is_slide);
    bool is_dogso = _is_last_defender_dogso(defender, attacker);

    float danger = 0.18f * tactic_aggression
                 + (p_is_slide ? 0.16f : 0.0f)
                 + std::max(0.0f, p_from_behind) * 0.28f
                 + (std::clamp(p_challenge_force, 0.5f, 1.5f) - 0.8f) * 0.18f
                 + (is_dogso ? 0.42f : 0.0f)
                 + (injured_on_play ? 0.08f : 0.0f);

    bool direct_red = (is_dogso && p_from_behind > 0.35f && UtilityFunctions::randf() < std::clamp(danger * 0.65f, 0.15f, 0.75f))
                   || (danger > 0.85f && UtilityFunctions::randf() < 0.28f);

    if (direct_red) {
        defender.is_sent_off = true;
        defender.position = Vector2(field_size.x * 0.5f, 4.0f);
        defender.velocity = Vector2(0.0f, 0.0f);
        if (defender.is_home) stats.home_red_cards++;
        else stats.away_red_cards++;
        if (user_controlled_index == p_defender_idx) {
            select_nearest_defender();
        }
        _log_event("red_card", defender.is_home, defender.name,
            String("RED CARD! ") + defender.name + String(" is sent off for a serious foul!"));
    } else if (UtilityFunctions::randf() < std::clamp(danger * 0.75f, 0.12f, 0.82f)) {
        defender.yellow_cards++;
        if (defender.yellow_cards >= 2) {
            defender.is_sent_off = true;
            defender.position = Vector2(field_size.x * 0.5f, 4.0f);
            defender.velocity = Vector2(0.0f, 0.0f);
            if (defender.is_home) stats.home_red_cards++;
            else stats.away_red_cards++;
            if (user_controlled_index == p_defender_idx) {
                select_nearest_defender();
            }
            _log_event("red_card", defender.is_home, defender.name,
                String("SECOND YELLOW -> RED CARD! ") + defender.name + String(" is dismissed!"));
        } else {
            if (defender.is_home) stats.home_yellow_cards++;
            else stats.away_yellow_cards++;
            _log_event("yellow_card", defender.is_home, defender.name,
                String("YELLOW CARD! ") + defender.name + String(" is booked by the referee."));
        }
    }
}

bool MatchEngine::_evaluate_pass_offside(int p_passer_idx, int p_receiver_idx) const {
    if (p_passer_idx < 0 || p_passer_idx >= 22 || p_receiver_idx < 0 || p_receiver_idx >= 22) {
        return false;
    }
    const SimPlayer& passer = sim_players[p_passer_idx];
    const SimPlayer& receiver = sim_players[p_receiver_idx];
    if (passer.is_home != receiver.is_home || receiver.role == "GK") {
        return false;
    }

    float mid_x = field_size.x * 0.5f;
    bool atk_home = passer.is_home;
    bool in_opp_half = atk_home ? (receiver.position.x > mid_x + 1.0f) : (receiver.position.x < mid_x - 1.0f);
    if (!in_opp_half) {
        return false;
    }

    int opp_start = atk_home ? 11 : 0;
    std::vector<float> opp_xs;
    opp_xs.reserve(11);
    for (int i = opp_start; i < opp_start + 11; i++) {
        if (sim_players[i].is_sent_off) continue;
        opp_xs.push_back(sim_players[i].position.x);
    }
    if (opp_xs.size() < 2) {
        return false;
    }

    if (atk_home) {
        std::sort(opp_xs.begin(), opp_xs.end(), std::greater<float>());
        float second_last_x = opp_xs[1];
        float offside_line = std::max(mid_x, std::max(second_last_x, ball_position.x));
        return receiver.position.x > (offside_line + 2.5f);
    } else {
        std::sort(opp_xs.begin(), opp_xs.end());
        float second_last_x = opp_xs[1];
        float offside_line = std::min(mid_x, std::min(second_last_x, ball_position.x));
        return receiver.position.x < (offside_line - 2.5f);
    }
}

void MatchEngine::_snapshot_pass_offside_flags(int p_passer_idx) {
    for (int i = 0; i < 22; i++) {
        pass_offside_flags[i] = false;
    }
    if (p_passer_idx < 0 || p_passer_idx >= 22) return;
    bool atk_home = sim_players[p_passer_idx].is_home;
    int start = atk_home ? 1 : 12;
    int end = atk_home ? 11 : 22;
    for (int i = start; i < end; i++) {
        if (i == p_passer_idx || sim_players[i].is_sent_off) continue;
        pass_offside_flags[i] = _evaluate_pass_offside(p_passer_idx, i);
    }
}

void MatchEngine::_begin_restart(int p_restart_type, bool p_to_home, Vector2 p_spot) {
    if (!match_active || sim_players.size() != 22) return;
    if (chance_state != CHANCE_NONE) {
        _end_active_chance(false);
    }
    cancel_gk_rush();

    restart_state = p_restart_type;
    restart_to_home = p_to_home;
    restart_spot.x = std::clamp(p_spot.x, 14.0f, field_size.x - 14.0f);
    restart_spot.y = std::clamp(p_spot.y, 14.0f, field_size.y - 14.0f);
    restart_timer = 0.72f;

    int team_start = p_to_home ? 0 : 11;
    int taker = -1;
    if (p_restart_type == RESTART_GOAL_KICK) {
        taker = team_start; // GK
    } else {
        float best_d = 1e9f;
        for (int i = team_start + 1; i < team_start + 11; i++) {
            if (sim_players[i].is_sent_off) continue;
            float d = sim_players[i].position.distance_to(restart_spot);
            if (d < best_d) {
                best_d = d;
                taker = i;
            }
        }
        if (taker == -1) taker = team_start;
    }

    restart_taker_idx = taker;
    sim_players[taker].position = restart_spot;
    sim_players[taker].velocity = Vector2(0.0f, 0.0f);
    ball_position = restart_spot;
    ball_velocity = Vector2(0.0f, 0.0f);
    ball_state = BALL_CARRIED;
    ball_carrier_index = taker;
    last_touch_index = taker;
    possession_team = p_to_home ? 0 : 1;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    pending_pass_offside = false;
    for (int i = 0; i < 22; i++) pass_offside_flags[i] = false;

    String side_name = p_to_home ? home_club_name : away_club_name;
    if (p_restart_type == RESTART_THROW_IN) {
        _log_event("throw_in", p_to_home, sim_players[taker].name,
            String("Throw-in to ") + side_name + String("."));
    } else if (p_restart_type == RESTART_GOAL_KICK) {
        _log_event("goal_kick", p_to_home, sim_players[taker].name,
            String("Goal kick to ") + side_name + String("."));
    } else if (p_restart_type == RESTART_CORNER) {
        _log_event("corner", p_to_home, sim_players[taker].name,
            String("Corner kick awarded to ") + side_name + String("!"));
    } else if (p_restart_type == RESTART_FREE_KICK) {
        _log_event("free_kick", p_to_home, sim_players[taker].name,
            String("Indirect free kick to ") + side_name + String("."));
    }
    emit_signal("restart_state_changed", restart_state, restart_to_home, restart_spot);
}

void MatchEngine::_complete_restart() {
    if (restart_state == RESTART_NONE || sim_players.size() != 22) {
        restart_state = RESTART_NONE;
        return;
    }

    int prev_restart = restart_state;
    bool to_home = restart_to_home;
    int taker = (restart_taker_idx >= 0 && restart_taker_idx < 22) ? restart_taker_idx : (to_home ? 0 : 11);
    restart_state = RESTART_NONE;
    restart_timer = 0.0f;
    restart_taker_idx = -1;
    emit_signal("restart_state_changed", RESTART_NONE, to_home, restart_spot);

    int team_start = to_home ? 0 : 11;
    int receiver = -1;
    float best_score = -1e9f;
    for (int i = team_start + 1; i < team_start + 11; i++) {
        if (i == taker || sim_players[i].is_sent_off) continue;
        float d = sim_players[i].position.distance_to(sim_players[taker].position);
        float score = -d;
        if (prev_restart == RESTART_CORNER) {
            float opp_goal_x = to_home ? field_size.x : 0.0f;
            float dist_to_goal = sim_players[i].position.distance_to(Vector2(opp_goal_x, field_size.y * 0.5f));
            score = -dist_to_goal + ((sim_players[i].role == "ST" || sim_players[i].role == "CB" || sim_players[i].role == "CAM") ? 45.0f : 0.0f);
        } else if (prev_restart == RESTART_GOAL_KICK) {
            score = (d > 65.0f && d < 260.0f) ? (120.0f - std::abs(d - 150.0f)) : -d;
        }
        if (score > best_score) {
            best_score = score;
            receiver = i;
        }
    }

    if (receiver != -1) {
        _execute_pass(taker, receiver, false);
        // No offside directly from a throw-in, goal kick, or corner
        if (prev_restart == RESTART_THROW_IN || prev_restart == RESTART_GOAL_KICK || prev_restart == RESTART_CORNER) {
            pending_pass_offside = false;
            for (int i = 0; i < 22; i++) pass_offside_flags[i] = false;
        }
        if (prev_restart == RESTART_CORNER && chance_state == CHANCE_NONE) {
            _spawn_wide_range_chance(to_home);
        }
    } else {
        action_decision_timer = 0.25f;
    }
}

void MatchEngine::_resolve_slide_tackle_contact(int p_defender_idx) {
    if (p_defender_idx < 0 || p_defender_idx >= 22) return;
    SimPlayer& defender = sim_players[p_defender_idx];
    defender.is_sliding = false;
    defender.slide_timer = 0.0f;

    float def_fatigue = _get_fatigue_multiplier(defender);
    int user_approach = user_is_home ? home_defensive_approach : away_defensive_approach;
    float approach_bonus = (user_approach == TIER_HIGH) ? 5.5f : ((user_approach == TIER_LOW) ? -3.5f : 0.0f);
    float own_goal_x = defender.is_home ? 0.0f : field_size.x;

    if (ball_state == BALL_CARRIED && ball_carrier_index >= 0 && ball_carrier_index < 22 && sim_players[ball_carrier_index].is_home != defender.is_home) {
        SimPlayer& attacker = sim_players[ball_carrier_index];
        float atk_fatigue = _get_fatigue_multiplier(attacker);
        bool in_box = std::abs(defender.position.x - own_goal_x) < 140.0f && std::abs(defender.position.y - field_size.y * 0.5f) < 168.0f;

        Vector2 atk_facing = (attacker.velocity.length() > 8.0f)
            ? attacker.velocity.normalized()
            : Vector2(attacker.is_home ? 1.0f : -1.0f, 0.0f);
        float from_behind = defender.slide_dir.dot(atk_facing); // > 0.45 means tackling from behind

        int def_mentality = defender.is_home ? home_mentality : away_mentality;
        int def_pressing = defender.is_home ? home_pressing : away_pressing;
        float tactic_aggression = 0.72f + (static_cast<float>(def_mentality) * 0.10f) + (static_cast<float>(def_pressing) * 0.14f);

        float foul_risk = std::clamp(
            (0.09f + std::max(0.0f, from_behind - 0.30f) * 0.36f - ((defender.defending * def_fatigue - 65.0f) * 0.0022f)) * tactic_aggression,
            0.03f, 0.55f
        );
        if (UtilityFunctions::randf() < foul_risk) {
            defender.recovery_timer = 0.45f;
            int fouled_idx = ball_carrier_index;
            float challenge_force = 0.85f + (defender.physical / 200.0f);
            _award_defensive_free_kick(attacker.is_home, attacker.position, defender.name);
            _evaluate_foul_and_discipline(p_defender_idx, fouled_idx, challenge_force, from_behind, true);
            return;
        }

        float def_core = (defender.defending * 0.64f + defender.physical * 0.24f + defender.pace * 0.12f) * def_fatigue;
        float atk_core = (attacker.dribbling * 0.62f + attacker.physical * 0.22f + attacker.pace * 0.16f) * atk_fatigue;
        float skill_diff = def_core - atk_core + approach_bonus + _get_role_defensive_bonus(defender, in_box, false);
        float win_prob = std::clamp(0.50f + 0.34f * std::tanh(skill_diff / 26.0f), 0.16f, 0.85f);

        if (UtilityFunctions::randf() < win_prob) {
            if (defender.is_home) stats.home_tackles++;
            else stats.away_tackles++;

            attacker.recovery_timer = std::clamp(0.24f + (defender.physical / 100.0f) * 0.18f, 0.24f, 0.42f);
            defender.recovery_timer = 0.26f;

            // Knock ball loose along slide + perpendicular deflection angle (can reach either team)
            Vector2 perp(-defender.slide_dir.y, defender.slide_dir.x);
            float lateral_jitter = static_cast<float>(UtilityFunctions::randf_range(-0.65, 0.65));
            Vector2 deflect_dir = (defender.slide_dir * 0.72f + perp * lateral_jitter).normalized();
            float knock_speed = std::clamp(145.0f + (defender.physical * 0.85f), 145.0f, 235.0f);

            possession_team = -1;
            ball_carrier_index = -1;
            pass_target_index = -1;
            is_through_ball_in_flight = false;
            ball_state = BALL_LOOSE;
            ball_position = attacker.position + deflect_dir * 12.0f;
            ball_velocity = deflect_dir * knock_speed;
            last_touch_index = p_defender_idx;

            _log_event("tackle", defender.is_home, defender.name,
                defender.name + String(" slides in and knocks the ball loose!"));
            return;
        }
    } else if (ball_state == BALL_LOOSE && defender.position.distance_to(ball_position) <= 22.0f) {
        if (defender.is_home) stats.home_tackles++;
        else stats.away_tackles++;

        Vector2 clear_dir = defender.slide_dir;
        possession_team = -1;
        ball_carrier_index = -1;
        ball_state = BALL_LOOSE;
        ball_velocity = clear_dir * 185.0f;
        last_touch_index = p_defender_idx;
        defender.recovery_timer = 0.30f;
        return;
    }

    // Missed slide tackle
    float phy_resilience = std::clamp(1.16f - (defender.physical / 100.0f) * 0.28f, 0.86f, 1.08f);
    defender.recovery_timer = 0.62f * phy_resilience;
    _log_event("tackle_missed", defender.is_home, defender.name,
        defender.name + String(" misses the slide tackle!"));
}

bool MatchEngine::begin_chance(bool p_is_home_attacking, int p_attacker_idx, float p_xg) {
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume) {
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
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume || chance_state != CHANCE_NONE) {
        return;
    }
    _spawn_wide_range_chance(p_is_home_attacking);
}

void MatchEngine::_spawn_wide_range_chance(bool p_is_home_attacking) {
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume || chance_state != CHANCE_NONE) {
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

    float team_ovr = static_cast<float>(p_is_home_attacking ? home_team_ovr : away_team_ovr);
    float opp_ovr = static_cast<float>(p_is_home_attacking ? away_team_ovr : home_team_ovr);
    float ovr_edge = std::clamp((team_ovr - opp_ovr) * 0.012f, -0.25f, 0.25f);
    float attr_factor = 0.78f + (((carrier.shooting * 0.45f) + (carrier.dribbling * 0.35f) + (carrier.passing * 0.20f)) / 100.0f) * 0.44f;
    float est_xg = std::clamp(dist_factor * angle_factor * 0.55f * attr_factor * (1.0f + ovr_edge), 0.05f, 0.72f);

    _sync_player_positions_array();
    _trigger_key_chance(p_is_home_attacking ? 1 : 4, p_is_home_attacking, carrier_idx, est_xg);
}

void MatchEngine::_end_active_chance(bool p_goal_scored) {
    if (chance_state == CHANCE_NONE && !manual_control_active) {
        return;
    }

    gk_rush_active = false;
    gk_rush_timer = 0.0f;

    if (user_controlled_index >= 0 && user_controlled_index < 22) {
        sim_players[user_controlled_index].dribble_boost_timer = 0.0f;
    }
    for (SimPlayer& p : sim_players) {
        p.dribble_carry_distance = 0.0f;
        p.is_sliding = false;
        p.slide_timer = 0.0f;
        p.slide_dir = Vector2(0.0f, 0.0f);
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

    if (pending_chance_signal && pending_chance_state_val != CHANCE_NONE) {
        // A chance may begin and end inside one native simulation step. Keep
        // both transitions so the scene never misses the active state.
        pending_chance_end_signal = true;
    } else {
        pending_chance_signal = true;
        pending_chance_state_val = CHANCE_NONE;
        pending_chance_ctrl_val = -1;
    }
}

// =========================================================================
// ORGANIZED DEFENSIVE LINE, PRESSING, LANE COVERAGE & SMART ATTACKING RUNS
// =========================================================================

int MatchEngine::_select_smart_presser(bool p_is_home, int p_exclude_idx) const {
    int start_idx = p_is_home ? 1 : 12;
    int end_idx = p_is_home ? 11 : 22;
    float own_goal_x = p_is_home ? 0.0f : field_size.x;
    float ball_dist_to_goal = std::abs(ball_position.x - own_goal_x);
    float center_y = field_size.y * 0.5f;

    int best_idx = -1;
    float best_score = 1e9f;

    for (int i = start_idx; i < end_idx; i++) {
        if (i == p_exclude_idx || sim_players[i].is_sent_off) continue;
        const SimPlayer& p = sim_players[i];
        float d = p.position.distance_to(ball_position);
        float iq = _get_defensive_positioning_iq(p);

        float role_bias = 0.0f;
        if (p.role == "CDM") {
            role_bias = -24.0f;
        } else if (p.role == "CM") {
            role_bias = -16.0f;
        } else if (p.role == "CB") {
            // CBs hold central line unless ball enters their immediate danger zone (< 185px)
            role_bias = (ball_dist_to_goal > 185.0f) ? 46.0f : -10.0f;
        } else if (p.role == "LB" || p.role == "RB" || p.role == "LWB" || p.role == "RWB") {
            bool ball_wide = std::abs(ball_position.y - center_y) > 105.0f;
            bool same_flank = ((p.anchor.y - center_y) * (ball_position.y - center_y)) >= 0.0f;
            role_bias = (ball_wide && same_flank && ball_dist_to_goal < 285.0f) ? -20.0f : 22.0f;
        } else if (p.role == "ST" || p.role == "CF") {
            // Strikers only lead press in opponent's defensive third
            role_bias = (ball_dist_to_goal > field_size.x * 0.62f) ? -14.0f : 38.0f;
        }

        // High-IQ and fast defenders read pressing triggers slightly faster
        float attr_bias = -((iq - 1.0f) * 18.0f + ((p.pace - 70.0f) * 0.14f));
        float score = d + role_bias + attr_bias + (p.recovery_timer * 65.0f);
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

    static const float MENTALITY_LINE_OFFSETS[5] = { -28.0f, -14.0f, 0.0f, 16.0f, 32.0f };
    float mentality_offset = MENTALITY_LINE_OFFSETS[std::clamp(team_mentality, 0, 4)];
    float line_instruction_offset = (team_def_line == TIER_HIGH) ? 36.0f : ((team_def_line == TIER_LOW) ? -26.0f : 0.0f);
    float press_offset = (team_pressing == PRESSING_HIGH) ? 24.0f : ((team_pressing == PRESSING_LOW) ? -16.0f : 0.0f);
    float approach_offset = (team_approach == TIER_HIGH) ? 12.0f : ((team_approach == TIER_LOW) ? -12.0f : 0.0f);

    float ball_depth_from_own_goal = std::abs(ball_position.x - own_goal_x);

    float desired_depth;
    if (ball_depth_from_own_goal >= 225.0f) {
        desired_depth = 188.0f + ((ball_depth_from_own_goal - 225.0f) * 0.42f);
    } else {
        desired_depth = std::max(112.0f, ball_depth_from_own_goal - 16.0f);
    }

    desired_depth += line_instruction_offset + press_offset + mentality_offset + approach_offset;

    float hard_min_depth = (team_def_line == TIER_HIGH) ? 138.0f : ((team_def_line == TIER_LOW) ? 102.0f : 120.0f);
    if (ball_depth_from_own_goal < 115.0f) {
        hard_min_depth = std::max(68.0f, ball_depth_from_own_goal - 16.0f);
    }

    float clamped_depth = std::clamp(desired_depth, hard_min_depth, field_size.x * 0.52f);
    return own_goal_x + (attack_dir * clamped_depth);
}

Vector2 MatchEngine::_compute_lane_cover_target(bool p_is_home, const SimPlayer& p_defender) const {
    float attack_dir = p_is_home ? 1.0f : -1.0f;
    float own_goal_x = p_is_home ? 0.0f : field_size.x;
    Vector2 own_goal_center(own_goal_x, field_size.y * 0.5f);
    float iq = _get_defensive_positioning_iq(p_defender);

    int opp_start = p_is_home ? 11 : 0;
    int best_receiver = -1;
    float best_threat = -1e9f;

    for (int j = opp_start + 1; j < opp_start + 11; j++) {
        if (j == ball_carrier_index) continue;
        const SimPlayer& opp = sim_players[j];
        float dist_ball = opp.position.distance_to(ball_position);
        if (dist_ball < 32.0f || dist_ball > 275.0f) continue;

        float dist_goal = opp.position.distance_to(own_goal_center);
        float central_danger = 90.0f - std::min(90.0f, std::abs(opp.position.y - own_goal_center.y) * 0.45f);
        float threat = (420.0f - dist_goal) + central_danger - (dist_ball * 0.30f);
        if (threat > best_threat) {
            best_threat = threat;
            best_receiver = j;
        }
    }

    // High-IQ defenders anticipate where the receiver is moving, not just static coordinates
    Vector2 danger_point = (best_receiver != -1)
        ? (sim_players[best_receiver].position + sim_players[best_receiver].velocity * (0.18f * iq))
        : own_goal_center;

    Vector2 lane_dir = danger_point - ball_position;
    float lane_len = lane_dir.length();
    float screen_depth = std::clamp(34.0f + iq * 14.0f, 34.0f, 54.0f);
    Vector2 screen_pos = (lane_len > 1.0f)
        ? (ball_position + (lane_dir / lane_len) * std::min(screen_depth, lane_len * 0.45f))
        : (ball_position - Vector2(attack_dir * 38.0f, 0.0f));

    // High-IQ defenders commit closer to the true passing lane; low-IQ defenders retain more anchor drift
    float lane_commit = std::clamp(0.62f + (iq - 0.70f) * 0.42f, 0.55f, 0.90f);
    screen_pos.y = (screen_pos.y * lane_commit) + (p_defender.position.y * (1.0f - lane_commit));
    return screen_pos;
}

Vector2 MatchEngine::_compute_runner_tracking_offset(bool p_is_home, const SimPlayer& p_defender, float p_line_x, float p_base_y) const {
    float attack_dir = p_is_home ? 1.0f : -1.0f;
    float own_goal_x = p_is_home ? 0.0f : field_size.x;
    int opp_start = p_is_home ? 11 : 0;

    float iq = _get_defensive_positioning_iq(p_defender);
    float pace_factor = std::clamp(0.82f + (p_defender.pace - 60.0f) * 0.005f, 0.72f, 1.12f);
    bool is_user_attack_chance = ((p_is_home != user_is_home) && chance_state == _user_attack_chance_state());

    float scan_radius = (is_user_attack_chance ? 96.0f : 112.0f) * std::clamp(iq, 0.78f, 1.25f);
    float track_weight = std::clamp((is_user_attack_chance ? 0.46f : 0.58f) * iq * pace_factor, 0.32f, 0.84f);

    int best_target = -1;
    float best_threat_score = -1e9f;

    for (int j = opp_start + 1; j < opp_start + 11; j++) {
        if (j == ball_carrier_index) continue;
        const SimPlayer& opp = sim_players[j];

        float dist = Vector2(p_line_x, p_base_y).distance_to(opp.position);
        if (dist > scan_radius) continue;

        // Avoid double-teaming a decoy runner if another teammate is already tightly marking them
        bool already_marked = false;
        int team_start = p_is_home ? 1 : 12;
        int team_end = p_is_home ? 11 : 22;
        for (int k = team_start; k < team_end; k++) {
            if (&sim_players[k] == &p_defender) continue;
            if (sim_players[k].position.distance_to(opp.position) < 22.0f) {
                already_marked = true;
                break;
            }
        }

        float opp_forward_vel = opp.velocity.x * (-attack_dir);
        float goal_danger = std::max(0.0f, 340.0f - std::abs(opp.position.x - own_goal_x)) * 0.25f;
        float threat_score = (scan_radius - dist) + goal_danger + (opp_forward_vel > 25.0f ? 28.0f : 0.0f) - (already_marked ? 42.0f : 0.0f);

        if (threat_score > best_threat_score) {
            best_threat_score = threat_score;
            best_target = j;
        }
    }

    Vector2 offset(0.0f, 0.0f);
    if (best_target != -1) {
        const SimPlayer& opp = sim_players[best_target];
        // Anticipate runner path scaled by defender IQ & Pace, positioning goal-side and ball-side
        Vector2 predicted_opp = opp.position + opp.velocity * (0.16f * iq * pace_factor);
        Vector2 to_ball = (ball_position - predicted_opp);
        Vector2 ball_side_nudge = (to_ball.length() > 1.0f) ? (to_ball.normalized() * 6.0f) : Vector2(0.0f, 0.0f);
        Vector2 goal_side_mark = predicted_opp - Vector2(attack_dir * 14.0f, 0.0f) + ball_side_nudge;

        offset = (goal_side_mark - Vector2(p_line_x, p_base_y)) * track_weight;
    }

    // Allow deeper drop-back tracking for fast/aware defenders while preventing reckless forward stepping
    float max_drop = 34.0f * pace_factor;
    float max_step = 18.0f * iq;
    if (p_is_home) {
        offset.x = std::clamp(offset.x, -max_drop, max_step);
    } else {
        offset.x = std::clamp(offset.x, -max_step, max_drop);
    }
    return offset;
}

Vector2 MatchEngine::_compute_smart_attacking_run_target(
    const SimPlayer& p_player,
    int p_index,
    float p_widened_anchor_y,
    float p_ball_prog
) const {
    bool is_home = p_player.is_home;
    float attack_dir = is_home ? 1.0f : -1.0f;
    float center_y = field_size.y * 0.5f;
    float ball_center_offset_y = ball_position.y - center_y;
    String r = p_player.role;

    int team_start = is_home ? 0 : 11;
    int team_end = is_home ? 11 : 22;
    int opp_outfield_start = is_home ? 12 : 1;
    int opp_outfield_end = is_home ? 22 : 11;

    int carrier_idx = (ball_carrier_index >= team_start && ball_carrier_index < team_end)
        ? ball_carrier_index
        : ((user_controlled_index >= team_start && user_controlled_index < team_end) ? user_controlled_index : -1);
    Vector2 carrier_pos = (carrier_idx != -1) ? sim_players[carrier_idx].position : ball_position;

    // Locate the deepest outfield opponent defender
    float deepest_opp_def_x = is_home ? (field_size.x - 105.0f) : 105.0f;
    for (int j = opp_outfield_start; j < opp_outfield_end; j++) {
        if (is_home) {
            deepest_opp_def_x = std::max(deepest_opp_def_x, sim_players[j].position.x);
        } else {
            deepest_opp_def_x = std::min(deepest_opp_def_x, sim_players[j].position.x);
        }
    }

    auto advance_x = [&](float cur_x, float min_target_x) {
        return is_home ? std::max(cur_x, min_target_x) : std::min(cur_x, min_target_x);
    };

    float base_x = p_player.anchor.x + attack_dir * ((p_ball_prog * 365.0f) + 145.0f);
    float base_y = p_widened_anchor_y + (ball_center_offset_y * 0.24f);

    if (r == "ST" || r == "CF") {
        float channel_y = (carrier_pos.y < center_y)
            ? std::clamp(center_y + 52.0f, 95.0f, field_size.y - 95.0f)
            : std::clamp(center_y - 52.0f, 95.0f, field_size.y - 95.0f);
        base_y = (base_y * 0.45f) + (channel_y * 0.55f);
        base_x = advance_x(base_x, advance_x(carrier_pos.x + attack_dir * 56.0f, deepest_opp_def_x + attack_dir * 26.0f));
    } else if (r == "LW" || r == "RW" || r == "LM" || r == "RM") {
        bool is_top_wing = (p_player.anchor.y < center_y);
        bool ball_on_same_side = ((carrier_pos.y < center_y) == is_top_wing);
        if (!ball_on_same_side) {
            float inside_channel_y = is_top_wing ? (center_y - 76.0f) : (center_y + 76.0f);
            base_y = (base_y * 0.35f) + (inside_channel_y * 0.65f);
            base_x = advance_x(base_x, advance_x(carrier_pos.x + attack_dir * 64.0f, deepest_opp_def_x + attack_dir * 22.0f));
        } else {
            float wide_y = is_top_wing ? std::max(54.0f, p_widened_anchor_y - 18.0f)
                                       : std::min(field_size.y - 54.0f, p_widened_anchor_y + 18.0f);
            base_y = wide_y;
            base_x = advance_x(base_x, carrier_pos.x + attack_dir * 48.0f);
        }
    } else if (r == "CAM" || r == "CM") {
        float pocket_y = center_y - (ball_center_offset_y * 0.45f);
        base_y = std::clamp((base_y * 0.40f) + (pocket_y * 0.60f), 85.0f, field_size.y - 85.0f);
        base_x = advance_x(base_x, carrier_pos.x + attack_dir * ((r == "CAM") ? 42.0f : 24.0f));
    } else if (r == "LB" || r == "RB" || r == "LWB" || r == "RWB") {
        base_x = advance_x(base_x, carrier_pos.x + attack_dir * 12.0f);
    }

    float min_x = is_home ? 70.0f : 42.0f;
    float max_x = is_home ? (field_size.x - 42.0f) : (field_size.x - 70.0f);
    base_x = std::clamp(base_x, min_x, max_x);
    base_y = std::clamp(base_y, 36.0f, field_size.y - 36.0f);
    Vector2 candidate(base_x, base_y);

    Vector2 to_cand = candidate - carrier_pos;
    float cand_dist = to_cand.length();
    if (cand_dist > 28.0f) {
        Vector2 lane_dir = to_cand / cand_dist;
        Vector2 perp_dir(-lane_dir.y, lane_dir.x);
        for (int j = opp_outfield_start; j < opp_outfield_end; j++) {
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

    candidate.x = std::clamp(candidate.x, is_home ? 60.0f : 38.0f, is_home ? (field_size.x - 38.0f) : (field_size.x - 60.0f));
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
        if (gk_rush_active && p_index == _user_gk_idx()) {
            Vector2 desired = (manual_control_active && p_index == user_controlled_index && has_user_move_target)
                ? user_move_target
                : (ball_position + (ball_velocity * 0.16f));
            float max_rush_x = is_home ? std::min(field_size.x * 0.34f, 260.0f)
                                       : std::max(field_size.x * 0.66f, field_size.x - 260.0f);
            float clamped_x = is_home ? std::clamp(desired.x, 14.0f, max_rush_x)
                                      : std::clamp(desired.x, max_rush_x, field_size.x - 14.0f);
            float clamped_y = std::clamp(desired.y, field_size.y * 0.22f, field_size.y * 0.78f);
            return Vector2(clamped_x, clamped_y);
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
        if (chance_state == _user_defend_chance_state()) {
            // Auto-jockey goal-side of the ball carrier until the user clicks a move/tackle point
            Vector2 goal_center(own_goal_x, center_y);
            Vector2 to_goal = goal_center - ball_position;
            Vector2 jockey_offset = (to_goal.length() > 1.0f)
                ? (to_goal.normalized() * 16.0f)
                : Vector2(-attack_dir * 16.0f, 0.0f);
            return ball_position + jockey_offset;
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
        if ((is_home == user_is_home) && chance_state == _user_attack_chance_state() && r != "CB") {
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
    float press_trigger_dist = (team_pressing == PRESSING_HIGH) ? 430.0f :
                               ((team_pressing == PRESSING_LOW) ? 245.0f : 325.0f);

    float def_team_ovr = static_cast<float>(is_home ? home_team_ovr : away_team_ovr);
    float def_quality = std::clamp((def_team_ovr - 50.0f) / 45.0f, 0.0f, 1.0f);
    float iq = _get_defensive_positioning_iq(p_player);

    if ((is_home != user_is_home) && chance_state == _user_attack_chance_state()) {
        press_trigger_dist = std::min(press_trigger_dist, 240.0f + def_quality * 115.0f);
    }

    // Primary presser closes down ball-carrier
    if (p_index == p_primary_presser && !(manual_control_active && (is_home == user_is_home))) {
        if (ball_dist_from_own_goal <= press_trigger_dist) {
            return ball_position - Vector2(attack_dir * 7.5f, 0.0f);
        }
        // Jockey/contain rather than standing idle when ball is outside high-press zone
        return ball_position - Vector2(attack_dir * 22.0f, (ball_position.y - center_y) * 0.14f);
    }

    // Secondary presser screens the most dangerous passing lane
    if (p_index == p_secondary_presser && (team_pressing >= PRESSING_NORMAL || ball_dist_from_own_goal <= press_trigger_dist + 55.0f)) {
        return _compute_lane_cover_target(is_home, p_player);
    }

    // Zone-entry containment: prevent defenders from being unrealistically passive when a carrier dribbles right into their zone
    float dist_to_ball = p_player.position.distance_to(ball_position);
    float zone_engage_radius = (r == "ST" || r == "CF") ? 42.0f : (56.0f * std::clamp(iq, 0.80f, 1.22f));
    if (dist_to_ball < zone_engage_radius && ball_dist_from_own_goal < field_size.x * 0.65f && !(manual_control_active && (is_home == user_is_home))) {
        return ball_position - Vector2(attack_dir * 10.0f, 0.0f);
    }

    // Role-based vertical line spacing & block compactness
    float block_compression = (team_def_line == TIER_LOW) ? 0.84f : ((team_def_line == TIER_HIGH) ? 1.08f : 1.0f);
    float line_x = p_team_backline_x;
    if (r == "CB") {
        line_x = p_team_backline_x;
    } else if (r == "LB" || r == "RB" || r == "LWB" || r == "RWB") {
        bool is_ball_side = ((p_player.anchor.y - center_y) * ball_center_offset_y) >= 0.0f;
        line_x = p_team_backline_x + (attack_dir * (is_ball_side ? 12.0f : -4.0f));
    } else if (r == "CDM") {
        line_x = p_team_backline_x + (attack_dir * (58.0f * block_compression));
    } else if (r == "CM" || r == "CAM") {
        float mid_gap = ((r == "CAM") ? 124.0f : 96.0f) * block_compression;
        line_x = p_team_backline_x + (attack_dir * mid_gap);
    } else if (r == "LW" || r == "RW" || r == "LM" || r == "RM") {
        float wing_gap = ((team_mentality >= MENTALITY_ATTACKING) ? 140.0f : 104.0f) * block_compression;
        line_x = p_team_backline_x + (attack_dir * wing_gap);
    } else {
        float st_gap = (team_mentality <= MENTALITY_DEFENSIVE) ? 168.0f : 208.0f;
        line_x = p_team_backline_x + (attack_dir * st_gap);
    }

    // Low-IQ defenders have slight vertical line alignment drift; high-IQ defenders hold a crisp line
    if (_is_backline_role(r)) {
        float line_discipline_jitter = (1.0f - std::clamp(iq, 0.55f, 1.25f)) * ((p_index % 2 == 0) ? 12.0f : -12.0f);
        line_x += line_discipline_jitter;
    }

    line_x = std::clamp(line_x, 76.0f, field_size.x - 76.0f);

    // Lateral compactness & weak-side fullback tuck
    float depth_compactness = std::clamp(0.64f + (ball_dist_from_own_goal / field_size.x) * 0.26f, 0.64f, 0.90f);
    depth_compactness -= (iq - 0.85f) * 0.08f; // Smarter defenses stay slightly more compact
    depth_compactness = std::clamp(depth_compactness, 0.60f, 0.92f);

    float compact_anchor_y = center_y + ((p_player.anchor.y - center_y) * depth_compactness);
    float ball_shift_y = ball_center_offset_y * 0.48f;
    float compact_y = std::clamp(compact_anchor_y + ball_shift_y, 34.0f, field_size.y - 34.0f);

    if (r == "LB" || r == "RB" || r == "LWB" || r == "RWB") {
        bool is_weak_side = ((p_player.anchor.y - center_y) * ball_center_offset_y) < 0.0f;
        if (is_weak_side && std::abs(ball_center_offset_y) > 75.0f) {
            // Far-side fullback tucks inside to protect the back post and close the CB-FB channel
            compact_y = compact_y * 0.78f + center_y * 0.22f;
        }
    }

    if (_is_backline_role(r) || r == "CDM") {
        Vector2 track_offset = _compute_runner_tracking_offset(is_home, p_player, line_x, compact_y);
        line_x += track_offset.x;
        compact_y += track_offset.y;

        float min_goal_clearance = (ball_dist_from_own_goal < 115.0f) ? 68.0f : 102.0f;
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
        if (p.is_sent_off) {
            p.velocity = Vector2(0.0f, 0.0f);
            continue;
        }
        if (p.slide_timer > 0.0f) {
            p.slide_timer = std::max(0.0f, p.slide_timer - dt);
            if (p.slide_timer <= 0.0f && p.is_sliding) {
                p.is_sliding = false;
                p.recovery_timer = std::max(p.recovery_timer, 0.56f);
            }
        }
        if (ball_state == BALL_CARRIED && ball_carrier_index == i) {
            p.dribble_carry_distance += p.velocity.length() * dt;
        } else {
            p.dribble_carry_distance = 0.0f;
        }
        if (p.recovery_timer > 0.0f) {
            p.recovery_timer = std::max(0.0f, p.recovery_timer - dt);
        }
        if (p.tackle_cooldown > 0.0f) {
            p.tackle_cooldown = std::max(0.0f, p.tackle_cooldown - dt);
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
        bool is_user_defend_chance = ((p.is_home == user_is_home) && chance_state == _user_defend_chance_state());
        bool is_user_defending_ctrl = (is_user_controlled && is_user_defend_chance);

        if (is_user_controlled && has_user_move_target && dist < 8.0f) {
            has_user_move_target = false;
        }

        float fatigue_mult = _get_fatigue_multiplier(p);
        int team_tempo = p.is_home ? home_tempo : away_tempo;
        float tempo_speed_mult = (team_tempo == TIER_HIGH) ? 1.07f : ((team_tempo == TIER_LOW) ? 0.93f : 1.0f);

        float ai_team_ovr = static_cast<float>(user_is_home ? away_team_ovr : home_team_ovr);
        float def_quality = std::clamp((ai_team_ovr - 50.0f) / 45.0f, 0.0f, 1.0f);
        float team_line_x = p.is_home ? home_backline_x : away_backline_x;
        bool is_recovering = _is_player_recovering_position(p, team_line_x) && (dist > 28.0f);

        float max_speed = (95.0f + p.pace * 0.88f) * fatigue_mult * tempo_speed_mult;
        if (p.role == "GK") {
            bool is_rushing_user_gk = (gk_rush_active && i == _user_gk_idx());
            max_speed = is_rushing_user_gk
                ? ((98.0f + p.pace * 0.92f) * fatigue_mult)
                : (max_speed * 0.68f);
        } else if ((p.is_home == user_is_home) && chance_state == _user_attack_chance_state() && i != user_controlled_index && p.role != "CB") {
            max_speed *= 1.07f;
        } else if (is_user_defending_ctrl) {
            // Responsive closing pace for the user's active defender during Defend Chances
            max_speed = (108.0f + p.pace * 0.96f) * fatigue_mult * tempo_speed_mult;
        } else if (is_user_defend_chance) {
            // Keep user's AI backline/midfield teammates alert when tracking runners
            max_speed *= 1.10f;
        }

        if (p.is_sliding) {
            max_speed = (155.0f + p.pace * 0.65f + p.physical * 0.35f) * fatigue_mult;
        } else if (p.recovery_timer > 0.0f) {
            max_speed *= 0.35f;
        } else if (is_user_ball_carrier) {
            float carry_fatigue_drag = 1.0f - std::clamp((p.dribble_carry_distance - 85.0f) / 260.0f, 0.0f, 0.13f);
            max_speed = (93.0f + p.pace * 0.84f + p.dribbling * 0.14f) * fatigue_mult * tempo_speed_mult * carry_fatigue_drag;
            if (p.dribble_boost_timer > 0.0f) {
                max_speed *= 1.07f;
            }
        } else if (p.sprint_boost_timer > 0.0f) {
            max_speed *= 1.15f;
        } else if (is_recovering) {
            float pace_recovery_boost = 1.05f + (std::clamp(p.pace / 100.0f, 0.30f, 1.0f) * 0.13f);
            max_speed *= pace_recovery_boost;
        } else if (ball_carrier_index == i && ball_state == BALL_CARRIED) {
            max_speed *= 0.93f;
        }

        Vector2 desired_vel(0.0f, 0.0f);
        if (dist > 1.5f) {
            float slow_radius = is_user_defending_ctrl ? 12.0f : (is_user_ball_carrier ? 24.0f : (is_recovering ? 22.0f : 32.0f));
            float speed_scale = std::clamp(dist / slow_radius, 0.18f, 1.0f);
            float target_speed = max_speed * speed_scale;
            desired_vel = (to_target / dist) * target_speed;
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
        if (desired_vel.length() > max_speed * 1.12f) {
            desired_vel = desired_vel.normalized() * (max_speed * 1.12f);
        }

        int team_pressing = p.is_home ? home_pressing : away_pressing;
        int team_mentality = p.is_home ? home_mentality : away_mentality;

        float press_mult = (team_pressing == PRESSING_HIGH) ? 1.46f :
                           ((team_pressing == PRESSING_LOW) ? 0.72f : 1.00f);
        static const float MENTALITY_DRAIN[5] = { 0.82f, 0.90f, 1.00f, 1.15f, 1.30f };
        float mentality_mult = MENTALITY_DRAIN[std::clamp(team_mentality, 0, 4)];
        float tempo_drain = (team_tempo == TIER_HIGH) ? 1.15f : ((team_tempo == TIER_LOW) ? 0.88f : 1.00f);

        float stamina_attr = std::clamp(p.starting_stamina, 1.0f, 99.0f);
        float endurance_factor = 1.24f - (stamina_attr / 100.0f) * 0.44f;
        float sprint_mult = (p.sprint_boost_timer > 0.0f || p.dribble_boost_timer > 0.0f) ? 1.32f :
                            (is_recovering ? 1.16f : 1.0f);

        float speed_ratio = p.velocity.length() / std::max(1.0f, max_speed);
        float base_drain = 0.43f * press_mult * mentality_mult * tempo_drain * endurance_factor * sprint_mult;
        p.stamina = std::max(10.0f, p.stamina - (base_drain * (0.45f + speed_ratio) * dt));

        float cur_speed = p.velocity.length();
        float des_speed = desired_vel.length();
        float turn_alignment = 1.0f;
        if (cur_speed > 8.0f && des_speed > 8.0f) {
            turn_alignment = std::clamp((p.velocity / cur_speed).dot(desired_vel / des_speed), -1.0f, 1.0f);
        }
        float turn_penalty = (turn_alignment < 0.25f)
            ? ((is_user_defending_ctrl ? 0.82f : 0.68f) + (is_user_defending_ctrl ? 0.18f : 0.32f) * std::max(0.0f, turn_alignment + 0.5f))
            : 1.0f;

        float agility_factor = 0.85f + ((p.pace * 0.45f + p.dribbling * 0.55f) / 100.0f) * 0.45f;
        float accel_rate = ((is_user_ball_carrier || is_user_defending_ctrl) ? 12.2f : 8.2f) * agility_factor * fatigue_mult;
        float decel_rate = ((is_user_ball_carrier || is_user_defending_ctrl) ? 14.0f : 10.0f) * agility_factor * fatigue_mult;
        float rate = (des_speed >= cur_speed) ? (accel_rate * turn_penalty) : decel_rate;
        float steer_alpha = 1.0f - std::exp(-rate * dt);
        p.velocity = p.velocity.lerp(desired_vel, steer_alpha);
        if (des_speed < 1.0f && p.velocity.length() < 3.5f) {
            p.velocity = Vector2(0.0f, 0.0f);
        }
        if (p.is_sliding) {
            float slide_decay = std::clamp(p.slide_timer / 0.34f, 0.35f, 1.0f);
            desired_vel = p.slide_dir * (max_speed * slide_decay);
        }
        p.position += p.velocity * dt;
        p.position.x = std::clamp(p.position.x, margin, field_size.x - margin);
        p.position.y = std::clamp(p.position.y, margin, field_size.y - margin);
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

    if (chance_state == _user_defend_chance_state() && (passer.is_home != user_is_home)) {
        pass_speed *= 0.86f;
    }

    ball_target = target;
    ball_velocity = (delta / dist) * pass_speed;
    ball_state = BALL_IN_PASS;
    ball_carrier_index = -1;
    pass_target_index = p_receiver_idx;
    is_through_ball_in_flight = p_is_through_ball;
    _snapshot_pass_offside_flags(p_passer_idx);
    pending_pass_offside = _evaluate_pass_offside(p_passer_idx, p_receiver_idx);
    offside_spot = receiver.position;
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
    if (!match_active || waiting_for_kickoff_resume || chance_state != CHANCE_NONE ||
        sim_players.size() != 22 || p_attacker_idx < 0 || p_attacker_idx >= 22 ||
        ((p_attacker_idx < 11) != p_is_home_attacking)) {
        UtilityFunctions::printerr("MatchEngine: Rejected chance with an invalid transition, attacker index, or team side.");
        return;
    }

    // Commit the chance state before assigning control or emitting callbacks.
    // MatchPitch and MatchHUD derive the user's input side from this value.
    chance_state = p_is_home_attacking ? CHANCE_HOME_ATTACK : CHANCE_AWAY_ATTACK;
    manual_control_active = true;
    bool is_user_attacking = (p_is_home_attacking == user_is_home);
    active_chance_timer = is_user_attacking ? 14.0f : 16.5f;
    event_cooldown = 12.0f;
    pending_chance_xg = p_xg;
    has_user_move_target = false;

    if (is_user_attacking) {
        int default_atk = user_is_home ? 9 : 20;
        user_controlled_index = _is_user_outfield_idx(p_attacker_idx) ? p_attacker_idx : default_atk;
    } else {
        select_nearest_defender();
    }

    if (!_is_user_player_idx(user_controlled_index)) {
        UtilityFunctions::printerr("MatchEngine: Rejected chance because no managed-side player could be selected.");
        chance_state = CHANCE_NONE;
        manual_control_active = false;
        return;
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

    pending_chance_signal = true;
    pending_chance_state_val = chance_state;
    pending_chance_ctrl_val = user_controlled_index;
    pending_interactive_signal = true;
    pending_interactive_type = p_scenario_type;
    pending_interactive_home_atk = p_is_home_attacking;
    pending_interactive_ball_3d = last_scenario_3d.ball_pos_3d;
}

void MatchEngine::_evaluate_carrier_action(int p_carrier_idx) {
    if (sim_players.size() != 22 || p_carrier_idx < 0 || p_carrier_idx >= 22) {
        return;
    }
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
    // 0. LIVE-POSITION CHANCE TRIGGER
    if (chance_state == CHANCE_NONE && event_cooldown <= 0.0f && carrier.role != "GK") {
        bool in_attacking_half = is_home ? (carrier.position.x >= field_size.x * 0.48f)
                                         : (carrier.position.x <= field_size.x * 0.52f);
        if (in_attacking_half) {
            float team_ovr = static_cast<float>(is_home ? home_team_ovr : away_team_ovr);
            float opp_ovr = static_cast<float>(is_home ? away_team_ovr : home_team_ovr);
            float ovr_edge = std::clamp((team_ovr - opp_ovr) * 0.012f, -0.25f, 0.25f);
            float carrier_threat = ((carrier.dribbling * 0.40f + carrier.passing * 0.30f + carrier.shooting * 0.30f) - 65.0f) * 0.004f;
            float trigger_prob = std::clamp((0.45f + ovr_edge + carrier_threat) * chance_creation_mult, 0.20f, 0.92f);

            if ((is_home == user_is_home) || UtilityFunctions::randf() < trigger_prob) {
                _spawn_wide_range_chance(is_home);
                return;
            }
        }
    }

    // 1. SHOOTING EVALUATION
    if (((is_home != user_is_home) || home_auto_shoot_enabled) && carrier.role != "GK" && dist_to_goal < 320.0f && lateral_offset < 235.0f) {
        float dist_factor = std::clamp(1.0f - (dist_to_goal / 340.0f), 0.06f, 0.90f);
        float angle_factor = std::clamp(1.0f - (lateral_offset / 250.0f), 0.25f, 1.0f);
        float pressure_factor = std::clamp(nearest_opp_dist / 55.0f, 0.45f, 1.15f);
        float xg = std::clamp(dist_factor * angle_factor * pressure_factor * (0.45f + carrier.shooting * fatigue * 0.004f), 0.04f, 0.65f);

        if (chance_state == _user_defend_chance_state() && (is_home != user_is_home)) {
            float long_shot_bias = (dist_to_goal > 180.0f) ? 0.22f : 0.44f;
            if (dist_to_goal < 145.0f || UtilityFunctions::randf() < (long_shot_bias + xg)) {
                _execute_shot(p_carrier_idx, xg);
                return;
            }
        }
    }

    // 1B. DEFENSIVE BOX CLEARANCE EVALUATION (Role-coupled CB/FB/CDM safety valve)
    if (_is_backline_role(carrier.role) || carrier.role == "CDM") {
        float own_goal_x = is_home ? 0.0f : field_size.x;
        float dist_own_goal = std::abs(carrier.position.x - own_goal_x);
        if (dist_own_goal < 190.0f && nearest_opp_dist < 54.0f) {
            float danger = std::clamp((1.0f - dist_own_goal / 190.0f) * 0.65f + (1.0f - nearest_opp_dist / 54.0f) * 0.55f, 0.0f, 1.0f);
            if (_try_defensive_clearance(p_carrier_idx, danger)) {
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
    SimPlayer& defender = sim_players[nearest_opp_idx];
    float duel_range = 19.0f + ((defender.defending + defender.physical) * 0.035f);
    if (nearest_opp_dist < duel_range && carrier.role != "GK" && defender.recovery_timer <= 0.0f && defender.tackle_cooldown <= 0.0f) {
        defender.tackle_cooldown = 0.46f;
        float def_fatigue = _get_fatigue_multiplier(defender);
        int def_pressing = defender.is_home ? home_pressing : away_pressing;
        float press_tackle_bonus = (def_pressing == PRESSING_HIGH) ? 4.5f : ((def_pressing == PRESSING_LOW) ? -3.5f : 0.0f);

        float def_own_goal_x = defender.is_home ? 0.0f : field_size.x;
        bool in_box = std::abs(defender.position.x - def_own_goal_x) < 140.0f && std::abs(defender.position.y - field_size.y * 0.5f) < 168.0f;
        bool body_duel = (nearest_opp_dist <= 16.0f);

        float atk_core = body_duel
            ? (carrier.dribbling * 0.52f + carrier.physical * 0.48f)
            : (carrier.dribbling * 0.66f + carrier.pace * 0.34f);
        float def_core = body_duel
            ? (defender.defending * 0.50f + defender.physical * 0.50f)
            : (defender.defending * 0.72f + defender.physical * 0.16f + defender.pace * 0.12f);

        float skill_diff = (atk_core * fatigue) - (def_core * def_fatigue)
                         - press_tackle_bonus - _get_role_defensive_bonus(defender, in_box, false);
        float atk_win_prob = std::clamp(0.50f + 0.34f * std::tanh(skill_diff / 28.0f), 0.15f, 0.85f);

        if (UtilityFunctions::randf() < atk_win_prob) {
            defender.recovery_timer = 0.42f;
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

            if (((defender.is_home != user_is_home) && chance_state == _user_attack_chance_state()) ||
                ((defender.is_home == user_is_home) && chance_state == _user_defend_chance_state())) {
                _end_active_chance(false);
            }

            float retain_prob = std::clamp(0.54f + (defender.defending * 0.0022f) + (defender.physical * 0.0014f), 0.52f, 0.88f);
            if (UtilityFunctions::randf() < retain_prob) {
                possession_team = defender.is_home ? 0 : 1;
                ball_carrier_index = nearest_opp_idx;
                last_touch_index = nearest_opp_idx;
                pass_target_index = -1;
                is_through_ball_in_flight = false;
                ball_state = BALL_CARRIED;
                action_decision_timer = 0.32f;

                float dist_own_goal = std::abs(defender.position.x - def_own_goal_x);
                if ((_is_backline_role(defender.role) || defender.role == "CDM") && dist_own_goal < 150.0f) {
                    _try_defensive_clearance(nearest_opp_idx, 0.68f);
                }
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

    if (gk_rush_active) {
        gk_rush_timer -= dt;
        int user_team = user_is_home ? 0 : 1;
        bool ball_in_opp_half = user_is_home ? (ball_position.x > field_size.x * 0.56f)
                                             : (ball_position.x < field_size.x * 0.44f);
        if (gk_rush_timer <= 0.0f || possession_team == user_team || ball_in_opp_half) {
            cancel_gk_rush();
        } else if (ball_state != BALL_IN_SHOT && _try_gk_rush_intercept()) {
            return;
        }
    }

    if (manual_control_active && _is_user_outfield_idx(user_controlled_index)) {
        SimPlayer& ctrl_def = sim_players[user_controlled_index];
        if (ctrl_def.is_sliding && ctrl_def.position.distance_to(ball_position) <= 19.5f) {
            _resolve_slide_tackle_contact(user_controlled_index);
            return;
        }
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

        if (((carrier.is_home != user_is_home) && chance_state == _user_attack_chance_state()) ||
            ((carrier.is_home == user_is_home) && chance_state == _user_defend_chance_state())) {
            _end_active_chance(false);
        }

        int opp_start = carrier.is_home ? 11 : 0;
        int def_pressing = carrier.is_home ? away_pressing : home_pressing;
        float press_bonus = (def_pressing == PRESSING_HIGH) ? 4.5f : ((def_pressing == PRESSING_LOW) ? -3.5f : 0.0f);
        bool is_user_manual_dribble = ((carrier.is_home == user_is_home) && manual_control_active && ball_carrier_index == user_controlled_index);

        for (int j = opp_start; j < opp_start + 11; j++) {
            SimPlayer& defender = sim_players[j];
            if (defender.is_sent_off || defender.recovery_timer > 0.0f || defender.tackle_cooldown > 0.0f) {
                continue;
            }

            float dist_to_carrier = defender.position.distance_to(carrier.position);
            float dist_to_ball = defender.position.distance_to(ball_position);
            float engage_dist = std::min(dist_to_carrier, dist_to_ball);

            float reach_bonus = ((defender.defending + defender.physical) * 0.032f);
            float tackle_radius = (defender.role == "GK")
                ? 14.5f
                : (is_user_manual_dribble ? (14.5f + reach_bonus * 0.6f) : (16.5f + reach_bonus));

            if (engage_dist <= tackle_radius) {
                // Cooldown prevents 60Hz per-frame spam so a single challenge resolves cleanly
                defender.tackle_cooldown = is_user_manual_dribble ? 0.58f : 0.44f;

                float atk_fatigue = _get_fatigue_multiplier(carrier);
                float def_fatigue = _get_fatigue_multiplier(defender);
                float def_own_goal_x = defender.is_home ? 0.0f : field_size.x;
                bool in_box = std::abs(defender.position.x - def_own_goal_x) < 140.0f && std::abs(defender.position.y - field_size.y * 0.5f) < 168.0f;
                bool body_duel = (engage_dist <= 15.5f);

                // Off-balance penalty when attacker executes a sharp turn or active dribble burst
                float agility_evade = 0.0f;
                if (carrier.dribble_boost_timer > 0.0f) {
                    agility_evade += 7.5f;
                }
                if (defender.velocity.length() > 15.0f && carrier.velocity.length() > 15.0f) {
                    float align = defender.velocity.normalized().dot(carrier.velocity.normalized());
                    if (align < 0.25f) {
                        agility_evade += 6.0f;
                    }
                }

                float atk_core = body_duel
                    ? (carrier.dribbling * 0.52f + carrier.physical * 0.48f)
                    : (carrier.dribbling * 0.66f + carrier.pace * 0.34f);
                float def_core = body_duel
                    ? (defender.defending * 0.50f + defender.physical * 0.50f)
                    : (defender.defending * 0.72f + defender.physical * 0.16f + defender.pace * 0.12f);

                float skill_diff = (def_core * def_fatigue) - (atk_core * atk_fatigue)
                                 + press_bonus + _get_role_defensive_bonus(defender, in_box, false) - agility_evade;
                float base_center = is_user_manual_dribble ? 0.40f : 0.48f;
                float tackle_win_prob = std::clamp(base_center + 0.34f * std::tanh(skill_diff / 27.0f), 0.14f, 0.84f);

                if (UtilityFunctions::randf() < tackle_win_prob) {
                    if (defender.is_home) {
                        stats.home_tackles++;
                    } else {
                        stats.away_tackles++;
                    }
                    _log_event("tackle", defender.is_home, defender.name,
                        defender.name + String(" dispossesses ") + carrier.name + String(" with a clean tackle!"));

                    if (((defender.is_home != user_is_home) && chance_state == _user_attack_chance_state()) ||
                        ((defender.is_home == user_is_home) && chance_state == _user_defend_chance_state())) {
                        _end_active_chance(false);
                    }

                    float retain_prob = std::clamp(0.56f + (defender.defending * 0.0022f) + (defender.physical * 0.0014f), 0.54f, 0.90f);
                    if (UtilityFunctions::randf() < retain_prob) {
                        possession_team = defender.is_home ? 0 : 1;
                        ball_carrier_index = j;
                        last_touch_index = j;
                        pass_target_index = -1;
                        is_through_ball_in_flight = false;
                        ball_state = BALL_CARRIED;
                        ball_velocity = Vector2(0.0f, 0.0f);
                        action_decision_timer = 0.32f;

                        float dist_own_goal = std::abs(defender.position.x - def_own_goal_x);
                        if ((_is_backline_role(defender.role) || defender.role == "CDM") && dist_own_goal < 150.0f) {
                            _try_defensive_clearance(j, 0.68f);
                        }
                    } else {
                        possession_team = -1;
                        ball_carrier_index = -1;
                        pass_target_index = -1;
                        is_through_ball_in_flight = false;
                        ball_state = BALL_LOOSE;
                        Vector2 deflect_dir = (carrier.position - defender.position).normalized();
                        ball_velocity = Vector2(-deflect_dir.y, deflect_dir.x) * 135.0f;
                    }
                    return;
                } else {
                    float phy_recovery = std::clamp(0.68f - (defender.physical / 100.0f) * 0.22f, 0.44f, 0.66f);
                    defender.recovery_timer = is_user_manual_dribble ? (phy_recovery * 1.35f) : phy_recovery;
                    int def_mentality = defender.is_home ? home_mentality : away_mentality;
                    float tactic_agg = 0.72f + (static_cast<float>(def_mentality) * 0.10f) + (static_cast<float>(def_pressing) * 0.14f);
                    if (body_duel && defender.role != "GK" && UtilityFunctions::randf() < std::clamp(0.035f * tactic_agg, 0.015f, 0.09f)) {
                        int fouled_idx = ball_carrier_index;
                        Vector2 atk_face = (carrier.velocity.length() > 8.0f) ? carrier.velocity.normalized() : Vector2(carrier.is_home ? 1.0f : -1.0f, 0.0f);
                        float from_behind = (defender.velocity.length() > 8.0f) ? defender.velocity.normalized().dot(atk_face) : 0.0f;
                        _award_defensive_free_kick(carrier.is_home, carrier.position, defender.name);
                        _evaluate_foul_and_discipline(j, fouled_idx, 0.75f + (defender.physical / 240.0f), from_behind, false);
                        return;
                    }
                }
            }
        }

        action_decision_timer -= dt;
        if (action_decision_timer <= 0.0f) {
            float fatigue = _get_fatigue_multiplier(carrier);
            float base_timer = std::clamp(0.68f - (carrier.passing * fatigue * 0.003f), 0.28f, 0.72f);
            if (chance_state == _user_defend_chance_state() && (carrier.is_home != user_is_home)) {
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
        int passer_idx = std::clamp(last_touch_index, 0, 21);
        float passer_passing = static_cast<float>(sim_players[passer_idx].passing);
        int opp_start = passer_is_home ? 11 : 0;
        int def_pressing = passer_is_home ? away_pressing : home_pressing;
        bool is_user_attack_chance = ((passer_is_home == user_is_home) && chance_state == _user_attack_chance_state());

        float base_intercept_radius = (def_pressing == PRESSING_HIGH) ? 14.5f : ((def_pressing == PRESSING_LOW) ? 10.5f : 12.5f);
        float press_rate_bonus = (def_pressing == PRESSING_HIGH) ? 1.4f : ((def_pressing == PRESSING_LOW) ? -0.9f : 0.0f);
        float ball_speed = ball_velocity.length();
        float speed_evasion = std::clamp((ball_speed - 280.0f) / 180.0f, 0.0f, 1.0f);

        for (int j = opp_start; j < opp_start + 11; j++) {
            const SimPlayer& def = sim_players[j];
            if (def.is_sent_off || def.recovery_timer > 0.0f) continue;

            float def_fatigue = _get_fatigue_multiplier(def);
            float iq = _get_defensive_positioning_iq(def);
            // Defending + Pace + Awareness expand interception read & lunge radius
            float attr_radius_scale = 0.80f + ((def.defending * 0.65f + def.pace * 0.35f) / 100.0f) * 0.42f;
            float effective_radius = base_intercept_radius * attr_radius_scale * def_fatigue * (is_user_attack_chance ? 0.82f : 1.0f);

            if (def.position.distance_to(ball_position) < effective_radius) {
                int tm_start = passer_is_home ? 1 : 12;
                for (int k = tm_start; k < tm_start + 10; k++) {
                    if (pass_offside_flags[k] && !sim_players[k].is_sent_off &&
                        sim_players[k].position.distance_to(ball_position) < 16.0f) {
                        if (passer_is_home) stats.home_offsides++;
                        else stats.away_offsides++;
                        _log_event("offside", passer_is_home, sim_players[k].name,
                            String("OFFSIDE! ") + sim_players[k].name + String(" interferes with play from an offside position."));
                        _begin_restart(RESTART_FREE_KICK, !passer_is_home, sim_players[k].position);
                        return;
                    }
                }
                float role_read = (def.role == "CDM") ? 2.2f : ((def.role == "CB") ? 1.5f : (_is_backline_role(def.role) ? 1.1f : -1.2f));
                float skill_edge = ((def.defending * def_fatigue) - (passer_passing * 0.65f)) * 0.08f;
                // Continuous hazard rate (per-second) so 60Hz tick rate doesn't inflate interception frequency
                float hazard_rate = std::clamp(
                    (5.8f + skill_edge + role_read + press_rate_bonus + (iq - 1.0f) * 3.0f) * (1.0f - speed_evasion * 0.35f),
                    1.2f, 14.5f
                );
                if (is_user_attack_chance) {
                    hazard_rate *= 0.64f;
                }

                float frame_prob = 1.0f - std::exp(-hazard_rate * dt);
                if (UtilityFunctions::randf() < frame_prob) {
                    if (def.is_home) {
                        stats.home_interceptions++;
                    } else {
                        stats.away_interceptions++;
                    }
                    possession_team = def.is_home ? 0 : 1;
                    ball_carrier_index = j;
                    last_touch_index = j;
                    pass_target_index = -1;
                    is_through_ball_in_flight = false;
                    pending_pass_offside = false;
                    for (int k = 0; k < 22; k++) pass_offside_flags[k] = false;
                    ball_state = BALL_CARRIED;
                    ball_velocity = Vector2(0.0f, 0.0f);
                    action_decision_timer = 0.32f;
                    _log_event("interception", def.is_home, def.name,
                        def.name + String(" intercepts the pass!"));

                    if (((def.is_home != user_is_home) && chance_state == _user_attack_chance_state()) ||
                        ((def.is_home == user_is_home) && chance_state == _user_defend_chance_state())) {
                        _end_active_chance(false);
                    }

                    float own_goal_x = def.is_home ? 0.0f : field_size.x;
                    float dist_own_goal = std::abs(def.position.x - own_goal_x);
                    if ((_is_backline_role(def.role) || def.role == "CDM") && dist_own_goal < 145.0f) {
                        _try_defensive_clearance(j, 0.62f);
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
                if (pending_pass_offside) {
                    pending_pass_offside = false;
                    if (passer_is_home) stats.home_offsides++;
                    else stats.away_offsides++;
                    _log_event("offside", passer_is_home, sim_players[pass_target_index].name,
                        String("OFFSIDE! ") + sim_players[pass_target_index].name + String(" is caught offside — indirect free kick."));
                    _begin_restart(RESTART_FREE_KICK, !passer_is_home, offside_spot);
                    return;
                }
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
                action_decision_timer = (chance_state == _user_defend_chance_state() && (rec_is_home != user_is_home)) ? 0.45f : 0.30f;

                if (chance_state == CHANCE_NONE && event_cooldown <= 0.0f && sim_players[receiver_idx].role != "GK") {
                    bool in_atk_half = rec_is_home ? (sim_players[receiver_idx].position.x >= field_size.x * 0.48f)
                                                   : (sim_players[receiver_idx].position.x <= field_size.x * 0.52f);
                    if (in_atk_half && ((rec_is_home == user_is_home) || UtilityFunctions::randf() < 0.45f)) {
                        _spawn_wide_range_chance(rec_is_home);
                    }
                }
                return;
            }
        }

        if (ball_position.y <= 10.0f || ball_position.y >= field_size.y - 10.0f) {
            bool last_home = (last_touch_index >= 0 && last_touch_index < 11);
            Vector2 spot(std::clamp(ball_position.x, 24.0f, field_size.x - 24.0f), (ball_position.y <= 10.0f) ? 14.0f : (field_size.y - 14.0f));
            _begin_restart(RESTART_THROW_IN, !last_home, spot);
            return;
        }
        if (ball_position.x <= 10.0f || ball_position.x >= field_size.x - 10.0f) {
            bool left_goal_line = (ball_position.x <= 10.0f);
            bool defending_is_home = left_goal_line;
            bool last_home = (last_touch_index >= 0 && last_touch_index < 11);
            if (last_home == defending_is_home) {
                if (!defending_is_home) stats.home_corners++;
                else stats.away_corners++;
                Vector2 corner_spot(left_goal_line ? 16.0f : (field_size.x - 16.0f), (ball_position.y < field_size.y * 0.5f) ? 16.0f : (field_size.y - 16.0f));
                _begin_restart(RESTART_CORNER, !defending_is_home, corner_spot);
            } else {
                Vector2 gk_spot(defending_is_home ? 48.0f : (field_size.x - 48.0f), field_size.y * 0.5f);
                _begin_restart(RESTART_GOAL_KICK, defending_is_home, gk_spot);
            }
            return;
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
        bool is_user_attack_chance = ((shooter_is_home == user_is_home) && chance_state == _user_attack_chance_state());
        for (int j = opp_start + 1; j < opp_start + 11; j++) {
            const SimPlayer& def = sim_players[j];
            if (def.recovery_timer > 0.0f) continue;

            float def_fatigue = _get_fatigue_multiplier(def);
            // Physical + Defending expand body-block reach
            float block_radius = (10.5f + ((def.defending * 0.55f + def.physical * 0.45f) / 100.0f) * 4.5f) * (is_user_attack_chance ? 0.85f : 1.0f);
            float d_ball = def.position.distance_to(ball_position);
            float d_shooter = def.position.distance_to(shooter.position);

            if (d_ball < block_radius && d_shooter > 14.0f) {
                float role_block = (def.role == "CB") ? 3.2f : ((def.role == "CDM") ? 2.2f : (_is_backline_role(def.role) ? 1.5f : -1.5f));
                float def_block_skill = (def.defending * 0.65f + def.physical * 0.35f) * def_fatigue;
                float hazard_rate = std::clamp(7.5f + ((def_block_skill - 65.0f) * 0.14f) + role_block, 2.0f, 18.0f);
                if (is_user_attack_chance) {
                    hazard_rate *= 0.62f;
                }

                float frame_prob = 1.0f - std::exp(-hazard_rate * dt);
                if (UtilityFunctions::randf() < frame_prob) {
                    if (def.is_home) {
                        stats.home_tackles++;
                    } else {
                        stats.away_tackles++;
                    }
                    _log_event("shot_blocked", def.is_home, def.name,
                        def.name + String(" throws in a vital block!"));

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
        bool in_goal_corridor = std::abs(ball_position.y - (field_size.y * 0.5f)) <= 48.0f;
        bool near_gk = !shot_hits_crossbar && in_goal_corridor && (ball_position.distance_to(gk.position) < 16.0f);

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
                if (near_gk) {
                    if (shooter_is_home) stats.home_corners++;
                    else stats.away_corners++;
                    Vector2 corner_spot(shooter_is_home ? (field_size.x - 16.0f) : 16.0f, (ball_position.y < field_size.y * 0.5f) ? 16.0f : (field_size.y - 16.0f));
                    _begin_restart(RESTART_CORNER, shooter_is_home, corner_spot);
                } else {
                    Vector2 gk_spot(!shooter_is_home ? 48.0f : (field_size.x - 48.0f), field_size.y * 0.5f);
                    _begin_restart(RESTART_GOAL_KICK, !shooter_is_home, gk_spot);
                }
                return;
            }

            if (shooter_is_home) {
                stats.home_shots_on_target++;
            } else {
                stats.away_shots_on_target++;
            }

            bool can_score_now = match_active && (
                ((shooter_is_home == user_is_home) && chance_state == _user_attack_chance_state()) ||
                ((shooter_is_home != user_is_home) && chance_state == _user_defend_chance_state())
            );

            float gk_fatigue = _get_fatigue_multiplier(gk);
            float shooter_fatigue = _get_fatigue_multiplier(shooter);
            float gk_reach_offset = std::abs(ball_position.y - gk.position.y);
            float corner_penalty = std::clamp(gk_reach_offset / 44.0f, 0.0f, 1.0f) * 0.22f;
            float power_penalty = std::clamp((last_shot_power_ratio - 0.65f) * 0.16f, 0.0f, 0.11f);

            Vector2 def_goal_center(shooter_is_home ? field_size.x : 0.0f, field_size.y * 0.5f);
            float gk_off_line_dist = std::max(0.0f, gk.position.distance_to(def_goal_center) - 36.0f);
            float exposed_goal_penalty = std::clamp(gk_off_line_dist / 150.0f, 0.0f, 0.32f);

            float raw_save_prob = 0.34f
                + (((gk.defending * gk_fatigue) - (shooter.shooting * shooter_fatigue)) * 0.0032f)
                - corner_penalty
                - power_penalty
                - exposed_goal_penalty;

            float save_prob = is_user_attack_chance
                ? std::clamp(raw_save_prob * 0.82f, 0.06f, 0.42f)
                : std::clamp(raw_save_prob, 0.04f, 0.52f);
            
            if (gk_rush_active && !shooter_is_home == user_is_home) {
                cancel_gk_rush();
            }

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

    // STATE 4: Loose Ball Recovery (Physical + Pace contest when two players reach a loose ball simultaneously)
    ball_position += ball_velocity * dt;
    ball_velocity *= std::exp(-2.8f * dt);
    if (ball_position.y <= 10.0f || ball_position.y >= field_size.y - 10.0f) {
        bool last_home = (last_touch_index >= 0 && last_touch_index < 11);
        Vector2 spot(std::clamp(ball_position.x, 24.0f, field_size.x - 24.0f), (ball_position.y <= 10.0f) ? 14.0f : (field_size.y - 14.0f));
        _begin_restart(RESTART_THROW_IN, !last_home, spot);
        return;
    }
    if (ball_position.x <= 10.0f || ball_position.x >= field_size.x - 10.0f) {
        bool left_goal_line = (ball_position.x <= 10.0f);
        bool defending_is_home = left_goal_line;
        bool last_home = (last_touch_index >= 0 && last_touch_index < 11);
        if (last_home == defending_is_home) {
            if (!defending_is_home) stats.home_corners++;
            else stats.away_corners++;
            Vector2 corner_spot(left_goal_line ? 16.0f : (field_size.x - 16.0f), (ball_position.y < field_size.y * 0.5f) ? 16.0f : (field_size.y - 16.0f));
            _begin_restart(RESTART_CORNER, !defending_is_home, corner_spot);
        } else {
            Vector2 gk_spot(defending_is_home ? 48.0f : (field_size.x - 48.0f), field_size.y * 0.5f);
            _begin_restart(RESTART_GOAL_KICK, defending_is_home, gk_spot);
        }
        return;
    }
    ball_position.x = std::clamp(ball_position.x, 12.0f, field_size.x - 12.0f);
    ball_position.y = std::clamp(ball_position.y, 12.0f, field_size.y - 12.0f);

    int closest_idx = -1;
    float best_contest_score = -1e9f;
    for (int i = 0; i < 22; i++) {
        if (sim_players[i].is_sent_off || sim_players[i].recovery_timer > 0.0f) continue;
        float d = sim_players[i].position.distance_to(ball_position);
        if (d < 16.5f) {
            float contest_score = (16.5f - d) * 2.2f + (sim_players[i].physical * 0.14f) + (sim_players[i].pace * 0.08f);
            if (contest_score > best_contest_score) {
                best_contest_score = contest_score;
                closest_idx = i;
            }
        }
    }

    if (closest_idx != -1) {
        const SimPlayer& winner = sim_players[closest_idx];
        float own_goal_x = winner.is_home ? 0.0f : field_size.x;
        float dist_own_goal = std::abs(winner.position.x - own_goal_x);
        if ((_is_backline_role(winner.role) || winner.role == "CDM") && dist_own_goal < 175.0f) {
            float danger = std::clamp(1.0f - (dist_own_goal / 175.0f), 0.25f, 1.0f);
            if (_try_defensive_clearance(closest_idx, danger)) {
                return;
            }
        }
        possession_team = winner.is_home ? 0 : 1;
        ball_carrier_index = closest_idx;
        last_touch_index = closest_idx;
        ball_state = BALL_CARRIED;
        ball_velocity = Vector2(0.0f, 0.0f);
        action_decision_timer = 0.30f;
    }
}

bool MatchEngine::command_gk_rush() {
    if (!match_active || sim_players.size() != 22 || waiting_for_kickoff_resume) {
        return false;
    }
    int user_team = user_is_home ? 0 : 1;
    if (possession_team == user_team && ball_state == BALL_CARRIED) {
        return false;
    }

    int gk_idx = _user_gk_idx();
    if (gk_idx < 0 || gk_idx >= 22) {
        return false;
    }

    gk_rush_active = true;
    gk_rush_timer = 3.2f;
    gk_rush_target = ball_position + (ball_velocity * 0.16f);
    is_paused_for_manual = false;

    _log_event("gk_rush", user_is_home, sim_players[gk_idx].name,
        sim_players[gk_idx].name + String(" rushes off the line!"));
    emit_signal("gk_rush_changed", true, gk_idx);
    return true;
}

void MatchEngine::cancel_gk_rush() {
    if (!gk_rush_active) return;
    gk_rush_active = false;
    gk_rush_timer = 0.0f;
    emit_signal("gk_rush_changed", false, _user_gk_idx());
}

void MatchEngine::_execute_gk_rush_clearance(int p_gk_idx) {
    SimPlayer& gk = sim_players[p_gk_idx];
    float clear_dir = gk.is_home ? 1.0f : -1.0f;
    float center_y = field_size.y * 0.5f;
    float flank_sign = (gk.position.y >= center_y) ? 1.0f : -1.0f;
    float kick_power = std::clamp(290.0f + gk.defending * 1.65f, 280.0f, 460.0f);

    Vector2 target(
        std::clamp(gk.position.x + clear_dir * 260.0f, 32.0f, field_size.x - 32.0f),
        std::clamp(gk.position.y + flank_sign * 165.0f, 22.0f, field_size.y - 22.0f)
    );
    Vector2 dir = (target - ball_position).normalized();

    ball_position = gk.position + Vector2(clear_dir * 10.0f, 0.0f);
    ball_target = target;
    ball_velocity = dir * kick_power;
    ball_state = BALL_LOOSE;
    possession_team = -1;
    ball_carrier_index = -1;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    last_touch_index = p_gk_idx;

    cancel_gk_rush();
    _log_event("gk_rush_clear", gk.is_home, gk.name,
        gk.name + String(" sweeps outside the area and clears into touch!"));
    if (chance_state == _user_defend_chance_state()) {
        _end_active_chance(false);
    }
}

bool MatchEngine::_try_gk_rush_intercept() {
    if (!gk_rush_active) return false;
    int gk_idx = _user_gk_idx();
    if (gk_idx < 0 || gk_idx >= 22) return false;

    SimPlayer& gk = sim_players[gk_idx];
    float dist_to_ball = gk.position.distance_to(ball_position);
    if (dist_to_ball > 19.5f) {
        return false;
    }

    float own_goal_x = gk.is_home ? 0.0f : field_size.x;
    float dist_from_goal = std::abs(gk.position.x - own_goal_x);
    bool inside_box = (dist_from_goal <= 145.0f && std::abs(gk.position.y - field_size.y * 0.5f) <= 168.0f);

    int opp_start = gk.is_home ? 11 : 0;
    float nearest_opp_dist = 1e9f;
    int nearest_opp_idx = -1;
    for (int j = opp_start; j < opp_start + 11; j++) {
        float d = sim_players[j].position.distance_to(ball_position);
        if (d < nearest_opp_dist) {
            nearest_opp_dist = d;
            nearest_opp_idx = j;
        }
    }

    float gk_fatigue = _get_fatigue_multiplier(gk);
    float gk_sweep_score = (gk.defending * 0.45f + gk.physical * 0.30f + gk.pace * 0.25f) * gk_fatigue;
    float opp_race_score = (nearest_opp_idx != -1)
        ? ((sim_players[nearest_opp_idx].dribbling * 0.50f + sim_players[nearest_opp_idx].pace * 0.50f) * _get_fatigue_multiplier(sim_players[nearest_opp_idx]))
        : 55.0f;

    // If an opponent attacker is right on the ball and wins the 1v1 race, GK is beaten
    if (nearest_opp_dist < dist_to_ball + 2.0f) {
        float win_race_prob = std::clamp(0.48f + (gk_sweep_score - opp_race_score) * 0.008f, 0.18f, 0.82f);
        if (UtilityFunctions::randf() >= win_race_prob) {
            gk.recovery_timer = 0.65f;
            cancel_gk_rush();
            _log_event("gk_rush_beaten", gk.is_home, gk.name,
                gk.name + String(" rushes out late and is rounded — goal exposed!"));
            return false;
        }
    }

    bool can_safely_catch = inside_box && (nearest_opp_dist > 24.0f) && (ball_state != BALL_IN_SHOT);
    if (can_safely_catch) {
        possession_team = gk.is_home ? 0 : 1;
        ball_carrier_index = gk_idx;
        last_touch_index = gk_idx;
        pass_target_index = -1;
        is_through_ball_in_flight = false;
        ball_state = BALL_CARRIED;
        ball_position = gk.position + Vector2(gk.is_home ? 8.0f : -8.0f, 0.0f);
        ball_velocity = Vector2(0.0f, 0.0f);
        action_decision_timer = 0.60f;
        cancel_gk_rush();
        _log_event("gk_rush_catch", gk.is_home, gk.name,
            gk.name + String(" charges out and smothers the ball!"));
        if (chance_state == _user_defend_chance_state()) {
            _end_active_chance(false);
        }
        return true;
    }

    _execute_gk_rush_clearance(gk_idx);
    return true;
}

void MatchEngine::_step_simulation(float dt) {
    // All simulation helpers use fixed HOME [0..10] / AWAY [11..21] slots.
    // Stop safely rather than indexing a partially initialized squad.
    if (!_has_valid_simulation_state()) {
        UtilityFunctions::printerr("MatchEngine: Simulation stopped because match state contains invalid player indices.");
        match_active = false;
        is_paused_for_manual = true;
        set_physics_process(false);
        return;
    }

    if (chance_state != CHANCE_NONE) {
        active_chance_timer -= dt;
        if (active_chance_timer <= 0.0f) {
            _log_event("chance_expired", chance_state == _user_attack_chance_state() ? user_is_home : !user_is_home, "Referee", "The attacking opportunity fizzles out.");
            _end_active_chance(false);
        }
    }

    _update_player_movement(dt);
    _update_ball_and_actions(dt);

    // Final safety boundary for every simulation path (including loose-ball rebounds).
    const Vector2 player_min(14.0f, 14.0f);
    const Vector2 player_max(field_size.x - 14.0f, field_size.y - 14.0f);
    for (SimPlayer& player : sim_players) {
        if (player.is_sent_off) continue;
        player.position.x = std::clamp(player.position.x, player_min.x, player_max.x);
        player.position.y = std::clamp(player.position.y, player_min.y, player_max.y);
        player.anchor.x = std::clamp(player.anchor.x, player_min.x, player_max.x);
        player.anchor.y = std::clamp(player.anchor.y, player_min.y, player_max.y);
    }
    ball_position.x = std::clamp(ball_position.x, 10.0f, field_size.x - 10.0f);
    ball_position.y = std::clamp(ball_position.y, 10.0f, field_size.y - 10.0f);
    _sync_player_positions_array();
}

void MatchEngine::_flush_deferred_signals() {
    if (pending_chance_signal) {
        pending_chance_signal = false;
        emit_signal("chance_state_changed", pending_chance_state_val, pending_chance_ctrl_val);
    }
    if (pending_chance_end_signal) {
        pending_chance_end_signal = false;
        emit_signal("chance_state_changed", CHANCE_NONE, -1);
    }
    if (pending_interactive_signal) {
        pending_interactive_signal = false;
        emit_signal("interactive_event_triggered", pending_interactive_type, pending_interactive_home_atk, pending_interactive_ball_3d);
    }
    if (pending_match_ended_signal) {
        pending_match_ended_signal = false;
        emit_signal("match_ended", home_score, away_score);
    }
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

    if (restart_state != RESTART_NONE) {
        restart_timer -= dt;
        _sync_player_positions_array();
        if (restart_timer <= 0.0f) {
            _complete_restart();
        }
        _flush_deferred_signals();
        return;
    }

    if (chance_state == _user_defend_chance_state()) {
        dt *= defensive_chance_time_scale;
    }

    match_timer += dt;
    int new_minute = std::min(90, static_cast<int>(match_timer / 1.0f));

    if (new_minute != current_minute) {
        current_minute = new_minute;
        emit_signal("minute_changed", current_minute);
        if (current_minute >= 45 && !halftime_triggered) {
            halftime_triggered = true;
            _trigger_halftime();
            _flush_deferred_signals();
            return;
        }
    }

    

    if (current_minute >= 90 && chance_state != CHANCE_NONE) {
        // Let the active 2D attacking/defending sequence reach its normal
        // resolution path before completing the match.
        _step_simulation(dt);
        _flush_deferred_signals();
        return;
    }

    if (current_minute >= 90) {
        _end_active_chance(false);
        waiting_for_kickoff_resume = false;
        kickoff_team = -1;
        match_active = false;
        is_paused_for_manual = false;
        set_physics_process(false);
        record_manual_match_completed();
        pending_match_ended_signal = true;
        _flush_deferred_signals();
        return;
    }

    if (event_cooldown > 0.0f) {
        event_cooldown = std::max(0.0f, event_cooldown - dt);
    }

    _step_simulation(dt);
    _flush_deferred_signals();
}

void MatchEngine::reset_standalone_match_setup() {
    if (match_active) {
        return;
    }
    match_active = false;
    is_paused_for_manual = false;
    waiting_for_kickoff_resume = false;
    kickoff_team = -1;
    chance_state = CHANCE_NONE;
    pending_chance_signal = false;
    pending_chance_end_signal = false;
    pending_interactive_signal = false;
    pending_match_ended_signal = false;
    user_controlled_index = 9;
    user_is_home = true;
    home_club_id = -1;
    away_club_id = -1;
    home_club_name = "HOME FC";
    away_club_name = "AWAY FC";
    home_kit_color = Color(0.12f, 0.23f, 0.54f);
    away_kit_color = Color(0.86f, 0.15f, 0.15f);
    home_team_ovr = 75;
    away_team_ovr = 72;
    home_formation = "4-3-3";
    away_formation = "4-4-2";
    _initialize_default_squads();
    _initialize_default_home_bench();
    _initialize_default_away_bench();
    _apply_formation_positions(true, true);
    _sync_player_positions_array();
    emit_signal("squad_updated");
}

void MatchEngine::start_match() {
    gk_rush_active = false;
    gk_rush_timer = 0.0f;
    restart_state = RESTART_NONE;
    restart_timer = 0.0f;
    restart_taker_idx = -1;
    pending_pass_offside = false;
    for (int i = 0; i < 22; i++) pass_offside_flags[i] = false;
    match_injuries.clear();
    for (SimPlayer& p : sim_players) {
        p.yellow_cards = 0;
        p.is_sent_off = false;
    }
    if (sim_players.size() != 22) {
        UtilityFunctions::printerr("MatchEngine: Cannot start match; expected 22 initialized players.");
        match_active = false;
        set_physics_process(false);
        return;
    }
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
    user_controlled_index = user_is_home ? 9 : 20;
    pending_chance_signal = false;
    pending_chance_end_signal = false;
    pending_interactive_signal = false;
    pending_match_ended_signal = false;
    pending_chance_state_val = CHANCE_NONE;
    pending_chance_ctrl_val = user_controlled_index;
    pending_interactive_type = 0;
    pending_interactive_home_atk = true;
    pending_interactive_ball_3d = Vector3();
    has_user_move_target = false;
    user_move_target = Vector2(-1.0f, -1.0f);
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    active_chance_timer = 0.0f;

    halftime_triggered = false;
    home_subs_used = 0;
    away_subs_used = 0;
    home_sub_windows_used = 0;
    away_sub_windows_used = 0;
    last_home_sub_minute = -999;
    last_away_sub_minute = -999;
    last_home_sub_was_halftime = false;
    last_away_sub_was_halftime = false;
    home_subbed_off_records.clear();
    away_subbed_off_records.clear();
    home_subbed_on_ids.clear();
    away_subbed_on_ids.clear();
    for (auto& b : home_bench) b.used = false;
    for (auto& b : away_bench) b.used = false;

    stats = MatchStats();

    for (auto& p : sim_players) {
        p.stamina = std::clamp(p.starting_stamina, 1.0f, 99.0f);
        p.recovery_timer = 0.0f;
        p.tackle_cooldown = 0.0f;
        p.sprint_boost_timer = 0.0f;
        p.dribble_boost_timer = 0.0f;
        p.dribble_carry_distance = 0.0f;
        p.is_sliding = false;
        p.slide_timer = 0.0f;
        p.slide_dir = Vector2(0.0f, 0.0f);
    }

    _reset_field_positions();
    if (!_has_valid_simulation_state()) {
        UtilityFunctions::printerr("MatchEngine: Cannot start match; kickoff did not produce a valid simulation state.");
        match_active = false;
        set_physics_process(false);
        return;
    }
    set_physics_process(true);
}

void MatchEngine::pause_match(bool p_pause) {
    if (!match_active) {
        return;
    }
    is_paused_for_manual = p_pause;
}

void MatchEngine::resolve_manual_scenario(bool p_success, bool p_is_home_team) {
    if (!match_active || sim_players.size() != 22) {
        UtilityFunctions::printerr("MatchEngine: Ignored resolve_manual_scenario() while match is inactive.");
        return;
    }

    bool valid_attack_chance = ((p_is_home_team == user_is_home) && chance_state == _user_attack_chance_state()) ||
                               ((p_is_home_team != user_is_home) && chance_state == _user_defend_chance_state());

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

void MatchEngine::_trigger_halftime() {
    gk_rush_active = false;
    gk_rush_timer = 0.0f;
    _end_active_chance(false);

    // +20% stamina recovery at half-time (capped at player's starting match condition)
    for (auto& p : sim_players) {
        float max_cap = std::clamp(p.starting_stamina, 10.0f, 100.0f);
        p.stamina = std::min(max_cap, p.stamina + 20.0f);
        p.recovery_timer = 0.0f;
        p.tackle_cooldown = 0.0f;
        p.sprint_boost_timer = 0.0f;
        p.dribble_boost_timer = 0.0f;
        p.dribble_carry_distance = 0.0f;
        p.is_sliding = false;
        p.slide_timer = 0.0f;
        p.slide_dir = Vector2(0.0f, 0.0f);
        p.velocity = Vector2(0.0f, 0.0f);
    }

    ball_velocity = Vector2(0.0f, 0.0f);
    ball_target = field_size * 0.5f;
    pass_target_index = -1;
    is_through_ball_in_flight = false;
    shot_hits_crossbar = false;
    event_cooldown = 5.0f;

    // Snap all 22 players to formation halves & place ball + taker on the middle line
    // false = Away kicks off 2nd half (Home kicked off 1st half); use true if you want Home kickoff
    bool second_half_home_kickoff = false;
    _apply_formation_positions(false, true);
    _give_kickoff(second_half_home_kickoff);
    _sync_player_positions_array();

    is_paused_for_manual = false;
    waiting_for_kickoff_resume = true;
    kickoff_team = second_half_home_kickoff ? 0 : 1;

    _log_event("halftime", true, "Referee", "Half-time! Players recover stamina and line up for the second half.");
    emit_signal("chance_state_changed", chance_state, -1);
    emit_signal("goal_kickoff_ready", kickoff_team, String("HALF TIME (45') · CLICK TO RESUME"));
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

    Array home_end_stamina;
    Array away_end_stamina;
    Array all_end_stamina;

    for (int i = 0; i < 22 && i < static_cast<int>(sim_players.size()); i++) {
        int end_stam = static_cast<int>(std::round(std::clamp(sim_players[i].stamina, 1.0f, 99.0f)));
        all_end_stamina.append(end_stam);
        if (i < 11) {
            home_end_stamina.append(end_stam);
        } else {
            away_end_stamina.append(end_stam);
        }
    }

    d["user_is_home"] = user_is_home;
    d["home_club_name"] = home_club_name;
    d["away_club_name"] = away_club_name;
    d["user_subs_used"] = user_is_home ? home_subs_used : away_subs_used;
    d["user_sub_windows_used"] = user_is_home ? home_sub_windows_used : away_sub_windows_used;
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
    d["home_clearances"] = stats.home_clearances;
    d["away_clearances"] = stats.away_clearances;
    d["home_corners"] = stats.home_corners;
    d["away_corners"] = stats.away_corners;
    d["home_fouls"] = stats.home_fouls;
    d["away_fouls"] = stats.away_fouls;
    d["home_offsides"] = stats.home_offsides;
    d["away_offsides"] = stats.away_offsides;
    d["home_yellow_cards"] = stats.home_yellow_cards;
    d["away_yellow_cards"] = stats.away_yellow_cards;
    d["home_red_cards"] = stats.home_red_cards;
    d["away_red_cards"] = stats.away_red_cards;
    d["match_injuries"] = match_injuries;
    d["home_subs_used"] = home_subs_used;
    d["away_subs_used"] = away_subs_used;
    d["home_sub_windows_used"] = home_sub_windows_used;
    d["away_sub_windows_used"] = away_sub_windows_used;
    d["home_subbed_off_records"] = home_subbed_off_records;
    d["away_subbed_off_records"] = away_subbed_off_records;
    d["home_subbed_on_ids"] = home_subbed_on_ids;
    d["away_subbed_on_ids"] = away_subbed_on_ids;
    d["home_player_stamina"] = home_end_stamina;
    d["away_player_stamina"] = away_end_stamina;
    d["player_stamina"] = all_end_stamina;
    return d;
    

}
