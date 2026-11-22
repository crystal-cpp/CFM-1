#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include <godot_cpp/variant/color.hpp>


namespace godot {

struct TacticalPosition {
    String role;
    float norm_x = 0.0f;
    float norm_y = 0.0f;
};

struct Scenario3DData {
    Vector3 ball_pos_3d;
    Vector3 attacker_pos_3d;
    Array defender_positions_3d;
    Array teammate_positions_3d;
};

struct SimPlayer {
    String name;
    String player_id = "";
    String role = "CM";
    bool is_home = true;
    int slot_index = 0;

    int ovr = 70;
    int pace = 70;
    int shooting = 70;
    int passing = 70;
    int dribbling = 70;
    int defending = 70;
    int physical = 70;
    float starting_stamina = 100.0f;
    float stamina = 100.0f;
    float recovery_timer = 0.0f;
    float sprint_boost_timer = 0.0f;
    float dribble_boost_timer = 0.0f;
    float tackle_cooldown = 0.0f;

    Vector2 anchor;
    Vector2 position;
    Vector2 velocity;
};

struct BenchPlayer {
    String name;
    String player_id = "";
    String role = "CM";
    int ovr = 70;
    int pace = 70;
    int shooting = 70;
    int passing = 70;
    int dribbling = 70;
    int defending = 70;
    int physical = 70;
    float stamina = 100.0f;
    bool used = false;
};

struct MatchStats {
    float home_possession_time = 0.0f;
    float away_possession_time = 0.0f;
    int home_shots = 0;
    int away_shots = 0;
    int home_shots_on_target = 0;
    int away_shots_on_target = 0;
    float home_xg = 0.0f;
    float away_xg = 0.0f;
    int home_passes = 0;
    int away_passes = 0;
    int home_passes_completed = 0;
    int away_passes_completed = 0;
    int home_tackles = 0;
    int away_tackles = 0;
    int home_interceptions = 0;
    int away_interceptions = 0;
    int home_clearances = 0;
    int away_clearances = 0;
    int home_saves = 0;
    int away_saves = 0;
    int home_corners = 0;
    int away_corners = 0;
};

class MatchEngine : public Node2D {
    GDCLASS(MatchEngine, Node2D)

public:
    static constexpr const char* PRIMARY_FORMATIONS_PATH = "res://data/formations.csv";
    static constexpr const char* FALLBACK_FORMATIONS_PATH = "res://../Players_Database/formations.csv";

    bool _is_backline_role(const String& p_role) const;
    float _get_defender_awareness_factor(const SimPlayer& p_defender) const;
    float _get_defensive_positioning_iq(const SimPlayer& p_defender) const;
    float _get_role_defensive_bonus(const SimPlayer& p_defender, bool p_in_box, bool p_is_aerial_or_block) const;
    bool _is_player_recovering_position(const SimPlayer& p_player, float p_team_backline_x) const;

    enum ChanceState {
        CHANCE_NONE = 0,
        CHANCE_HOME_ATTACK = 1,
        CHANCE_AWAY_ATTACK = 2
    };

    enum Mentality {
        MENTALITY_VERY_DEFENSIVE = 0,
        MENTALITY_DEFENSIVE = 1,
        MENTALITY_BALANCED = 2,
        MENTALITY_ATTACKING = 3,
        MENTALITY_VERY_ATTACKING = 4
    };

    enum PressingIntensity {
        PRESSING_LOW = 0,
        PRESSING_NORMAL = 1,
        PRESSING_HIGH = 2
    };

    enum TacticalTier {
        TIER_LOW = 0,
        TIER_NORMAL = 1,
        TIER_HIGH = 2
    };

    MatchEngine();
    ~MatchEngine();

    void _ready() override;
    void _physics_process(double delta) override;

    // Match Flow Controls
    void start_match();
    void reset_standalone_match_setup();
    void pause_match(bool p_pause);
    void resolve_manual_scenario(bool p_success, bool p_is_home_team = true);
    bool begin_chance(bool p_is_home_attacking, int p_attacker_idx = -1, float p_xg = 0.25f);

    // Post-Goal Kickoff Resume API
    bool is_waiting_for_kickoff_resume() const { return waiting_for_kickoff_resume; }
    int get_kickoff_team() const { return kickoff_team; }
    void resume_from_goal_kickoff();

    // Direct Player Control & Defensive Chance Pacing/Camera API
    int get_chance_state() const { return chance_state; }
    bool is_chance_active() const { return chance_state != CHANCE_NONE; }
    bool is_manual_control_active() const { return manual_control_active; }
    bool is_manual_dribble_active() const;
    int get_controlled_player_index() const { return user_controlled_index; }
    bool set_controlled_player(int p_player_idx);
    int select_nearest_defender();
    Array get_valid_pass_targets() const;

    void set_defensive_chance_time_scale(float p_scale);
    float get_defensive_chance_time_scale() const { return defensive_chance_time_scale; }
    float get_recommended_camera_zoom() const;
    Vector2 get_chance_camera_focus() const;
    Rect2 get_chance_framing_rect() const;

    bool command_move_player(Vector2 p_target_pos);
    bool command_pass(int p_target_player_idx);
    bool command_through_ball(Vector2 p_direction_or_target);
    bool command_shot(Vector2 p_aim_target, float p_power_ratio = 0.75f);
    bool command_tackle();

    // Skip Match & Token API
    bool can_skip_match() const;
    bool skip_match();
    void record_manual_match_completed();

    int get_skip_tokens() const { return skip_tokens; }
    void set_skip_tokens(int p_tokens);
    int get_manual_matches_played() const { return manual_matches_played; }

    // Formations & Pitch Dimensions API
    bool load_formations_from_csv(const String& p_filepath);
    bool load_default_formations();
    bool is_formations_loaded() const { return !formation_db.empty(); }
    String get_loaded_formations_path() const { return loaded_formations_path; }
    bool has_formation(const String& p_formation_name) const;
    PackedStringArray get_available_formations() const;
    bool set_home_formation(const String& p_formation_name);
    bool set_away_formation(const String& p_formation_name);
    String get_home_formation() const { return home_formation; }
    String get_away_formation() const { return away_formation; }

    void set_field_size(Vector2 p_size);
    Vector2 get_field_size() const { return field_size; }

    // Team OVR, Squad Lineup & Live Substitution API (2A: 5 subs / 3 stoppage windows)
    void set_home_team_ovr(int p_ovr);
    int get_home_team_ovr() const { return home_team_ovr; }
    void set_away_team_ovr(int p_ovr);
    int get_away_team_ovr() const { return away_team_ovr; }
    // Match Identity & User Side API (Home vs Away)
    void set_user_is_home(bool p_is_home);
    bool is_user_home() const { return user_is_home; }
    int get_user_attack_chance_state() const { return _user_attack_chance_state(); }
    int get_user_defend_chance_state() const { return _user_defend_chance_state(); }
    int get_user_start_index() const { return _user_start_idx(); }
    int get_user_end_index() const { return _user_end_idx(); }
    int get_user_gk_index() const { return _user_gk_idx(); }
    bool is_user_player_index(int p_idx) const { return _is_user_player_idx(p_idx); }

    void set_home_club_id(int p_id) { home_club_id = p_id; }
    int get_home_club_id() const { return home_club_id; }
    void set_away_club_id(int p_id) { away_club_id = p_id; }
    int get_away_club_id() const { return away_club_id; }

    void set_home_club_name(const String& p_name) { home_club_name = p_name; }
    String get_home_club_name() const { return home_club_name; }
    void set_away_club_name(const String& p_name) { away_club_name = p_name; }
    String get_away_club_name() const { return away_club_name; }

    void set_home_kit_color(const Color& p_color) { home_kit_color = p_color; }
    Color get_home_kit_color() const { return home_kit_color; }
    void set_away_kit_color(const Color& p_color) { away_kit_color = p_color; }
    Color get_away_kit_color() const { return away_kit_color; }

    Array get_away_squad_snapshot() const;
    Array get_away_bench_snapshot() const;
    Array get_user_squad_snapshot() const;
    Array get_user_bench_snapshot() const;
    bool can_make_user_substitution() const;
    int get_user_subs_used() const { return user_is_home ? home_subs_used : away_subs_used; }
    int get_user_sub_windows_used() const { return user_is_home ? home_sub_windows_used : away_sub_windows_used; }

    void set_home_lineup(const Array& p_players);
    void set_away_lineup(const Array& p_players);
    void set_home_bench(const Array& p_players);
    void set_away_bench(const Array& p_players);
    Array get_home_squad_snapshot() const;
    Array get_home_bench_snapshot() const;
    bool apply_substitution(bool p_is_home, int p_pitch_slot, int p_bench_slot);
    bool can_make_home_substitution() const;
    int get_home_subs_used() const { return home_subs_used; }
    int get_home_sub_windows_used() const { return home_sub_windows_used; }
    int get_max_subs() const { return MAX_MATCH_SUBS; }
    int get_max_sub_windows() const { return MAX_SUB_WINDOWS; }
    bool is_halftime_break() const { return waiting_for_kickoff_resume && current_minute == 45; }

    // Tactical Instructions API
    void set_home_mentality(int p_mentality);
    int get_home_mentality() const { return home_mentality; }
    void set_away_mentality(int p_mentality);
    int get_away_mentality() const { return away_mentality; }

    void set_home_pressing(int p_pressing);
    int get_home_pressing() const { return home_pressing; }
    void set_away_pressing(int p_pressing);
    int get_away_pressing() const { return away_pressing; }

    void set_home_defensive_line(int p_line);
    int get_home_defensive_line() const { return home_defensive_line; }
    void set_away_defensive_line(int p_line);
    int get_away_defensive_line() const { return away_defensive_line; }

    void set_home_passing_style(int p_style);
    int get_home_passing_style() const { return home_passing_style; }
    void set_away_passing_style(int p_style);
    int get_away_passing_style() const { return away_passing_style; }

    void set_home_attacking_width(int p_width);
    int get_home_attacking_width() const { return home_attacking_width; }
    void set_away_attacking_width(int p_width);
    int get_away_attacking_width() const { return away_attacking_width; }

    void set_home_tempo(int p_tempo);
    int get_home_tempo() const { return home_tempo; }
    void set_away_tempo(int p_tempo);
    int get_away_tempo() const { return away_tempo; }

    void set_home_defensive_approach(int p_approach);
    int get_home_defensive_approach() const { return home_defensive_approach; }
    void set_away_defensive_approach(int p_approach);
    int get_away_defensive_approach() const { return away_defensive_approach; }

    // Match State & Live Telemetry Getters
    int get_home_score() const { return home_score; }
    int get_away_score() const { return away_score; }
    int get_current_minute() const { return current_minute; }
    bool is_match_paused() const { return is_paused_for_manual || waiting_for_kickoff_resume; }
    bool is_match_active() const { return match_active; }
    int get_possession_team() const { return possession_team; }
    int get_ball_carrier_index() const { return ball_carrier_index; }
    int get_pass_target_index() const { return pass_target_index; }
    Array get_player_positions() const;
    PackedStringArray get_player_roles() const;
    PackedStringArray get_player_names() const;
    PackedFloat32Array get_player_stamina() const;
    Vector2 get_ball_position() const;
    Dictionary get_match_stats() const;

    void set_home_auto_shoot_enabled(bool p_enabled);
    bool is_home_auto_shoot_enabled() const;
    void trigger_wide_range_chance(bool p_is_home_attacking);

    

protected:
    static void _bind_methods();

private:
    static constexpr int MAX_MATCH_SUBS = 5;
    static constexpr int MAX_SUB_WINDOWS = 3;

    enum BallState {
        BALL_CARRIED = 0,
        BALL_IN_PASS = 1,
        BALL_IN_SHOT = 2,
        BALL_LOOSE = 3
    };

    void _trigger_halftime();
    bool halftime_triggered = false;

    void _initialize_default_squads();
    void _initialize_default_home_bench();
    void _initialize_default_away_bench();
    void _apply_role_profile(SimPlayer& p_player, int p_base_ovr) const;
    void _populate_sim_player_from_dict(SimPlayer& p_player, const Dictionary& p_dict, int p_fallback_ovr) const;
    BenchPlayer _parse_bench_player_from_dict(const Dictionary& p_dict, const String& p_fallback_name, const String& p_fallback_role, int p_fallback_ovr) const;
    void _recompute_team_ovrs_from_lineups();
    void _reset_field_positions();
    void _apply_formation_positions(bool p_reset_ball, bool p_force_snap_all = false);
    void _reset_after_goal(bool p_conceding_is_home);
    void _sync_player_positions_array();
    bool _has_valid_formation(const std::string& p_name) const;
    bool _has_valid_simulation_state() const;
    float _get_fatigue_multiplier(const SimPlayer& p_player) const;
    int _resolve_user_action_player() const;

    // Dedicated Defensive Clearance Helper (1A)
    bool _try_defensive_clearance(int p_defender_idx, float p_danger_score);
    void _execute_clearance(int p_defender_idx);

    // Organized Defensive & Attacking Run Coordination Helpers
    int _select_smart_presser(bool p_is_home, int p_exclude_idx = -1) const;
    float _compute_organized_backline_x(bool p_is_home) const;
    Vector2 _compute_lane_cover_target(bool p_is_home, const SimPlayer& p_defender) const;
    Vector2 _compute_runner_tracking_offset(bool p_is_home, const SimPlayer& p_defender, float p_line_x, float p_base_y) const;
    Vector2 _compute_smart_attacking_run_target(const SimPlayer& p_player, int p_index, float p_widened_anchor_y, float p_ball_prog) const;

    // Coordinated 2D Football Simulation
    void _step_simulation(float dt);
    void _update_player_movement(float dt);
    Vector2 _compute_tactical_target(const SimPlayer& p_player, int p_index, int p_primary_presser, int p_secondary_presser, float p_team_backline_x) const;
    void _update_ball_and_actions(float dt);
    void _evaluate_carrier_action(int p_carrier_idx);
    void _execute_pass(int p_passer_idx, int p_receiver_idx, bool p_is_through_ball);
    void _execute_shot(int p_shooter_idx, float p_xg, Vector2 p_custom_target = Vector2(-1.0f, -1.0f), float p_power_ratio = 0.75f);
    void _trigger_key_chance(int p_scenario_type, bool p_is_home_attacking, int p_attacker_idx, float p_xg);
    void _end_active_chance(bool p_goal_scored);
    void _give_kickoff(bool p_home_kickoff);
    void _give_gk_possession(bool p_home_gk);
    void _log_event(const String& p_type, bool p_is_home, const String& p_player_name, const String& p_desc);
    Vector3 _to_3d_coords(Vector2 p_pos_2d) const;
    void _spawn_wide_range_chance(bool p_is_home_attacking);

    float match_timer = 0.0f;
    int current_minute = 0;
    int home_score = 0;
    int away_score = 0;

    bool is_paused_for_manual = false;
    bool match_active = false;
    float event_cooldown = 0.0f;

    // Post-Goal Kickoff Reset & Pause State
    bool waiting_for_kickoff_resume = false;
    int kickoff_team = -1; // 0 = home kickoff, 1 = away kickoff, -1 = none

    // Substitution & Window Tracking (2A: 5 subs, max 3 stoppage windows, halftime free)
    int home_subs_used = 0;
    int away_subs_used = 0;
    int home_sub_windows_used = 0;
    int away_sub_windows_used = 0;
    int last_home_sub_minute = -999;
    int last_away_sub_minute = -999;
    bool last_home_sub_was_halftime = false;
    bool last_away_sub_was_halftime = false;

    // Authoritative Chance & Direct Control State
    int chance_state = CHANCE_NONE;
    bool manual_control_active = false;
    int user_controlled_index = -1;
    bool pending_chance_signal = false;
    bool pending_chance_end_signal = false;
    int pending_chance_state_val = 0;
    int pending_chance_ctrl_val = -1;
    bool pending_interactive_signal = false;
    int pending_interactive_type = 0;
    bool pending_interactive_home_atk = true;
    Vector3 pending_interactive_ball_3d;
    bool pending_match_ended_signal = false;
    void _flush_deferred_signals();
    bool has_user_move_target = false;
    Vector2 user_move_target = Vector2(-1.0f, -1.0f);
    float active_chance_timer = 0.0f;
    float defensive_chance_time_scale = 0.65f;

    // Token Management State
    int manual_matches_played = 0;
    int skip_tokens = 1;

    int home_team_ovr = 75;
    int away_team_ovr = 72;

    int home_mentality = MENTALITY_BALANCED;
    int away_mentality = MENTALITY_BALANCED;
    int home_pressing = PRESSING_NORMAL;
    int away_pressing = PRESSING_NORMAL;
    int home_defensive_line = TIER_NORMAL;
    int away_defensive_line = TIER_NORMAL;
    int home_passing_style = TIER_NORMAL;
    int away_passing_style = TIER_NORMAL;
    int home_attacking_width = TIER_NORMAL;
    int away_attacking_width = TIER_NORMAL;
    int home_tempo = TIER_NORMAL;
    int away_tempo = TIER_NORMAL;
    int home_defensive_approach = TIER_NORMAL;
    int away_defensive_approach = TIER_NORMAL;

    // User-side index & direction helpers (supports playing both Home [0..10] and Away [11..21])
    int _user_start_idx() const { return user_is_home ? 0 : 11; }
    int _user_end_idx() const { return user_is_home ? 11 : 22; }
    int _user_gk_idx() const { return user_is_home ? 0 : 11; }
    int _user_attack_chance_state() const { return user_is_home ? CHANCE_HOME_ATTACK : CHANCE_AWAY_ATTACK; }
    int _user_defend_chance_state() const { return user_is_home ? CHANCE_AWAY_ATTACK : CHANCE_HOME_ATTACK; }
    float _user_attack_dir() const { return user_is_home ? 1.0f : -1.0f; }
    float _user_opp_goal_x() const { return user_is_home ? field_size.x : 0.0f; }
    bool _is_user_player_idx(int p_idx) const { return p_idx >= _user_start_idx() && p_idx < _user_end_idx(); }
    bool _is_user_outfield_idx(int p_idx) const { return p_idx > _user_gk_idx() && p_idx < _user_end_idx(); }



    bool user_is_home = true;
    int home_club_id = -1;
    int away_club_id = -1;
    String home_club_name = "HOME FC";
    String away_club_name = "AWAY FC";
    Color home_kit_color = Color(0.12f, 0.23f, 0.54f);
    Color away_kit_color = Color(0.86f, 0.15f, 0.15f);

    String home_formation = "4-3-3";
    String away_formation = "4-4-2";
    String loaded_formations_path = "";
    std::unordered_map<std::string, std::vector<TacticalPosition>> formation_db;

    Vector2 field_size = Vector2(1050.0f, 680.0f);
    Vector2 ball_position = Vector2(525.0f, 340.0f);
    Vector2 ball_velocity = Vector2(0.0f, 0.0f);
    Vector2 ball_target = Vector2(525.0f, 340.0f);

    int ball_state = BALL_CARRIED;
    int possession_team = 0;      // -1 = loose, 0 = home, 1 = away
    int ball_carrier_index = 9;   // 0..10 home, 11..21 away, -1 none
    int pass_target_index = -1;
    bool is_through_ball_in_flight = false;
    int last_touch_index = 9;
    float action_decision_timer = 0.25f;
    float pending_chance_xg = 0.0f;
    bool home_auto_shoot_enabled = false;
    bool shot_hits_crossbar = false;
    float last_shot_power_ratio = 0.75f;

    std::vector<SimPlayer> sim_players;
    std::vector<BenchPlayer> home_bench;
    std::vector<BenchPlayer> away_bench;
    Array home_subbed_off_records;
    Array away_subbed_off_records;
    Array home_subbed_on_ids;
    Array away_subbed_on_ids;
    Array player_positions;
    MatchStats stats;
    Scenario3DData last_scenario_3d;
};

} // namespace godot
