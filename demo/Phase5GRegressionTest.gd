class_name Phase5GRegressionTest
extends Node

var match_engine: MatchEngine
var _passed: int = 0
var _failed: int = 0

func run_all_5g_tests() -> void:
	_passed = 0
	_failed = 0
	var parent_node: Node = get_parent()
	match_engine = parent_node.get_node_or_null("MatchEngine") as MatchEngine if is_instance_valid(parent_node) else null

	print("\n========================================================")
	print("  PHASE 5G: RESTART, OFFSIDE & DISCIPLINE API SMOKE SUITE")
	print("========================================================")

	_assert(is_instance_valid(match_engine), "MatchEngine is available to the 5G regression runner")
	if not is_instance_valid(match_engine):
		_print_results()
		return

	_assert(match_engine.has_signal("restart_state_changed"), "Restart-state signal is exposed")
	_assert(match_engine.has_signal("match_event_logged"), "Match-event telemetry signal is exposed")
	_assert(match_engine.has_method("get_restart_state"), "Restart-state getter is exposed")
	_assert(match_engine.has_method("is_restart_active"), "Restart-active query is exposed")
	_assert(match_engine.has_method("is_restart_indirect"), "Direct/indirect restart query is exposed")
	_assert(match_engine.has_method("get_match_stats"), "Match statistics API is exposed")
	_assert(
		match_engine.has_method("command_pass")
		and match_engine.has_method("command_through_ball")
		and match_engine.has_method("command_shot"),
		"Pass, through-ball and shot commands are exposed for offside enforcement"
	)

	var restart_state: int = int(match_engine.call("get_restart_state"))
	var restart_active: bool = bool(match_engine.call("is_restart_active"))
	_assert(restart_active == (restart_state != 0), "Restart-active state agrees with restart type")
	if not restart_active:
		_assert(not bool(match_engine.call("is_restart_indirect")), "No active restart is reported as indirect")

	var stats: Dictionary = match_engine.get_match_stats()
	var injuries: Variant = stats.get("match_injuries", null)
	_assert(injuries is Array, "Match telemetry exposes match_injuries as an Array")

	_print_results()
	if is_instance_valid(parent_node) and parent_node.has_node("2DOverviewLayer/HUD"):
		var hud: Node = parent_node.get_node("2DOverviewLayer/HUD")
		if hud.has_method("show_toast"):
			hud.call("show_toast", "Phase 5G API Checks: %d Passed, %d Failed" % [_passed, _failed], 4.0)

func _assert(condition: bool, label: String) -> void:
	if condition:
		_passed += 1
		print("[PASS] ", label)
	else:
		_failed += 1
		push_error("[FAIL] " + label)

func _print_results() -> void:
	print("--------------------------------------------------------")
	print("  RESULTS: %d PASSED | %d FAILED" % [_passed, _failed])
	print("========================================================\n")
