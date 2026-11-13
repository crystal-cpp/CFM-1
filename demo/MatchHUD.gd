extends Control

signal skip_requested
signal next_match_requested

@onready var match_engine: MatchEngine = $"../../MatchEngine"

var _score_label: Label
var _minute_label: Label
var _home_info_label: Label
var _away_info_label: Label
var _skip_button: Button
var _toast_label: Label
var _toast_timer: Timer

var _fulltime_panel: PanelContainer
var _fulltime_score_label: Label
var _fulltime_progress_label: Label
var _next_match_button: Button

func _ready() -> void:
	# Expand HUD across the entire viewport without blocking pitch mouse clicks
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE

	_build_ui()

	_toast_timer = Timer.new()
	_toast_timer.one_shot = true
	_toast_timer.timeout.connect(_on_toast_timeout)
	add_child(_toast_timer)

	if not match_engine:
		push_error("MatchHUD: MatchEngine node missing at '../../MatchEngine'.")
		return

	match_engine.minute_changed.connect(_on_minute_changed)
	match_engine.goal_scored.connect(_on_goal_scored)

	refresh_all()

func _build_ui() -> void:
	# --- Top Scoreboard Bar ---
	var top_bar := PanelContainer.new()
	top_bar.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	top_bar.offset_left = 16.0
	top_bar.offset_top = 12.0
	top_bar.offset_right = -16.0
	top_bar.offset_bottom = 64.0
	top_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var bar_style := StyleBoxFlat.new()
	bar_style.bg_color = Color(0.08, 0.10, 0.14, 0.88)
	bar_style.corner_radius_top_left = 8
	bar_style.corner_radius_top_right = 8
	bar_style.corner_radius_bottom_left = 8
	bar_style.corner_radius_bottom_right = 8
	bar_style.content_margin_left = 16.0
	bar_style.content_margin_right = 16.0
	bar_style.content_margin_top = 8.0
	bar_style.content_margin_bottom = 8.0
	top_bar.add_theme_stylebox_override("panel", bar_style)
	add_child(top_bar)

	var hbox := HBoxContainer.new()
	hbox.alignment = BoxContainer.ALIGNMENT_CENTER
	hbox.add_theme_constant_override("separation", 18)
	hbox.mouse_filter = Control.MOUSE_FILTER_IGNORE
	top_bar.add_child(hbox)

	_home_info_label = Label.new()
	_home_info_label.text = "HOME (4-3-3)"
	_home_info_label.add_theme_color_override("font_color", Color(1.0, 0.45, 0.45))
	_home_info_label.add_theme_font_size_override("font_size", 16)
	hbox.add_child(_home_info_label)

	_score_label = Label.new()
	_score_label.text = "0 - 0"
	_score_label.add_theme_color_override("font_color", Color.WHITE)
	_score_label.add_theme_font_size_override("font_size", 22)
	hbox.add_child(_score_label)

	_away_info_label = Label.new()
	_away_info_label.text = "(4-4-2) AWAY"
	_away_info_label.add_theme_color_override("font_color", Color(0.45, 0.70, 1.0))
	_away_info_label.add_theme_font_size_override("font_size", 16)
	hbox.add_child(_away_info_label)

	var sep := VSeparator.new()
	hbox.add_child(sep)

	_minute_label = Label.new()
	_minute_label.text = "00'"
	_minute_label.custom_minimum_size = Vector2(54, 0)
	_minute_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_minute_label.add_theme_color_override("font_color", Color(0.95, 0.85, 0.25))
	_minute_label.add_theme_font_size_override("font_size", 18)
	hbox.add_child(_minute_label)

	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	spacer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	hbox.add_child(spacer)

	_skip_button = Button.new()
	_skip_button.text = "Skip Match (1) [S]"
	_skip_button.focus_mode = Control.FOCUS_NONE
	_skip_button.mouse_filter = Control.MOUSE_FILTER_STOP
	_skip_button.pressed.connect(func() -> void: emit_signal("skip_requested"))
	hbox.add_child(_skip_button)

	# --- Bottom Status Toast ---
	_toast_label = Label.new()
	_toast_label.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	_toast_label.offset_top = -48.0
	_toast_label.offset_bottom = -14.0
	_toast_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_toast_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_toast_label.add_theme_color_override("font_color", Color(1.0, 0.92, 0.45))
	_toast_label.add_theme_color_override("font_outline_color", Color.BLACK)
	_toast_label.add_theme_constant_override("outline_size", 4)
	_toast_label.add_theme_font_size_override("font_size", 16)
	_toast_label.visible = false
	add_child(_toast_label)

	# --- Center Full-Time Summary Card ---
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	_fulltime_panel = PanelContainer.new()
	_fulltime_panel.custom_minimum_size = Vector2(360, 200)
	_fulltime_panel.visible = false

	var ft_style := StyleBoxFlat.new()
	ft_style.bg_color = Color(0.07, 0.09, 0.13, 0.94)
	ft_style.border_color = Color(0.95, 0.85, 0.25, 0.9)
	ft_style.set_border_width_all(2)
	ft_style.set_corner_radius_all(12)
	ft_style.set_content_margin_all(24)
	_fulltime_panel.add_theme_stylebox_override("panel", ft_style)
	center.add_child(_fulltime_panel)

	var vbox := VBoxContainer.new()
	vbox.alignment = BoxContainer.ALIGNMENT_CENTER
	vbox.add_theme_constant_override("separation", 12)
	_fulltime_panel.add_child(vbox)

	var ft_title := Label.new()
	ft_title.text = "FULL TIME"
	ft_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	ft_title.add_theme_color_override("font_color", Color(0.95, 0.85, 0.25))
	ft_title.add_theme_font_size_override("font_size", 22)
	vbox.add_child(ft_title)

	_fulltime_score_label = Label.new()
	_fulltime_score_label.text = "HOME 0 - 0 AWAY"
	_fulltime_score_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fulltime_score_label.add_theme_font_size_override("font_size", 20)
	vbox.add_child(_fulltime_score_label)

	_fulltime_progress_label = Label.new()
	_fulltime_progress_label.text = "Skip Tokens: 1 | Manual Matches: 0/5"
	_fulltime_progress_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fulltime_progress_label.add_theme_color_override("font_color", Color(0.8, 0.85, 0.9))
	_fulltime_progress_label.add_theme_font_size_override("font_size", 14)
	vbox.add_child(_fulltime_progress_label)

	_next_match_button = Button.new()
	_next_match_button.text = "Play Next Match [R]"
	_next_match_button.focus_mode = Control.FOCUS_NONE
	_next_match_button.pressed.connect(func() -> void: emit_signal("next_match_requested"))
	vbox.add_child(_next_match_button)

func refresh_all() -> void:
	if not is_instance_valid(match_engine):
		return

	_home_info_label.text = "HOME (%s)" % match_engine.get_home_formation()
	_away_info_label.text = "(%s) AWAY" % match_engine.get_away_formation()
	_score_label.text = "%d - %d" % [match_engine.get_home_score(), match_engine.get_away_score()]
	_minute_label.text = "%02d'" % match_engine.get_current_minute()
	_update_skip_button()

func _update_skip_button() -> void:
	if not is_instance_valid(match_engine) or not _skip_button:
		return
	var tokens: int = match_engine.get_skip_tokens()
	_skip_button.text = "Skip Match (%d) [S]" % tokens
	_skip_button.disabled = not match_engine.can_skip_match()

func show_toast(message: String, duration: float = 2.5) -> void:
	if not _toast_label:
		return
	_toast_label.text = message
	_toast_label.visible = true
	_toast_timer.start(duration)

func hide_fulltime_summary() -> void:
	if _fulltime_panel:
		_fulltime_panel.visible = false
	refresh_all()

func _on_toast_timeout() -> void:
	if _toast_label:
		_toast_label.visible = false

func _on_minute_changed(minute: int) -> void:
	if _minute_label:
		_minute_label.text = "%02d'" % minute
	_update_skip_button()

func _on_goal_scored(home: int, away: int, is_home: bool) -> void:
	if _score_label:
		_score_label.text = "%d - %d" % [home, away]
	var scorer_side: String = "HOME" if is_home else "AWAY"
	show_toast("GOAL! %s SCORES (%d - %d)" % [scorer_side, home, away], 3.0)

func show_fulltime_summary(final_home: int, final_away: int) -> void:
	refresh_all()
	if _fulltime_score_label:
		_fulltime_score_label.text = "HOME %d - %d AWAY" % [final_home, final_away]
	if _fulltime_progress_label and is_instance_valid(match_engine):
		var played: int = match_engine.get_manual_matches_played()
		var tokens: int = match_engine.get_skip_tokens()
		_fulltime_progress_label.text = "Skip Tokens: %d  |  Token Progress: %d / 5 Manual Matches" % [tokens, played % 5]
	if _fulltime_panel:
		_fulltime_panel.visible = true