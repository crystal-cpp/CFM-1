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

	if not is_instance_valid(match_engine) or not is_instance_valid(overview_2d):
		push_error("MatchScene: Missing MatchEngine or 2DOverviewLayer child nodes!")
		return

	overview_2d.show()

	post_match_modal = PostMatchModal.new()
	post_match_modal.name = "PostMatchModal"
	add_child(post_match_modal)
	post_match_modal.continue_requested.connect(_on_post_match_continue)

	match_engine.chance_state_changed.connect(_on_chance_state_changed)
	match_engine.match_ended.connect(_on_match_ended)

	if hud:
		if hud.has_signal("skip_requested"):
			hud.skip_requested.connect(_request_skip_match)
		if hud.has_signal("next_match_requested"):
			hud.next_match_requested.connect(_request_next_match)

func _unhandled_input(event: InputEvent) -> void:
	if not (event is InputEventKey):
		return

	var key_event := event as InputEventKey
	if not key_event.pressed or key_event.is_echo():
		return

	if key_event.keycode == KEY_T:
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
		if is_instance_valid(match_engine) and match_engine.get_chance_state() == 2:
			match_engine.select_nearest_defender()
			get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_E:
		if is_instance_valid(match_engine) and match_engine.get_chance_state() == 2:
			match_engine.command_tackle()
			get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_G:
		if is_instance_valid(match_engine) and match_engine.get_chance_state() == 2:
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
	pre_match_view.start_match_requested.connect(
		func(context: Dictionary) -> void:
			launch_career_match(p_career_state, context)
	)
	pre_match_view.back_requested.connect(_close_pre_match_center)
	overview_2d.hide()
	return true


func _close_pre_match_center() -> void:
	if is_instance_valid(pre_match_view):
		pre_match_view.queue_free()
	pre_match_view = null
	overview_2d.show()
	if is_instance_valid(career_shell):
		career_shell.visible = true
		career_shell.refresh_all_views()


func launch_career_match(p_career_state: CareerState, context: Dictionary) -> void:
	if not is_instance_valid(match_engine) or not is_instance_valid(p_career_state) or context.is_empty():
		push_error("MatchScene: Cannot launch career match without a valid engine, CareerState, and match context.")
		return

	career_state = p_career_state
	active_match_context = context
	if career_state.has_method("prepare_next_match_payload"):
		career_state.prepare_next_match_payload(match_engine)
	elif career_state.has_method("apply_context_to_match_engine"):
		career_state.call("apply_context_to_match_engine", match_engine, active_match_context)

	# Apply the selected home/away kit colors to the 2D player markers.
	var kits: Dictionary = active_match_context.get("kits", {})
	var home_primary: Color = kits.get("home_primary", Color(0.06, 0.78, 0.48))
	var away_primary: Color = kits.get("active_away_primary", kits.get("away_primary", kits.get("away_default_primary", Color(0.92, 0.26, 0.30))))
	if is_instance_valid(match_pitch) and match_pitch.has_method("set_kit_colors"):
		match_pitch.set_kit_colors(home_primary, away_primary)

	if is_instance_valid(pre_match_view):
		pre_match_view.queue_free()
	pre_match_view = null
	pending_match_ended = false
	overview_2d.show()
	match_engine.start_match()
	if hud and hud.has_method("hide_fulltime_summary"):
		hud.hide_fulltime_summary()
	if hud and hud.has_method("show_toast"):
		hud.show_toast("KICK OFF! Career Match Started", 2.0)


func _on_post_match_continue(_summary: Dictionary) -> void:
	active_match_context.clear()
	if is_instance_valid(career_shell):
		career_shell.visible = true
		career_shell.refresh_all_views()


func _request_skip_match() -> void:
	if not is_instance_valid(match_engine) or not match_engine.is_match_active():
		return
	if match_engine.is_chance_active() or match_engine.is_match_paused():
		return

	if match_engine.can_skip_match():
		match_engine.skip_match()
	else:
		var played: int = match_engine.get_manual_matches_played()
		var remaining: int = 5 - (played % 5)
		if hud and hud.has_method("show_toast"):
			hud.show_toast("No Skip Tokens! Play %d more manual match(es) to earn 1 token." % remaining, 3.0)

func _request_next_match() -> void:
	if not is_instance_valid(match_engine) or match_engine.is_match_active():
		return

	pending_match_ended = false
	career_state = null
	active_match_context.clear()
	overview_2d.show()
	match_engine.start_match()
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
	pending_match_ended = false

	if is_instance_valid(career_state) and not active_match_context.is_empty():
		# The native CareerState commit API returns bool, not a report Dictionary.
		var committed: bool = career_state.commit_match_result(match_engine)
		if not committed:
			push_warning("CareerState could not commit this match result; showing the match report without a confirmed career update.")

		var summary: Dictionary = {
			"home_club": String(active_match_context.get("home_club", "Home")),
			"away_club": String(active_match_context.get("away_club", "Away")),
			"home_goals": final_home,
			"away_goals": final_away,
			"user_is_home": bool(active_match_context.get("user_is_home", true)),
			"stats": match_engine.get_match_stats() if is_instance_valid(match_engine) else {},
			"stamina_report": [],
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
	if not is_instance_valid(career_shell):
		career_shell = CareerShell.new()
		career_shell.name = "CareerShell"
		add_child(career_shell)
		career_shell.visible = true
	else:
		career_shell.visible = not career_shell.visible
