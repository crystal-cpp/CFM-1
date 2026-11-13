extends Node

@onready var match_scene: Node3D = get_parent()
@onready var match_engine: MatchEngine = $"../MatchEngine"
@onready var overview_2d: CanvasLayer = $"../2DOverviewLayer"
@onready var hud: Control = $"../2DOverviewLayer/HUD"
@onready var highlight_3d: Node3D = $"../3DHighlightLayer"

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
	print("  PHASE 8: END-TO-END INTEGRATION & REGRESSION SUITE")
	print("========================================================")

	_test_scene_hierarchy()
	_test_formations_and_pitch_bounds()
	_test_ovr_clamping_and_validation()
	await _test_3d_highlight_lifecycle()
	await _test_deferred_fulltime_whistle()
	_test_skip_match_economy()

	# Restore clean playable match state after tests
	match_engine.set_home_formation("4-3-3")
	match_engine.set_away_formation("4-4-2")
	match_engine.set_home_team_ovr(75)
	match_engine.set_away_team_ovr(72)
	match_engine.set_skip_tokens(1)
	match_engine.start_match()
	if hud and hud.has_method("hide_fulltime_summary"):
		hud.hide_fulltime_summary()

	print("--------------------------------------------------------")
	print("  RESULTS: %d PASSED | %d FAILED" % [_passed, _failed])
	print("========================================================\n")
	if hud and hud.has_method("show_toast"):
		hud.show_toast("Regression Suite: %d Passed, %d Failed" % [_passed, _failed], 4.0)

	_is_running = false

func _assert(condition: bool, label: String) -> void:
	if condition:
		_passed += 1
		print("[PASS] ", label)
	else:
		_failed += 1
		push_error("[FAIL] " + label)

func _test_scene_hierarchy() -> void:
	_assert(is_instance_valid(match_engine), "MatchEngine GDExtension node instantiated")
	_assert(is_instance_valid(overview_2d), "2DOverviewLayer present")
	_assert(is_instance_valid(hud), "MatchHUD present")
	_assert(is_instance_valid(highlight_3d), "3DHighlightLayer present")

	var ball_3d := highlight_3d.get_node_or_null("Ball3D") as RigidBody3D
	var ball_col := highlight_3d.get_node_or_null("Ball3D/BallCollision") as CollisionShape3D
	var floor_body := highlight_3d.get_node_or_null("Pitch3D/PitchFloorBody") as StaticBody3D
	_assert(ball_3d != null and ball_col != null and ball_col.shape != null, "Ball3D has valid CollisionShape3D")
	_assert(floor_body != null, "Pitch3D has StaticBody3D floor collider")

func _test_formations_and_pitch_bounds() -> void:
	_assert(match_engine.is_formations_loaded(), "Formations CSV loaded (" + match_engine.get_loaded_formations_path() + ")")
	var formations: PackedStringArray = match_engine.get_available_formations()
	_assert(formations.size() >= 24, "At least 24 validated formations available (got %d)" % formations.size())
	_assert(match_engine.has_formation("4-3-3") and match_engine.has_formation("4-4-2"), "4-3-3 and 4-4-2 exist in DB")

	# Reject invalid formation without overwriting current valid formation
	var rejected: bool = match_engine.set_home_formation("9-9-9")
	_assert(not rejected and match_engine.get_home_formation() == "4-3-3", "Invalid formation '9-9-9' rejected safely")

	# Mid-match formation change should not reset ball position
	var ball_before: Vector2 = match_engine.get_ball_position()
	var swapped: bool = match_engine.set_home_formation("3-5-2")
	var ball_after: Vector2 = match_engine.get_ball_position()
	_assert(swapped and match_engine.get_home_formation() == "3-5-2", "Valid formation '3-5-2' applied")
	_assert(ball_before.is_equal_approx(ball_after), "Mid-match formation swap preserves live ball position")

	# Verify all 22 players stay strictly inside pitch halves
	var field_size: Vector2 = match_engine.get_field_size()
	var half_x: float = field_size.x * 0.5
	var positions: Array = match_engine.get_player_positions()
	_assert(positions.size() == 22, "Exactly 22 player positions returned")

	var all_in_bounds := true
	for i in range(positions.size()):
		var p := Vector2(positions[i])
		if p.y < 0.0 or p.y > field_size.y:
			all_in_bounds = false
		if i < 11 and (p.x < 0.0 or p.x > half_x):
			all_in_bounds = false
		if i >= 11 and (p.x < half_x or p.x > field_size.x):
			all_in_bounds = false
	_assert(all_in_bounds, "All 11 Home & 11 Away players bounded within respective pitch halves")

func _test_ovr_clamping_and_validation() -> void:
	match_engine.set_home_team_ovr(150)
	match_engine.set_away_team_ovr(-25)
	_assert(match_engine.get_home_team_ovr() == 99, "Home OVR clamped to upper bound 99")
	_assert(match_engine.get_away_team_ovr() == 1, "Away OVR clamped to lower bound 1")

func _test_3d_highlight_lifecycle() -> void:
	match_engine.start_match()
	await get_tree().process_frame

	# Trigger attacking scenario (1) via MatchScene handler
	match_scene._on_interactive_event_triggered(1, true, Vector3(12.0, 0.0, -5.0))
	await get_tree().process_frame

	var cam := highlight_3d.get_node_or_null("Camera3D") as Camera3D
	var ball_3d := highlight_3d.get_node_or_null("Ball3D") as RigidBody3D
	var players_3d := highlight_3d.get_node_or_null("Players3D") as Node3D

	_assert(not overview_2d.visible and highlight_3d.visible, "Viewport switched from 2D to 3D during highlight")
	_assert(cam != null and cam.current, "3D Camera3D is active during highlight")
	_assert(ball_3d != null and ball_3d.freeze, "Ball3D physics frozen at spawn surface")
	_assert(players_3d != null and players_3d.get_child_count() == 3, "3D actors spawned (Attacker, Defender, GK)")

	# Resolve highlight manually and verify clean restoration
	highlight_3d.cleanup_highlight()
	match_scene._on_highlight_finished(true, true)
	await get_tree().process_frame

	_assert(overview_2d.visible and not highlight_3d.visible, "Viewport returned cleanly to 2D after highlight")
	_assert(cam != null and not cam.current, "3D Camera3D deactivated after highlight")
	_assert(players_3d != null and players_3d.get_child_count() == 0, "Spawned 3D actors freed on cleanup")
	_assert(match_engine.get_home_score() == 1, "Successful attacking highlight incremented Home score to 1")

func _test_deferred_fulltime_whistle() -> void:
	match_engine.start_match()
	await get_tree().process_frame

	# Enter 3D highlight state, then emit deferred whistle
	match_scene._on_interactive_event_triggered(4, false, Vector3(-8.0, 0.0, 4.0))
	await get_tree().process_frame
	match_scene._on_match_ended(2, 1)

	_assert(match_scene.pending_match_ended, "Full-time whistle deferred while 3D highlight is active")

	# Finish highlight and confirm deferred whistle executes
	highlight_3d.cleanup_highlight()
	match_scene._on_highlight_finished(false, false)
	await get_tree().process_frame

	_assert(not match_scene.pending_match_ended, "Deferred full-time whistle processed immediately after highlight")

func _test_skip_match_economy() -> void:
	match_engine.set_skip_tokens(1)
	match_engine.start_match()
	_assert(match_engine.can_skip_match(), "can_skip_match() true with 1 token during active match")

	var skipped: bool = match_engine.skip_match()
	_assert(skipped, "skip_match() succeeded")
	_assert(match_engine.get_skip_tokens() == 0, "Skip token decremented to 0")
	_assert(match_engine.get_current_minute() == 90 and not match_engine.is_match_active(), "Match jumped to 90' and ended")
	_assert(not match_engine.can_skip_match(), "can_skip_match() false when tokens == 0")