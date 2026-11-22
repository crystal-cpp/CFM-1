extends Node

var match_engine: MatchEngine
var overview_2d: CanvasLayer
var hud: Control
var match_pitch: Node
var career_shell: CareerShell
var career_state: CareerState
var active_match_context: Dictionary = {}
var pre_match_view: PreMatchView
var post_match_modal: PostMatchModal

var _match_was_skipped: bool = false

var pending_match_ended: bool = false
var cached_final_home: int = 0
var cached_final_away: int = 0

func _ready() -> void:
	add_to_group("match_scene")
	match_engine = get_node_or_null("MatchEngine") as MatchEngine
	if not is_instance_valid(match_engine):
		match_engine = find_child("MatchEngine", true, false) as MatchEngine

	overview_2d = get_node_or_null("2DOverviewLayer") as CanvasLayer
	match_pitch = get_node_or_null("2DOverviewLayer/Pitch2D")
	if not is_instance_valid(match_pitch):
		match_pitch = get_node_or_null("2DOverviewLayer/MatchPitch")

	# The scene node is named "HUD"; MatchHUD is its attached script/class.
	hud = get_node_or_null("2DOverviewLayer/HUD") as Control
	if not is_instance_valid(hud):
		hud = find_child("HUD", true, false) as Control

	if not is_instance_valid(match_engine) or not is_instance_valid(overview_2d):
		push_error("MatchScene: Missing MatchEngine or 2DOverviewLayer child nodes!")
		return

	var existing_shell := _resolve_career_shell()
	if is_instance_valid(existing_shell) and existing_shell.visible:
		overview_2d.hide()
	else:
		overview_2d.show()
	# Do not auto-start here: AppRoot instantiates MatchScene before CareerShell
	# exists. Career fixtures and the standalone Next Match action own kickoff.

	post_match_modal = PostMatchModal.new()
	post_match_modal.name = "PostMatchModal"
	add_child(post_match_modal)
	post_match_modal.continue_requested.connect(_on_post_match_continue)

	match_engine.chance_state_changed.connect(_on_chance_state_changed)
	match_engine.match_ended.connect(_on_match_ended)

	if is_instance_valid(hud):
		if hud.has_signal("skip_requested") and not hud.skip_requested.is_connected(_request_skip_match):
			hud.skip_requested.connect(_request_skip_match)
		if hud.has_signal("next_match_requested") and not hud.next_match_requested.is_connected(_request_next_match):
			hud.next_match_requested.connect(_request_next_match)

	# Directly opened MatchScene instances still support standalone kickoff. AppRoot
	# hides its persistent instance before this deferred check runs.
	call_deferred("_start_standalone_match_if_visible")

func _start_standalone_match_if_visible() -> void:
	if not is_instance_valid(overview_2d) or not overview_2d.visible:
		return
	if is_instance_valid(_resolve_career_shell()):
		return
	if is_instance_valid(match_engine) and not match_engine.is_match_active():
		match_engine.reset_standalone_match_setup()
		match_engine.start_match()
		match_engine.pause_match(true)

func _resolve_career_shell() -> CareerShell:
	if is_instance_valid(career_shell):
		return career_shell
	var parent_node := get_parent()
	if is_instance_valid(parent_node):
		var sibling := parent_node.get_node_or_null("CareerShell") as CareerShell
		if is_instance_valid(sibling):
			career_shell = sibling
			return career_shell
	if get_tree() and get_tree().root:
		var found := get_tree().root.find_child("CareerShell", true, false) as CareerShell
		if is_instance_valid(found):
			career_shell = found
			return career_shell
	return null

func _normalize_club_name(raw_name: String) -> String:
	return raw_name.strip_edges().to_lower()

func _parse_color_safe(val: Variant, fallback: Color) -> Color:
	if val is Color:
		return val as Color
	if val is String:
		var s := (val as String).strip_edges()
		if not s.is_empty():
			return Color.from_string(s, fallback)
	return fallback

func _lineup_matches_club(lineup: Array, target_club_id: int, target_club_name: String) -> bool:
	if lineup.is_empty():
		return false
	var norm_target := _normalize_club_name(target_club_name)
	var id_hits: int = 0
	var name_hits: int = 0
	for item in lineup:
		if not (item is Dictionary):
			continue
		var d := item as Dictionary
		if target_club_id > 0:
			var p_club_id: int = int(d.get("club_id", d.get("team_id", -1)))
			if p_club_id == target_club_id:
				id_hits += 1
		if not norm_target.is_empty():
			var p_club_name := _normalize_club_name(String(d.get("club", d.get("club_name", d.get("team", "")))))
			if not p_club_name.is_empty() and p_club_name == norm_target:
				name_hits += 1
	return id_hits >= 6 or name_hits >= 6

func _lineup_matches_snapshot(lineup: Array, snapshot: Array) -> bool:
	if lineup.size() != 11 or snapshot.size() != 11:
		return false
	var matches: int = 0
	for i in range(11):
		var a: Dictionary = lineup[i] if lineup[i] is Dictionary else {}
		var b: Dictionary = snapshot[i] if snapshot[i] is Dictionary else {}
		var id_a := String(a.get("id", a.get("player_id", ""))).strip_edges()
		var id_b := String(b.get("id", b.get("player_id", ""))).strip_edges()
		if not id_a.is_empty() and id_a == id_b:
			matches += 1
		else:
			var name_a := String(a.get("name", "")).strip_edges()
			var name_b := String(b.get("name", "")).strip_edges()
			if not name_a.is_empty() and name_a == name_b:
				matches += 1
	return matches >= 6

func _unhandled_input(event: InputEvent) -> void:
	if not (event is InputEventKey):
		return

	var key_event := event as InputEventKey
	if not key_event.pressed or key_event.is_echo():
		return

	if not is_instance_valid(overview_2d) or not overview_2d.visible:
		if key_event.keycode == KEY_C:
			_toggle_career_shell()
			get_viewport().set_input_as_handled()
		return

	if key_event.keycode == KEY_T and is_instance_valid(match_engine) and match_engine.is_match_active():
		if hud:
			if hud.has_method("is_tactical_screen_open") and hud.is_tactical_screen_open():
				hud.close_tactical_screen()
			elif hud.has_method("open_tactical_screen"):
				hud.open_tactical_screen()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_C:
		_toggle_career_shell()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_Q:
		var def_state: int = 2 if (not match_engine.has_method("is_user_home") or match_engine.is_user_home()) else 1
		if is_instance_valid(match_engine) and match_engine.get_chance_state() == def_state:
			match_engine.select_nearest_defender()
			get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_E:
		var def_state: int = 2 if (not match_engine.has_method("is_user_home") or match_engine.is_user_home()) else 1
		if is_instance_valid(match_engine) and match_engine.get_chance_state() == def_state:
			match_engine.command_tackle()
			get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_G:
		var def_state: int = 2 if (not match_engine.has_method("is_user_home") or match_engine.is_user_home()) else 1
		if is_instance_valid(match_engine) and match_engine.get_chance_state() == def_state:
			if match_pitch and match_pitch.has_method("toggle_gk_rush"):
				match_pitch.toggle_gk_rush()
			get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_S:
		_request_skip_match()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_R:
		_request_next_match()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_F10:
		var career_test = CareerRegressionTest.new()
		add_child(career_test)
		career_test.run_all_career_tests()
		career_test.queue_free()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_SPACE:
		if match_pitch and match_pitch.has_method("is_attack_decision_paused") and match_pitch.is_attack_decision_paused():
			match_pitch.continue_attack_play()
		else:
			toggle_manual_pause()
		get_viewport().set_input_as_handled()

func toggle_manual_pause() -> void:
	if not is_instance_valid(match_engine) or not match_engine.is_match_active():
		return
	if hud and hud.has_method("is_tactical_screen_open") and hud.is_tactical_screen_open():
		return

	var next_paused: bool = not match_engine.is_match_paused()
	match_engine.pause_match(next_paused)
	if hud and hud.has_method("refresh_all"):
		hud.refresh_all()
	if hud and hud.has_method("_on_pause_btn_pressed"):
		hud._on_pause_btn_pressed()

func open_pre_match_center(p_career_state: CareerState) -> bool:
	if not is_instance_valid(p_career_state):
		return false

	var fixture: Dictionary = p_career_state.get_next_fixture()
	if fixture.is_empty():
		if is_instance_valid(career_shell):
			career_shell.show_toast("No upcoming fixture is available.", 2.5)
		return false

	if is_instance_valid(pre_match_view):
		pre_match_view.queue_free()

	pre_match_view = PreMatchView.new()
	pre_match_view.name = "PreMatchView"
	add_child(pre_match_view)
	pre_match_view.setup(p_career_state, fixture)
	pre_match_view.start_match_requested.connect(func(context: Dictionary) -> void: launch_career_match(p_career_state, context))
	pre_match_view.back_requested.connect(_close_pre_match_center)
	overview_2d.hide()
	return true


func _close_pre_match_center() -> void:
	if is_instance_valid(pre_match_view):
		pre_match_view.queue_free()
	pre_match_view = null
	active_match_context.clear()
	pending_match_ended = false

	# Keep the 2D match CanvasLayer hidden so it cannot render over CareerShell.
	if is_instance_valid(overview_2d):
		overview_2d.hide()

	var shell := _resolve_career_shell()
	if is_instance_valid(shell):
		shell.visible = true
		shell.refresh_all_views()


func launch_career_match(p_career_state: CareerState, context: Dictionary) -> void:
	if not is_instance_valid(match_engine) or not is_instance_valid(p_career_state) or context.is_empty():
		push_error("MatchScene: Cannot launch career match without a valid engine, CareerState, and match context.")
		return

	career_state = p_career_state
	active_match_context = context.duplicate(true)

	var next_fixture: Dictionary = career_state.get_next_fixture() if career_state.has_method("get_next_fixture") else {}
	var managed_club_id: int = int(career_state.get_managed_club_id()) if career_state.has_method("get_managed_club_id") else int(active_match_context.get("managed_club_id", active_match_context.get("user_club_id", -1)))
	var managed_club_name: String = ""
	if career_state.has_method("get_managed_club_name"):
		managed_club_name = String(career_state.get_managed_club_name())
	elif career_state.has_method("get_club_name") and managed_club_id > 0:
		managed_club_name = String(career_state.get_club_name(managed_club_id))
	if managed_club_name.strip_edges().is_empty():
		managed_club_name = String(active_match_context.get("managed_club_name", active_match_context.get("user_club", "")))

	if active_match_context.has("tactics") and career_state.has_method("set_club_tactics") and managed_club_id > 0:
		career_state.set_club_tactics(managed_club_id, active_match_context["tactics"])
	if career_state.has_method("prepare_next_match_payload"):
		if not career_state.prepare_next_match_payload(match_engine):
			push_error("MatchScene: CareerState failed to prepare the selected fixture payload.")
			return
	elif career_state.has_method("apply_context_to_match_engine"):
		career_state.call("apply_context_to_match_engine", match_engine, active_match_context)

	var prepared_home_snapshot: Array = match_engine.get_home_squad_snapshot() if match_engine.has_method("get_home_squad_snapshot") else []
	var prepared_away_snapshot: Array = match_engine.get_away_squad_snapshot() if match_engine.has_method("get_away_squad_snapshot") else []

	# Resolve fixture club IDs and names from MatchEngine, context, and next_fixture
	var fixture_home_id: int = int(match_engine.get_home_club_id()) if match_engine.has_method("get_home_club_id") else -1
	var fixture_away_id: int = int(match_engine.get_away_club_id()) if match_engine.has_method("get_away_club_id") else -1
	if fixture_home_id <= 0:
		fixture_home_id = int(active_match_context.get("home_club_id", next_fixture.get("home_club_id", next_fixture.get("home_id", -1))))
	if fixture_away_id <= 0:
		fixture_away_id = int(active_match_context.get("away_club_id", next_fixture.get("away_club_id", next_fixture.get("away_id", -1))))

	var fixture_home_name: String = String(match_engine.get_home_club_name()) if match_engine.has_method("get_home_club_name") else ""
	if fixture_home_name.strip_edges().is_empty() or fixture_home_name == "HOME FC":
		var ctx_home := String(active_match_context.get("home_club", next_fixture.get("home_club", next_fixture.get("home_name", ""))))
		if not ctx_home.strip_edges().is_empty():
			fixture_home_name = ctx_home
	var fixture_away_name: String = String(match_engine.get_away_club_name()) if match_engine.has_method("get_away_club_name") else ""
	if fixture_away_name.strip_edges().is_empty() or fixture_away_name == "AWAY FC":
		var ctx_away := String(active_match_context.get("away_club", next_fixture.get("away_club", next_fixture.get("away_name", ""))))
		if not ctx_away.strip_edges().is_empty():
			fixture_away_name = ctx_away

	if fixture_home_id > 0 and match_engine.has_method("set_home_club_id"):
		match_engine.set_home_club_id(fixture_home_id)
	if fixture_away_id > 0 and match_engine.has_method("set_away_club_id"):
		match_engine.set_away_club_id(fixture_away_id)
	if not fixture_home_name.strip_edges().is_empty() and match_engine.has_method("set_home_club_name"):
		match_engine.set_home_club_name(fixture_home_name)
	if not fixture_away_name.strip_edges().is_empty() and match_engine.has_method("set_away_club_name"):
		match_engine.set_away_club_name(fixture_away_name)

	# Authoritatively resolve HOME vs AWAY side for the managed club (e.g., FC Barcelona).
	var authoritative_user_is_home: bool = true
	var resolved_side: bool = false
	if managed_club_id > 0 and (managed_club_id == fixture_home_id or managed_club_id == fixture_away_id):
		authoritative_user_is_home = (managed_club_id == fixture_home_id)
		resolved_side = true
	elif not managed_club_name.strip_edges().is_empty():
		var norm_managed := _normalize_club_name(managed_club_name)
		if norm_managed == _normalize_club_name(fixture_home_name):
			authoritative_user_is_home = true
			resolved_side = true
		elif norm_managed == _normalize_club_name(fixture_away_name):
			authoritative_user_is_home = false
			resolved_side = true

	if not resolved_side:
		push_error("MatchScene: Managed club (%d / '%s') does not match fixture HOME (%d / '%s') or AWAY (%d / '%s')." % [
			managed_club_id, managed_club_name, fixture_home_id, fixture_home_name, fixture_away_id, fixture_away_name
		])
		return

	match_engine.set_user_is_home(authoritative_user_is_home)
	active_match_context["user_is_home"] = authoritative_user_is_home
	active_match_context["home_club"] = fixture_home_name
	active_match_context["away_club"] = fixture_away_name

	# Map user/opponent and home/away lineups into authoritative HOME [0..10] and AWAY [11..21] slots.
	var user_lineup: Array = active_match_context.get("user_lineup", [])
	var opponent_lineup: Array = active_match_context.get("opponent_lineup", [])
	var home_lineup: Array = active_match_context.get("home_lineup", [])
	var away_lineup: Array = active_match_context.get("away_lineup", [])

	if user_lineup.size() == 11:
		if authoritative_user_is_home:
			home_lineup = user_lineup
		else:
			away_lineup = user_lineup
	if opponent_lineup.size() == 11:
		if authoritative_user_is_home:
			away_lineup = opponent_lineup
		else:
			home_lineup = opponent_lineup

	# Guard against views that supplied home_lineup/away_lineup in inverted slots when user is AWAY (or HOME)
	if user_lineup.is_empty() and home_lineup.size() == 11 and away_lineup.size() == 11:
		var home_is_managed := _lineup_matches_club(home_lineup, managed_club_id, managed_club_name) \
			or (_lineup_matches_snapshot(home_lineup, prepared_away_snapshot) and not _lineup_matches_snapshot(home_lineup, prepared_home_snapshot))
		var away_is_managed := _lineup_matches_club(away_lineup, managed_club_id, managed_club_name) \
			or (_lineup_matches_snapshot(away_lineup, prepared_home_snapshot) and not _lineup_matches_snapshot(away_lineup, prepared_away_snapshot))
		if (authoritative_user_is_home and away_is_managed and not home_is_managed) \
			or (not authoritative_user_is_home and home_is_managed and not away_is_managed):
			var tmp_lineup := home_lineup
			home_lineup = away_lineup
			away_lineup = tmp_lineup

	if home_lineup.size() != 11 and prepared_home_snapshot.size() == 11:
		home_lineup = prepared_home_snapshot
	if away_lineup.size() != 11 and prepared_away_snapshot.size() == 11:
		away_lineup = prepared_away_snapshot

	active_match_context["home_lineup"] = home_lineup
	active_match_context["away_lineup"] = away_lineup
	if home_lineup.size() != 11 or away_lineup.size() != 11:
		push_error("MatchScene: Refusing to start fixture with incomplete lineups (HOME %d/11, AWAY %d/11)." % [home_lineup.size(), away_lineup.size()])
		return
	if match_engine.has_method("set_home_lineup"):
		match_engine.call("set_home_lineup", home_lineup)
	if match_engine.has_method("set_away_lineup"):
		match_engine.call("set_away_lineup", away_lineup)

	# Verify that the engine's user-side snapshot is the managed club's XI.
	var expected_user_lineup: Array = home_lineup if authoritative_user_is_home else away_lineup
	var actual_user_lineup: Array = match_engine.call("get_user_squad_snapshot") if match_engine.has_method("get_user_squad_snapshot") else []
	if actual_user_lineup.size() != 11:
		push_error("MatchScene: MatchEngine did not expose an 11-player managed squad snapshot.")
		return
	for player_idx in range(11):
		var expected_player: Dictionary = expected_user_lineup[player_idx] if expected_user_lineup[player_idx] is Dictionary else {}
		var actual_player: Dictionary = actual_user_lineup[player_idx] if actual_user_lineup[player_idx] is Dictionary else {}
		var expected_id: String = str(expected_player.get("id", expected_player.get("player_id", "")))
		var actual_id: String = str(actual_player.get("id", actual_player.get("player_id", "")))
		if not expected_id.is_empty() and not actual_id.is_empty() and expected_id != actual_id:
			push_error("MatchScene: Managed squad identity mismatch at slot %d (expected player %s, engine has %s)." % [player_idx, expected_id, actual_id])
			return

	print("[MatchScene] Fixture %s vs %s | Managed club %d (%s) | Side: %s | User XI verified (%d players)." % [
		String(match_engine.get_home_club_name()),
		String(match_engine.get_away_club_name()),
		managed_club_id,
		managed_club_name,
		"HOME" if authoritative_user_is_home else "AWAY",
		actual_user_lineup.size()
	])

	var user_bench: Array = active_match_context.get("user_bench", [])
	var opponent_bench: Array = active_match_context.get("opponent_bench", [])
	var home_bench: Array = active_match_context.get("home_bench", [])
	var away_bench: Array = active_match_context.get("away_bench", [])
	if user_bench.size() > 0:
		if authoritative_user_is_home:
			home_bench = user_bench
		else:
			away_bench = user_bench
	if opponent_bench.size() > 0:
		if authoritative_user_is_home:
			away_bench = opponent_bench
		else:
			home_bench = opponent_bench
	if user_bench.is_empty() and (home_bench.size() > 0 or away_bench.size() > 0):
		var home_bench_is_managed := _lineup_matches_club(home_bench, managed_club_id, managed_club_name)
		var away_bench_is_managed := _lineup_matches_club(away_bench, managed_club_id, managed_club_name)
		if (authoritative_user_is_home and away_bench_is_managed and not home_bench_is_managed) \
			or (not authoritative_user_is_home and home_bench_is_managed and not away_bench_is_managed):
			var tmp_bench := home_bench
			home_bench = away_bench
			away_bench = tmp_bench
	active_match_context["home_bench"] = home_bench
	active_match_context["away_bench"] = away_bench
	if match_engine.has_method("set_home_bench"):
		match_engine.call("set_home_bench", home_bench)
	if match_engine.has_method("set_away_bench"):
		match_engine.call("set_away_bench", away_bench)

	# Reassert side after all lineup/bench setters so the native control slot
	# is initialized against the final fixture ordering.
	match_engine.set_user_is_home(authoritative_user_is_home)

	var home_name: String = String(match_engine.get_home_club_name()) if match_engine.has_method("get_home_club_name") else String(active_match_context.get("home_club", "Home"))
	var away_name: String = String(match_engine.get_away_club_name()) if match_engine.has_method("get_away_club_name") else String(active_match_context.get("away_club", "Away"))

	var kits: Dictionary = active_match_context.get("kits", {})
	var default_home_col := Color(0.06, 0.78, 0.48)
	var default_away_col := Color(0.92, 0.26, 0.30)
	var home_primary: Color = match_engine.get_home_kit_color() if match_engine.has_method("get_home_kit_color") else _parse_color_safe(kits.get("home_primary", default_home_col), default_home_col)
	var fallback_away: Color = match_engine.get_away_kit_color() if match_engine.has_method("get_away_kit_color") else _parse_color_safe(kits.get("away_primary", kits.get("away_default_primary", default_away_col)), default_away_col)
	var away_primary: Color = _parse_color_safe(kits.get("active_away_primary", fallback_away), fallback_away)
	if match_engine.has_method("set_away_kit_color"):
		match_engine.set_away_kit_color(away_primary)
	if is_instance_valid(match_pitch):
		if match_pitch.has_method("set_kit_colors"):
			match_pitch.set_kit_colors(home_primary, away_primary)

	if is_instance_valid(pre_match_view):
		pre_match_view.queue_free()
	pre_match_view = null
	pending_match_ended = false
	overview_2d.show()
	if is_instance_valid(match_pitch):
		if match_pitch.has_method("set_team_names"):
			match_pitch.call(
				"set_team_names",
				match_engine.get_home_club_name(),
				match_engine.get_away_club_name()
			)
		if match_pitch.has_method("set_match_lineups"):
			match_pitch.call(
				"set_match_lineups",
				active_match_context.get("home_lineup", []),
				active_match_context.get("away_lineup", [])
			)

	match_engine.start_match()
	if not match_engine.is_match_active() or match_engine.is_user_home() != authoritative_user_is_home:
		push_error("MatchScene: MatchEngine failed to retain the managed HOME/AWAY side at kickoff.")
		return

	if is_instance_valid(match_pitch):
		if match_pitch.has_method("reset_for_new_match"):
			match_pitch.reset_for_new_match()
		if match_pitch.has_method("refresh_player_names"):
			match_pitch.call("refresh_player_names")
		match_pitch.queue_redraw()

	if hud:
		if hud.has_method("set_team_names"):
			hud.set_team_names(home_name, away_name)
		if hud.has_method("hide_fulltime_summary"):
			hud.hide_fulltime_summary()
		if hud.has_method("refresh_all"):
			hud.refresh_all()
		if hud.has_method("show_toast"):
			hud.show_toast("KICK OFF! Career Match Started", 2.0)


func _on_post_match_continue(_summary: Dictionary) -> void:
	active_match_context.clear()
	if is_instance_valid(overview_2d):
		overview_2d.hide()
	var shell := _resolve_career_shell()
	if is_instance_valid(shell):
		shell.visible = true
		shell.refresh_all_views()

func _request_skip_match() -> void:
	if not is_instance_valid(match_engine) or not match_engine.is_match_active():
		return
	if match_engine.is_chance_active() or match_engine.is_match_paused():
		return

	if match_engine.can_skip_match():
		match_engine.skip_match()
		_match_was_skipped = true
	else:
		var played: int = match_engine.get_manual_matches_played()
		var remaining: int = 5 - (played % 5)
		if hud and hud.has_method("show_toast"):
			hud.show_toast("No Skip Tokens! Play %d more manual match(es) to earn 1 token." % remaining, 3.0)

func _request_next_match() -> void:
	if not is_instance_valid(match_engine) or match_engine.is_match_active():
		return

	Engine.time_scale = 1.0
	if is_instance_valid(hud) and hud.has_method("_set_sim_speed"):
		hud.call("_set_sim_speed", 1.0)
	pending_match_ended = false
	career_state = null
	active_match_context.clear()
	if match_engine.has_method("reset_standalone_match_setup"):
		match_engine.reset_standalone_match_setup()
	else:
		# Compatibility fallback for older MatchEngine binaries.
		if match_engine.has_method("set_user_is_home"):
			match_engine.set_user_is_home(true)
		if match_engine.has_method("set_home_club_name"):
			match_engine.set_home_club_name("HOME FC")
		if match_engine.has_method("set_away_club_name"):
			match_engine.set_away_club_name("AWAY FC")
	overview_2d.show()
	match_engine.start_match()
	if is_instance_valid(match_pitch):
		if match_pitch.has_method("set_team_names"):
			match_pitch.set_team_names(match_engine.get_home_club_name(), match_engine.get_away_club_name())
		if match_pitch.has_method("set_match_lineups"):
			match_pitch.set_match_lineups([], [])
		if match_pitch.has_method("reset_for_new_match"):
			match_pitch.reset_for_new_match()
	if hud and hud.has_method("hide_fulltime_summary"):
		hud.hide_fulltime_summary()
		hud.show_toast("KICK OFF! New Match Started", 2.0)

func _on_chance_state_changed(new_state: int, _ctrl_idx: int) -> void:
	if new_state == 0 and pending_match_ended:
		_process_final_whistle(cached_final_home, cached_final_away)

func _on_match_ended(final_home: int, final_away: int) -> void:
	if is_instance_valid(match_engine) and match_engine.is_chance_active():
		pending_match_ended = true
		cached_final_home = final_home
		cached_final_away = final_away
		return

	_process_final_whistle(final_home, final_away)

func _process_final_whistle(final_home: int, final_away: int) -> void:
	Engine.time_scale = 1.0
	if is_instance_valid(hud) and hud.has_method("_set_sim_speed"):
		hud.call("_set_sim_speed", 1.0)
	pending_match_ended = false

	if is_instance_valid(match_engine) and not _match_was_skipped:
		match_engine.record_manual_match_completed()
	_match_was_skipped = false

	if is_instance_valid(career_state) and not active_match_context.is_empty():
		var committed: bool = career_state.commit_match_result(match_engine)
		if not committed:
			push_warning("CareerState could not commit this match result; showing the match report without a confirmed career update.")

		var user_is_home: bool = bool(active_match_context.get("user_is_home", true))
		var match_stats: Dictionary = match_engine.get_match_stats() if is_instance_valid(match_engine) else {}
		var summary: Dictionary = {
			"home_club": String(active_match_context.get("home_club", "Home")),
			"away_club": String(active_match_context.get("away_club", "Away")),
			"home_goals": final_home,
			"away_goals": final_away,
			"user_is_home": user_is_home,
			"stats": match_stats,
			"stamina_report": match_stats.get("home_player_stamina", []) if user_is_home else match_stats.get("away_player_stamina", []),
			"bench": active_match_context.get("home_bench", []) if user_is_home else active_match_context.get("away_bench", []),
			"new_injuries": [],
			"skip_tokens": match_engine.get_skip_tokens() if is_instance_valid(match_engine) else 0,
			"manual_matches_played": match_engine.get_manual_matches_played() if is_instance_valid(match_engine) else 0,
		}
		if is_instance_valid(career_shell):
			career_shell.refresh_all_views()
		if is_instance_valid(post_match_modal):
			post_match_modal.show_report(summary)
		return

	if hud and hud.has_method("show_fulltime_summary"):
		hud.show_fulltime_summary(final_home, final_away)
func _toggle_career_shell() -> void:
	var shell := _resolve_career_shell()
	if not is_instance_valid(shell):
		career_shell = CareerShell.new()
		career_shell.name = "CareerShell"
		add_child(career_shell)
		career_shell.visible = true
	else:
		shell.visible = not shell.visible
