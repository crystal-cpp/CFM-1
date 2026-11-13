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
    load_default_formations();
    _reset_field_positions();
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
}

void MatchEngine::set_away_team_ovr(int p_ovr) {
    away_team_ovr = std::clamp(p_ovr, 1, 99);
}

bool MatchEngine::can_skip_match() const {
    return skip_tokens > 0 && match_active && !is_paused_for_manual;
}

bool MatchEngine::skip_match() {
    if (!can_skip_match()) {
        UtilityFunctions::printerr("MatchEngine: Cannot skip match — insufficient tokens or invalid match state.");
        return false;
    }

    skip_tokens--;

    // Scoreboard Auto-Simulation Engine (Clamped OVR Difference + Variance)
    float ovr_diff = static_cast<float>(home_team_ovr - away_team_ovr);
    float home_exp = 1.35f + (ovr_diff * 0.05f);
    float away_exp = 1.10f - (ovr_diff * 0.05f);

    float home_luck = UtilityFunctions::randf_range(-1.25f, 1.25f);
    float away_luck = UtilityFunctions::randf_range(-1.25f, 1.25f);

    home_score = std::max(0, static_cast<int>(std::round(home_exp + home_luck)));
    away_score = std::max(0, static_cast<int>(std::round(away_exp + away_luck)));

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

    // 1. Validate CSV Header (must have >= 4 columns and non-numeric coordinate headers)
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

    // 2. Parse & Validate Coordinate Rows
    std::unordered_set<std::string> corrupted_formations;
    int line_number = 1;

    while (!file->eof_reached()) {
        line_number++;
        PackedStringArray row = file->get_csv_line();

        // Ignore trailing empty line at EOF
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

    // 3. Remove Corrupted Formations & Enforce Exactly 11 Positions Per Formation
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

void MatchEngine::_update_bounded_2d_motion(float dt) {
    const float player_margin = 12.0f;
    const float max_tether_radius = 24.0f;
    const float ball_margin = 10.0f;

    // Bounded 2D tactical drift around formation anchors (not full tactical AI)
    int count = std::min(static_cast<int>(player_positions.size()), static_cast<int>(base_player_positions.size()));
    for (int i = 0; i < count; i++) {
        Vector2 pos = player_positions[i];
        Vector2 anchor = base_player_positions[i];

        Vector2 drift_step(
            UtilityFunctions::randf_range(-1.0f, 1.0f) * 35.0f * dt,
            UtilityFunctions::randf_range(-1.0f, 1.0f) * 35.0f * dt
        );

        Vector2 next_pos = pos + drift_step;

        Vector2 offset_from_anchor = next_pos - anchor;
        if (offset_from_anchor.length() > max_tether_radius) {
            next_pos = anchor + offset_from_anchor.normalized() * max_tether_radius;
        }

        next_pos.x = std::clamp(next_pos.x, player_margin, field_size.x - player_margin);
        next_pos.y = std::clamp(next_pos.y, player_margin, field_size.y - player_margin);

        player_positions[i] = next_pos;
    }

    // Smooth, frame-rate-independent ball motion clamped to pitch boundaries
    ball_velocity.x += UtilityFunctions::randf_range(-80.0f, 80.0f) * dt;
    ball_velocity.y += UtilityFunctions::randf_range(-80.0f, 80.0f) * dt;
    if (ball_velocity.length() > 110.0f) {
        ball_velocity = ball_velocity.normalized() * 110.0f;
    }

    ball_position += ball_velocity * dt;

    if (ball_position.x < ball_margin) {
        ball_position.x = ball_margin;
        ball_velocity.x = std::abs(ball_velocity.x);
    } else if (ball_position.x > field_size.x - ball_margin) {
        ball_position.x = field_size.x - ball_margin;
        ball_velocity.x = -std::abs(ball_velocity.x);
    }

    if (ball_position.y < ball_margin) {
        ball_position.y = ball_margin;
        ball_velocity.y = std::abs(ball_velocity.y);
    } else if (ball_position.y > field_size.y - ball_margin) {
        ball_position.y = field_size.y - ball_margin;
        ball_velocity.y = -std::abs(ball_velocity.y);
    }
}

void MatchEngine::_physics_process(double delta) {
    if (!match_active || is_paused_for_manual) {
        return;
    }

    float dt = static_cast<float>(delta);
    if (dt <= 0.0f) {
        return;
    }

    match_timer += dt;
    int new_minute = std::min(90, static_cast<int>(match_timer / 1.0f));

    if (new_minute != current_minute) {
        current_minute = new_minute;
        emit_signal("minute_changed", current_minute);
    }

    if (current_minute >= 90) {
        match_active = false;
        is_paused_for_manual = false;
        set_physics_process(false);
        record_manual_match_completed();
        emit_signal("match_ended", home_score, away_score);
        return;
    }

    _update_bounded_2d_motion(dt);

    // Frame-rate-independent chance generation (Poisson arrival probability: P = 1 - exp(-rate * dt))
    if (event_cooldown > 0.0f) {
        event_cooldown = std::max(0.0f, event_cooldown - dt);
    } else {
        const float chance_rate_per_second = 0.11f;
        float trigger_probability = 1.0f - std::exp(-chance_rate_per_second * dt);

        if (UtilityFunctions::randf() < trigger_probability) {
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
    if (formation_db.empty()) {
        load_default_formations();
    }

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
    if (!match_active) {
        return;
    }
    is_paused_for_manual = p_pause;
}

void MatchEngine::resolve_manual_scenario(bool p_success, bool p_is_home_team) {
    if (!match_active || !is_paused_for_manual) {
        UtilityFunctions::printerr("MatchEngine: Ignored resolve_manual_scenario() while no event is active.");
        return;
    }

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
    _apply_formation_positions(true);
}

void MatchEngine::_apply_formation_positions(bool p_reset_ball) {
    player_positions.clear();
    base_player_positions.clear();
    
    float half_width = field_size.x * 0.5f;
    float field_height = field_size.y;
    const float margin = 12.0f;

    std::string home_str = home_formation.utf8().get_data();
    std::string away_str = away_formation.utf8().get_data();

    // 1. Home Team Positions (Left Half, attacking left-to-right)
    if (_has_valid_formation(home_str)) {
        const auto& nodes = formation_db[home_str];
        for (size_t i = 0; i < 11; i++) {
            float posX = std::clamp(nodes[i].norm_x * half_width, margin, half_width - margin);
            float posY = std::clamp(nodes[i].norm_y * field_height, margin, field_height - margin);
            Vector2 p(posX, posY);
            base_player_positions.push_back(p);
            player_positions.append(p);
        }
    } else {
        if (!formation_db.empty()) {
            UtilityFunctions::printerr("MatchEngine: Home formation '", home_formation, "' unavailable. Using bounded fallback.");
        }
        for (int i = 0; i < 11; i++) {
            float norm_x = 0.12f + (static_cast<float>(i % 4) * 0.22f);
            float norm_y = 0.15f + (static_cast<float>(i) / 10.0f) * 0.70f;
            Vector2 p(norm_x * half_width, norm_y * field_height);
            base_player_positions.push_back(p);
            player_positions.append(p);
        }
    }

    // 2. Away Team Positions (Mirrored Right Half, attacking right-to-left: both X and Y inverted for true 180-deg symmetry)
    if (_has_valid_formation(away_str)) {
        const auto& nodes = formation_db[away_str];
        for (size_t i = 0; i < 11; i++) {
            float posX = std::clamp(field_size.x - (nodes[i].norm_x * half_width), half_width + margin, field_size.x - margin);
            float posY = std::clamp((1.0f - nodes[i].norm_y) * field_height, margin, field_height - margin);
            Vector2 p(posX, posY);
            base_player_positions.push_back(p);
            player_positions.append(p);
        }
    } else {
        if (!formation_db.empty()) {
            UtilityFunctions::printerr("MatchEngine: Away formation '", away_formation, "' unavailable. Using bounded fallback.");
        }
        for (int i = 0; i < 11; i++) {
            float norm_x = 0.12f + (static_cast<float>(i % 4) * 0.22f);
            float norm_y = 0.15f + (static_cast<float>(i) / 10.0f) * 0.70f;
            Vector2 p(field_size.x - (norm_x * half_width), (1.0f - norm_y) * field_height);
            base_player_positions.push_back(p);
            player_positions.append(p);
        }
    }

    if (p_reset_ball) {
        ball_position = field_size * 0.5f;
        ball_velocity = Vector2(45.0f, 30.0f);
    }
}

Array MatchEngine::get_player_positions() const { return player_positions; }
Vector2 MatchEngine::get_ball_position() const { return ball_position; }