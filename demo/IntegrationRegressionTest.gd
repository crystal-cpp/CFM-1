extends Node

@onready var match_scene: Node = get_parent()
@onready var match_engine: MatchEngine = $"../MatchEngine"
@onready var overview_2d: CanvasLayer = $"../2DOverviewLayer"
@onready var hud: Control = $"../2DOverviewLayer/HUD"

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
	_is_running = true
	_passed = 0
	_failed = 0

	print("\n========================================================")
	print("  PHASE 2A + 2B: DIRECT CONTROL, TACTICS & UI SUITE")
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
	if hud and hud.has_method("hide_fulltime_summary"):
		hud.hide_fulltime_summary()

	print("--------------------------------------------------------")
	print("  RESULTS: %d PASSED | %d FAILED" % [_passed, _failed])
	print("========================================================\n")
	if hud and hud.has_method("show_toast"):
		hud.show_toast("Phase 2 Suite: %d Passed, %d Failed" % [_passed, _failed], 4.0)

	_is_running = false

func _assert(condition: bool, label: String) -> void:
	if condition:
		_passed += 1
		print("[PASS] ", label)
	else:
		_failed += 1
		push_error("[FAIL] " + label)

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
	match_engine.start_match()
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
	match_engine.start_match()
	match_engine.set_home_team_ovr(99)
	match_engine.set_away_team_ovr(1)

	for _f in range(90):
		if match_engine.is_chance_active():
			match_engine.resolve_manual_scenario(false, match_engine.get_chance_state() == 1)
		await get_tree().physics_frame

	_assert(match_engine.get_home_score() == 0 and match_engine.get_away_score() == 0, "Zero goals awarded outside active ATTACK CHANCE")

func _test_chance_scoring_and_defensive_safety() -> void:
	match_engine.start_match()
	await get_tree().process_frame

	match_engine.begin_chance(true, 9, 0.35)
	_assert(match_engine.get_chance_state() == 1 and match_engine.is_chance_active(), "Home ATTACK CHANCE activated (chance_state == 1)")
	match_engine.resolve_manual_scenario(true, true)
	_assert(match_engine.get_home_score() == 1 and match_engine.get_away_score() == 0, "Home ATTACK CHANCE conversion awarded goal exclusively to Home")
	_assert(match_engine.get_chance_state() == 0 and not match_engine.is_chance_active(), "Chance state reset to CHANCE_NONE after conversion")

	match_engine.begin_chance(false, 20, 0.35)
	_assert(match_engine.get_chance_state() == 2, "Away ATTACK CHANCE / Home DEFEND CHANCE activated (chance_state == 2)")
	match_engine.resolve_manual_scenario(false, false)
	_assert(match_engine.get_home_score() == 1 and match_engine.get_away_score() == 0, "Defensive stop prevented Away goal and awarded zero accidental goals")
	_assert(match_engine.get_chance_state() == 0, "Chance state reset to CHANCE_NONE after defensive stop")

func _test_direct_player_control_commands() -> void:
	match_engine.start_match()
	match_engine.begin_chance(true, 9, 0.30)
	await get_tree().process_frame

	var ctrl_idx: int = match_engine.get_controlled_player_index()
	_assert(ctrl_idx == 9 and match_engine.is_manual_control_active(), "Controlled attacker assigned on ATTACK CHANCE")

	var pos_before := Vector2(match_engine.get_player_positions()[ctrl_idx])
	var target_move := pos_before + Vector2(30.0, 20.0)
	var moved_ok: bool = match_engine.command_move_player(target_move)
	await get_tree().physics_frame
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
	match_engine.start_match()
	for _f in range(60):
		if match_engine.is_chance_active():
			match_engine.resolve_manual_scenario(false, match_engine.get_chance_state() == 1)
		await get_tree().physics_frame
	var low_intensity_stamina: float = match_engine.get_player_stamina()[8]

	match_engine.set_home_mentality(4)
	match_engine.set_home_pressing(2)
	match_engine.start_match()
	for _f in range(60):
		if match_engine.is_chance_active():
			match_engine.resolve_manual_scenario(false, match_engine.get_chance_state() == 1)
		await get_tree().physics_frame
	var high_intensity_stamina: float = match_engine.get_player_stamina()[8]

	_assert(high_intensity_stamina < low_intensity_stamina,
		"High Pressing + Very Attacking drains more stamina (%.2f) than Low Pressing + Very Defensive (%.2f)" % [high_intensity_stamina, low_intensity_stamina])

func _test_fullscreen_tactical_screen() -> void:
	match_engine.set_home_formation("4-3-3")
	match_engine.set_home_mentality(2)
	match_engine.start_match()
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
	match_engine.start_match()
	await get_tree().process_frame

	match_engine.begin_chance(false, 20, 0.30)
	await get_tree().process_frame
	match_scene._on_match_ended(2, 1)

	_assert(match_scene.pending_match_ended, "Full-time whistle deferred while 2D chance is active")

	match_engine.resolve_manual_scenario(false, false)
	await get_tree().process_frame

	_assert(not match_scene.pending_match_ended, "Deferred full-time whistle processed immediately after 2D chance")

func _test_pause_and_resume() -> void:
	match_engine.start_match()
	_assert(not match_engine.is_match_paused(), "Match starts unpaused")

	match_scene.toggle_manual_pause()
	_assert(match_engine.is_match_paused(), "Manual pause froze MatchEngine")

	match_scene.toggle_manual_pause()
	_assert(not match_engine.is_match_paused(), "Manual resume unpaused MatchEngine")

func _test_skip_match_economy() -> void:
	match_engine.set_skip_tokens(1)
	match_engine.start_match()
	_assert(match_engine.can_skip_match(), "can_skip_match() true with 1 token during active match")

	var skipped: bool = match_engine.skip_match()
	_assert(skipped, "skip_match() succeeded")
	_assert(match_engine.get_skip_tokens() == 0, "Skip token decremented to 0")
	_assert(match_engine.get_current_minute() == 90 and not match_engine.is_match_active(), "Match jumped to 90' and ended")
	_assert(not match_engine.can_skip_match(), "can_skip_match() false when tokens == 0")