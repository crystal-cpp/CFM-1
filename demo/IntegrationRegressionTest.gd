extends Node

@onready var match_scene: Node3D = get_parent()
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
	print("  PHASE 1: 2D MATCH ENGINE & REGRESSION SUITE")
	print("========================================================")

	_test_scene_hierarchy_and_no_3d()
	_test_formations_and_kickoff_bounds()
	await _test_coordinated_2d_simulation()
	_test_ovr_and_tactics_validation()
	await _test_2d_chance_lifecycle()
	await _test_deferred_fulltime_whistle()
	_test_pause_and_resume()
	_test_skip_match_economy()

	# Restore clean playable match state after tests
	match_engine.set_home_formation("4-3-3")
	match_engine.set_away_formation("4-4-2")
	match_engine.set_home_team_ovr(75)
	match_engine.set_away_team_ovr(72)
	match_engine.set_home_mentality(1)
	match_engine.set_away_mentality(1)
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
		hud.show_toast("Phase 1 Suite: %d Passed, %d Failed" % [_passed, _failed], 4.0)

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

	var ball_before: Vector2 = match_engine.get_ball_position()
	var swapped: bool = match_engine.set_home_formation("3-5-2")
	var ball_after: Vector2 = match_engine.get_ball_position()
	_assert(swapped and match_engine.get_home_formation() == "3-5-2", "Valid formation '3-5-2' applied")
	_assert(ball_before.is_equal_approx(ball_after), "Mid-match formation swap preserves live ball position")

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

	# Step 1.2s of live physics simulation
	for _f in range(72):
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
	_assert(match_engine.get_home_mentality() == 2 and match_engine.get_home_pressing() == 0, "Mentality & pressing clamped safely")

func _test_2d_chance_lifecycle() -> void:
	match_engine.start_match()
	await get_tree().process_frame

	match_scene._on_interactive_event_triggered(1, true, Vector3(12.0, 0.0, -5.0))
	await get_tree().process_frame

	_assert(overview_2d.visible, "2DOverviewLayer stays visible during attacking chance")
	_assert(match_scene._chance_banner_label.text == "ATTACK CHANCE", "HUD chance banner displays 'ATTACK CHANCE' without scenario ID")
	_assert(match_engine.is_match_paused(), "MatchEngine paused while 2D chance banner is active")

	match_scene.resolve_active_2d_chance(1)
	match_scene.resolve_active_2d_chance(1)
	await get_tree().process_frame

	_assert(not match_engine.is_match_paused(), "2D simulation resumed cleanly after chance resolution")
	_assert(match_engine.get_home_score() == 1, "Attacking chance resolved once and incremented Home score to 1")

	match_scene._on_interactive_event_triggered(4, false, Vector3(-10.0, 0.0, 3.0))
	await get_tree().process_frame
	_assert(match_scene._chance_banner_label.text == "DEFEND CHANCE", "HUD chance banner displays 'DEFEND CHANCE' without scenario ID")
	match_scene.resolve_active_2d_chance(0)
	await get_tree().process_frame

	_assert(match_engine.get_away_score() == 0, "Successful defensive stop prevented Away goal")

func _test_deferred_fulltime_whistle() -> void:
	match_engine.start_match()
	await get_tree().process_frame

	match_scene._on_interactive_event_triggered(4, false, Vector3(-8.0, 0.0, 4.0))
	await get_tree().process_frame
	match_scene._on_match_ended(2, 1)

	_assert(match_scene.pending_match_ended, "Full-time whistle deferred while 2D chance is active")

	match_scene.resolve_active_2d_chance(0)
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