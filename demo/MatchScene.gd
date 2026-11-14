extends Node

var match_engine: MatchEngine
var overview_2d: CanvasLayer
var hud: Control
var match_pitch: Node

var pending_match_ended: bool = false
var cached_final_home: int = 0
var cached_final_away: int = 0

func _ready() -> void:
	match_engine = get_node_or_null("MatchEngine") as MatchEngine
	if not is_instance_valid(match_engine):
		match_engine = find_child("MatchEngine", true, false) as MatchEngine

	overview_2d = get_node_or_null("2DOverviewLayer") as CanvasLayer
	match_pitch = get_node_or_null("2DOverviewLayer/MatchPitch")
	hud = get_node_or_null("2DOverviewLayer/HUD") as Control

	if not is_instance_valid(match_engine) or not is_instance_valid(overview_2d):
		push_error("MatchScene: Missing MatchEngine or 2DOverviewLayer child nodes!")
		return

	overview_2d.show()

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
	if hud and hud.has_method("show_fulltime_summary"):
		hud.show_fulltime_summary(final_home, final_away)