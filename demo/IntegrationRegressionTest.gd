extends Node

@onready var match_scene: Node = get_parent()
@onready var match_engine: MatchEngine = $"../MatchEngine"
@onready var overview_2d: CanvasLayer = $"../2DOverviewLayer"
@onready var hud: Control = $"../2DOverviewLayer/HUD"
@onready var match_pitch: Node2D = $"../2DOverviewLayer/Pitch2D"

var _passed: int = 0
var _failed: int = 0
var _is_running: bool = false

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey:
		var key := event as InputEventKey
		if key.pressed and not key.is_echo() and key.keycode == KEY_F9:
			run_all_tests()
			get_viewport().set_input_as_handled()

func run_all_tests() -> void:
	if _is_running:
		return
	if is_instance_valid(match_pitch) and match_pitch.has_method("is_waiting_for_kickoff_or_resume") and match_pitch.is_waiting_for_kickoff_or_resume():
		match_pitch.confirm_kickoff_or_resume()
	if is_instance_valid(hud) and hud.has_method("is_tactical_screen_open") and hud.is_tactical_screen_open():
		hud.close_tactical_screen()
	_is_running = true
	add_to_group("regression_test_running")
	Engine.time_scale = 1.0
	if is_instance_valid(match_engine):
		match_engine.pause_match(false)
	_passed = 0
	_failed = 0

	print("\n========================================================")
	print("  PHASE 2A + 2B: DIRECT CONTROL, TACTICS & GESTURE SUITE")
	print("========================================================")

	_test_scene_hierarchy_and_no_3d()
	_test_formations_and_kickoff_bounds()
	await _test_coordinated_2d_simulation()
	_test_ovr_and_tactics_validation()
	await _test_no_scoring_outside_active_chances()
	await _test_chance_scoring_and_defensive_safety()
	await _test_direct_player_control_commands()
	await _test_mentality_pressing_and_stamina_effects()
	await _test_fullscreen_tactical_screen()
	await _test_deferred_fulltime_whistle()
	_test_pause_and_resume()
	_test_skip_match_economy()
	await _test_pointer_lifecycle_and_gestures()

	match_engine.set_home_formation("4-3-3")
	match_engine.set_away_formation("4-4-2")
	match_engine.set_home_team_ovr(75)
	match_engine.set_away_team_ovr(72)
	match_engine.set_home_mentality(2)
	match_engine.set_away_mentality(2)
	match_engine.set_home_pressing(1)
	match_engine.set_away_pressing(1)
	match_engine.set_skip_tokens(1)
	match_engine.start_match()
	if is_instance_valid(match_pitch) and match_pitch.has_method("reset_for_new_match"):
		remove_from_group("regression_test_running")
		_is_running = false
		match_pitch.reset_for_new_match()
	if hud and hud.has_method("hide_fulltime_summary"):
		hud.hide_fulltime_summary()

	print("--------------------------------------------------------")
	print("  RESULTS: %d PASSED | %d FAILED" % [_passed, _failed])
	print("========================================================\n")
	if hud and hud.has_method("show_toast"):
		hud.show_toast("Phase 2 Suite: %d Passed, %d Failed" % [_passed, _failed], 4.0)

	remove_from_group("regression_test_running")
	_is_running = false

func _assert(condition: bool, label: String) -> void:
	if condition:
		_passed += 1
		print("[PASS] ", label)
	else:
		_failed += 1
		push_error("[FAIL] " + label)

func _start_live_match() -> void:
	match_engine.start_match()
	if match_engine.has_method("is_waiting_for_kickoff_resume") and match_engine.is_waiting_for_kickoff_resume():
		match_engine.resume_from_goal_kickoff()
	match_engine.pause_match(false)

func _test_scene_hierarchy_and_no_3d() -> void:
	_assert(is_instance_valid(match_engine), "MatchEngine GDExtension node instantiated")
	_assert(is_instance_valid(overview_2d) and overview_2d.visible, "2DOverviewLayer present and visible")
	_assert(is_instance_valid(hud), "MatchHUD present")
	_assert(get_node_or_null("../3DHighlightLayer") == null, "3DHighlightLayer disconnected from active match scene")
	_assert(get_viewport().get_camera_3d() == null, "No active Camera3D in viewport")

func _test_formations_and_kickoff_bounds() -> void:
	match_engine.start_match()
	_assert(match_engine.is_formations_loaded(), "Formations CSV loaded (" + match_engine.get_loaded_formations_path() + ")")
	var formations: PackedStringArray = match_engine.get_available_formations()
	_assert(formations.size() >= 24, "At least 24 validated formations available (got %d)" % formations.size())
	_assert(match_engine.has_formation("4-3-3") and match_engine.has_formation("4-4-2"), "4-3-3 and 4-4-2 exist in DB")

	var rejected: bool = match_engine.set_home_formation("9-9-9")
	_assert(not rejected and match_engine.get_home_formation() == "4-3-3", "Invalid formation '9-9-9' rejected safely")

	var names_before: PackedStringArray = match_engine.get_player_names()
	var ball_before: Vector2 = match_engine.get_ball_position()
	var swapped: bool = match_engine.set_home_formation("3-5-2")
	var names_after: PackedStringArray = match_engine.get_player_names()
	var ball_after: Vector2 = match_engine.get_ball_position()
	_assert(swapped and match_engine.get_home_formation() == "3-5-2", "Valid formation '3-5-2' applied")
	_assert(ball_before.is_equal_approx(ball_after), "Mid-match formation swap preserves live ball position")
	_assert(names_before == names_after and names_after.size() == 22, "Formation swap preserves starting XI player identities without duplicates")

	var field_size: Vector2 = match_engine.get_field_size()
	var half_x: float = field_size.x * 0.5
	var positions: Array = match_engine.get_player_positions()
	_assert(positions.size() == 22, "Exactly 22 player positions returned")

	var all_in_kickoff_halves := true
	for i in range(positions.size()):
		var p := Vector2(positions[i])
		if p.y < 0.0 or p.y > field_size.y:
			all_in_kickoff_halves = false
		if i < 11 and (p.x < 0.0 or p.x > half_x):
			all_in_kickoff_halves = false
		if i >= 11 and (p.x < half_x or p.x > field_size.x):
			all_in_kickoff_halves = false
	_assert(all_in_kickoff_halves, "Kickoff formation places Home & Away players in respective pitch halves")

func _test_coordinated_2d_simulation() -> void:
	_start_live_match()
	_assert(match_engine.get_possession_team() == 0 and match_engine.get_ball_carrier_index() >= 0, "Kickoff assigns initial possession & ball carrier")

	for _f in range(72):
		if match_engine.is_chance_active():
			match_engine.resolve_manual_scenario(false, match_engine.get_chance_state() == 1)
		await get_tree().physics_frame

	var field_size: Vector2 = match_engine.get_field_size()
	var positions: Array = match_engine.get_player_positions()
	var within_pitch := true
	var min_pair_dist := 9999.0

	for i in range(positions.size()):
		var p := Vector2(positions[i])
		if p.x < 0.0 or p.x > field_size.x or p.y < 0.0 or p.y > field_size.y:
			within_pitch = false
		for j in range(i + 1, positions.size()):
			min_pair_dist = minf(min_pair_dist, p.distance_to(Vector2(positions[j])))

	_assert(within_pitch, "All 22 players stay strictly inside pitch boundaries during live play")
	_assert(min_pair_dist > 10.0, "Player separation prevents overlapping positions (min dist: %.1f)" % min_pair_dist)

	var stats: Dictionary = match_engine.get_match_stats()
	_assert(stats.has("home_possession") and stats.has("home_passes"), "Match telemetry dictionary exposes live stats")

func _test_ovr_and_tactics_validation() -> void:
	match_engine.set_home_team_ovr(150)
	match_engine.set_away_team_ovr(-25)
	_assert(match_engine.get_home_team_ovr() == 99, "Home OVR clamped to upper bound 99")
	_assert(match_engine.get_away_team_ovr() == 1, "Away OVR clamped to lower bound 1")

	match_engine.set_home_mentality(99)
	match_engine.set_home_pressing(-4)
	_assert(match_engine.get_home_mentality() == 4 and match_engine.get_home_pressing() == 0, "5-tier Mentality [0..4] & 3-tier Pressing [0..2] clamped safely")

func _test_no_scoring_outside_active_chances() -> void:
	_start_live_match()
	match_engine.set_home_team_ovr(99)
	match_engine.set_away_team_ovr(1)

	for _f in range(90):
		if match_engine.is_chance_active():
			match_engine.resolve_manual_scenario(false, match_engine.get_chance_state() == 1)
		await get_tree().physics_frame

	_assert(match_engine.get_home_score() == 0 and match_engine.get_away_score() == 0, "Zero goals awarded outside active ATTACK CHANCE")

func _test_chance_scoring_and_defensive_safety() -> void:
	_start_live_match()
	await get_tree().process_frame

	match_engine.begin_chance(true, 9, 0.35)
	_assert(match_engine.get_chance_state() == 1 and match_engine.is_chance_active(), "Home ATTACK CHANCE activated (chance_state == 1)")
	match_engine.resolve_manual_scenario(true, true)
	_assert(match_engine.get_home_score() == 1 and match_engine.get_away_score() == 0, "Home ATTACK CHANCE conversion awarded goal exclusively to Home")
	_assert(match_engine.get_chance_state() == 0 and not match_engine.is_chance_active(), "Chance state reset to CHANCE_NONE after conversion")

	if match_engine.is_waiting_for_kickoff_resume():
		match_engine.resume_from_goal_kickoff()

	match_engine.begin_chance(false, 20, 0.35)
	_assert(match_engine.get_chance_state() == 2, "Away ATTACK CHANCE / Home DEFEND CHANCE activated (chance_state == 2)")
	match_engine.resolve_manual_scenario(false, false)
	_assert(match_engine.get_home_score() == 1 and match_engine.get_away_score() == 0, "Defensive stop prevented Away goal and awarded zero accidental goals")
	_assert(match_engine.get_chance_state() == 0, "Chance state reset to CHANCE_NONE after defensive stop")

func _test_direct_player_control_commands() -> void:
	_start_live_match()
	match_engine.begin_chance(true, 9, 0.30)
	await get_tree().process_frame

	var ctrl_idx: int = match_engine.get_controlled_player_index()
	_assert(ctrl_idx == 9 and match_engine.is_manual_control_active(), "Controlled attacker assigned on ATTACK CHANCE")

	var pos_before := Vector2(match_engine.get_player_positions()[ctrl_idx])
	var target_move := pos_before + Vector2(30.0, 20.0)
	var moved_ok: bool = match_engine.command_move_player(target_move)
	await get_tree().physics_frame
	await get_tree().process_frame
	var pos_after := Vector2(match_engine.get_player_positions()[ctrl_idx])
	_assert(moved_ok and pos_after.distance_to(pos_before) > 0.1 and not pos_after.is_equal_approx(target_move), "command_move_player moves player smoothly without teleporting")

	var valid_targets: Array = match_engine.get_valid_pass_targets()
	_assert(valid_targets.size() > 0, "get_valid_pass_targets() returns available teammates")

	var target_tm: int = int(valid_targets[0])
	var pass_ok: bool = match_engine.command_pass(target_tm)
	_assert(pass_ok and match_engine.get_controlled_player_index() == target_tm, "command_pass executes pass and switches control to receiver")

	match_engine.begin_chance(false, 20, 0.30)
	var def_idx: int = match_engine.select_nearest_defender()
	_assert(def_idx >= 1 and def_idx < 11 and match_engine.get_controlled_player_index() == def_idx, "select_nearest_defender() selects closest Home defender")
	match_engine.resolve_manual_scenario(false, false)

func _test_mentality_pressing_and_stamina_effects() -> void:
	match_engine.set_home_mentality(0)
	match_engine.set_home_pressing(0)
	_start_live_match()
	for _f in range(120):
		if match_engine.is_chance_active():
			match_engine.resolve_manual_scenario(false, match_engine.get_chance_state() == 1)
		await get_tree().physics_frame
	var low_intensity_stamina: float = match_engine.get_player_stamina()[8]

	match_engine.set_home_mentality(4)
	match_engine.set_home_pressing(2)
	_start_live_match()
	for _f in range(120):
		if match_engine.is_chance_active():
			match_engine.resolve_manual_scenario(false, match_engine.get_chance_state() == 1)
		await get_tree().physics_frame
	var high_intensity_stamina: float = match_engine.get_player_stamina()[8]

	_assert(high_intensity_stamina < low_intensity_stamina,
		"High Pressing + Very Attacking drains more stamina (%.2f) than Low Pressing + Very Defensive (%.2f)" % [high_intensity_stamina, low_intensity_stamina])

func _test_fullscreen_tactical_screen() -> void:
	match_engine.set_home_formation("4-3-3")
	match_engine.set_home_mentality(2)
	_start_live_match()
	await get_tree().process_frame

	var min_before: int = match_engine.get_current_minute()
	hud.open_tactical_screen()
	await get_tree().process_frame

	_assert(hud.is_tactical_screen_open() and match_engine.is_match_paused(), "TACTIC button pauses match and opens full-screen tactical UI")

	for _f in range(15):
		await get_tree().physics_frame
	_assert(match_engine.get_current_minute() == min_before, "Match simulation remains frozen while TacticalScreen is open")

	match_engine.set_home_formation("4-2-3-1")
	match_engine.set_home_mentality(4)
	hud.close_tactical_screen()
	await get_tree().process_frame

	_assert(not hud.is_tactical_screen_open() and not match_engine.is_match_paused(), "RESUME MATCH closes TacticalScreen and unpauses match")
	_assert(match_engine.get_home_formation() == "4-2-3-1" and match_engine.get_home_mentality() == 4, "Tactical changes persist after resuming match")

func _test_deferred_fulltime_whistle() -> void:
	_start_live_match()
	await get_tree().process_frame

	match_engine.begin_chance(false, 20, 0.30)
	await get_tree().process_frame
	match_scene._on_match_ended(2, 1)

	_assert(match_scene.pending_match_ended, "Full-time whistle deferred while 2D chance is active")

	match_engine.resolve_manual_scenario(false, false)
	await get_tree().process_frame

	_assert(not match_scene.pending_match_ended, "Deferred full-time whistle processed immediately after 2D chance")

func _test_pause_and_resume() -> void:
	_start_live_match()
	_assert(not match_engine.is_match_paused(), "Match starts unpaused")

	match_scene.toggle_manual_pause()
	_assert(match_engine.is_match_paused(), "Manual pause froze MatchEngine")

	match_scene.toggle_manual_pause()
	_assert(not match_engine.is_match_paused(), "Manual resume unpaused MatchEngine")

func _test_skip_match_economy() -> void:
	match_engine.set_skip_tokens(1)
	_start_live_match()
	_assert(match_engine.can_skip_match(), "can_skip_match() true with 1 token during active match")

	var skipped: bool = match_engine.skip_match()
	_assert(skipped, "skip_match() succeeded")
	_assert(match_engine.get_skip_tokens() == 0, "Skip token decremented to 0")
	_assert(match_engine.get_current_minute() == 90 and not match_engine.is_match_active(), "Match jumped to 90' and ended")
	_assert(not match_engine.can_skip_match(), "can_skip_match() false when tokens == 0")

# =========================================================================
# PHASE 2C: POINTER LIFECYCLE, GESTURES, CAMERA ZOOM & C++ EXECUTION TESTS
# =========================================================================

func _make_mouse_btn(local_pos: Vector2, pressed: bool) -> InputEventMouseButton:
	var ev := InputEventMouseButton.new()
	ev.button_index = MOUSE_BUTTON_LEFT
	ev.pressed = pressed
	ev.position = local_pos
	return ev

func _make_mouse_motion(local_pos: Vector2) -> InputEventMouseMotion:
	var ev := InputEventMouseMotion.new()
	ev.position = local_pos
	return ev

func _make_touch(local_pos: Vector2, pressed: bool, idx: int = 0) -> InputEventScreenTouch:
	var ev := InputEventScreenTouch.new()
	ev.index = idx
	ev.pressed = pressed
	ev.position = local_pos
	return ev

func _make_touch_drag(local_pos: Vector2, idx: int = 0) -> InputEventScreenDrag:
	var ev := InputEventScreenDrag.new()
	ev.index = idx
	ev.position = local_pos
	return ev

func _setup_attack_decision_state(attacker_idx: int = 9) -> void:
	_start_live_match()
	match_engine.begin_chance(true, attacker_idx, 0.32)
	match_pitch._atk_phase = match_pitch.AttackPhase.DECIDING
	match_pitch._atk_phase_timer = 0.0
	match_pitch._atk_decision_paused = true
	match_engine.pause_match(true)

func _test_pointer_lifecycle_and_gestures() -> void:
	if not is_instance_valid(match_pitch):
		_assert(false, "MatchPitch node available for gesture tests")
		return

	# 1. Pointer press does not reset itself when carrier is already selected & _ensure_home_ball_control() is repeated
	_setup_attack_decision_state(9)
	var positions: Array = match_engine.get_player_positions()
	var carrier_local: Vector2 = match_pitch._logical_to_canvas(Vector2(positions[9]))
	match_pitch._on_pitch_gui_input(_make_mouse_btn(carrier_local, true))
	var down_after_press: bool = match_pitch._pointer_down
	match_engine.set_controlled_player(9)
	match_pitch._ensure_home_ball_control()
	match_pitch._ensure_home_ball_control()
	var down_after_repeated_calls: bool = match_pitch._pointer_down
	_assert(down_after_press and down_after_repeated_calls,
		"Pointer press stays active when carrier is already selected and _ensure_home_ball_control() is called repeatedly")

	# 2. Second touch cannot overwrite active gesture & drag remains active until matching release
	var drag_mid: Vector2 = carrier_local + Vector2(36.0, 0.0)
	match_pitch._on_pitch_gui_input(_make_mouse_motion(drag_mid))
	match_pitch._on_pitch_gui_input(_make_touch(carrier_local + Vector2(10.0, 10.0), true, 1))
	var drag_still_active: bool = match_pitch._pointer_down and match_pitch._pointer_index == -1
	match_pitch._on_pitch_gui_input(_make_mouse_btn(drag_mid, false))
	var released_cleanly: bool = not match_pitch._pointer_down
	_assert(drag_still_active and released_cleanly,
		"Drag remains active across motion, ignores secondary touch, and clears cleanly on matching release")

	# 3. Quick tap on open pitch -> Dribble & target movement respected
	_setup_attack_decision_state(9)
	positions = match_engine.get_player_positions()
	var c_pos := Vector2(positions[9])
	var dribble_dest := Vector2(minf(980.0, c_pos.x + 55.0), clampf(c_pos.y + 35.0, 40.0, 640.0))
	var dribble_local: Vector2 = match_pitch._logical_to_canvas(dribble_dest)
	match_pitch._on_pitch_gui_input(_make_mouse_btn(dribble_local, true))
	match_pitch._on_pitch_gui_input(_make_mouse_btn(dribble_local, false))
	var dribble_started: bool = (match_pitch._atk_phase == match_pitch.AttackPhase.EXECUTING_DRIBBLE) and match_engine.is_manual_dribble_active()
	for _f in range(15):
		await get_tree().physics_frame
	var c_pos_after := Vector2(match_engine.get_player_positions()[9])
	_assert(dribble_started and c_pos_after.distance_to(dribble_dest) < c_pos.distance_to(dribble_dest) and match_engine.get_ball_carrier_index() == 9,
		"Quick tap on open pitch triggers dribble, moves carrier toward target, and keeps ball attached")

	# 4. Tap near ball carrier -> Forward dribble
	_setup_attack_decision_state(9)
	positions = match_engine.get_player_positions()
	var start_x: float = Vector2(positions[9]).x
	var self_local: Vector2 = match_pitch._logical_to_canvas(Vector2(positions[9]))
	match_pitch._on_pitch_gui_input(_make_mouse_btn(self_local, true))
	match_pitch._on_pitch_gui_input(_make_mouse_btn(self_local, false))
	var fwd_target_set: bool = match_pitch._atk_dribble_target.x > start_x + 20.0
	for _f in range(12):
		await get_tree().physics_frame
	var end_x: float = Vector2(match_engine.get_player_positions()[9]).x
	_assert(fwd_target_set and end_x > start_x + 5.0,
		"Tap near ball carrier triggers forward dribble (x: %.1f -> %.1f)" % [start_x, end_x])

	# 5. Quick tap on teammate -> Pass, receiver gains possession, next decision pause occurs, no duplicate action
	_setup_attack_decision_state(9)
	positions = match_engine.get_player_positions()
	var valid_tms: Array = match_engine.get_valid_pass_targets()
	var target_tm: int = int(valid_tms[0]) if valid_tms.size() > 0 else 8
	var tm_local: Vector2 = match_pitch._logical_to_canvas(Vector2(positions[target_tm]))
	var passes_before: int = int(match_engine.get_match_stats()["home_passes"])
	match_pitch._on_pitch_gui_input(_make_mouse_btn(tm_local, true))
	match_pitch._on_pitch_gui_input(_make_mouse_btn(tm_local, false))
	# Duplicate release must not fire a second pass
	match_pitch._on_pitch_gui_input(_make_mouse_btn(tm_local, false))
	var passes_after: int = int(match_engine.get_match_stats()["home_passes"])
	var pass_entered_exec: bool = (match_pitch._atk_phase == match_pitch.AttackPhase.EXECUTING_PASS)
	for _f in range(65):
		match_pitch._update_attack_stop_and_go(0.016)
		if match_pitch._atk_phase == match_pitch.AttackPhase.DECIDING:
			break
		await get_tree().physics_frame
	_assert(pass_entered_exec and (passes_after - passes_before) == 1 and match_engine.get_ball_carrier_index() == target_tm and match_pitch._atk_phase == match_pitch.AttackPhase.DECIDING,
		"Quick tap on teammate executes pass once, receiver gains possession, and next DECIDING pause triggers")

	# 6. Slow directional drag (Android Touch) -> Through ball & runner gains possession
	_setup_attack_decision_state(9)
	positions = match_engine.get_player_positions()
	var tb_start_local: Vector2 = match_pitch._logical_to_canvas(Vector2(positions[9]))
	var tb_end_local: Vector2 = tb_start_local + Vector2(65.0, -25.0)
	match_pitch._on_pitch_gui_input(_make_touch(tb_start_local, true, 0))
	match_pitch._drag_start_ms = Time.get_ticks_msec() - 750
	match_pitch._last_drag_sample_ms = Time.get_ticks_msec() - 400
	match_pitch._peak_swipe_speed = 95.0
	match_pitch._on_pitch_gui_input(_make_touch_drag(tb_end_local, 0))
	var tb_mode_ok: bool = (match_pitch._gesture_mode == "THROUGH_BALL")
	match_pitch._on_pitch_gui_input(_make_touch(tb_end_local, false, 0))
	var tb_runner: int = match_pitch._atk_expected_receiver
	var tb_in_flight: bool = (match_engine.get_ball_carrier_index() == -1) and tb_runner >= 1
	for _f in range(75):
		if match_engine.get_ball_carrier_index() == tb_runner:
			break
		await get_tree().physics_frame
	_assert(tb_mode_ok and tb_in_flight and match_engine.get_ball_carrier_index() == tb_runner,
		"Android slow drag triggers through ball into space and runner (#%d) gains possession" % tb_runner)

	# 7. Fast swipe toward goal -> Shot & actual shot outcome
	_setup_attack_decision_state(9)
	positions = match_engine.get_player_positions()
	var shot_start_local: Vector2 = match_pitch._logical_to_canvas(Vector2(positions[9]))
	var shot_end_local: Vector2 = shot_start_local + Vector2(140.0, 0.0)
	var shots_before: int = int(match_engine.get_match_stats()["home_shots"])
	match_pitch._on_pitch_gui_input(_make_mouse_btn(shot_start_local, true))
	match_pitch._drag_start_ms = Time.get_ticks_msec() - 60
	match_pitch._last_drag_sample_ms = Time.get_ticks_msec() - 30
	match_pitch._peak_swipe_speed = 920.0
	match_pitch._on_pitch_gui_input(_make_mouse_motion(shot_end_local))
	var shot_mode_ok: bool = (match_pitch._gesture_mode == "SHOT")
	match_pitch._on_pitch_gui_input(_make_mouse_btn(shot_end_local, false))
	var shots_after: int = int(match_engine.get_match_stats()["home_shots"])
	var shot_exec_ok: bool = (match_pitch._atk_phase == match_pitch.AttackPhase.EXECUTING_SHOT)
	for _f in range(90):
		if match_engine.get_chance_state() == 0 or match_engine.is_waiting_for_kickoff_resume():
			break
		await get_tree().physics_frame
	var outcome_resolved: bool = (match_engine.get_chance_state() == 0 or match_engine.is_waiting_for_kickoff_resume())
	_assert(shot_mode_ok and shot_exec_ok and (shots_after - shots_before) == 1 and outcome_resolved,
		"Fast swipe toward goal executes shot once and resolves to a match outcome")
	if match_engine.is_waiting_for_kickoff_resume():
		match_engine.resume_from_goal_kickoff()

	# 8. Zoomed-in camera coordinate accuracy & unsuccessful command recovery
	_setup_attack_decision_state(9)
	match_pitch._cam_zoom = 1.52
	match_pitch._cam_focus_logical = Vector2(match_engine.get_player_positions()[9])
	positions = match_engine.get_player_positions()
	var zoomed_tms: Array = match_engine.get_valid_pass_targets()
	var zoomed_tm: int = int(zoomed_tms[0]) if zoomed_tms.size() > 0 else 8
	var visible_box := Rect2(Vector2.ZERO, match_pitch.get_base_pitch_rect().size)
	for cand in zoomed_tms:
		var cand_idx: int = int(cand)
		var cand_local: Vector2 = match_pitch._logical_to_canvas(Vector2(positions[cand_idx]))
		if visible_box.has_point(cand_local):
			zoomed_tm = cand_idx
			break
	var zoomed_tm_local: Vector2 = match_pitch._logical_to_canvas(Vector2(positions[zoomed_tm]))
	match_pitch._on_pitch_gui_input(_make_mouse_btn(zoomed_tm_local, true))
	match_pitch._on_pitch_gui_input(_make_mouse_btn(zoomed_tm_local, false))
	var zoomed_pass_ok: bool = (match_pitch._atk_expected_receiver == zoomed_tm) and (match_pitch._atk_phase == match_pitch.AttackPhase.EXECUTING_PASS)
	var invalid_pass_rejected: bool = not match_engine.command_pass(99)
	_assert(zoomed_pass_ok and invalid_pass_rejected,
		"Gestures remain accurate with chance camera zoomed in (1.52x) and invalid commands fail safely")