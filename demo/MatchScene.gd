extends Node3D

enum EventState { IDLE, EVENT_TRIGGERED, CHANCE_ACTIVE, RESOLVED }

@export var chance_display_duration: float = 2.5

@onready var match_engine: MatchEngine = $MatchEngine
@onready var overview_2d: CanvasLayer = $"2DOverviewLayer"
@onready var hud: Control = $"2DOverviewLayer/HUD"

var current_event_state: int = EventState.IDLE
var pending_match_ended: bool = false
var cached_final_home: int = 0
var cached_final_away: int = 0

var _active_event_scenario: int = 1
var _active_event_is_home: bool = true

var _chance_timer: Timer
var _chance_overlay: CanvasLayer
var _chance_banner_label: Label
var _choice_btn_1: Button
var _choice_btn_2: Button
var _choice_btn_3: Button

func _ready() -> void:
	if not match_engine or not overview_2d:
		push_error("MatchScene: Missing critical child nodes! Check scene hierarchy.")
		return

	_chance_timer = Timer.new()
	_chance_timer.name = "Chance2DTimer"
	_chance_timer.one_shot = true
	_chance_timer.timeout.connect(_on_chance_timer_timeout)
	add_child(_chance_timer)

	_build_chance_banner()

	# Keep 2D pitch as the sole active match presentation
	overview_2d.show()
	_chance_overlay.hide()

	# Connect C++ MatchEngine signals
	match_engine.interactive_event_triggered.connect(_on_interactive_event_triggered)
	match_engine.match_ended.connect(_on_match_ended)

	# Connect HUD button signals
	if hud:
		if hud.has_signal("skip_requested"):
			hud.skip_requested.connect(_request_skip_match)
		if hud.has_signal("next_match_requested"):
			hud.next_match_requested.connect(_request_next_match)

func _exit_tree() -> void:
	if _chance_timer and not _chance_timer.is_stopped():
		_chance_timer.stop()

func _build_chance_banner() -> void:
	_chance_overlay = CanvasLayer.new()
	_chance_overlay.layer = 5
	add_child(_chance_overlay)

	var panel := PanelContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	panel.offset_left = 220.0
	panel.offset_top = 72.0
	panel.offset_right = -220.0
	panel.offset_bottom = 148.0
	panel.mouse_filter = Control.MOUSE_FILTER_STOP

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.08, 0.12, 0.94)
	style.border_color = Color(0.95, 0.85, 0.25, 0.85)
	style.set_border_width_all(2)
	style.set_corner_radius_all(8)
	style.set_content_margin_all(10)
	panel.add_theme_stylebox_override("panel", style)
	_chance_overlay.add_child(panel)

	var vbox := VBoxContainer.new()
	vbox.alignment = BoxContainer.ALIGNMENT_CENTER
	vbox.add_theme_constant_override("separation", 8)
	panel.add_child(vbox)

	_chance_banner_label = Label.new()
	_chance_banner_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_chance_banner_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_chance_banner_label.add_theme_font_size_override("font_size", 18)
	vbox.add_child(_chance_banner_label)

	var btn_row := HBoxContainer.new()
	btn_row.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_row.add_theme_constant_override("separation", 12)
	vbox.add_child(btn_row)

	_choice_btn_1 = Button.new()
	_choice_btn_1.focus_mode = Control.FOCUS_NONE
	_choice_btn_1.pressed.connect(func() -> void: _on_tactical_choice_picked(0))
	btn_row.add_child(_choice_btn_1)

	_choice_btn_2 = Button.new()
	_choice_btn_2.focus_mode = Control.FOCUS_NONE
	_choice_btn_2.pressed.connect(func() -> void: _on_tactical_choice_picked(1))
	btn_row.add_child(_choice_btn_2)

	_choice_btn_3 = Button.new()
	_choice_btn_3.focus_mode = Control.FOCUS_NONE
	_choice_btn_3.pressed.connect(func() -> void: _on_tactical_choice_picked(2))
	btn_row.add_child(_choice_btn_3)

func _unhandled_input(event: InputEvent) -> void:
	if not (event is InputEventKey):
		return

	var key_event := event as InputEventKey
	if not key_event.pressed or key_event.is_echo():
		return

	# Interactive Key Chance Quick-Pick Hotkeys [1], [2], [3]
	if current_event_state == EventState.CHANCE_ACTIVE:
		if key_event.keycode == KEY_1:
			_on_tactical_choice_picked(0)
			get_viewport().set_input_as_handled()
			return
		elif key_event.keycode == KEY_2:
			_on_tactical_choice_picked(1)
			get_viewport().set_input_as_handled()
			return
		elif key_event.keycode == KEY_3:
			_on_tactical_choice_picked(2)
			get_viewport().set_input_as_handled()
			return

	if key_event.keycode == KEY_T:
		if hud and hud.has_method("toggle_tactics_drawer"):
			hud.toggle_tactics_drawer()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_S:
		_request_skip_match()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_R:
		_request_next_match()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_SPACE:
		toggle_manual_pause()
		get_viewport().set_input_as_handled()

func toggle_manual_pause() -> void:
	if current_event_state != EventState.IDLE or not is_instance_valid(match_engine) or not match_engine.is_match_active():
		return

	var next_paused: bool = not match_engine.is_match_paused()
	match_engine.pause_match(next_paused)
	if hud and hud.has_method("show_toast"):
		hud.show_toast("Match Paused [Space]" if next_paused else "Match Resumed", 1.8)

func _request_skip_match() -> void:
	if current_event_state != EventState.IDLE or not is_instance_valid(match_engine) or not match_engine.is_match_active():
		return

	if match_engine.can_skip_match():
		print("MatchScene: Executing Skip Match...")
		match_engine.skip_match()
	else:
		var played: int = match_engine.get_manual_matches_played()
		var remaining: int = 5 - (played % 5)
		var msg := "No Skip Tokens! Play %d more manual match(es) to earn 1 token." % remaining
		print("MatchScene: ", msg)
		if hud and hud.has_method("show_toast"):
			hud.show_toast(msg, 3.0)

func _request_next_match() -> void:
	if current_event_state != EventState.IDLE or not is_instance_valid(match_engine):
		return
	if match_engine.is_match_active():
		return

	if _chance_timer and not _chance_timer.is_stopped():
		_chance_timer.stop()

	pending_match_ended = false
	overview_2d.show()
	_chance_overlay.hide()

	match_engine.start_match()
	if hud and hud.has_method("hide_fulltime_summary"):
		hud.hide_fulltime_summary()
		hud.show_toast("KICK OFF! New Match Started", 2.0)

func _on_interactive_event_triggered(scenario_type: int, is_home_team: bool, _ball_spawn_3d: Vector3) -> void:
	# Reentrancy Guard: Ignore incoming events while busy
	if current_event_state != EventState.IDLE:
		print("MatchScene: Event ignored — busy in state ", current_event_state)
		return

	current_event_state = EventState.EVENT_TRIGGERED
	if not match_engine.is_match_paused():
		match_engine.pause_match(true)

	_active_event_scenario = scenario_type
	_active_event_is_home = is_home_team

	# Keep 2D presentation active (no 3D viewport/camera switch)
	overview_2d.show()

	# Gameplay UI strictly displays ATTACK CHANCE or DEFEND CHANCE (never internal scenario IDs)
	if is_home_team:
		_chance_banner_label.text = "ATTACK CHANCE"
		_chance_banner_label.add_theme_color_override("font_color", Color(0.35, 0.95, 0.50))
		_choice_btn_1.text = "[1] Finesse Shot"
		_choice_btn_2.text = "[2] Square Pass"
		_choice_btn_3.text = "[3] Beat Defender"
	else:
		_chance_banner_label.text = "DEFEND CHANCE"
		_chance_banner_label.add_theme_color_override("font_color", Color(1.0, 0.55, 0.35))
		_choice_btn_1.text = "[1] Stand Tackle"
		_choice_btn_2.text = "[2] Block Passing Lane"
		_choice_btn_3.text = "[3] GK Rush Out"

	_chance_overlay.show()
	current_event_state = EventState.CHANCE_ACTIVE
	_chance_timer.start(chance_display_duration)

func _on_tactical_choice_picked(choice_idx: int) -> void:
	if current_event_state != EventState.CHANCE_ACTIVE:
		return

	var ovr_diff: float = float(match_engine.get_home_team_ovr() - match_engine.get_away_team_ovr())
	# Tactical choice gives a slight skill bonus over passive timeout
	var choice_bonus: float = [0.08, 0.06, 0.05][clampi(choice_idx, 0, 2)]
	var base_prob: float = 0.48 + choice_bonus + (ovr_diff * 0.008 if _active_event_is_home else -ovr_diff * 0.008)
	var rolled_success: bool = randf() < clampf(base_prob, 0.22, 0.82)
	resolve_active_2d_chance(1 if rolled_success else 0)

func _on_chance_timer_timeout() -> void:
	resolve_active_2d_chance(-1)

# override_success: -1 = OVR-weighted roll, 0 = false, 1 = true
func resolve_active_2d_chance(override_success: int = -1) -> void:
	# Single-Resolution Guard: Ensure event resolves exactly once
	if current_event_state != EventState.CHANCE_ACTIVE:
		print("MatchScene: Duplicate chance resolution blocked!")
		return

	if _chance_timer and not _chance_timer.is_stopped():
		_chance_timer.stop()

	current_event_state = EventState.RESOLVED

	var success: bool
	if override_success == 1:
		success = true
	elif override_success == 0:
		success = false
	else:
		var ovr_diff: float = float(match_engine.get_home_team_ovr() - match_engine.get_away_team_ovr())
		var base_prob: float = 0.45 + (ovr_diff * 0.008 if _active_event_is_home else -ovr_diff * 0.008)
		success = randf() < clampf(base_prob, 0.20, 0.75)

	# Authoritative scenario resolution with C++ Engine
	match_engine.resolve_manual_scenario(success, _active_event_is_home)

	_chance_overlay.hide()
	overview_2d.show()
	current_event_state = EventState.IDLE

	if hud and hud.has_method("refresh_all"):
		hud.refresh_all()

	# Process delayed full-time whistle if triggered during active chance
	if pending_match_ended:
		_process_final_whistle(cached_final_home, cached_final_away)

func _on_match_ended(final_home: int, final_away: int) -> void:
	if current_event_state != EventState.IDLE:
		print("MatchScene: Match whistle deferred until active 2D chance completes.")
		pending_match_ended = true
		cached_final_home = final_home
		cached_final_away = final_away
		return

	_process_final_whistle(final_home, final_away)

func _process_final_whistle(final_home: int, final_away: int) -> void:
	print("MatchScene: FULL TIME! Final Scoreboard: Home ", final_home, " - ", final_away, " Away")
	print("Manual Matches Played: ", match_engine.get_manual_matches_played(), " | Available Skip Tokens: ", match_engine.get_skip_tokens())
	pending_match_ended = false
	if hud and hud.has_method("show_fulltime_summary"):
		hud.show_fulltime_summary(final_home, final_away)