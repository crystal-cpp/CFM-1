#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>
#include <unordered_map>
#include <string>

namespace godot {

struct TacticalPosition {
    String role;
    float norm_x = 0.0f;
    float norm_y = 0.0f;
};

// Preserved for future Phase 10 3D highlight compatibility
struct Scenario3DData {
    Vector3 ball_pos_3d;
    Vector3 attacker_pos_3d;
    Array defender_positions_3d;
    Array teammate_positions_3d;
};

struct SimPlayer {
    String name;
    String role = "CM";
    bool is_home = true;
    int slot_index = 0;

    int pace = 70;
    int shooting = 70;
    int passing = 70;
    int dribbling = 70;
    int defending = 70;
    int physical = 70;
    float stamina = 100.0f;

    Vector2 anchor;
    Vector2 position;
    Vector2 velocity;
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

    enum Mentality {
        MENTALITY_DEFENSIVE = 0,
        MENTALITY_BALANCED = 1,
        MENTALITY_ATTACKING = 2
    };

    enum PressingIntensity {
        PRESSING_LOW = 0,
        PRESSING_MEDIUM = 1,
        PRESSING_HIGH = 2,
        PRESSING_EXTREME = 3
    };

    MatchEngine();
    ~MatchEngine();

    void _ready() override;
    void _physics_process(double delta) override;

    // Match Flow Controls
    void start_match();
    void pause_match(bool p_pause);
    void resolve_manual_scenario(bool p_success, bool p_is_home_team = true);

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

    // Team OVR & Squad Lineup API
    void set_home_team_ovr(int p_ovr);
    int get_home_team_ovr() const { return home_team_ovr; }
    void set_away_team_ovr(int p_ovr);
    int get_away_team_ovr() const { return away_team_ovr; }
    void set_home_lineup(const Array& p_players);
    void set_away_lineup(const Array& p_players);

    // Tactical Instructions API (Phase 2 Ready)
    void set_home_mentality(int p_mentality);
    int get_home_mentality() const { return home_mentality; }
    void set_away_mentality(int p_mentality);
    int get_away_mentality() const { return away_mentality; }
    void set_home_pressing(int p_pressing);
    int get_home_pressing() const { return home_pressing; }
    void set_away_pressing(int p_pressing);
    int get_away_pressing() const { return away_pressing; }

    // Match State & Live Telemetry Getters
    int get_home_score() const { return home_score; }
    int get_away_score() const { return away_score; }
    int get_current_minute() const { return current_minute; }
    bool is_match_paused() const { return is_paused_for_manual; }
    bool is_match_active() const { return match_active; }

    int get_possession_team() const { return possession_team; }
    int get_ball_carrier_index() const { return ball_carrier_index; }
    Array get_player_positions() const;
    PackedStringArray get_player_roles() const;
    PackedStringArray get_player_names() const;
    Vector2 get_ball_position() const;
    Dictionary get_match_stats() const;

protected:
    static void _bind_methods();

private:
    enum BallState {
        BALL_CARRIED = 0,
        BALL_IN_PASS = 1,
        BALL_IN_SHOT = 2,
        BALL_LOOSE = 3
    };

    void _initialize_default_squads();
    void _apply_role_profile(SimPlayer& p_player, int p_base_ovr) const;
    void _reset_field_positions();
    void _apply_formation_positions(bool p_reset_ball);
    void _sync_player_positions_array();
    bool _has_valid_formation(const std::string& p_name) const;

    // Coordinated 2D Football Simulation
    void _step_simulation(float dt);
    void _update_player_movement(float dt);
    Vector2 _compute_tactical_target(const SimPlayer& p_player, int p_index, int p_primary_presser, int p_secondary_presser) const;
    void _update_ball_and_actions(float dt);
    void _evaluate_carrier_action(int p_carrier_idx);
    void _execute_pass(int p_passer_idx, int p_receiver_idx, bool p_is_through_ball);
    void _execute_shot(int p_shooter_idx, float p_xg);
    void _trigger_key_chance(int p_scenario_type, bool p_is_home_attacking, int p_attacker_idx, float p_xg);
    void _give_kickoff(bool p_home_kickoff);
    void _give_gk_possession(bool p_home_gk);
    void _log_event(const String& p_type, bool p_is_home, const String& p_player_name, const String& p_desc);
    Vector3 _to_3d_coords(Vector2 p_pos_2d) const;

    float match_timer = 0.0f;
    int current_minute = 0;
    int home_score = 0;
    int away_score = 0;
    
    bool is_paused_for_manual = false;
    bool match_active = false;
    float event_cooldown = 0.0f;

    // Token Management State
    int manual_matches_played = 0;
    int skip_tokens = 1;

    int home_team_ovr = 75;
    int away_team_ovr = 72;

    int home_mentality = MENTALITY_BALANCED;
    int away_mentality = MENTALITY_BALANCED;
    int home_pressing = PRESSING_MEDIUM;
    int away_pressing = PRESSING_MEDIUM;

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
    int last_touch_index = 9;
    float action_decision_timer = 0.25f;
    float pending_chance_xg = 0.0f;

    std::vector<SimPlayer> sim_players;
    Array player_positions;
    MatchStats stats;
    Scenario3DData last_scenario_3d;
};

} // namespace godot