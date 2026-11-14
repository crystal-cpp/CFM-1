extends Control

signal skip_requested
signal next_match_requested

@onready var match_engine: MatchEngine = $"../../MatchEngine"

var _score_label: Label
var _minute_label: Label
var _home_info_label: Label
var _away_info_label: Label
var _tactics_toggle_btn: Button
var _skip_button: Button

var _commentary_label: Label
var _toast_label: Label
var _toast_timer: Timer

var _tactics_panel: PanelContainer
var _home_fmt_option: OptionButton
var _away_fmt_option: OptionButton
var _mentality_option: OptionButton
var _pressing_option: OptionButton
var _home_ovr_label: Label
var _away_ovr_label: Label

var _fulltime_panel: PanelContainer
var _fulltime_score_label: Label
var _fulltime_stats_label: Label
var _fulltime_progress_label: Label
var _next_match_button: Button

func _ready() -> void:
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
	match_engine.match_event_logged.connect(_on_match_event_logged)

	_populate_tactics_controls()
	refresh_all()

func _build_ui() -> void:
	# --- Top Scoreboard Bar ---
	var top_bar := PanelContainer.new()
	top_bar.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	top_bar.offset_left = 16.0
	top_bar.offset_top = 10.0
	top_bar.offset_right = -16.0
	top_bar.offset_bottom = 60.0
	top_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE

	var bar_style := StyleBoxFlat.new()
	bar_style.bg_color = Color(0.08, 0.10, 0.14, 0.90)
	bar_style.set_corner_radius_all(8)
	bar_style.content_margin_left = 16.0
	bar_style.content_margin_right = 16.0
	bar_style.content_margin_top = 6.0
	bar_style.content_margin_bottom = 6.0
	top_bar.add_theme_stylebox_override("panel", bar_style)
	add_child(top_bar)

	var hbox := HBoxContainer.new()
	hbox.alignment = BoxContainer.ALIGNMENT_CENTER
	hbox.add_theme_constant_override("separation", 16)
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

	hbox.add_child(VSeparator.new())

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

	_tactics_toggle_btn = Button.new()
	_tactics_toggle_btn.text = "Tactics [T]"
	_tactics_toggle_btn.focus_mode = Control.FOCUS_NONE
	_tactics_toggle_btn.mouse_filter = Control.MOUSE_FILTER_STOP
	_tactics_toggle_btn.pressed.connect(toggle_tactics_drawer)
	hbox.add_child(_tactics_toggle_btn)

	_skip_button = Button.new()
	_skip_button.text = "Skip Match (1) [S]"
	_skip_button.focus_mode = Control.FOCUS_NONE
	_skip_button.mouse_filter = Control.MOUSE_FILTER_STOP
	_skip_button.pressed.connect(func() -> void: emit_signal("skip_requested"))
	hbox.add_child(_skip_button)

	# --- Collapsible Live Tactics Drawer (Top-Right) ---
	_tactics_panel = PanelContainer.new()
	_tactics_panel.set_anchors_and_offsets_preset(Control.PRESET_TOP_RIGHT)
	_tactics_panel.offset_left = -310.0
	_tactics_panel.offset_top = 68.0
	_tactics_panel.offset_right = -16.0
	_tactics_panel.offset_bottom = 350.0
	_tactics_panel.visible = false
	_tactics_panel.mouse_filter = Control.MOUSE_FILTER_STOP

	var tac_style := StyleBoxFlat.new()
	tac_style.bg_color = Color(0.08, 0.10, 0.14, 0.94)
	tac_style.border_color = Color(0.35, 0.55, 0.85, 0.8)
	tac_style.set_border_width_all(1)
	tac_style.set_corner_radius_all(8)
	tac_style.set_content_margin_all(12)
	_tactics_panel.add_theme_stylebox_override("panel", tac_style)
	add_child(_tactics_panel)

	var tac_vbox := VBoxContainer.new()
	tac_vbox.add_theme_constant_override("separation", 8)
	_tactics_panel.add_child(tac_vbox)

	var tac_header := Label.new()
	tac_header.text = "TACTICAL INSTRUCTIONS"
	tac_header.add_theme_color_override("font_color", Color(0.95, 0.85, 0.25))
	tac_header.add_theme_font_size_override("font_size", 14)
	tac_vbox.add_child(tac_header)

	# Home Formation
	var h_fmt_row := HBoxContainer.new()
	var h_fmt_lbl := Label.new()
	h_fmt_lbl.text = "Home Shape:"
	h_fmt_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	h_fmt_row.add_child(h_fmt_lbl)
	_home_fmt_option = OptionButton.new()
	_home_fmt_option.focus_mode = Control.FOCUS_NONE
	_home_fmt_option.item_selected.connect(_on_home_formation_selected)
	h_fmt_row.add_child(_home_fmt_option)
	tac_vbox.add_child(h_fmt_row)

	# Away Formation
	var a_fmt_row := HBoxContainer.new()
	var a_fmt_lbl := Label.new()
	a_fmt_lbl.text = "Away Shape:"
	a_fmt_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	a_fmt_row.add_child(a_fmt_lbl)
	_away_fmt_option = OptionButton.new()
	_away_fmt_option.focus_mode = Control.FOCUS_NONE
	_away_fmt_option.item_selected.connect(_on_away_formation_selected)
	a_fmt_row.add_child(_away_fmt_option)
	tac_vbox.add_child(a_fmt_row)

	# Home Mentality
	var ment_row := HBoxContainer.new()
	var ment_lbl := Label.new()
	ment_lbl.text = "Mentality:"
	ment_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	ment_row.add_child(ment_lbl)
	_mentality_option = OptionButton.new()
	_mentality_option.focus_mode = Control.FOCUS_NONE
	_mentality_option.add_item("Defensive", 0)
	_mentality_option.add_item("Balanced", 1)
	_mentality_option.add_item("Attacking", 2)
	_mentality_option.select(1)
	_mentality_option.item_selected.connect(func(idx: int) -> void:
		if is_instance_valid(match_engine):
			match_engine.set_home_mentality(idx)
			show_toast("Mentality: " + _mentality_option.get_item_text(idx), 1.5)
	)
	ment_row.add_child(_mentality_option)
	tac_vbox.add_child(ment_row)

	# Home Pressing
	var press_row := HBoxContainer.new()
	var press_lbl := Label.new()
	press_lbl.text = "Pressing:"
	press_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	press_row.add_child(press_lbl)
	_pressing_option = OptionButton.new()
	_pressing_option.focus_mode = Control.FOCUS_NONE
	_pressing_option.add_item("Low", 0)
	_pressing_option.add_item("Medium", 1)
	_pressing_option.add_item("High", 2)
	_pressing_option.add_item("Extreme", 3)
	_pressing_option.select(1)
	_pressing_option.item_selected.connect(func(idx: int) -> void:
		if is_instance_valid(match_engine):
			match_engine.set_home_pressing(idx)
			show_toast("Pressing: " + _pressing_option.get_item_text(idx), 1.5)
	)
	press_row.add_child(_pressing_option)
	tac_vbox.add_child(press_row)

	# Home OVR Adjustment
	var h_ovr_row := HBoxContainer.new()
	_home_ovr_label = Label.new()
	_home_ovr_label.text = "Home OVR: 75"
	_home_ovr_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	h_ovr_row.add_child(_home_ovr_label)
	var h_minus := Button.new()
	h_minus.text = "-5"
	h_minus.focus_mode = Control.FOCUS_NONE
	h_minus.pressed.connect(func() -> void: _adjust_ovr(true, -5))
	var h_plus := Button.new()
	h_plus.text = "+5"
	h_plus.focus_mode = Control.FOCUS_NONE
	h_plus.pressed.connect(func() -> void: _adjust_ovr(true, 5))
	h_ovr_row.add_child(h_minus)
	h_ovr_row.add_child(h_plus)
	tac_vbox.add_child(h_ovr_row)

	# Away OVR Adjustment
	var a_ovr_row := HBoxContainer.new()
	_away_ovr_label = Label.new()
	_away_ovr_label.text = "Away OVR: 75"
	_away_ovr_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	a_ovr_row.add_child(_away_ovr_label)
	var a_minus := Button.new()
	a_minus.text = "-5"
	a_minus.focus_mode = Control.FOCUS_NONE
	a_minus.pressed.connect(func() -> void: _adjust_ovr(false, -5))
	var a_plus := Button.new()
	a_plus.text = "+5"
	a_plus.focus_mode = Control.FOCUS_NONE
	a_plus.pressed.connect(func() -> void: _adjust_ovr(false, 5))
	a_ovr_row.add_child(a_minus)
	a_ovr_row.add_child(a_plus)
	tac_vbox.add_child(a_ovr_row)

	# --- Bottom Live Commentary Ticker ---
	var comm_bar := PanelContainer.new()
	comm_bar.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	comm_bar.offset_left = 180.0
	comm_bar.offset_top = -42.0
	comm_bar.offset_right = -180.0
	comm_bar.offset_bottom = -10.0
	comm_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var comm_style := StyleBoxFlat.new()
	comm_style.bg_color = Color(0.06, 0.08, 0.11, 0.82)
	comm_style.set_corner_radius_all(6)
	comm_style.content_margin_left = 12.0
	comm_style.content_margin_right = 12.0
	comm_bar.add_theme_stylebox_override("panel", comm_style)
	add_child(comm_bar)

	_commentary_label = Label.new()
	_commentary_label.text = "00'  Kick-off!"
	_commentary_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_commentary_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_commentary_label.add_theme_color_override("font_color", Color(0.88, 0.92, 0.98))
	_commentary_label.add_theme_font_size_override("font_size", 14)
	comm_bar.add_child(_commentary_label)

	# --- Event Toast Banner ---
	_toast_label = Label.new()
	_toast_label.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	_toast_label.offset_top = -78.0
	_toast_label.offset_bottom = -46.0
	_toast_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_toast_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_toast_label.add_theme_color_override("font_color", Color(1.0, 0.92, 0.45))
	_toast_label.add_theme_color_override("font_outline_color", Color.BLACK)
	_toast_label.add_theme_constant_override("outline_size", 4)
	_toast_label.add_theme_font_size_override("font_size", 16)
	_toast_label.visible = false
	add_child(_toast_label)

	# --- Center Full-Time Summary & Match Stats Card ---
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	_fulltime_panel = PanelContainer.new()
	_fulltime_panel.custom_minimum_size = Vector2(440, 320)
	_fulltime_panel.visible = false

	var ft_style := StyleBoxFlat.new()
	ft_style.bg_color = Color(0.07, 0.09, 0.13, 0.95)
	ft_style.border_color = Color(0.95, 0.85, 0.25, 0.9)
	ft_style.set_border_width_all(2)
	ft_style.set_corner_radius_all(12)
	ft_style.set_content_margin_all(22)
	_fulltime_panel.add_theme_stylebox_override("panel", ft_style)
	center.add_child(_fulltime_panel)

	var vbox := VBoxContainer.new()
	vbox.alignment = BoxContainer.ALIGNMENT_CENTER
	vbox.add_theme_constant_override("separation", 10)
	_fulltime_panel.add_child(vbox)

	var ft_title := Label.new()
	ft_title.text = "FULL TIME REPORT"
	ft_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	ft_title.add_theme_color_override("font_color", Color(0.95, 0.85, 0.25))
	ft_title.add_theme_font_size_override("font_size", 22)
	vbox.add_child(ft_title)

	_fulltime_score_label = Label.new()
	_fulltime_score_label.text = "HOME 0 - 0 AWAY"
	_fulltime_score_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fulltime_score_label.add_theme_font_size_override("font_size", 20)
	vbox.add_child(_fulltime_score_label)

	_fulltime_stats_label = Label.new()
	_fulltime_stats_label.text = ""
	_fulltime_stats_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fulltime_stats_label.add_theme_color_override("font_color", Color(0.88, 0.92, 0.98))
	_fulltime_stats_label.add_theme_font_size_override("font_size", 14)
	vbox.add_child(_fulltime_stats_label)

	_fulltime_progress_label = Label.new()
	_fulltime_progress_label.text = "Skip Tokens: 1 | Manual Matches: 0/5"
	_fulltime_progress_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fulltime_progress_label.add_theme_color_override("font_color", Color(0.75, 0.82, 0.92))
	_fulltime_progress_label.add_theme_font_size_override("font_size", 13)
	vbox.add_child(_fulltime_progress_label)

	_next_match_button = Button.new()
	_next_match_button.text = "Play Next Match [R]"
	_next_match_button.focus_mode = Control.FOCUS_NONE
	_next_match_button.pressed.connect(func() -> void: emit_signal("next_match_requested"))
	vbox.add_child(_next_match_button)

func _populate_tactics_controls() -> void:
	if not is_instance_valid(match_engine):
		return

	_home_fmt_option.clear()
	_away_fmt_option.clear()

	var formations: PackedStringArray = match_engine.get_available_formations()
	var current_home: String = match_engine.get_home_formation()
	var current_away: String = match_engine.get_away_formation()

	for i in range(formations.size()):
		var fmt: String = formations[i]
		_home_fmt_option.add_item(fmt, i)
		_away_fmt_option.add_item(fmt, i)
		if fmt == current_home:
			_home_fmt_option.select(i)
		if fmt == current_away:
			_away_fmt_option.select(i)

	_home_ovr_label.text = "Home OVR: %d" % match_engine.get_home_team_ovr()
	_away_ovr_label.text = "Away OVR: %d" % match_engine.get_away_team_ovr()

func toggle_tactics_drawer() -> void:
	if _tactics_panel:
		_tactics_panel.visible = not _tactics_panel.visible
		if _tactics_panel.visible:
			_populate_tactics_controls()

func _on_home_formation_selected(index: int) -> void:
	if not is_instance_valid(match_engine):
		return
	var fmt: String = _home_fmt_option.get_item_text(index)
	if match_engine.set_home_formation(fmt):
		refresh_all()
		show_toast("Home Formation -> " + fmt, 1.8)

func _on_away_formation_selected(index: int) -> void:
	if not is_instance_valid(match_engine):
		return
	var fmt: String = _away_fmt_option.get_item_text(index)
	if match_engine.set_away_formation(fmt):
		refresh_all()
		show_toast("Away Formation -> " + fmt, 1.8)

func _adjust_ovr(is_home: bool, delta: int) -> void:
	if not is_instance_valid(match_engine):
		return
	if is_home:
		var next_ovr: int = clampi(match_engine.get_home_team_ovr() + delta, 40, 99)
		match_engine.set_home_team_ovr(next_ovr)
		_home_ovr_label.text = "Home OVR: %d" % next_ovr
	else:
		var next_ovr: int = clampi(match_engine.get_away_team_ovr() + delta, 40, 99)
		match_engine.set_away_team_ovr(next_ovr)
		_away_ovr_label.text = "Away OVR: %d" % next_ovr
	refresh_all()

func refresh_all() -> void:
	if not is_instance_valid(match_engine):
		return

	_home_info_label.text = "HOME %d (%s)" % [match_engine.get_home_team_ovr(), match_engine.get_home_formation()]
	_away_info_label.text = "(%s) %d AWAY" % [match_engine.get_away_formation(), match_engine.get_away_team_ovr()]
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
	if _commentary_label:
		_commentary_label.text = "00'  Kick-off! Both teams set up in formation."
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

func _on_match_event_logged(minute: int, _event_type: String, is_home_team: bool, _player_name: String, description: String) -> void:
	if not _commentary_label:
		return
	var side_tag: String = "[HOME]" if is_home_team else "[AWAY]"
	_commentary_label.text = "%02d'  %s %s" % [minute, side_tag, description]

func show_fulltime_summary(final_home: int, final_away: int) -> void:
	refresh_all()
	if _fulltime_score_label:
		_fulltime_score_label.text = "HOME %d - %d AWAY" % [final_home, final_away]

	if _fulltime_stats_label and is_instance_valid(match_engine):
		var st: Dictionary = match_engine.get_match_stats()
		_fulltime_stats_label.text = (
			"Possession: %d%%  -  %d%%\n" +
			"Shots (On Target): %d (%d)  -  %d (%d)\n" +
			"Expected Goals (xG): %.2f  -  %.2f\n" +
			"Passes Completed: %d/%d  -  %d/%d\n" +
			"Tackles / Saves: %d / %d  -  %d / %d"
		) % [
			st.get("home_possession", 50), st.get("away_possession", 50),
			st.get("home_shots", 0), st.get("home_shots_on_target", 0),
			st.get("away_shots", 0), st.get("away_shots_on_target", 0),
			st.get("home_xg", 0.0), st.get("away_xg", 0.0),
			st.get("home_passes_completed", 0), st.get("home_passes", 0),
			st.get("away_passes_completed", 0), st.get("away_passes", 0),
			st.get("home_tackles", 0), st.get("home_saves", 0),
			st.get("away_tackles", 0), st.get("away_saves", 0)
		]

	if _fulltime_progress_label and is_instance_valid(match_engine):
		var played: int = match_engine.get_manual_matches_played()
		var tokens: int = match_engine.get_skip_tokens()
		_fulltime_progress_label.text = "Skip Tokens: %d  |  Token Progress: %d / 5 Manual Matches" % [tokens, played % 5]

	if _fulltime_panel:
		_fulltime_panel.visible = true