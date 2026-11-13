extends Node3D

enum EventState { IDLE, EVENT_TRIGGERED, HIGHLIGHT_ACTIVE, RESOLVED }

@onready var match_engine: MatchEngine = $MatchEngine
@onready var overview_2d: CanvasLayer = $"2DOverviewLayer"
@onready var hud: Control = $"2DOverviewLayer/HUD"
@onready var highlight_3d: Node3D = $"3DHighlightLayer"

var current_event_state: int = EventState.IDLE
var pending_match_ended: bool = false
var cached_final_home: int = 0
var cached_final_away: int = 0

var _highlight_overlay: CanvasLayer
var _highlight_banner_label: Label

func _ready() -> void:
	if not match_engine or not overview_2d or not highlight_3d:
		push_error("MatchScene: Missing critical child nodes! Check scene hierarchy.")
		return

	_build_highlight_banner()

	# Ensure default view state
	overview_2d.show()
	highlight_3d.hide()
	_highlight_overlay.hide()

	# Connect C++ MatchEngine signals
	match_engine.interactive_event_triggered.connect(_on_interactive_event_triggered)
	match_engine.match_ended.connect(_on_match_ended)

	# Connect 3D Highlight Manager signal
	if highlight_3d.has_signal("highlight_finished"):
		highlight_3d.highlight_finished.connect(_on_highlight_finished)

	# Connect HUD button signals if MatchHUD.gd is attached
	if hud:
		if hud.has_signal("skip_requested"):
			hud.skip_requested.connect(_request_skip_match)
		if hud.has_signal("next_match_requested"):
			hud.next_match_requested.connect(_request_next_match)

func _build_highlight_banner() -> void:
	_highlight_overlay = CanvasLayer.new()
	_highlight_overlay.layer = 5
	add_child(_highlight_overlay)

	var panel := PanelContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	panel.offset_left = 24.0
	panel.offset_top = 16.0
	panel.offset_right = -24.0
	panel.offset_bottom = 58.0
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.06, 0.08, 0.12, 0.85)
	style.set_corner_radius_all(8)
	style.set_content_margin_all(8)
	panel.add_theme_stylebox_override("panel", style)
	_highlight_overlay.add_child(panel)

	_highlight_banner_label = Label.new()
	_highlight_banner_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_highlight_banner_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_highlight_banner_label.add_theme_font_size_override("font_size", 18)
	panel.add_child(_highlight_banner_label)

func _unhandled_input(event: InputEvent) -> void:
	if not (event is InputEventKey):
		return

	var key_event := event as InputEventKey
	if not key_event.pressed or key_event.is_echo():
		return

	if key_event.keycode == KEY_S:
		_request_skip_match()
		get_viewport().set_input_as_handled()
	elif key_event.keycode == KEY_R:
		_request_next_match()
		get_viewport().set_input_as_handled()

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

	pending_match_ended = false
	overview_2d.show()
	highlight_3d.hide()
	_highlight_overlay.hide()

	match_engine.start_match()
	if hud and hud.has_method("hide_fulltime_summary"):
		hud.hide_fulltime_summary()
		hud.show_toast("KICKOFF! New Match Started", 2.0)

func _on_interactive_event_triggered(scenario_type: int, is_home_team: bool, ball_spawn_3d: Vector3) -> void:
	# Reentrancy Guard: Ignore incoming events while busy
	if current_event_state != EventState.IDLE:
		print("MatchScene: Event ignored — busy in state ", current_event_state)
		return

	current_event_state = EventState.EVENT_TRIGGERED
	if not match_engine.is_match_paused():
		match_engine.pause_match(true)
	print("MatchScene: Event Triggered -> Launching 3D Highlight [Scenario: ", scenario_type, "]")

	# Switch Viewports cleanly
	overview_2d.hide()
	highlight_3d.show()

	# Show contextual 3D highlight banner
	var is_attacking: bool = (scenario_type <= 3)
	var team_label: String = "HOME" if is_home_team else "AWAY"
	if is_attacking:
		_highlight_banner_label.text = "KEY HIGHLIGHT: %s ATTACKING CHANCE (Scenario %d)" % [team_label, scenario_type]
		_highlight_banner_label.add_theme_color_override("font_color", Color(0.35, 0.95, 0.50))
	else:
		_highlight_banner_label.text = "KEY HIGHLIGHT: %s DEFENSIVE CHANCE (Scenario %d)" % [team_label, scenario_type]
		_highlight_banner_label.add_theme_color_override("font_color", Color(1.0, 0.55, 0.35))
	_highlight_overlay.show()

	current_event_state = EventState.HIGHLIGHT_ACTIVE

	# Delegate setup to Highlight3DManager
	if highlight_3d.has_method("setup_highlight"):
		highlight_3d.setup_highlight(scenario_type, is_home_team, ball_spawn_3d)

func _on_highlight_finished(success: bool, is_home_team: bool) -> void:
	# Single-Resolution Guard: Ensure event resolves exactly once
	if current_event_state != EventState.HIGHLIGHT_ACTIVE:
		print("MatchScene: Duplicate highlight_finished signal blocked!")
		return

	current_event_state = EventState.RESOLVED
	print("MatchScene: Highlight Completed | Success: ", success)

	# Authoritative scenario resolution with C++ Engine
	match_engine.resolve_manual_scenario(success, is_home_team)

	# Return to 2D Match Viewport
	_highlight_overlay.hide()
	highlight_3d.hide()
	overview_2d.show()

	current_event_state = EventState.IDLE

	if hud and hud.has_method("refresh_all"):
		hud.refresh_all()

	# Process delayed full-time whistle if triggered during highlight
	if pending_match_ended:
		_process_final_whistle(cached_final_home, cached_final_away)

func _on_match_ended(final_home: int, final_away: int) -> void:
	if current_event_state != EventState.IDLE:
		print("MatchScene: Match whistle deferred until active 3D highlight completes.")
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