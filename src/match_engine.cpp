#include "match_engine.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <cmath>

using namespace godot;

void MatchEngine::_bind_methods() {
    // Control Methods
    ClassDB::bind_method(D_METHOD("start_match"), &MatchEngine::start_match);
    ClassDB::bind_method(D_METHOD("pause_match", "pause"), &MatchEngine::pause_match);
    ClassDB::bind_method(D_METHOD("resolve_manual_scenario", "success", "is_home_team"), &MatchEngine::resolve_manual_scenario, DEFVAL(true));

    // Skip Match API Methods
    ClassDB::bind_method(D_METHOD("can_skip_match"), &MatchEngine::can_skip_match);
    ClassDB::bind_method(D_METHOD("skip_match"), &MatchEngine::skip_match);
    ClassDB::bind_method(D_METHOD("get_skip_tokens"), &MatchEngine::get_skip_tokens);
    ClassDB::bind_method(D_METHOD("set_skip_tokens", "tokens"), &MatchEngine::set_skip_tokens);
    ClassDB::bind_method(D_METHOD("get_manual_matches_played"), &MatchEngine::get_manual_matches_played);

    // Formation & Field Methods
    ClassDB::bind_method(D_METHOD("load_formations_from_csv", "filepath"), &MatchEngine::load_formations_from_csv);
    ClassDB::bind_method(D_METHOD("set_home_formation", "formation_name"), &MatchEngine::set_home_formation);
    ClassDB::bind_method(D_METHOD("set_away_formation", "formation_name"), &MatchEngine::set_away_formation);
    ClassDB::bind_method(D_METHOD("get_home_formation"), &MatchEngine::get_home_formation);
    ClassDB::bind_method(D_METHOD("get_away_formation"), &MatchEngine::get_away_formation);
    ClassDB::bind_method(D_METHOD("set_field_size", "size"), &MatchEngine::set_field_size);
    ClassDB::bind_method(D_METHOD("get_field_size"), &MatchEngine::get_field_size);

    // Team OVR Methods
    ClassDB::bind_method(D_METHOD("set_home_team_ovr", "ovr"), &MatchEngine::set_home_team_ovr);
    ClassDB::bind_method(D_METHOD("get_home_team_ovr"), &MatchEngine::get_home_team_ovr);
    ClassDB::bind_method(D_METHOD("set_away_team_ovr", "ovr"), &MatchEngine::set_away_team_ovr);
    ClassDB::bind_method(D_METHOD("get_away_team_ovr"), &MatchEngine::get_away_team_ovr);

    // Getters
    ClassDB::bind_method(D_METHOD("get_home_score"), &MatchEngine::get_home_score);
    ClassDB::bind_method(D_METHOD("get_away_score"), &MatchEngine::get_away_score);
    ClassDB::bind_method(D_METHOD("get_current_minute"), &MatchEngine::get_current_minute);
    ClassDB::bind_method(D_METHOD("is_match_paused"), &MatchEngine::is_match_paused);
    ClassDB::bind_method(D_METHOD("is_match_active"), &MatchEngine::is_match_active);
    ClassDB::bind_method(D_METHOD("get_player_positions"), &MatchEngine::get_player_positions);
    ClassDB::bind_method(D_METHOD("get_ball_position"), &MatchEngine::get_ball_position);

    // Inspector Properties
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "home_formation"), "set_home_formation", "get_home_formation");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "away_formation"), "set_away_formation", "get_away_formation");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "field_size"), "set_field_size", "get_field_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "home_team_ovr"), "set_home_team_ovr", "get_home_team_ovr");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "away_team_ovr"), "set_away_team_ovr", "get_away_team_ovr");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "skip_tokens"), "set_skip_tokens", "get_skip_tokens");

    // Signals
    ADD_SIGNAL(MethodInfo("interactive_event_triggered", 
        PropertyInfo(Variant::INT, "scenario_type"), 
        PropertyInfo(Variant::BOOL, "is_home_team"), 
        PropertyInfo(Variant::VECTOR3, "ball_spawn_3d")));
        
    ADD_SIGNAL(MethodInfo("goal_scored", 
        PropertyInfo(Variant::INT, "home_score"), 
        PropertyInfo(Variant::INT, "away_score"), 
        PropertyInfo(Variant::BOOL, "is_home_team")));
        
    ADD_SIGNAL(MethodInfo("minute_changed", 
        PropertyInfo(Variant::INT, "minute")));
        
    ADD_SIGNAL(MethodInfo("match_ended", 
        PropertyInfo(Variant::INT, "final_home_score"), 
        PropertyInfo(Variant::INT, "final_away_score")));
}

MatchEngine::MatchEngine() {}
MatchEngine::~MatchEngine() {}

void MatchEngine::_ready() {
    set_physics_process(false);
    load_formations_from_csv("res://../Players_Database/formations.csv");
    _reset_field_positions();
}

void MatchEngine::set_skip_tokens(int p_tokens) {
    skip_tokens = std::max(0, p_tokens);
}

bool MatchEngine::can_skip_match() const {
    return skip_tokens > 0 && match_active && !is_paused_for_manual;
}

bool MatchEngine::skip_match() {
    if (!can_skip_match()) {
        UtilityFunctions::printerr("Cannot skip match: Insufficient tokens or match in invalid state.");
        return false;
    }

    // Deduct token
    skip_tokens--;

    // Scoreboard Auto-Simulation Engine (OVR Difference + Gaussian Variance)
    float ovr_diff = static_cast<float>(home_team_ovr - away_team_ovr);
    float home_exp = 1.35f + (ovr_diff * 0.05f);
    float away_exp = 1.10f - (ovr_diff * 0.05f);

    float home_luck = UtilityFunctions::randf_range(-1.25f, 1.25f);
    float away_luck = UtilityFunctions::randf_range(-1.25f, 1.25f);

    home_score = std::max(0, static_cast<int>(std::round(home_exp + home_luck)));
    away_score = std::max(0, static_cast<int>(std::round(away_exp + away_luck)));

    // Set state directly to full-time
    current_minute = 90;
    match_active = false;
    is_paused_for_manual = false;
    set_physics_process(false);

    UtilityFunctions::print("Match Skipped! Score: ", home_score, " - ", away_score, " | Remaining Tokens: ", skip_tokens);

    emit_signal("minute_changed", 90);
    emit_signal("match_ended", home_score, away_score);
    return true;
}

void MatchEngine::record_manual_match_completed() {
    manual_matches_played++;
    if (manual_matches_played % 5 == 0) {
        skip_tokens++;
        UtilityFunctions::print("Reward: Earned +1 Skip Token! Total tokens: ", skip_tokens);
    }
}

bool MatchEngine::load_formations_from_csv(const String& p_filepath) {
    Ref<FileAccess> file = FileAccess::open(p_filepath, FileAccess::READ);
    if (file.is_null()) {
        UtilityFunctions::printerr("Failed to open formations CSV: ", p_filepath);
        return false;
    }

    formation_db.clear();
    bool is_header = true;

    while (!file->eof_reached()) {
        PackedStringArray row = file->get_csv_line();
        if (row.size() < 4) continue;

        if (is_header) {
            is_header = false;
            continue;
        }

        std::string fmt_name = row[0].utf8().get_data();
        TacticalPosition pos;
        pos.role = row[1];
        pos.norm_x = row[2].to_float();
        pos.norm_y = row[3].to_float();

        formation_db[fmt_name].push_back(pos);
    }

    UtilityFunctions::print("Loaded ", static_cast<int64_t>(formation_db.size()), " tactical formations.");
    return true;
}

void MatchEngine::set_home_formation(const String& p_formation_name) {
    home_formation = p_formation_name;
    if (match_active) _reset_field_positions();
}

void MatchEngine::set_away_formation(const String& p_formation_name) {
    away_formation = p_formation_name;
    if (match_active) _reset_field_positions();
}

void MatchEngine::_physics_process(double delta) {
    if (!match_active || is_paused_for_manual) {
        return;
    }

    match_timer += static_cast<float>(delta);
    int new_minute = static_cast<int>(match_timer / 1.0f);

    if (new_minute != current_minute) {
        current_minute = new_minute;
        emit_signal("minute_changed", current_minute);
    }

    if (current_minute >= 90) {
        match_active = false;
        set_physics_process(false);
        record_manual_match_completed();
        emit_signal("match_ended", home_score, away_score);
        return;
    }

    float dt = static_cast<float>(delta);

    // 1. Simulate 2D Player & Ball Drift Movement
    for (int i = 0; i < player_positions.size(); i++) {
        Vector2 pos = player_positions[i];
        float jitterX = (UtilityFunctions::randf() - 0.5f) * 20.0f * dt;
        float jitterY = (UtilityFunctions::randf() - 0.5f) * 20.0f * dt;
        player_positions[i] = pos + Vector2(jitterX, jitterY);
    }
    ball_position += Vector2((UtilityFunctions::randf() - 0.5f) * 15.0f * dt, (UtilityFunctions::randf() - 0.5f) * 10.0f * dt);

    // 2. Dynamic Chance Generation System (Team OVR Weighted)
    if (event_cooldown > 0.0f) {
        event_cooldown -= dt;
    } else {
        if (UtilityFunctions::randf() < 0.0020f) {
            is_paused_for_manual = true;
            event_cooldown = 12.0f;
            
            float ovr_diff = static_cast<float>(home_team_ovr - away_team_ovr);
            float home_attack_probability = std::clamp(0.50f + (ovr_diff * 0.015f), 0.20f, 0.80f);

            bool is_home_event = UtilityFunctions::randf() < home_attack_probability;

            int scenario_type;
            if (is_home_event) {
                scenario_type = static_cast<int>(UtilityFunctions::randi_range(1, 3));
            } else {
                scenario_type = static_cast<int>(UtilityFunctions::randi_range(4, 5));
            }
            
            Vector3 ball_3d = Vector3(
                (ball_position.x - (field_size.x * 0.5f)) * 0.1f, 
                0.0f, 
                (ball_position.y - (field_size.y * 0.5f)) * 0.1f
            );

            emit_signal("interactive_event_triggered", scenario_type, is_home_event, ball_3d);
        }
    }
}

void MatchEngine::start_match() {
    match_timer = 0.0f;
    current_minute = 0;
    home_score = 0;
    away_score = 0;
    is_paused_for_manual = false;
    match_active = true;
    event_cooldown = 5.0f;

    _reset_field_positions();
    set_physics_process(true);
}

void MatchEngine::pause_match(bool p_pause) {
    is_paused_for_manual = p_pause;
}

void MatchEngine::resolve_manual_scenario(bool p_success, bool p_is_home_team) {
    if (p_success) {
        if (p_is_home_team) {
            home_score++;
        } else {
            away_score++;
        }
        emit_signal("goal_scored", home_score, away_score, p_is_home_team);
    }
    
    is_paused_for_manual = false;
}

void MatchEngine::_reset_field_positions() {
    player_positions.clear();
    
    float half_width = field_size.x * 0.5f;
    float field_height = field_size.y;

    std::string home_str = home_formation.utf8().get_data();
    std::string away_str = away_formation.utf8().get_data();

    bool has_home = formation_db.find(home_str) != formation_db.end();
    bool has_away = formation_db.find(away_str) != formation_db.end();

    if (has_home) {
        const auto& nodes = formation_db[home_str];
        for (size_t i = 0; i < 11 && i < nodes.size(); i++) {
            float posX = nodes[i].norm_x * half_width;
            float posY = nodes[i].norm_y * field_height;
            player_positions.append(Vector2(posX, posY));
        }
    } else {
        for (int i = 0; i < 11; i++) {
            player_positions.append(Vector2(50.0f + (i * 35.0f), field_height * 0.5f));
        }
    }

    if (has_away) {
        const auto& nodes = formation_db[away_str];
        for (size_t i = 0; i < 11 && i < nodes.size(); i++) {
            float posX = field_size.x - (nodes[i].norm_x * half_width);
            float posY = nodes[i].norm_y * field_height;
            player_positions.append(Vector2(posX, posY));
        }
    } else {
        for (int i = 0; i < 11; i++) {
            player_positions.append(Vector2(field_size.x - (50.0f + (i * 35.0f)), field_height * 0.5f));
        }
    }

    ball_position = field_size * 0.5f;
}

Array MatchEngine::get_player_positions() const { return player_positions; }
Vector2 MatchEngine::get_ball_position() const { return ball_position; }