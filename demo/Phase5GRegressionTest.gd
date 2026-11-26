class_name Phase5GRegressionTest
extends Node

var _runner: CareerRegressionTest

func run_all_5g_tests() -> void:
	_runner = CareerRegressionTest.new()
	add_child(_runner)

	print("\n========================================================")
	print("  PHASE 5G: MATCH LAWS, RESTARTS & DISCIPLINE REGRESSION")
	print("========================================================")

	# Reuse the dedicated Phase 5G acceptance checks already maintained in
	# CareerRegressionTest rather than maintaining a second, divergent copy.
	_runner.call("_test_phase_5g_match_laws")

	var passed: int = int(_runner.get("_passed"))
	var failed: int = int(_runner.get("_failed"))
	print("--------------------------------------------------------")
	print("  PHASE 5G RESULTS: %d PASSED | %d FAILED" % [passed, failed])
	print("========================================================\n")

	if is_instance_valid(get_parent()) and get_parent().has_node("2DOverviewLayer/HUD"):
		var hud: Node = get_parent().get_node("2DOverviewLayer/HUD")
		if hud.has_method("show_toast"):
			hud.call("show_toast", "Phase 5G: %d Passed, %d Failed" % [passed, failed], 4.0)

	_runner.queue_free()
	_runner = null
