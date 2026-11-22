extends Control

@onready var match_engine: MatchEngine = $"../../../MatchEngine"

const CHANCE_NONE: int = 0
const CHANCE_HOME_ATTACK: int = 1
const CHANCE_AWAY_ATTACK: int = 2

const TAP_MAX_DIST: float = 18.0
const MIN_DRAG_DIST: float = 22.0
const SWIPE_SHOT_SPEED: float = 780.0
const SWIPE_MAX_DURATION: float = 0.42

var is_pointer_down: bool = false
var drag_start_pos: Vector2 = Vector2.ZERO
var drag_current_pos: Vector2 = Vector2.ZERO
var drag_start_msec: int = 0
var peak_pointer_speed: float = 0.0
var last_sample_pos: Vector2 = Vector2.ZERO
var last_sample_msec: int = 0

func _user_attack_chance() -> int:
	return CHANCE_HOME_ATTACK if match_engine.is_user_home() else CHANCE_AWAY_ATTACK

func _user_start_idx() -> int:
	return 0 if match_engine.is_user_home() else 11

func _user_end_idx() -> int:
	return 11 if match_engine.is_user_home() else 22

func _process(_delta: float) -> void:
	if is_pointer_down:
		queue_redraw()

func _unhandled_input(event: InputEvent) -> void:
	if not match_engine or not match_engine.is_match_active() or match_engine.is_match_paused():
		_cancel_drag()
		return

	if match_engine.get_chance_state() != _user_attack_chance():
		_cancel_drag()
		return

	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		var local_pos: Vector2 = get_local_mouse_position()
		if event.pressed:
			_begin_drag(local_pos)
			get_viewport().set_input_as_handled()
		elif is_pointer_down:
			_finish_drag(local_pos)
			get_viewport().set_input_as_handled()

	elif event is InputEventMouseMotion and is_pointer_down:
		_update_drag(get_local_mouse_position())
		get_viewport().set_input_as_handled()

	elif event is InputEventScreenTouch:
		var local_pos: Vector2 = make_input_local(event).position
		if event.pressed:
			_begin_drag(local_pos)
			get_viewport().set_input_as_handled()
		elif is_pointer_down:
			_finish_drag(local_pos)
			get_viewport().set_input_as_handled()

	elif event is InputEventScreenDrag and is_pointer_down:
		_update_drag(make_input_local(event).position)
		get_viewport().set_input_as_handled()

func _begin_drag(pos: Vector2) -> void:
	is_pointer_down = true
	drag_start_pos = pos
	drag_current_pos = pos
	last_sample_pos = pos
	drag_start_msec = Time.get_ticks_msec()
	last_sample_msec = drag_start_msec
	peak_pointer_speed = 0.0
	queue_redraw()

func _update_drag(pos: Vector2) -> void:
	var now_msec: int = Time.get_ticks_msec()
	var dt_sec: float = maxf(0.008, float(now_msec - last_sample_msec) / 1000.0)
	var inst_speed: float = pos.distance_to(last_sample_pos) / dt_sec

	peak_pointer_speed = maxf(peak_pointer_speed * 0.92, inst_speed)
	drag_current_pos = pos
	last_sample_pos = pos
	last_sample_msec = now_msec
	queue_redraw()

func _finish_drag(end_pos: Vector2) -> void:
	drag_current_pos = end_pos
	var drag_vec: Vector2 = drag_current_pos - drag_start_pos
	var drag_dist: float = drag_vec.length()
	var hold_sec: float = maxf(0.016, float(Time.get_ticks_msec() - drag_start_msec) / 1000.0)
	var avg_speed: float = drag_dist / hold_sec
	var effective_speed: float = maxf(avg_speed, peak_pointer_speed * 0.85)

	is_pointer_down = false
	queue_redraw()

	if drag_dist < TAP_MAX_DIST:
		var tapped_teammate: int = _find_teammate_near(end_pos, 28.0)
		if tapped_teammate != -1 and tapped_teammate != match_engine.get_controlled_player_index():
			match_engine.command_pass(tapped_teammate)
		else:
			match_engine.command_move_player(end_pos)
		return

	var is_fast_swipe: bool = (drag_dist >= 30.0 and hold_sec <= SWIPE_MAX_DURATION and effective_speed >= SWIPE_SHOT_SPEED)
	if is_fast_swipe:
		var power_ratio: float = clampf(remap(effective_speed, SWIPE_SHOT_SPEED, 2300.0, 0.72, 1.35), 0.65, 1.35)
		var ball_pos: Vector2 = match_engine.get_ball_position()
		match_engine.command_shot(ball_pos + drag_vec, power_ratio)
		return

	if drag_dist >= MIN_DRAG_DIST:
		var ball_pos: Vector2 = match_engine.get_ball_position()
		match_engine.command_through_ball(ball_pos + drag_vec)

func _cancel_drag() -> void:
	if is_pointer_down:
		is_pointer_down = false
		queue_redraw()

func _find_teammate_near(pos: Vector2, radius: float) -> int:
	var positions: Array = match_engine.get_player_positions()
	var best_idx: int = -1
	var best_dist: float = radius
	for i in range(_user_start_idx() + 1, min(_user_end_idx(), positions.size())):
		var d: float = pos.distance_to(positions[i])
		if d < best_dist:
			best_dist = d
			best_idx = i
	return best_idx

func _draw() -> void:
	if not is_pointer_down or not match_engine:
		return
	if match_engine.get_chance_state() != _user_attack_chance():
		return

	var drag_vec: Vector2 = drag_current_pos - drag_start_pos
	var drag_dist: float = drag_vec.length()
	if drag_dist < 12.0:
		return

	var hold_sec: float = maxf(0.016, float(Time.get_ticks_msec() - drag_start_msec) / 1000.0)
	var avg_speed: float = drag_dist / hold_sec
	var effective_speed: float = maxf(avg_speed, peak_pointer_speed * 0.85)
	var is_fast_swipe: bool = (drag_dist >= 30.0 and hold_sec <= SWIPE_MAX_DURATION and effective_speed >= SWIPE_SHOT_SPEED)

	var ball_pos: Vector2 = match_engine.get_ball_position()
	var field: Vector2 = match_engine.get_field_size()
	var end_point: Vector2 = (ball_pos + drag_vec).clamp(Vector2(18.0, 18.0), field - Vector2(18.0, 18.0))

	if is_fast_swipe:
		var power_ratio: float = clampf(remap(effective_speed, SWIPE_SHOT_SPEED, 2300.0, 0.72, 1.35), 0.65, 1.35)
		var line_color: Color = Color(1.0, 0.32, 0.22, 0.92) if power_ratio > 1.04 else Color(1.0, 0.78, 0.18, 0.90)
		draw_line(ball_pos, end_point, line_color, 4.0, true)
		draw_circle(end_point, 8.0, line_color)
	else:
		var through_color: Color = Color(0.18, 0.92, 1.0, 0.90)
		draw_line(ball_pos, end_point, through_color, 3.5, true)
		draw_arc(end_point, 11.0, 0.0, TAU, 24, through_color, 2.5, true)
		draw_circle(end_point, 4.0, through_color)
