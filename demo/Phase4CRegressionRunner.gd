extends SceneTree

var _runner: CareerRegressionTest
var _started: bool = false

func _initialize() -> void:
	_runner = CareerRegressionTest.new()
	root.add_child(_runner)

func _process(_delta: float) -> bool:
	if _started:
		return false
	_started = true
	await process_frame
	await process_frame
	var result: Dictionary = _runner.run_all_career_tests()
	print("HEADLESS_REGRESSION_RESULT=%d_PASSED_%d_FAILED" % [int(result.get("passed", 0)), int(result.get("failed", 0))])
	quit(0 if int(result.get("failed", 0)) == 0 else 1)
	return false
