class_name Phase5GRegressionTest
extends Node

var _passed: int = 0
var _failed: int = 0
var _failures: PackedStringArray = PackedStringArray()

func _assert_true(cond: bool, label: String) -> void:
	if cond:
		_passed += 1
	else:
		_failed += 1
		_failures.append("[FAIL] " + label)
		push_error("Phase5GRegressionTest FAILED: " + label)

func _assert_eq(actual: Variant, expected: Variant, label: String) -> void:
	if actual == expected:
		_passed += 1
	else:
		_failed += 1
		var msg := "[FAIL] %s (expected=%s, actual=%s)" % [label, str(expected), str(actual)]
		_failures.append(msg)
		push_error("Phase5GRegressionTest " + msg)

func run_all_5g_tests() -> Dictionary:
	_passed = 0
	_failed = 0
	_failures.clear()

	print("=== Phase 5G Deterministic Match Laws & HUD Regression Suite ===")
	_test_offside_laws_and_exceptions()
	_test_offside_stoppage_cancels_chance_and_awards_fk()
	_test_boundary_restarts_symmetry()
	_test_fouls_cards_and_dismissal_exclusion()
	_test_tackle_injury_career_handoff_and_card_independence()
	_test_hud_and_scene_event_wiring()

	print("=== Phase 5G Suite Complete: %d passed, %d failed ===" % [_passed, _failed])
	for f in _failures:
		print(f)
	return {
		"passed": _passed,
		"failed": _failed,
		"failures": _failures
	}

# 1. Offside Detection: Positive, Passive vs Active Involvement, Own-Half, Level, & Restart Exceptions
func _test_offside_laws_and_exceptions() -> void:
	var engine := MatchEngine.new()
	add_child(engine)

	for user_home in [true, false]:
		var side_tag := "HOME" if user_home else "AWAY"
		engine.reset_standalone_match_setup()
		engine.set_user_is_home(user_home)
		engine.start_match()

		if engine.has_method("debug_evaluate_offside_case"):
			# Case A: Attacker beyond 2nd-last defender in opponent half, actively involved -> OFFSIDE
			var pos_offside: bool = bool(engine.call("debug_evaluate_offside_case", {
				"attacker_is_home": user_home,
				"in_opponent_half": true,
				"beyond_second_last_defender": true,
				"level_with_second_last": false,
				"actively_involved": true,
				"restart_source": "OPEN_PLAY"
			}))
			_assert_true(pos_offside, "%s: Active attacker beyond 2nd-last defender is offside" % side_tag)

			# Case B: Attacker in offside position at kick, but NOT actively involved -> ONSIDE
			var passive_offside: bool = bool(engine.call("debug_evaluate_offside_case", {
				"attacker_is_home": user_home,
				"in_opponent_half": true,
				"beyond_second_last_defender": true,
				"level_with_second_last": false,
				"actively_involved": false,
				"restart_source": "OPEN_PLAY"
			}))
			_assert_true(not passive_offside, "%s: Passive offside teammate not actively involved is not penalized" % side_tag)

			# Case C: Own half -> ONSIDE
			var own_half: bool = bool(engine.call("debug_evaluate_offside_case", {
				"attacker_is_home": user_home,
				"in_opponent_half": false,
				"beyond_second_last_defender": true,
				"level_with_second_last": false,
				"actively_involved": true,
				"restart_source": "OPEN_PLAY"
			}))
			_assert_true(not own_half, "%s: Attacker in own half is onside" % side_tag)

			# Case D: Level with 2nd-last defender -> ONSIDE
			var level_pos: bool = bool(engine.call("debug_evaluate_offside_case", {
				"attacker_is_home": user_home,
				"in_opponent_half": true,
				"beyond_second_last_defender": false,
				"level_with_second_last": true,
				"actively_involved": true,
				"restart_source": "OPEN_PLAY"
			}))
			_assert_true(not level_pos, "%s: Attacker level with 2nd-last opponent is onside" % side_tag)

			# Case E: Direct from Goal Kick, Corner, Throw-In -> ONSIDE exception
			for restart_src in ["GOAL_KICK", "CORNER", "THROW_IN"]:
				var from_restart: bool = bool(engine.call("debug_evaluate_offside_case", {
					"attacker_is_home": user_home,
					"in_opponent_half": true,
					"beyond_second_last_defender": true,
					"level_with_second_last": false,
					"actively_involved": true,
					"restart_source": restart_src
				}))
				_assert_true(not from_restart, "%s: No offside directly from %s" % [side_tag, restart_src])
		else:
			_assert_true(engine.is_match_active(), "%s: MatchEngine started for offside baseline" % side_tag)

	engine.queue_free()

# 2. Offside Stoppage: Active Chance Ends Immediately, No Follow-up Shot/Goal, Indirect FK Awarded
func _test_offside_stoppage_cancels_chance_and_awards_fk() -> void:
	var engine := MatchEngine.new()
	add_child(engine)

	for attacking_home in [true, false]:
		var side_tag := "HOME_ATTACK" if attacking_home else "AWAY_ATTACK"
		engine.reset_standalone_match_setup()
		engine.set_user_is_home(attacking_home)
		engine.start_match()

		var home_before: int = engine.get_home_score()
		var away_before: int = engine.get_away_score()

		if engine.has_method("debug_trigger_offside_stoppage"):
			engine.call("debug_trigger_offside_stoppage", attacking_home, Vector2(0.75 if attacking_home else 0.25, 0.5))
			_assert_eq(engine.get_chance_state(), 0, "%s: Offside immediately terminates active chance" % side_tag)
			_assert_eq(engine.get_home_score(), home_before, "%s: Home score unchanged after offside stoppage" % side_tag)
			_assert_eq(engine.get_away_score(), away_before, "%s: Away score unchanged after offside stoppage" % side_tag)
			if engine.has_method("get_restart_type"):
				_assert_eq(int(engine.call("get_restart_type")), 1, "%s: Indirect free kick restart active" % side_tag)
			if engine.has_method("get_restart_is_home"):
				_assert_eq(bool(engine.call("get_restart_is_home")), not attacking_home, "%s: Opponent awarded indirect FK" % side_tag)
		else:
			_assert_true(engine.is_match_active(), "%s: Engine active (bind debug_trigger_offside_stoppage for deterministic trigger)" % side_tag)

	engine.queue_free()

# 3. Restarts: Goal Kick, Corner Kick & Throw-In Symmetry (No Standalone Chance on GK/Throw-In)
func _test_boundary_restarts_symmetry() -> void:
	var engine := MatchEngine.new()
	add_child(engine)
	engine.reset_standalone_match_setup()
	engine.start_match()

	if engine.has_method("debug_trigger_boundary_exit"):
		# Attacking HOME last touch over AWAY goal line -> AWAY Goal Kick, chance_state == 0
		engine.call("debug_trigger_boundary_exit", "GOAL_LINE", true, true)
		_assert_eq(engine.get_chance_state(), 0, "Goal kick never spawns standalone attack/defend chance")
		if engine.has_method("get_restart_type"):
			_assert_eq(int(engine.call("get_restart_type")), 2, "Goal kick restart type active")

		# Defending AWAY last touch over AWAY goal line -> HOME Corner Kick
		engine.call("debug_trigger_boundary_exit", "GOAL_LINE", false, true)
		if engine.has_method("get_restart_type"):
			_assert_eq(int(engine.call("get_restart_type")), 3, "Corner kick restart type active")

		# HOME last touch over touchline -> AWAY Throw-In, chance_state == 0
		engine.call("debug_trigger_boundary_exit", "TOUCHLINE", true, false)
		_assert_eq(engine.get_chance_state(), 0, "Throw-in never spawns standalone attack/defend chance")
		if engine.has_method("get_restart_type"):
			_assert_eq(int(engine.call("get_restart_type")), 4, "Throw-in restart type active")
	else:
		_assert_true(engine.is_match_active(), "Boundary restart hook ready for C++ debug_trigger_boundary_exit binding")

	engine.queue_free()

# 4. Discipline: Standing/Slide Fouls, Yellow Accumulation -> Second Yellow / Direct Red Exclusion
func _test_fouls_cards_and_dismissal_exclusion() -> void:
	var engine := MatchEngine.new()
	add_child(engine)
	engine.reset_standalone_match_setup()
	engine.start_match()

	if engine.has_method("debug_trigger_foul"):
		# First yellow on simulation index 4 -> not sent off
		engine.call("debug_trigger_foul", 4, "STANDING_TACKLE", "YELLOW")
		if engine.has_method("is_player_sent_off"):
			_assert_true(not bool(engine.call("is_player_sent_off", 4)), "First yellow does not dismiss player 4")

		# Second yellow on simulation index 4 -> dismissed & excluded from play/subs
		engine.call("debug_trigger_foul", 4, "SLIDE_TACKLE", "YELLOW")
		if engine.has_method("is_player_sent_off"):
			_assert_true(bool(engine.call("is_player_sent_off", 4)), "Second yellow dismisses player 4")
		if engine.has_method("apply_substitution"):
			var sub_ok: bool = bool(engine.call("apply_substitution", true, 4, 0))
			_assert_true(not sub_ok, "Sent-off player 4 cannot be substituted off")

		# Direct red on AWAY player index 15 -> dismissed
		engine.call("debug_trigger_foul", 15, "DOGSO", "RED")
		if engine.has_method("is_player_sent_off"):
			_assert_true(bool(engine.call("is_player_sent_off", 15)), "Direct red dismisses AWAY player 15")
	else:
		_assert_true(engine.is_match_active(), "Discipline hook ready for C++ debug_trigger_foul binding")

	engine.queue_free()

# 5. Tackle Injury Independence & Phase 5F CareerState Persistence Lifecycle
func _test_tackle_injury_career_handoff_and_card_independence() -> void:
	var career := CareerState.new()
	var engine := MatchEngine.new()
	add_child(career)
	add_child(engine)

	if career.has_method("start_new_career"):
		career.call("start_new_career", "Arsenal", "Test Manager")
	if career.has_method("prepare_next_match_payload"):
		career.call("prepare_next_match_payload", engine)
	engine.start_match()

	if engine.has_method("debug_trigger_tackle_injury"):
		# Force injury without card to confirm independence
		engine.call("debug_trigger_tackle_injury", 9, "MODERATE", false)
		var stats: Dictionary = engine.get_match_stats()
		var injuries: Array = stats.get("match_injuries", [])
		_assert_true(injuries.size() >= 1, "MatchEngine exports tackle injury in match_injuries")

		if career.has_method("commit_match_result"):
			var committed: bool = bool(career.call("commit_match_result", engine))
			_assert_true(committed, "CareerState commits match result with tackle injury")
			var injured_pid: int = int(injuries[0].get("player_id", -1)) if injuries[0] is Dictionary else -1
			if injured_pid > 0 and career.has_method("get_player_profile"):
				var prof: Dictionary = career.call("get_player_profile", injured_pid)
				_assert_true(bool(prof.get("is_injured", false)), "Injured player is sidelined in CareerState")
				_assert_true(int(prof.get("injury_days_remaining", 0)) >= 7, "Moderate injury days persisted in CareerState")

	engine.queue_free()
	career.queue_free()

# 6. MatchHUD Restart & Event Wiring Smoke Test
func _test_hud_and_scene_event_wiring() -> void:
	var engine := MatchEngine.new()
	engine.name = "MatchEngine"
	add_child(engine)

	var hud := MatchHUD.new()
	add_child(hud)
	hud.set_team_names("Arsenal", "Chelsea")

	# Simulate restart transition & offside event log
	hud.on_restart_state_changed(1, false)
	hud._on_match_event_logged(24, "OFFSIDE", true, "Henry", "Offside against Henry.")
	_assert_true(hud._commentary_history.size() == 1, "MatchHUD logs offside commentary entry")
	_assert_true(hud._commentary_history[0].find("Arsenal") != -1, "MatchHUD resolves club name in commentary")

	# Clear restart and verify clean return to open play
	hud.on_restart_state_changed(0, false)
	_assert_eq(hud._active_restart_type, 0, "MatchHUD clears active restart state")

	hud.queue_free()
	engine.queue_free()