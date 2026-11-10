#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/array.hpp>
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

struct Scenario3DData {
    Vector3 ball_pos_3d;
    Vector3 attacker_pos_3d;
    Array defender_positions_3d;
    Array teammate_positions_3d;
};

class MatchEngine : public Node2D {
    GDCLASS(MatchEngine, Node2D)

public:
    MatchEngine();
    ~MatchEngine();

    void _ready() override;
    void _physics_process(double delta) override;

    void start_match();
    void pause_match(bool p_pause);
    void resolve_manual_scenario(bool p_success, bool p_is_home_team = true);

    // Formations & Pitch Dimensions API
    bool load_formations_from_csv(const String& p_filepath);
    void set_home_formation(const String& p_formation_name);
    void set_away_formation(const String& p_formation_name);
    String get_home_formation() const { return home_formation; }
    String get_away_formation() const { return away_formation; }

    void set_field_size(Vector2 p_size) { field_size = p_size; }
    Vector2 get_field_size() const { return field_size; }

    // Team Overall Ratings (OVR) API
    void set_home_team_ovr(int p_ovr) { home_team_ovr = p_ovr; }
    int get_home_team_ovr() const { return home_team_ovr; }
    void set_away_team_ovr(int p_ovr) { away_team_ovr = p_ovr; }
    int get_away_team_ovr() const { return away_team_ovr; }

    // Match State Getters
    int get_home_score() const { return home_score; }
    int get_away_score() const { return away_score; }
    int get_current_minute() const { return current_minute; }
    bool is_match_paused() const { return is_paused_for_manual; }
    bool is_match_active() const { return match_active; }

    Array get_player_positions() const;
    Vector2 get_ball_position() const;

protected:
    static void _bind_methods();

private:
    void _reset_field_positions();

    float match_timer = 0.0f;
    int current_minute = 0;
    int home_score = 0;
    int away_score = 0;
    
    bool is_paused_for_manual = false;
    bool match_active = false;
    float event_cooldown = 0.0f;

    int home_team_ovr = 75;
    int away_team_ovr = 72;

    String home_formation = "4-3-3";
    String away_formation = "4-4-2";
    std::unordered_map<std::string, std::vector<TacticalPosition>> formation_db;

    Vector2 field_size = Vector2(1050.0f, 680.0f);
    Vector2 ball_position = Vector2(525.0f, 340.0f);
    Array player_positions;
};

} // namespace godot