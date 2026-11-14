extends Node2D

var match_engine: MatchEngine
var _backdrop_canvas: Control
var _viewport_clip: PanelContainer
var _pitch_canvas: Control

var _ball_trail: PackedVector2Array = PackedVector2Array()
const MAX_TRAIL_POINTS: int = 12

# Camera Zoom & Smooth Focus State
var _cam_zoom: float = 1.0
var _cam_focus_logical: Vector2 = Vector2(525.0, 340.0)

# Kick-Off & Post-Goal Resume State
var _waiting_for_kickoff: bool = false
var _goal_settling: bool = false
var _goal_settle_timer: float = 0.0
var _waiting_for_goal_resume: bool = false
var _last_goal_banner_text: String = "GOAL!"

# Attack Chance Stop-and-Go Decision State Machine
enum AttackPhase {
	INACTIVE,
	SETTLING,
	DECIDING,
	EXECUTING_PASS,
	EXECUTING_DRIBBLE,
	EXECUTING_SHOT
}
var _atk_phase: int = AttackPhase.INACTIVE
var _atk_phase_timer: float = 0.0
var _atk_expected_receiver: int = -1
var _atk_dribble_target: Vector2 = Vector2.ZERO
var _atk_decision_paused: bool = false
var _open_play_chance_timer: float = 0.0

# Through-Ball Preview State (while dragging)
var _preview_through_runner: int = -1
var _preview_through_target: Vector2 = Vector2.ZERO

# Defend Chance Real-Time State
var _gk_rush_active: bool = false
var _slide_tackle_timer: float = 0.0
var _slide_tackle_pos: Vector2 = Vector2.ZERO
var _slide_tackle_dir: Vector2 = Vector2.RIGHT

# Unified Desktop Mouse + Android Touch Gesture State
var _pointer_down: bool = false
var _pointer_index: int = -1
var _drag_start_screen: Vector2 = Vector2.ZERO
var _drag_current_screen: Vector2 = Vector2.ZERO
var _drag_start_ms: int = 0
var _gesture_mode: String = "NONE" # "NONE", "THROUGH_BALL", "SHOT", "SLIDE_TACKLE"

var _target_marker_pos: Vector2 = Vector2(-1.0, -1.0)
var _target_marker_timer: float = 0.0
var _target_marker_color: Color = Color(0.25, 0.95, 0.55)

const RIGHT_PANEL_MIN_W: float = 276.0
const RIGHT_PANEL_MAX_W: float = 348.0
const PITCH_APRONS_LOGICAL: Vector2 = Vector2(22.0, 10.0)
const GOAL_SETTLE_DURATION: float = 0.65

func _ready() -> void:
	_backdrop_canvas = Control.new()
	_backdrop_canvas.name = "StadiumBackdrop"
	_backdrop_canvas.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_backdrop_canvas.draw.connect(_draw_stadium_backdrop)
	add_child(_backdrop_canvas)

	_viewport_clip = PanelContainer.new()
	_viewport_clip.name = "PitchViewportClip"
	_viewport_clip.clip_children = CanvasItem.CLIP_CHILDREN_AND_DRAW
	_viewport_clip.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var pitch_frame_style := StyleBoxFlat.new()
	pitch_frame_style.bg_color = Color(0.06, 0.36, 0.20, 1.0)
	pitch_frame_style.border_color = Color(0.20, 0.28, 0.40, 0.95)
	pitch_frame_style.set_border_width_all(2)
	pitch_frame_style.set_corner_radius_all(14)
	_viewport_clip.add_theme_stylebox_override("panel", pitch_frame_style)
	add_child(_viewport_clip)

	_pitch_canvas = Control.new()
	_pitch_canvas.name = "PitchCanvas"
	_pitch_canvas.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_pitch_canvas.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_pitch_canvas.draw.connect(_draw_pitch_scene)
	_viewport_clip.add_child(_pitch_canvas)

	match_engine = _resolve_match_engine()
	if not is_instance_valid(match_engine):
		push_error("MatchPitch: MatchEngine node missing in scene tree.")
		return

	if match_engine.has_method("set_home_auto_shoot_enabled"):
		match_engine.set_home_auto_shoot_enabled(false)

	var field_sz: Vector2 = match_engine.get_field_size()
	if field_sz.x > 1.0 and field_sz.y > 1.0:
		_cam_focus_logical = field_sz * 0.5

	if not match_engine.is_formations_loaded() and not match_engine.load_default_formations():
		push_error("MatchPitch: CRITICAL — formations.csv failed to load!")
		return

	var available: PackedStringArray = match_engine.get_available_formations()
	var home_fmt: String = "4-3-3" if match_engine.has_formation("4-3-3") else (available[0] if available.size() > 0 else "4-3-3")
	var away_fmt: String = "4-4-2" if match_engine.has_formation("4-4-2") else (available[0] if available.size() > 0 else "4-4-2")

	match_engine.set_home_formation(home_fmt)
	match_engine.set_away_formation(away_fmt)

	match_engine.minute_changed.connect(_on_minute_changed)
	match_engine.goal_scored.connect(_on_goal_scored)
	match_engine.match_ended.connect(_on_match_ended)
	match_engine.chance_state_changed.connect(_on_chance_state_changed)

	match_engine.start_match()
	reset_for_new_match()

func _resolve_match_engine() -> MatchEngine:
	var candidate: Node = get_node_or_null("../../MatchEngine")
	if candidate is MatchEngine:
		return candidate as MatchEngine
	if get_tree() and get_tree().root:
		var found: Node = get_tree().root.find_child("MatchEngine", true, false)
		if found is MatchEngine:
			return found as MatchEngine
	return null

func _get_hud() -> Node:
	return get_node_or_null("../HUD")

func _is_regression_test_running() -> bool:
	var test_node: Node = get_node_or_null("../../IntegrationRegressionTest")
	if test_node != null and bool(test_node.get("_is_running")):
		return true
	return false

func _is_tactical_overlay_open() -> bool:
	var hud_node: Node = _get_hud()
	if hud_node != null and hud_node.has_method("is_tactical_screen_open"):
		return bool(hud_node.is_tactical_screen_open())
	return false

func _set_tempo_factor(factor: float) -> void:
	var hud_node: Node = _get_hud()
	if hud_node != null and hud_node.has_method("set_temporary_tempo_factor"):
		hud_node.set_temporary_tempo_factor(factor)

func reset_for_new_match() -> void:
	_ball_trail.clear()
	_pointer_down = false
	_gesture_mode = "NONE"
	_preview_through_runner = -1
	_gk_rush_active = false
	_atk_phase = AttackPhase.INACTIVE
	_atk_decision_paused = false
	_open_play_chance_timer = 0.0
	_goal_settling = false
	_goal_settle_timer = 0.0
	_waiting_for_goal_resume = false
	_set_tempo_factor(1.0)

	if is_instance_valid(match_engine):
		if match_engine.has_method("set_home_auto_shoot_enabled"):
			match_engine.set_home_auto_shoot_enabled(false)

		var field_sz: Vector2 = match_engine.get_field_size()
		if field_sz.x > 1.0 and field_sz.y > 1.0:
			_cam_focus_logical = field_sz * 0.5
		_cam_zoom = 1.0

		if not _is_regression_test_running():
			_waiting_for_kickoff = true
			match_engine.pause_match(true)
		else:
			_waiting_for_kickoff = false

	var hud_node: Node = _get_hud()
	if hud_node != null and hud_node.has_method("refresh_all"):
		hud_node.refresh_all()

func is_waiting_for_kickoff_or_resume() -> bool:
	return _waiting_for_kickoff or _waiting_for_goal_resume

func confirm_kickoff_or_resume() -> bool:
	if not is_waiting_for_kickoff_or_resume():
		return false
	if not is_instance_valid(match_engine) or not match_engine.is_match_active():
		_waiting_for_kickoff = false
		_waiting_for_goal_resume = false
		return false

	var was_kickoff: bool = _waiting_for_kickoff
	_waiting_for_kickoff = false
	_waiting_for_goal_resume = false
	_goal_settling = false
	_open_play_chance_timer = 0.0
	_set_tempo_factor(1.0)
	match_engine.pause_match(false)

	var hud_node: Node = _get_hud()
	if hud_node != null:
		if hud_node.has_method("refresh_all"):
			hud_node.refresh_all()
		if hud_node.has_method("show_toast"):
			hud_node.show_toast("KICK-OFF!" if was_kickoff else "PLAY RESUMED!", 1.6)
	return true

func get_right_panel_width() -> float:
	var vp_w: float = get_viewport_rect().size.x
	return clampf(vp_w * 0.265, RIGHT_PANEL_MIN_W, RIGHT_PANEL_MAX_W)

func get_base_pitch_rect() -> Rect2:
	var logical_size: Vector2 = match_engine.get_field_size() if is_instance_valid(match_engine) else Vector2(1050.0, 680.0)
	if logical_size.x <= 1.0 or logical_size.y <= 1.0:
		logical_size = Vector2(1050.0, 680.0)

	var total_logical: Vector2 = logical_size + PITCH_APRONS_LOGICAL * 2.0
	var viewport_size: Vector2 = get_viewport_rect().size
	var top_pad: float = 72.0
	var bottom_pad: float = 96.0
	var left_pad: float = 16.0
	var right_reserved: float = get_right_panel_width() + 26.0

	var avail_w: float = maxf(260.0, viewport_size.x - left_pad - right_reserved)
	var avail_h: float = maxf(200.0, viewport_size.y - top_pad - bottom_pad)

	var scale_factor: float = minf(avail_w / total_logical.x, avail_h / total_logical.y)
	var draw_size: Vector2 = total_logical * scale_factor
	var offset_x: float = left_pad + (avail_w - draw_size.x) * 0.5
	var offset_y: float = top_pad + (avail_h - draw_size.y) * 0.5
	return Rect2(Vector2(offset_x, offset_y), draw_size)

func is_attack_decision_paused() -> bool:
	return _atk_decision_paused and _atk_phase == AttackPhase.DECIDING

func is_gk_rush_active() -> bool:
	return _gk_rush_active

func toggle_gk_rush() -> bool:
	if not is_instance_valid(match_engine) or match_engine.get_chance_state() != 2:
		_gk_rush_active = false
		return false
	_gk_rush_active = not _gk_rush_active
	if _gk_rush_active:
		match_engine.set_controlled_player(0)
		var positions: Array = match_engine.get_player_positions()
		if positions.size() > 0:
			_show_marker(Vector2(positions[0]), Color(1.0, 0.78, 0.18))
	else:
		match_engine.select_nearest_defender()
	return _gk_rush_active

func continue_attack_play() -> void:
	if _atk_phase == AttackPhase.DECIDING and is_instance_valid(match_engine):
		var ctrl_idx: int = _ensure_home_ball_control()
		var positions: Array = match_engine.get_player_positions()
		var field_sz: Vector2 = match_engine.get_field_size()
		if ctrl_idx >= 0 and ctrl_idx < positions.size():
			var p := Vector2(positions[ctrl_idx])
			var advance_target := Vector2(minf(field_sz.x - 30.0, p.x + 65.0), p.y)
			match_engine.command_move_player(advance_target)
			_atk_dribble_target = advance_target
			_set_attack_decision_paused(false)
			_atk_phase = AttackPhase.EXECUTING_DRIBBLE
			_atk_phase_timer = 0.0

func _set_attack_decision_paused(should_pause: bool) -> void:
	if _is_regression_test_running() or _is_tactical_overlay_open():
		_atk_decision_paused = false
		return
	_atk_decision_paused = should_pause
	if is_instance_valid(match_engine) and match_engine.is_match_active():
		match_engine.pause_match(should_pause)

func _on_chance_state_changed(new_state: int, _ctrl_idx: int) -> void:
	_pointer_down = false
	_gesture_mode = "NONE"
	_preview_through_runner = -1
	_gk_rush_active = false
	_open_play_chance_timer = 0.0

	if _waiting_for_kickoff or _goal_settling or _waiting_for_goal_resume:
		return

	if new_state == 1:
		_atk_phase = AttackPhase.SETTLING
		_atk_phase_timer = 0.0
		_atk_expected_receiver = -1
		if not _is_regression_test_running() and not _is_tactical_overlay_open():
			match_engine.pause_match(false)
	elif new_state == 2:
		_atk_phase = AttackPhase.INACTIVE
		_atk_decision_paused = false
		if not _is_regression_test_running() and not _is_tactical_overlay_open():
			match_engine.pause_match(false)
	else:
		_atk_phase = AttackPhase.INACTIVE
		if _atk_decision_paused and not _is_tactical_overlay_open() and is_instance_valid(match_engine):
			match_engine.pause_match(false)
		_atk_decision_paused = false

func _process(delta: float) -> void:
	if _target_marker_timer > 0.0:
		_target_marker_timer = maxf(0.0, _target_marker_timer - delta)
	if _slide_tackle_timer > 0.0:
		_slide_tackle_timer = maxf(0.0, _slide_tackle_timer - delta)

	if _is_regression_test_running() and (is_waiting_for_kickoff_or_resume() or _goal_settling):
		_waiting_for_kickoff = false
		_waiting_for_goal_resume = false
		_goal_settling = false
		_set_tempo_factor(1.0)

	var vp_size: Vector2 = get_viewport_rect().size
	if _backdrop_canvas:
		_backdrop_canvas.position = Vector2.ZERO
		_backdrop_canvas.size = vp_size
		_backdrop_canvas.queue_redraw()

	var base_rect: Rect2 = get_base_pitch_rect()
	if _viewport_clip:
		_viewport_clip.position = base_rect.position
		_viewport_clip.size = base_rect.size

	if is_instance_valid(match_engine) and match_engine.is_match_active():
		if _goal_settling:
			if not match_engine.is_match_paused() and not _is_tactical_overlay_open():
				_goal_settle_timer += delta
				if _goal_settle_timer >= GOAL_SETTLE_DURATION:
					_goal_settling = false
					_waiting_for_goal_resume = true
					_set_tempo_factor(1.0)
					match_engine.pause_match(true)
					var hud_node: Node = _get_hud()
					if hud_node != null and hud_node.has_method("refresh_all"):
						hud_node.refresh_all()
		elif not is_waiting_for_kickoff_or_resume():
			var c_state: int = match_engine.get_chance_state()

			if c_state == 1 and _atk_decision_paused and not match_engine.is_match_paused():
				_atk_decision_paused = false
				if _atk_phase == AttackPhase.DECIDING:
					_atk_phase = AttackPhase.EXECUTING_DRIBBLE
					_atk_phase_timer = 0.0

			if not match_engine.is_match_paused():
				var ball_pos: Vector2 = match_engine.get_ball_position()
				if _ball_trail.is_empty() or _ball_trail[_ball_trail.size() - 1].distance_to(ball_pos) > 3.0:
					_ball_trail.append(ball_pos)
					if _ball_trail.size() > MAX_TRAIL_POINTS:
						_ball_trail.remove_at(0)

				if c_state == 0:
					_update_open_play_chance_guarantee(delta)
				elif c_state == 1:
					_update_attack_stop_and_go(delta)
				elif c_state == 2:
					_update_defend_realtime()

	_update_camera_framing(delta)
	if _pitch_canvas:
		_pitch_canvas.queue_redraw()

## Guarantees BOTH Home Attack Chances and Away Defend Chances spawn across wide-range pitch zones
func _update_open_play_chance_guarantee(delta: float) -> void:
	if _is_regression_test_running() or _is_tactical_overlay_open():
		return
	var carrier_idx: int = match_engine.get_ball_carrier_index()
	var field_sz: Vector2 = match_engine.get_field_size()
	var ball_pos: Vector2 = match_engine.get_ball_position()

	if carrier_idx >= 1 and carrier_idx < 11 and ball_pos.x >= field_sz.x * 0.48:
		_open_play_chance_timer += delta
		if _open_play_chance_timer >= 0.35 and match_engine.has_method("trigger_wide_range_chance"):
			_open_play_chance_timer = 0.0
			match_engine.trigger_wide_range_chance(true)
	elif carrier_idx >= 12 and carrier_idx < 22 and ball_pos.x <= field_sz.x * 0.52:
		_open_play_chance_timer += delta
		if _open_play_chance_timer >= 0.45 and match_engine.has_method("trigger_wide_range_chance"):
			_open_play_chance_timer = 0.0
			match_engine.trigger_wide_range_chance(false)
	else:
		_open_play_chance_timer = 0.0

func _draw_stadium_backdrop() -> void:
	if not _backdrop_canvas:
		return
	var vp: Vector2 = _backdrop_canvas.size
	if vp.x <= 1.0 or vp.y <= 1.0:
		return

	_backdrop_canvas.draw_rect(Rect2(Vector2.ZERO, vp), Color(0.026, 0.040, 0.068, 1.0), true)

	var pitch_rect: Rect2 = get_base_pitch_rect()
	var glow_center: Vector2 = pitch_rect.get_center()
	_backdrop_canvas.draw_circle(glow_center, pitch_rect.size.x * 0.56, Color(0.05, 0.14, 0.12, 0.28))
	_backdrop_canvas.draw_circle(glow_center, pitch_rect.size.x * 0.36, Color(0.06, 0.18, 0.14, 0.22))
	_backdrop_canvas.draw_rect(pitch_rect.grow(4.0), Color(0.012, 0.020, 0.036, 0.85), true)

func _has_confirmed_home_possession() -> bool:
	if not is_instance_valid(match_engine):
		return false
	var carrier_idx: int = match_engine.get_ball_carrier_index()
	if carrier_idx < 0 or carrier_idx >= 11:
		return false
	var positions: Array = match_engine.get_player_positions()
	if carrier_idx >= positions.size():
		return false
	var ball_pos: Vector2 = match_engine.get_ball_position()
	return ball_pos.distance_to(Vector2(positions[carrier_idx])) <= 12.5

func _ensure_home_ball_control() -> int:
	if not is_instance_valid(match_engine):
		return -1
	var carrier_idx: int = match_engine.get_ball_carrier_index()
	if carrier_idx >= 0 and carrier_idx < 11:
		match_engine.set_controlled_player(carrier_idx)
		return carrier_idx
	return match_engine.get_controlled_player_index()

func _update_attack_stop_and_go(delta: float) -> void:
	if _is_regression_test_running() or _is_tactical_overlay_open():
		return

	_atk_phase_timer += delta
	var ball_pos: Vector2 = match_engine.get_ball_position()
	var carrier_idx: int = match_engine.get_ball_carrier_index()
	var ctrl_idx: int = match_engine.get_controlled_player_index()
	var positions: Array = match_engine.get_player_positions()

	match _atk_phase:
		AttackPhase.SETTLING:
			if _atk_phase_timer >= 0.12 and (_has_confirmed_home_possession() or _atk_phase_timer >= 0.45):
				_ensure_home_ball_control()
				_atk_phase = AttackPhase.DECIDING
				_atk_phase_timer = 0.0
				_set_attack_decision_paused(true)

		AttackPhase.EXECUTING_PASS:
			var truly_received := false
			if _atk_phase_timer >= 0.15:
				if _atk_expected_receiver >= 0 and _atk_expected_receiver < 11 and _atk_expected_receiver < positions.size():
					var rec_pos := Vector2(positions[_atk_expected_receiver])
					if carrier_idx != _atk_expected_receiver and ball_pos.distance_to(rec_pos) <= 45.0:
						match_engine.set_controlled_player(_atk_expected_receiver)
						match_engine.command_move_player(ball_pos)
					if carrier_idx == _atk_expected_receiver and ball_pos.distance_to(rec_pos) <= 12.5:
						truly_received = true
				elif _has_confirmed_home_possession():
					truly_received = true

			if truly_received or _atk_phase_timer >= 1.85:
				_ensure_home_ball_control()
				_atk_expected_receiver = -1
				_atk_phase = AttackPhase.DECIDING
				_atk_phase_timer = 0.0
				_set_attack_decision_paused(true)

		AttackPhase.EXECUTING_DRIBBLE:
			var reached := false
			if ctrl_idx >= 0 and ctrl_idx < positions.size():
				var p_pos := Vector2(positions[ctrl_idx])
				if p_pos.distance_to(_atk_dribble_target) <= 14.0 and (_has_confirmed_home_possession() or _atk_phase_timer >= 0.35):
					reached = true
			if reached or _atk_phase_timer >= 0.70:
				_ensure_home_ball_control()
				_atk_phase = AttackPhase.DECIDING
				_atk_phase_timer = 0.0
				_set_attack_decision_paused(true)

		AttackPhase.EXECUTING_SHOT:
			if _atk_phase_timer >= 2.2:
				_ensure_home_ball_control()
				_atk_phase = AttackPhase.DECIDING
				_atk_phase_timer = 0.0
				_set_attack_decision_paused(true)

func _update_defend_realtime() -> void:
	if not _gk_rush_active:
		return
	var positions: Array = match_engine.get_player_positions()
	var carrier_idx: int = match_engine.get_ball_carrier_index()
	var target_pos: Vector2 = Vector2(positions[carrier_idx]) if (carrier_idx >= 11 and carrier_idx < positions.size()) else match_engine.get_ball_position()
	match_engine.set_controlled_player(0)
	match_engine.command_move_player(target_pos)

	if positions.size() > 0 and Vector2(positions[0]).distance_to(target_pos) <= 34.0:
		match_engine.command_tackle()

func _update_camera_framing(delta: float) -> void:
	if not is_instance_valid(match_engine):
		return

	var logical_size: Vector2 = match_engine.get_field_size()
	if logical_size.x <= 1.0 or logical_size.y <= 1.0:
		logical_size = Vector2(1050.0, 680.0)

	var target_zoom: float = 1.0
	var target_focus: Vector2 = logical_size * 0.5

	var c_state: int = match_engine.get_chance_state()
	var suppress_zoom: bool = _waiting_for_kickoff or _goal_settling or _waiting_for_goal_resume
	if match_engine.is_match_active() and not suppress_zoom and (c_state == 1 or c_state == 2):
		var ball_pos: Vector2 = match_engine.get_ball_position()
		var positions: Array = match_engine.get_player_positions()
		var ctrl_idx: int = match_engine.get_controlled_player_index()

		var min_pt: Vector2 = ball_pos
		var max_pt: Vector2 = ball_pos

		if ctrl_idx >= 0 and ctrl_idx < positions.size():
			var cp := Vector2(positions[ctrl_idx])
			min_pt = min_pt.min(cp)
			max_pt = max_pt.max(cp)

		# Always include the active goal mouth in frame for BOTH wide-range Attack & Defend chances
		if c_state == 1:
			var goal_top := Vector2(logical_size.x + 12.0, logical_size.y * 0.5 - 75.0)
			var goal_bot := Vector2(logical_size.x + 12.0, logical_size.y * 0.5 + 75.0)
			min_pt = min_pt.min(goal_top)
			max_pt = max_pt.max(goal_bot)
		else:
			var def_goal_top := Vector2(-12.0, logical_size.y * 0.5 - 75.0)
			var def_goal_bot := Vector2(-12.0, logical_size.y * 0.5 + 75.0)
			min_pt = min_pt.min(def_goal_top)
			max_pt = max_pt.max(def_goal_bot)

		for i in range(positions.size()):
			var p := Vector2(positions[i])
			if p.distance_to(ball_pos) <= 300.0:
				min_pt = min_pt.min(p)
				max_pt = max_pt.max(p)

		var focus_x: float = (min_pt.x + max_pt.x) * 0.5
		var focus_y: float = (min_pt.y + max_pt.y) * 0.5

		var span: Vector2 = (max_pt - min_pt) + Vector2(210.0, 220.0)
		var desired_zoom: float = minf(logical_size.x / maxf(460.0, span.x), logical_size.y / maxf(320.0, span.y))
		target_zoom = clampf(desired_zoom, 1.06, 1.58)

		var total_logical: Vector2 = logical_size + PITCH_APRONS_LOGICAL * 2.0
		var half_vis: Vector2 = (total_logical * 0.5) / target_zoom
		target_focus = Vector2(
			clampf(focus_x, half_vis.x - PITCH_APRONS_LOGICAL.x, logical_size.x + PITCH_APRONS_LOGICAL.x - half_vis.x),
			clampf(focus_y, half_vis.y - PITCH_APRONS_LOGICAL.y, logical_size.y + PITCH_APRONS_LOGICAL.y - half_vis.y)
		)

	var zoom_speed: float = 4.2 if (_goal_settling or _waiting_for_goal_resume) else 6.2
	var smooth_t: float = 1.0 - exp(-zoom_speed * delta)
	_cam_zoom = lerpf(_cam_zoom, target_zoom, smooth_t)
	_cam_focus_logical = _cam_focus_logical.lerp(target_focus, smooth_t)

func _logical_to_canvas(logical_pos: Vector2) -> Vector2:
	var base_rect: Rect2 = get_base_pitch_rect()
	var logical_size: Vector2 = match_engine.get_field_size() if is_instance_valid(match_engine) else Vector2(1050.0, 680.0)
	var total_logical: Vector2 = logical_size + PITCH_APRONS_LOGICAL * 2.0
	var base_scale: float = base_rect.size.x / maxf(1.0, total_logical.x)
	var eff_scale: float = base_scale * _cam_zoom
	var canvas_center: Vector2 = base_rect.size * 0.5
	return canvas_center + (logical_pos - _cam_focus_logical) * eff_scale

func _screen_to_pitch(screen_pos: Vector2) -> Vector2:
	var base_rect: Rect2 = get_base_pitch_rect()
	var logical_size: Vector2 = match_engine.get_field_size() if is_instance_valid(match_engine) else Vector2(1050.0, 680.0)
	var total_logical: Vector2 = logical_size + PITCH_APRONS_LOGICAL * 2.0
	var base_scale: float = base_rect.size.x / maxf(1.0, total_logical.x)
	var eff_scale: float = maxf(0.001, base_scale * _cam_zoom)
	var canvas_rel: Vector2 = screen_pos - base_rect.position
	var canvas_center: Vector2 = base_rect.size * 0.5
	return _cam_focus_logical + (canvas_rel - canvas_center) / eff_scale

func _is_goal_directed_shot(carrier_pos: Vector2, drag_vec_pitch: Vector2, _swipe_speed: float) -> bool:
	var field_sz: Vector2 = match_engine.get_field_size()
	if drag_vec_pitch.x <= 12.0:
		return false

	var goal_center := Vector2(field_sz.x, field_sz.y * 0.5)
	var dist_to_goal: float = carrier_pos.distance_to(goal_center)
	if dist_to_goal > 520.0:
		return false

	var to_goal_dir: Vector2 = (goal_center - carrier_pos).normalized()
	var drag_dir: Vector2 = drag_vec_pitch.normalized()
	var alignment: float = drag_dir.dot(to_goal_dir)

	var t_goal: float = (field_sz.x - carrier_pos.x) / maxf(1.0, drag_vec_pitch.x)
	var projected_y: float = carrier_pos.y + drag_vec_pitch.y * t_goal
	var aimed_at_frame: bool = absf(projected_y - goal_center.y) <= 135.0

	var end_pitch: Vector2 = _screen_to_pitch(_drag_current_screen)
	var ended_in_goal: bool = end_pitch.x >= (field_sz.x - 36.0) and absf(end_pitch.y - goal_center.y) <= 90.0

	return ended_in_goal or aimed_at_frame or alignment >= 0.78

func _compute_smart_through_ball(carrier_idx: int, carrier_pos: Vector2, drag_vec_pitch: Vector2) -> Dictionary:
	var field_sz: Vector2 = match_engine.get_field_size()
	var positions: Array = match_engine.get_player_positions()
	var drag_dir: Vector2 = drag_vec_pitch.normalized() if drag_vec_pitch.length() > 1.0 else Vector2.RIGHT

	var best_tm: int = -1
	var best_score: float = -9999.0

	for i in range(1, mini(11, positions.size())):
		if i == carrier_idx:
			continue
		var tm_pos := Vector2(positions[i])
		var to_tm: Vector2 = tm_pos - carrier_pos
		var dist: float = to_tm.length()
		if dist < 24.0 or dist > 440.0:
			continue

		var dir_to_tm: Vector2 = to_tm / dist
		var align: float = drag_dir.dot(dir_to_tm)
		if align < 0.25:
			continue

		var forward_bonus: float = clampf((tm_pos.x - carrier_pos.x) / 260.0, -0.25, 0.55)
		var score: float = (align * 2.2) + forward_bonus - (dist / 520.0)
		if score > best_score:
			best_score = score
			best_tm = i

	var lead_target: Vector2
	if best_tm != -1:
		var runner_pos := Vector2(positions[best_tm])
		var forward_lead: float = clampf(95.0 + absf(drag_vec_pitch.x) * 0.35, 85.0, 175.0)
		var vertical_lead: float = clampf(drag_dir.y * 42.0, -48.0, 48.0)
		lead_target = Vector2(
			clampf(runner_pos.x + forward_lead, 40.0, field_sz.x - 24.0),
			clampf(runner_pos.y + vertical_lead, 24.0, field_sz.y - 24.0)
		)
	else:
		var forward_push: float = maxf(95.0, drag_vec_pitch.x + 85.0)
		var vertical_push: float = clampf(drag_vec_pitch.y * 1.15, -220.0, 220.0)
		lead_target = Vector2(
			clampf(carrier_pos.x + forward_push, 40.0, field_sz.x - 24.0),
			clampf(carrier_pos.y + vertical_push, 24.0, field_sz.y - 24.0)
		)

	return {"runner": best_tm, "target": lead_target}

func _unhandled_input(event: InputEvent) -> void:
	if not is_instance_valid(match_engine) or not match_engine.is_match_active():
		_pointer_down = false
		_gesture_mode = "NONE"
		_preview_through_runner = -1
		return

	if _is_tactical_overlay_open():
		_pointer_down = false
		return

	if is_waiting_for_kickoff_or_resume():
		var clicked_pitch := false
		if event is InputEventMouseButton:
			var mb_start := event as InputEventMouseButton
			if mb_start.button_index == MOUSE_BUTTON_LEFT and mb_start.pressed and get_base_pitch_rect().grow(10.0).has_point(mb_start.position):
				clicked_pitch = true
		elif event is InputEventScreenTouch:
			var touch_start := event as InputEventScreenTouch
			if touch_start.pressed and get_base_pitch_rect().grow(10.0).has_point(touch_start.position):
				clicked_pitch = true

		if clicked_pitch:
			confirm_kickoff_or_resume()
			get_viewport().set_input_as_handled()
		return

	var c_state: int = match_engine.get_chance_state()
	if c_state == 0:
		return
	if match_engine.is_match_paused() and not is_attack_decision_paused():
		return

	if event is InputEventScreenTouch:
		var touch := event as InputEventScreenTouch
		if touch.pressed and not _pointer_down:
			_on_pointer_pressed(touch.position, touch.index)
		elif not touch.pressed and _pointer_down and touch.index == _pointer_index:
			_on_pointer_released(touch.position)
	elif event is InputEventScreenDrag:
		var drag := event as InputEventScreenDrag
		if _pointer_down and drag.index == _pointer_index:
			_on_pointer_moved(drag.position)
	elif event is InputEventMouseButton:
		var mb := event as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_LEFT:
			if mb.pressed and not _pointer_down:
				_on_pointer_pressed(mb.position, -1)
			elif not mb.pressed and _pointer_down and _pointer_index == -1:
				_on_pointer_released(mb.position)
	elif event is InputEventMouseMotion:
		var mm := event as InputEventMouseMotion
		if _pointer_down and _pointer_index == -1:
			_on_pointer_moved(mm.position)

func _on_pointer_pressed(screen_pos: Vector2, ptr_idx: int) -> void:
	if not get_base_pitch_rect().grow(10.0).has_point(screen_pos):
		return

	_pointer_down = true
	_pointer_index = ptr_idx
	_drag_start_screen = screen_pos
	_drag_current_screen = screen_pos
	_drag_start_ms = Time.get_ticks_msec()
	_gesture_mode = "NONE"
	_preview_through_runner = -1
	get_viewport().set_input_as_handled()

func _on_pointer_moved(screen_pos: Vector2) -> void:
	_drag_current_screen = screen_pos
	var drag_vec: Vector2 = _drag_current_screen - _drag_start_screen
	var drag_dist: float = drag_vec.length()
	var elapsed_sec: float = maxf(0.016, float(Time.get_ticks_msec() - _drag_start_ms) / 1000.0)
	var swipe_speed: float = drag_dist / elapsed_sec
	var c_state: int = match_engine.get_chance_state()

	if c_state == 1 and drag_dist >= 20.0:
		var ctrl_idx: int = _ensure_home_ball_control()
		var positions: Array = match_engine.get_player_positions()
		var carrier_pos: Vector2 = Vector2(positions[ctrl_idx]) if (ctrl_idx >= 0 and ctrl_idx < positions.size()) else match_engine.get_ball_position()
		var drag_pitch: Vector2 = _screen_to_pitch(_drag_current_screen) - _screen_to_pitch(_drag_start_screen)

		if _is_goal_directed_shot(carrier_pos, drag_pitch, swipe_speed):
			_gesture_mode = "SHOT"
			_preview_through_runner = -1
		else:
			_gesture_mode = "THROUGH_BALL"
			var tb_info: Dictionary = _compute_smart_through_ball(ctrl_idx, carrier_pos, drag_pitch)
			_preview_through_runner = int(tb_info["runner"])
			_preview_through_target = Vector2(tb_info["target"])
	elif c_state == 2 and drag_dist >= 26.0:
		_gesture_mode = "SLIDE_TACKLE"

func _on_pointer_released(screen_pos: Vector2) -> void:
	_drag_current_screen = screen_pos
	_pointer_down = false
	_pointer_index = -1

	var chance_state: int = match_engine.get_chance_state()
	var drag_vec_screen: Vector2 = _drag_current_screen - _drag_start_screen
	var drag_dist_screen: float = drag_vec_screen.length()
	var elapsed_sec: float = maxf(0.016, float(Time.get_ticks_msec() - _drag_start_ms) / 1000.0)
	var swipe_speed: float = drag_dist_screen / elapsed_sec

	var start_pitch: Vector2 = _screen_to_pitch(_drag_start_screen)
	var end_pitch: Vector2 = _screen_to_pitch(_drag_current_screen)
	var drag_vec_pitch: Vector2 = end_pitch - start_pitch
	var positions: Array = match_engine.get_player_positions()

	# MODE A: ATTACK CHANCE — STOP-AND-GO DECISION GAMEPLAY
	if chance_state == 1:
		var ctrl_idx: int = _ensure_home_ball_control()
		var carrier_pos: Vector2 = Vector2(positions[ctrl_idx]) if (ctrl_idx >= 0 and ctrl_idx < positions.size()) else match_engine.get_ball_position()
		var ball_pos: Vector2 = match_engine.get_ball_position()

		if ctrl_idx >= 0 and ctrl_idx < 11 and ball_pos.distance_to(carrier_pos) > 13.0:
			match_engine.command_move_player(ball_pos)
			_atk_dribble_target = ball_pos
			_set_attack_decision_paused(false)
			_atk_phase = AttackPhase.EXECUTING_DRIBBLE
			_atk_phase_timer = 0.0
			_gesture_mode = "NONE"
			_preview_through_runner = -1
			get_viewport().set_input_as_handled()
			return

		if drag_dist_screen >= 20.0:
			var is_shot: bool = (_gesture_mode == "SHOT") or _is_goal_directed_shot(carrier_pos, drag_vec_pitch, swipe_speed)
			if is_shot:
				var aim_target: Vector2 = carrier_pos + drag_vec_pitch
				var power_ratio: float = clampf(drag_dist_screen / 145.0, 0.55, 1.25)
				if match_engine.command_shot(aim_target, power_ratio):
					_show_marker(aim_target, Color(1.0, 0.42, 0.25))
					_set_attack_decision_paused(false)
					_atk_phase = AttackPhase.EXECUTING_SHOT
					_atk_phase_timer = 0.0
			else:
				var tb_info: Dictionary = _compute_smart_through_ball(ctrl_idx, carrier_pos, drag_vec_pitch)
				var runner_idx: int = int(tb_info["runner"])
				var lead_target: Vector2 = Vector2(tb_info["target"])
				if match_engine.command_through_ball(lead_target):
					_show_marker(lead_target, Color(0.25, 0.85, 1.0))
					_atk_expected_receiver = runner_idx
					_set_attack_decision_paused(false)
					_atk_phase = AttackPhase.EXECUTING_PASS
					_atk_phase_timer = 0.0
			_gesture_mode = "NONE"
			_preview_through_runner = -1
			get_viewport().set_input_as_handled()
			return

		var clicked_tm: int = _find_nearest_player_in_range(end_pitch, positions, 1, 10, 32.0, ctrl_idx)
		if clicked_tm != -1:
			if match_engine.command_pass(clicked_tm):
				_show_marker(Vector2(positions[clicked_tm]), Color(0.30, 0.95, 0.55))
				_atk_expected_receiver = clicked_tm
				_set_attack_decision_paused(false)
				_atk_phase = AttackPhase.EXECUTING_PASS
				_atk_phase_timer = 0.0
			_gesture_mode = "NONE"
			_preview_through_runner = -1
			get_viewport().set_input_as_handled()
			return

		if match_engine.command_move_player(end_pitch):
			_show_marker(end_pitch, Color(0.95, 0.88, 0.30))
			_atk_dribble_target = end_pitch
			_set_attack_decision_paused(false)
			_atk_phase = AttackPhase.EXECUTING_DRIBBLE
			_atk_phase_timer = 0.0

	# MODE B: DEFEND CHANCE — CONTINUOUS REAL-TIME GAMEPLAY
	elif chance_state == 2:
		if drag_dist_screen >= 26.0 or _gesture_mode == "SLIDE_TACKLE":
			_gk_rush_active = false
			var carrier_idx: int = match_engine.get_ball_carrier_index()
			var target_pt: Vector2 = Vector2(positions[carrier_idx]) if (carrier_idx >= 11 and carrier_idx < positions.size()) else match_engine.get_ball_position()
			var ctrl_def: int = match_engine.get_controlled_player_index()
			if ctrl_def < 1 or ctrl_def >= 11 or Vector2(positions[ctrl_def]).distance_to(target_pt) > 70.0:
				ctrl_def = match_engine.select_nearest_defender()
			if ctrl_def >= 1 and ctrl_def < positions.size():
				var def_pos := Vector2(positions[ctrl_def])
				_slide_tackle_pos = def_pos
				_slide_tackle_dir = (target_pt - def_pos).normalized() if def_pos.distance_to(target_pt) > 1.0 else drag_vec_pitch.normalized()
				_slide_tackle_timer = 0.45
				match_engine.command_move_player(target_pt)
				match_engine.command_tackle()
				_show_marker(target_pt, Color(1.0, 0.38, 0.30))
			_gesture_mode = "NONE"
			get_viewport().set_input_as_handled()
			return

		if positions.size() > 0 and end_pitch.distance_to(Vector2(positions[0])) <= 28.0:
			toggle_gk_rush()
			_gesture_mode = "NONE"
			get_viewport().set_input_as_handled()
			return

		var clicked_def: int = _find_nearest_player_in_range(end_pitch, positions, 1, 10, 30.0, -1)
		if clicked_def != -1:
			_gk_rush_active = false
			match_engine.set_controlled_player(clicked_def)
			_show_marker(Vector2(positions[clicked_def]), Color(0.35, 0.82, 1.0))
			_gesture_mode = "NONE"
			get_viewport().set_input_as_handled()
			return

		var opp_carrier: int = match_engine.get_ball_carrier_index()
		if opp_carrier >= 11 and opp_carrier < positions.size():
			var opp_pos := Vector2(positions[opp_carrier])
			if end_pitch.distance_to(opp_pos) <= 32.0:
				_gk_rush_active = false
				var ctrl_d: int = match_engine.get_controlled_player_index()
				if ctrl_d < 1 or ctrl_d >= 11:
					ctrl_d = match_engine.select_nearest_defender()
				if ctrl_d >= 1 and ctrl_d < positions.size():
					if Vector2(positions[ctrl_d]).distance_to(opp_pos) <= 38.0:
						match_engine.command_tackle()
					else:
						match_engine.command_move_player(opp_pos)
					_show_marker(opp_pos, Color(1.0, 0.38, 0.35))
					_gesture_mode = "NONE"
					get_viewport().set_input_as_handled()
					return

		_gk_rush_active = false
		if match_engine.command_move_player(end_pitch):
			_show_marker(end_pitch, Color(0.35, 0.80, 1.0))

	_gesture_mode = "NONE"
	_preview_through_runner = -1
	get_viewport().set_input_as_handled()

func _find_nearest_player_in_range(pitch_pos: Vector2, positions: Array, start_idx: int, end_idx: int, max_radius: float, exclude_idx: int) -> int:
	var best_idx: int = -1
	var best_dist: float = max_radius
	for i in range(start_idx, mini(end_idx + 1, positions.size())):
		if i == exclude_idx:
			continue
		var d: float = pitch_pos.distance_to(Vector2(positions[i]))
		if d < best_dist:
			best_dist = d
			best_idx = i
	return best_idx

func _show_marker(pitch_pos: Vector2, col: Color) -> void:
	_target_marker_pos = pitch_pos
	_target_marker_color = col
	_target_marker_timer = 0.65

func _draw_goal_structure(goal_top_left: Vector2, goal_size: Vector2, is_left_goal: bool, eff_scale: float) -> void:
	var goal_rect := Rect2(goal_top_left, goal_size)
	_pitch_canvas.draw_rect(goal_rect, Color(0.03, 0.13, 0.08, 0.68), true)

	var net_col := Color(0.88, 0.94, 1.0, 0.34)
	var cols: int = 4
	var rows: int = 10
	for c in range(1, cols):
		var gx: float = goal_rect.position.x + (goal_rect.size.x * float(c) / float(cols))
		_pitch_canvas.draw_line(Vector2(gx, goal_rect.position.y), Vector2(gx, goal_rect.end.y), net_col, 1.0)
	for r in range(1, rows):
		var gy: float = goal_rect.position.y + (goal_rect.size.y * float(r) / float(rows))
		_pitch_canvas.draw_line(Vector2(goal_rect.position.x, gy), Vector2(goal_rect.end.x, gy), net_col, 1.0)

	var frame_col := Color(0.98, 0.99, 1.0, 0.98)
	_pitch_canvas.draw_rect(goal_rect, frame_col, false, 3.0)

	var post_x: float = goal_rect.end.x if is_left_goal else goal_rect.position.x
	var top_post := Vector2(post_x, goal_rect.position.y)
	var bot_post := Vector2(post_x, goal_rect.end.y)
	var post_r: float = maxf(3.8, 4.4 * eff_scale)

	_pitch_canvas.draw_circle(top_post, post_r, Color.WHITE)
	_pitch_canvas.draw_arc(top_post, post_r, 0.0, TAU, 16, Color(0.08, 0.12, 0.18), 1.5)
	_pitch_canvas.draw_circle(bot_post, post_r, Color.WHITE)
	_pitch_canvas.draw_arc(bot_post, post_r, 0.0, TAU, 16, Color(0.08, 0.12, 0.18), 1.5)

func _draw_pitch_scene() -> void:
	if not is_instance_valid(match_engine) or not _pitch_canvas:
		return

	var base_rect: Rect2 = get_base_pitch_rect()
	var logical_size: Vector2 = match_engine.get_field_size()
	if base_rect.size.x <= 0.0 or base_rect.size.y <= 0.0 or logical_size.x <= 0.0:
		return

	var total_logical: Vector2 = logical_size + PITCH_APRONS_LOGICAL * 2.0
	var base_scale: float = base_rect.size.x / total_logical.x
	var eff_scale: float = base_scale * _cam_zoom
	var default_font: Font = ThemeDB.fallback_font
	var line_color := Color(1.0, 1.0, 1.0, 0.88)

	var pitch_tl: Vector2 = _logical_to_canvas(Vector2.ZERO)
	var pitch_br: Vector2 = _logical_to_canvas(logical_size)
	var pitch_box := Rect2(pitch_tl, pitch_br - pitch_tl)

	_pitch_canvas.draw_rect(Rect2(Vector2.ZERO, base_rect.size), Color(0.06, 0.36, 0.20), true)
	_pitch_canvas.draw_rect(pitch_box, Color(0.08, 0.44, 0.25), true)
	var stripe_count: int = 12
	var stripe_w: float = pitch_box.size.x / float(stripe_count)
	for s in range(0, stripe_count, 2):
		_pitch_canvas.draw_rect(Rect2(pitch_tl + Vector2(s * stripe_w, 0.0), Vector2(stripe_w, pitch_box.size.y)), Color(0.10, 0.50, 0.29), true)

	_pitch_canvas.draw_rect(pitch_box, line_color, false, 2.2)
	var center: Vector2 = _logical_to_canvas(logical_size * 0.5)
	_pitch_canvas.draw_line(_logical_to_canvas(Vector2(logical_size.x * 0.5, 0.0)), _logical_to_canvas(Vector2(logical_size.x * 0.5, logical_size.y)), line_color, 2.0)
	_pitch_canvas.draw_arc(center, 68.0 * eff_scale, 0.0, TAU, 64, line_color, 2.0)
	_pitch_canvas.draw_circle(center, 3.2 * eff_scale, line_color)

	var box_w: float = logical_size.x * 0.16
	var box_h: float = logical_size.y * 0.55
	var box_y: float = (logical_size.y - box_h) * 0.5
	_pitch_canvas.draw_rect(Rect2(_logical_to_canvas(Vector2(0.0, box_y)), Vector2(box_w, box_h) * eff_scale), line_color, false, 2.0)
	_pitch_canvas.draw_rect(Rect2(_logical_to_canvas(Vector2(logical_size.x - box_w, box_y)), Vector2(box_w, box_h) * eff_scale), line_color, false, 2.0)

	var small_w: float = logical_size.x * 0.055
	var small_h: float = logical_size.y * 0.26
	var small_y: float = (logical_size.y - small_h) * 0.5
	_pitch_canvas.draw_rect(Rect2(_logical_to_canvas(Vector2(0.0, small_y)), Vector2(small_w, small_h) * eff_scale), line_color, false, 2.0)
	_pitch_canvas.draw_rect(Rect2(_logical_to_canvas(Vector2(logical_size.x - small_w, small_y)), Vector2(small_w, small_h) * eff_scale), line_color, false, 2.0)

	var pen_dist: float = logical_size.x * 0.105
	_pitch_canvas.draw_circle(_logical_to_canvas(Vector2(pen_dist, logical_size.y * 0.5)), 2.8 * eff_scale, line_color)
	_pitch_canvas.draw_circle(_logical_to_canvas(Vector2(logical_size.x - pen_dist, logical_size.y * 0.5)), 2.8 * eff_scale, line_color)

	var goal_h: float = 96.0
	var goal_depth: float = 16.0
	var goal_y: float = (logical_size.y - goal_h) * 0.5
	_draw_goal_structure(_logical_to_canvas(Vector2(-goal_depth, goal_y)), Vector2(goal_depth, goal_h) * eff_scale, true, eff_scale)
	_draw_goal_structure(_logical_to_canvas(Vector2(logical_size.x, goal_y)), Vector2(goal_depth, goal_h) * eff_scale, false, eff_scale)

	if _target_marker_timer > 0.0 and _target_marker_pos.x >= 0.0:
		var pulse_r: float = (10.0 + (1.0 - (_target_marker_timer / 0.65)) * 16.0) * eff_scale
		var col := Color(_target_marker_color.r, _target_marker_color.g, _target_marker_color.b, _target_marker_timer / 0.65)
		_pitch_canvas.draw_arc(_logical_to_canvas(_target_marker_pos), pulse_r, 0.0, TAU, 28, col, 2.4)

	if _slide_tackle_timer > 0.0:
		var t_pos: Vector2 = _logical_to_canvas(_slide_tackle_pos)
		var t_end: Vector2 = t_pos + _slide_tackle_dir * (38.0 * eff_scale)
		var alpha: float = _slide_tackle_timer / 0.45
		_pitch_canvas.draw_line(t_pos, t_end, Color(1.0, 0.42, 0.32, alpha), 4.0)
		_pitch_canvas.draw_arc(t_end, 16.0 * eff_scale, 0.0, TAU, 24, Color(1.0, 0.85, 0.30, alpha), 2.2)

	for t in range(_ball_trail.size()):
		var alpha: float = (float(t + 1) / float(_ball_trail.size())) * 0.34
		_pitch_canvas.draw_circle(_logical_to_canvas(_ball_trail[t]), 3.5 * eff_scale, Color(1.0, 0.95, 0.45, alpha))

	var positions: Array = match_engine.get_player_positions()
	var roles: PackedStringArray = match_engine.get_player_roles()
	var carrier_idx: int = match_engine.get_ball_carrier_index()
	var ctrl_idx: int = match_engine.get_controlled_player_index()
	var chance_state: int = match_engine.get_chance_state()

	if not is_waiting_for_kickoff_or_resume() and chance_state == 1 and ctrl_idx >= 0 and ctrl_idx < positions.size():
		var ctrl_canvas: Vector2 = _logical_to_canvas(Vector2(positions[ctrl_idx]))
		var pass_targets: Array = match_engine.get_valid_pass_targets()
		for target_var in pass_targets:
			var t_idx: int = int(target_var)
			if t_idx >= 0 and t_idx < positions.size():
				var tm_canvas: Vector2 = _logical_to_canvas(Vector2(positions[t_idx]))
				var lane_alpha: float = 0.48 if is_attack_decision_paused() else 0.22
				_pitch_canvas.draw_line(ctrl_canvas, tm_canvas, Color(0.30, 0.95, 0.60, lane_alpha), 2.0 if is_attack_decision_paused() else 1.4)
				_pitch_canvas.draw_arc(tm_canvas, 15.0 * eff_scale, 0.0, TAU, 28, Color(0.30, 0.95, 0.60, 0.85), 2.0)

	if _pointer_down and chance_state == 1 and ctrl_idx >= 0 and ctrl_idx < positions.size():
		var ctrl_canvas: Vector2 = _logical_to_canvas(Vector2(positions[ctrl_idx]))
		var drag_vec: Vector2 = _drag_current_screen - _drag_start_screen
		if drag_vec.length() >= 20.0:
			if _gesture_mode == "SHOT":
				var aim_end: Vector2 = ctrl_canvas + drag_vec
				_pitch_canvas.draw_line(ctrl_canvas, aim_end, Color(1.0, 0.42, 0.25, 0.95), 3.4)
				_pitch_canvas.draw_circle(aim_end, 7.0, Color(1.0, 0.42, 0.25, 0.95))
			else:
				var tb_canvas: Vector2 = _logical_to_canvas(_preview_through_target)
				_pitch_canvas.draw_line(ctrl_canvas, tb_canvas, Color(0.25, 0.88, 1.0, 0.95), 3.0)
				_pitch_canvas.draw_arc(tb_canvas, 12.0 * eff_scale, 0.0, TAU, 24, Color(0.25, 0.88, 1.0, 0.95), 2.4)
				if _preview_through_runner >= 0 and _preview_through_runner < positions.size():
					var run_canvas: Vector2 = _logical_to_canvas(Vector2(positions[_preview_through_runner]))
					_pitch_canvas.draw_line(run_canvas, tb_canvas, Color(1.0, 0.92, 0.30, 0.85), 2.0)
					_pitch_canvas.draw_arc(run_canvas, 16.0 * eff_scale, 0.0, TAU, 28, Color(1.0, 0.92, 0.30, 0.95), 2.4)

	var p_radius: float = maxf(8.0, 10.0 * eff_scale)
	for i in range(positions.size()):
		var pos: Vector2 = _logical_to_canvas(Vector2(positions[i]))
		var is_home: bool = i < 11
		var is_gk: bool = (i == 0 or i == 11)
		var fill_col: Color
		if is_gk:
			fill_col = Color(0.95, 0.74, 0.16) if is_home else Color(0.18, 0.80, 0.68)
		else:
			fill_col = Color(0.06, 0.78, 0.48) if is_home else Color(0.92, 0.26, 0.30)

		if not is_waiting_for_kickoff_or_resume():
			if i == ctrl_idx:
				var ring_col: Color = Color(1.0, 0.90, 0.22) if chance_state == 1 else Color(0.35, 0.82, 1.0)
				_pitch_canvas.draw_arc(pos, p_radius + 6.0, 0.0, TAU, 32, ring_col, 2.8)
			elif i == carrier_idx:
				_pitch_canvas.draw_arc(pos, p_radius + 3.5, 0.0, TAU, 32, Color(1.0, 1.0, 1.0, 0.75), 1.8)

			if i == 0 and _gk_rush_active:
				_pitch_canvas.draw_arc(pos, p_radius + 9.0, 0.0, TAU, 32, Color(1.0, 0.78, 0.18, 0.95), 2.5)

			if chance_state == 2 and i == carrier_idx and not is_home and ctrl_idx >= 1 and ctrl_idx < positions.size():
				if Vector2(positions[ctrl_idx]).distance_to(Vector2(positions[i])) <= 38.0:
					_pitch_canvas.draw_arc(pos, p_radius + 8.0, 0.0, TAU, 32, Color(1.0, 0.35, 0.30, 0.95), 2.5)

		_pitch_canvas.draw_circle(pos, p_radius, fill_col)
		_pitch_canvas.draw_arc(pos, p_radius, 0.0, TAU, 32, Color.WHITE if is_home else Color(0.12, 0.14, 0.18), 1.6)

		var label_text: String = roles[i] if i < roles.size() else ("H" + str(i + 1) if is_home else "A" + str(i - 10))
		_pitch_canvas.draw_string(default_font, pos + Vector2(-20.0, -(p_radius + 4.0)), label_text, HORIZONTAL_ALIGNMENT_CENTER, 40, 10, Color.WHITE)

	var ball_pos: Vector2 = _logical_to_canvas(match_engine.get_ball_position())
	var ball_r: float = maxf(4.5, 5.5 * eff_scale)
	_pitch_canvas.draw_circle(ball_pos, ball_r, Color.WHITE)
	_pitch_canvas.draw_arc(ball_pos, ball_r, 0.0, TAU, 16, Color.BLACK, 1.4)

	if _waiting_for_kickoff or _waiting_for_goal_resume:
		_pitch_canvas.draw_rect(Rect2(Vector2.ZERO, base_rect.size), Color(0.02, 0.04, 0.08, 0.34), true)
		var pulse: float = 0.82 + 0.18 * sin(float(Time.get_ticks_msec()) * 0.006)
		var card_w: float = minf(base_rect.size.x - 40.0, 360.0)
		var card_h: float = 88.0
		var card_pos: Vector2 = (base_rect.size - Vector2(card_w, card_h)) * 0.5
		var card_rect := Rect2(card_pos, Vector2(card_w, card_h))

		var is_goal_card: bool = _waiting_for_goal_resume
		var border_col: Color = Color(1.0, 0.85, 0.25, pulse) if is_goal_card else Color(0.15, 0.94, 0.56, pulse)
		_pitch_canvas.draw_rect(card_rect, Color(0.05, 0.08, 0.14, 0.95), true)
		_pitch_canvas.draw_rect(card_rect, border_col, false, 2.4)

		var sub_title: String = _last_goal_banner_text if is_goal_card else "MATCH READY · 00' KICK-OFF"
		var main_prompt: String = "CLICK TO RESUME" if is_goal_card else "CLICK TO START"
		var sub_col: Color = Color(1.0, 0.88, 0.32) if is_goal_card else Color(0.20, 0.94, 0.58)

		_pitch_canvas.draw_string(default_font, card_pos + Vector2(0.0, 32.0), sub_title, HORIZONTAL_ALIGNMENT_CENTER, int(card_w), 13, sub_col)
		_pitch_canvas.draw_string(default_font, card_pos + Vector2(0.0, 64.0), main_prompt, HORIZONTAL_ALIGNMENT_CENTER, int(card_w), 21, Color.WHITE)
		return

	if _goal_settling:
		var settle_rect := Rect2(Vector2(14.0, 12.0), Vector2(minf(base_rect.size.x - 28.0, 420.0), 32.0))
		_pitch_canvas.draw_rect(settle_rect, Color(0.06, 0.14, 0.22, 0.90), true)
		_pitch_canvas.draw_rect(settle_rect, Color(1.0, 0.85, 0.25, 0.90), false, 1.6)
		_pitch_canvas.draw_string(default_font, settle_rect.position + Vector2(10.0, 21.0), "%s — Returning to center line..." % _last_goal_banner_text, HORIZONTAL_ALIGNMENT_LEFT, int(settle_rect.size.x - 16.0), 11, Color.WHITE)
		return

	# Top-Left Chance Banner with Wide-Range Zone & Distance Indicator for BOTH Attack & Defend
	if chance_state == 1:
		var b_pos: Vector2 = match_engine.get_ball_position()
		var dist_m: int = int(round(b_pos.distance_to(Vector2(logical_size.x, logical_size.y * 0.5)) * 0.1))
		var zone_tag: String = "IN BOX (%dm)" % dist_m if b_pos.x >= (logical_size.x * 0.84) else "OUTSIDE BOX (%dm)" % dist_m

		var banner_rect := Rect2(Vector2(14.0, 12.0), Vector2(minf(base_rect.size.x - 28.0, 510.0), 32.0))
		var b_bg := Color(0.06, 0.24, 0.16, 0.92) if is_attack_decision_paused() else Color(0.06, 0.12, 0.20, 0.86)
		var b_border := Color(1.0, 0.88, 0.25, 0.95) if is_attack_decision_paused() else Color(0.20, 0.92, 0.55, 0.85)
		_pitch_canvas.draw_rect(banner_rect, b_bg, true)
		_pitch_canvas.draw_rect(banner_rect, b_border, false, 1.6)
		var banner_txt: String = "%s · Tap=Pass · Drag=Through · Tap Pitch=Dribble · Swipe Goal=Shoot" % zone_tag if is_attack_decision_paused() else "%s · Executing your move..." % zone_tag
		_pitch_canvas.draw_string(default_font, banner_rect.position + Vector2(10.0, 21.0), banner_txt, HORIZONTAL_ALIGNMENT_LEFT, int(banner_rect.size.x - 16.0), 11, Color.WHITE)
	elif chance_state == 2:
		var b_pos_def: Vector2 = match_engine.get_ball_position()
		var dist_def_m: int = int(round(b_pos_def.distance_to(Vector2(0.0, logical_size.y * 0.5)) * 0.1))
		var def_zone_tag: String = "IN BOX (%dm)" % dist_def_m if b_pos_def.x <= (logical_size.x * 0.16) else "OUTSIDE BOX (%dm)" % dist_def_m

		var def_rect := Rect2(Vector2(14.0, 12.0), Vector2(minf(base_rect.size.x - 28.0, 510.0), 32.0))
		_pitch_canvas.draw_rect(def_rect, Color(0.28, 0.08, 0.10, 0.90), true)
		_pitch_canvas.draw_rect(def_rect, Color(1.0, 0.42, 0.35, 0.90), false, 1.6)
		var def_txt: String = "%s · GK RUSHING OUT! Tap defender to cancel" % def_zone_tag if _gk_rush_active else "%s · Swipe=Slide Tackle · Tap=Block Lane · Tap GK=Rush" % def_zone_tag
		_pitch_canvas.draw_string(default_font, def_rect.position + Vector2(10.0, 21.0), def_txt, HORIZONTAL_ALIGNMENT_LEFT, int(def_rect.size.x - 16.0), 11, Color.WHITE)

func _on_minute_changed(minute: int) -> void:
	if minute % 15 == 0:
		print("Match Time: ", minute, "'")

func _on_goal_scored(home: int, away: int, is_home: bool) -> void:
	_ball_trail.clear()
	_pointer_down = false
	_gesture_mode = "NONE"
	_preview_through_runner = -1
	_gk_rush_active = false
	_atk_phase = AttackPhase.INACTIVE
	_atk_decision_paused = false

	var scorer_side: String = "HOME FC" if is_home else "AWAY FC"
	_last_goal_banner_text = "GOAL! %s  (%d : %d)" % [scorer_side, home, away]

	if not _is_regression_test_running() and is_instance_valid(match_engine) and match_engine.is_match_active():
		_goal_settling = true
		_goal_settle_timer = 0.0
		_waiting_for_goal_resume = false
		_set_tempo_factor(0.58)
		match_engine.pause_match(false)

	print("GOAL! Current Score: ", home, " - ", away, " | Scored By: ", "Home" if is_home else "Away")

func _on_match_ended(final_home: int, final_away: int) -> void:
	_ball_trail.clear()
	_waiting_for_kickoff = false
	_goal_settling = false
	_waiting_for_goal_resume = false
	_set_tempo_factor(1.0)
	print("FULL TIME! Score: ", final_home, " - ", final_away)