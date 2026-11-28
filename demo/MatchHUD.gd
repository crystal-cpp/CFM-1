class_name MatchHUD
extends Control

signal skip_requested
signal next_match_requested
signal tactic_screen_opened
signal tactic_screen_closed

var match_engine: MatchEngine
var _active_restart_type: int = 0
var _active_restart_is_home: bool = true

var _score_label: Label
var _minute_label: Label
var _status_pill: Label
var _home_info_label: Label
var _away_info_label: Label

var _pause_btn: Button
var _speed_1x_btn: Button
var _speed_2x_btn: Button
var _speed_4x_btn: Button

# Right-Hand Match Analytics Panel
var _right_panel: PanelContainer
var _poss_val_label: Label
var _poss_bar_home: ColorRect
var _xg_val_label: Label
var _xg_bar_home: ColorRect
var _shots_val_label: Label
var _shots_bar_home: ColorRect
var _passes_val_label: Label
var _def_actions_label: Label
var _chance_status_label: Label
var _gk_rush_btn: Button
var _slide_tackle_btn: Button
var _nearest_def_button: Button

# Bottom Live Commentary Bar
var _bottom_bar: PanelContainer
var _tactic_button: Button
var _skip_button: Button
var _commentary_label: Label
var _toast_label: Label
var _toast_timer: Timer

var _tactical_screen: TacticalScreen

# Post-Goal Kickoff Resume Overlay
var _kickoff_overlay: Control
var _kickoff_label: Label

var _fulltime_panel: PanelContainer
var _fulltime_score_label: Label
var _fulltime_stats_label: Label
var _fulltime_progress_label: Label
var _next_match_button: Button

var _commentary_history: PackedStringArray = PackedStringArray()
var _paused_by_tactics: bool = false
var _manual_paused: bool = false

var _home_club_name: String = "HOME FC"
var _away_club_name: String = "AWAY FC"

func _is_user_home() -> bool:
	return match_engine.is_user_home() if (is_instance_valid(match_engine) and match_engine.has_method("is_user_home")) else true

func _user_attack_chance() -> int:
	if is_instance_valid(match_engine) and match_engine.has_method("get_user_attack_chance_state"):
		return int(match_engine.get_user_attack_chance_state())
	return 1 if _is_user_home() else 2

func _user_defend_chance() -> int:
	if is_instance_valid(match_engine) and match_engine.has_method("get_user_defend_chance_state"):
		return int(match_engine.get_user_defend_chance_state())
	return 2 if _is_user_home() else 1

func set_team_names(home_name: String, away_name: String) -> void:
	_home_club_name = home_name.strip_edges() if not home_name.strip_edges().is_empty() else "HOME FC"
	_away_club_name = away_name.strip_edges() if not away_name.strip_edges().is_empty() else "AWAY FC"
	refresh_all()

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE

	_build_ui()
	_sync_right_panel_geometry()

	_toast_timer = Timer.new()
	_toast_timer.one_shot = true
	_toast_timer.timeout.connect(_on_toast_timeout)
	add_child(_toast_timer)

	match_engine = _resolve_match_engine()
	if not is_instance_valid(match_engine):
		push_error("MatchHUD: MatchEngine node missing in scene tree.")
		return

	var existing_tac: Node = get_node_or_null("TacticalScreen")
	if existing_tac is TacticalScreen:
		_tactical_screen = existing_tac as TacticalScreen
	else:
		_tactical_screen = TacticalScreen.new()
		_tactical_screen.name = "TacticalScreen"
		add_child(_tactical_screen)

	_tactical_screen.bind_match_engine(match_engine)
	if not _tactical_screen.resume_requested.is_connected(close_tactical_screen):
		_tactical_screen.resume_requested.connect(close_tactical_screen)
	if _tactical_screen.has_signal("substitution_applied") and not _tactical_screen.substitution_applied.is_connected(_on_substitution_applied):
		_tactical_screen.substitution_applied.connect(_on_substitution_applied)
	_tactical_screen.visible = false

	if not match_engine.minute_changed.is_connected(_on_minute_changed):
		match_engine.minute_changed.connect(_on_minute_changed)
	if not match_engine.goal_scored.is_connected(_on_goal_scored):
		match_engine.goal_scored.connect(_on_goal_scored)
	if not match_engine.match_event_logged.is_connected(_on_match_event_logged):
		match_engine.match_event_logged.connect(_on_match_event_logged)
	if not match_engine.chance_state_changed.is_connected(_on_chance_state_changed):
		match_engine.chance_state_changed.connect(_on_chance_state_changed)
	if not match_engine.goal_kickoff_ready.is_connected(_on_goal_kickoff_ready):
		match_engine.goal_kickoff_ready.connect(_on_goal_kickoff_ready)
	if not match_engine.goal_kickoff_resumed.is_connected(_on_goal_kickoff_resumed):
		match_engine.goal_kickoff_resumed.connect(_on_goal_kickoff_resumed)
	if match_engine.has_signal("squad_updated") and not match_engine.squad_updated.is_connected(_on_squad_updated):
		match_engine.squad_updated.connect(_on_squad_updated)
	if match_engine.has_signal("restart_state_changed") and not match_engine.restart_state_changed.is_connected(on_restart_state_changed):
		match_engine.restart_state_changed.connect(on_restart_state_changed)

	_set_sim_speed(1.0)
	refresh_all()

func _exit_tree() -> void:
	_set_sim_speed(1.0)
	if not is_instance_valid(match_engine):
		return
	if match_engine.minute_changed.is_connected(_on_minute_changed):
		match_engine.minute_changed.disconnect(_on_minute_changed)
	if match_engine.goal_scored.is_connected(_on_goal_scored):
		match_engine.goal_scored.disconnect(_on_goal_scored)
	if match_engine.match_event_logged.is_connected(_on_match_event_logged):
		match_engine.match_event_logged.disconnect(_on_match_event_logged)
	if match_engine.chance_state_changed.is_connected(_on_chance_state_changed):
		match_engine.chance_state_changed.disconnect(_on_chance_state_changed)
	if match_engine.goal_kickoff_ready.is_connected(_on_goal_kickoff_ready):
		match_engine.goal_kickoff_ready.disconnect(_on_goal_kickoff_ready)
	if match_engine.goal_kickoff_resumed.is_connected(_on_goal_kickoff_resumed):
		match_engine.goal_kickoff_resumed.disconnect(_on_goal_kickoff_resumed)
	if match_engine.has_signal("squad_updated") and match_engine.squad_updated.is_connected(_on_squad_updated):
		match_engine.squad_updated.disconnect(_on_squad_updated)
	if match_engine.has_signal("restart_state_changed") and match_engine.restart_state_changed.is_connected(on_restart_state_changed):
		match_engine.restart_state_changed.disconnect(on_restart_state_changed)

func on_restart_state_changed(restart_type: int, is_home_team: bool, _spot: Vector2 = Vector2.ZERO) -> void:
	_active_restart_type = restart_type
	_active_restart_is_home = is_home_team
	refresh_all()

func _format_restart_label(restart_type: int) -> String:
	match restart_type:
		1: return "THROW-IN"
		2: return "GOAL KICK"
		3: return "CORNER"
		4: return "FREE KICK"
		5: return "PENALTY"
		_: return "RESTART"

func _process(_delta: float) -> void:
	_sync_right_panel_geometry()
	_update_chance_controls()

func _unhandled_input(event: InputEvent) -> void:
	if is_tactical_screen_open():
		return
	if is_instance_valid(match_engine) and match_engine.is_waiting_for_kickoff_resume():
		if (event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT) \
		or (event is InputEventScreenTouch and event.pressed) \
		or event.is_action_pressed("ui_accept"):
			_resume_kickoff_from_hud()
			get_viewport().set_input_as_handled()

func _resume_kickoff_from_hud() -> void:
	var pitch_node: Node = _get_match_pitch()
	if pitch_node != null and pitch_node.has_method("confirm_kickoff_or_resume"):
		pitch_node.confirm_kickoff_or_resume()
	elif is_instance_valid(match_engine) and match_engine.is_waiting_for_kickoff_resume():
		match_engine.resume_from_goal_kickoff()

func _resolve_match_engine() -> MatchEngine:
	var candidate: Node = get_node_or_null("../../MatchEngine")
	if candidate is MatchEngine:
		return candidate as MatchEngine
	if get_tree() and get_tree().root:
		var found: Node = get_tree().root.find_child("MatchEngine", true, false)
		if found is MatchEngine:
			return found as MatchEngine
	return null

func _get_match_pitch() -> Node:
	var p: Node = get_node_or_null("../Pitch2D")
	if p != null:
		return p
	return get_node_or_null("../MatchPitch")

func _get_camera_2d() -> Camera2D:
	var cam: Node = get_node_or_null("../Pitch2D/Camera2D")
	if cam is Camera2D:
		return cam as Camera2D
	return null

func _sync_right_panel_geometry() -> void:
	if not _right_panel:
		return
	var vp: Vector2 = get_viewport_rect().size
	var panel_w: float = clampf(vp.x * 0.265, 276.0, 348.0)
	var pitch_node: Node = _get_match_pitch()
	if pitch_node != null and pitch_node.has_method("get_right_panel_width"):
		panel_w = float(pitch_node.get_right_panel_width())

	_right_panel.position = Vector2(vp.x - panel_w - 16.0, 72.0)
	_right_panel.size = Vector2(panel_w, maxf(200.0, vp.y - 72.0 - 96.0))

func _style_hud_btn(btn: Button, bg: Color, border: Color, fg: Color = Color.WHITE) -> void:
	var n := StyleBoxFlat.new()
	n.bg_color = bg
	n.border_color = border
	n.set_border_width_all(1)
	n.set_corner_radius_all(6)
	n.content_margin_left = 8
	n.content_margin_right = 8
	n.content_margin_top = 4
	n.content_margin_bottom = 4

	var h := n.duplicate()
	h.bg_color = bg.lightened(0.10)
	h.border_color = border.lightened(0.15)

	var p := n.duplicate()
	p.bg_color = bg.darkened(0.10)

	var d := n.duplicate()
	d.bg_color = Color(0.05, 0.07, 0.11, 0.65)
	d.border_color = Color(0.12, 0.16, 0.24, 0.55)

	btn.add_theme_stylebox_override("normal", n)
	btn.add_theme_stylebox_override("hover", h)
	btn.add_theme_stylebox_override("pressed", p)
	btn.add_theme_stylebox_override("disabled", d)
	btn.add_theme_color_override("font_color", fg)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)
	btn.add_theme_color_override("font_disabled_color", Color(0.45, 0.52, 0.62))

func _build_ui() -> void:
	# 1. TOP HERO SCOREBOARD CARD
	var top_bar := PanelContainer.new()
	top_bar.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	top_bar.offset_left = 16.0
	top_bar.offset_top = 8.0
	top_bar.offset_right = -16.0
	top_bar.offset_bottom = 64.0
	top_bar.mouse_filter = Control.MOUSE_FILTER_STOP

	var bar_style := StyleBoxFlat.new()
	bar_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	bar_style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	bar_style.set_border_width_all(1)
	bar_style.set_corner_radius_all(10)
	bar_style.content_margin_left = 14.0
	bar_style.content_margin_right = 14.0
	bar_style.content_margin_top = 6.0
	bar_style.content_margin_bottom = 6.0
	top_bar.add_theme_stylebox_override("panel", bar_style)
	add_child(top_bar)

	var hbox := HBoxContainer.new()
	hbox.alignment = BoxContainer.ALIGNMENT_CENTER
	hbox.add_theme_constant_override("separation", 12)
	top_bar.add_child(hbox)

	var home_badge := PanelContainer.new()
	var hb_style := StyleBoxFlat.new()
	hb_style.bg_color = Color(0.06, 0.72, 0.44, 0.22)
	hb_style.border_color = Color(0.06, 0.82, 0.50, 0.9)
	hb_style.set_border_width_all(1)
	hb_style.set_corner_radius_all(6)
	hb_style.set_content_margin_all(6)
	home_badge.add_theme_stylebox_override("panel", hb_style)
	var hb_lbl := Label.new()
	hb_lbl.text = "HOM"
	hb_lbl.add_theme_color_override("font_color", Color(0.15, 0.92, 0.58))
	hb_lbl.add_theme_font_size_override("font_size", 12)
	home_badge.add_child(hb_lbl)
	hbox.add_child(home_badge)

	_home_info_label = Label.new()
	_home_info_label.text = "HOME FC  4-3-3 · OVR 75"
	_home_info_label.add_theme_color_override("font_color", Color.WHITE)
	_home_info_label.add_theme_font_size_override("font_size", 15)
	hbox.add_child(_home_info_label)

	var left_spacer := Control.new()
	left_spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox.add_child(left_spacer)

	_score_label = Label.new()
	_score_label.text = "0  :  0"
	_score_label.add_theme_color_override("font_color", Color.WHITE)
	_score_label.add_theme_font_size_override("font_size", 24)
	hbox.add_child(_score_label)

	_minute_label = Label.new()
	_minute_label.text = "00'"
	_minute_label.custom_minimum_size = Vector2(44, 0)
	_minute_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_minute_label.add_theme_color_override("font_color", Color(0.12, 0.90, 0.55))
	_minute_label.add_theme_font_size_override("font_size", 16)
	hbox.add_child(_minute_label)

	_status_pill = Label.new()
	_status_pill.text = "LIVE PLAY"
	_status_pill.add_theme_color_override("font_color", Color(0.75, 0.84, 0.95))
	_status_pill.add_theme_font_size_override("font_size", 12)
	hbox.add_child(_status_pill)

	var right_spacer := Control.new()
	right_spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox.add_child(right_spacer)

	_away_info_label = Label.new()
	_away_info_label.text = "4-4-2 · OVR 72  AWAY FC"
	_away_info_label.add_theme_color_override("font_color", Color.WHITE)
	_away_info_label.add_theme_font_size_override("font_size", 15)
	hbox.add_child(_away_info_label)

	var away_badge := PanelContainer.new()
	var ab_style := StyleBoxFlat.new()
	ab_style.bg_color = Color(0.90, 0.24, 0.30, 0.22)
	ab_style.border_color = Color(0.95, 0.32, 0.38, 0.9)
	ab_style.set_border_width_all(1)
	ab_style.set_corner_radius_all(6)
	ab_style.set_content_margin_all(6)
	away_badge.add_theme_stylebox_override("panel", ab_style)
	var ab_lbl := Label.new()
	ab_lbl.text = "AWY"
	ab_lbl.add_theme_color_override("font_color", Color(1.0, 0.45, 0.50))
	ab_lbl.add_theme_font_size_override("font_size", 12)
	away_badge.add_child(ab_lbl)
	hbox.add_child(away_badge)

	hbox.add_child(VSeparator.new())

	_pause_btn = Button.new()
	_pause_btn.text = "||"
	_pause_btn.custom_minimum_size = Vector2(36, 34)
	_pause_btn.focus_mode = Control.FOCUS_NONE
	_pause_btn.pressed.connect(_on_pause_btn_pressed)
	hbox.add_child(_pause_btn)

	_speed_1x_btn = Button.new()
	_speed_1x_btn.text = "1x"
	_speed_1x_btn.custom_minimum_size = Vector2(36, 34)
	_speed_1x_btn.focus_mode = Control.FOCUS_NONE
	_speed_1x_btn.pressed.connect(func() -> void: _set_sim_speed(1.0))
	hbox.add_child(_speed_1x_btn)

	_speed_2x_btn = Button.new()
	_speed_2x_btn.text = "2x"
	_speed_2x_btn.custom_minimum_size = Vector2(36, 34)
	_speed_2x_btn.focus_mode = Control.FOCUS_NONE
	_speed_2x_btn.pressed.connect(func() -> void: _set_sim_speed(2.0))
	hbox.add_child(_speed_2x_btn)

	_speed_4x_btn = Button.new()
	_speed_4x_btn.text = "4x"
	_speed_4x_btn.custom_minimum_size = Vector2(36, 34)
	_speed_4x_btn.focus_mode = Control.FOCUS_NONE
	_speed_4x_btn.pressed.connect(func() -> void: _set_sim_speed(4.0))
	hbox.add_child(_speed_4x_btn)

	# 2. RIGHT-HAND LIVE MATCH ANALYTICS & CHANCE CONTROLS PANEL
	_right_panel = PanelContainer.new()
	_right_panel.mouse_filter = Control.MOUSE_FILTER_STOP
	var rp_style := StyleBoxFlat.new()
	rp_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	rp_style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	rp_style.set_border_width_all(1)
	rp_style.set_corner_radius_all(10)
	rp_style.set_content_margin_all(14)
	_right_panel.add_theme_stylebox_override("panel", rp_style)
	add_child(_right_panel)

	var rp_vbox := VBoxContainer.new()
	rp_vbox.add_theme_constant_override("separation", 10)
	_right_panel.add_child(rp_vbox)

	var rp_title := Label.new()
	rp_title.text = "MATCH ANALYTICS"
	rp_title.add_theme_color_override("font_color", Color(0.12, 0.90, 0.55))
	rp_title.add_theme_font_size_override("font_size", 14)
	rp_vbox.add_child(rp_title)

	var poss_pair := _add_stat_bar(rp_vbox, "POSSESSION", "50%  -  50%")
	_poss_val_label = poss_pair[0]
	_poss_bar_home = poss_pair[1]

	var xg_pair := _add_stat_bar(rp_vbox, "GOAL PREDICTED (xG)", "0.00  -  0.00")
	_xg_val_label = xg_pair[0]
	_xg_bar_home = xg_pair[1]

	var shots_pair := _add_stat_bar(rp_vbox, "SHOTS (ON TARGET)", "0 (0)  -  0 (0)")
	_shots_val_label = shots_pair[0]
	_shots_bar_home = shots_pair[1]

	_passes_val_label = _add_stat_text_row(rp_vbox, "PASSES COMPLETED", "0/0  -  0/0")
	_def_actions_label = _add_stat_text_row(rp_vbox, "TACKLES / INT / SAVES", "0/0/0  -  0/0/0")

	rp_vbox.add_child(HSeparator.new())

	var chance_hdr := Label.new()
	chance_hdr.text = "LIVE CHANCE CONTROL"
	chance_hdr.add_theme_color_override("font_color", Color(0.88, 0.94, 1.0))
	chance_hdr.add_theme_font_size_override("font_size", 13)
	rp_vbox.add_child(chance_hdr)

	_chance_status_label = Label.new()
	_chance_status_label.text = "Simulating open play..."
	_chance_status_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_chance_status_label.add_theme_color_override("font_color", Color(0.68, 0.78, 0.90))
	_chance_status_label.add_theme_font_size_override("font_size", 12)
	rp_vbox.add_child(_chance_status_label)

	_nearest_def_button = Button.new()
	_nearest_def_button.text = "Switch Defender [Q]"
	_nearest_def_button.custom_minimum_size = Vector2(0, 38)
	_nearest_def_button.focus_mode = Control.FOCUS_NONE
	_nearest_def_button.visible = false
	_nearest_def_button.pressed.connect(func() -> void:
		if _can_use_defensive_controls():
			match_engine.select_nearest_defender()
	)
	rp_vbox.add_child(_nearest_def_button)

	_slide_tackle_btn = Button.new()
	_slide_tackle_btn.text = "Slide Tackle [E]"
	_slide_tackle_btn.custom_minimum_size = Vector2(0, 38)
	_slide_tackle_btn.focus_mode = Control.FOCUS_NONE
	_slide_tackle_btn.visible = false
	_slide_tackle_btn.pressed.connect(func() -> void:
		if _can_use_defensive_controls():
			match_engine.command_tackle()
	)
	rp_vbox.add_child(_slide_tackle_btn)

	_gk_rush_btn = Button.new()
	_gk_rush_btn.text = "GK Rush Out [G]"
	_gk_rush_btn.custom_minimum_size = Vector2(0, 40)
	_gk_rush_btn.focus_mode = Control.FOCUS_NONE
	_gk_rush_btn.visible = false
	var gk_style := StyleBoxFlat.new()
	gk_style.bg_color = Color(0.75, 0.52, 0.10, 0.92)
	gk_style.set_corner_radius_all(8)
	_gk_rush_btn.add_theme_stylebox_override("normal", gk_style)
	_gk_rush_btn.add_theme_color_override("font_color", Color.WHITE)
	_gk_rush_btn.pressed.connect(func() -> void:
		var pitch_node: Node = _get_match_pitch()
		if _can_use_defensive_controls() and pitch_node and pitch_node.has_method("toggle_gk_rush"):
			pitch_node.toggle_gk_rush()
	)
	rp_vbox.add_child(_gk_rush_btn)

	# 3. BOTTOM BAR (Clean Live Commentary + Skip + TACTIC)
	_bottom_bar = PanelContainer.new()
	_bottom_bar.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	_bottom_bar.offset_left = 16.0
	_bottom_bar.offset_top = -86.0
	_bottom_bar.offset_right = -16.0
	_bottom_bar.offset_bottom = -10.0
	_bottom_bar.mouse_filter = Control.MOUSE_FILTER_STOP

	var bot_style := StyleBoxFlat.new()
	bot_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	bot_style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	bot_style.set_border_width_all(1)
	bot_style.set_corner_radius_all(10)
	bot_style.content_margin_left = 14.0
	bot_style.content_margin_right = 14.0
	bot_style.content_margin_top = 8.0
	bot_style.content_margin_bottom = 8.0
	_bottom_bar.add_theme_stylebox_override("panel", bot_style)
	add_child(_bottom_bar)

	var bot_hbox := HBoxContainer.new()
	bot_hbox.add_theme_constant_override("separation", 14)
	_bottom_bar.add_child(bot_hbox)

	var comm_vbox := VBoxContainer.new()
	comm_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	comm_vbox.alignment = BoxContainer.ALIGNMENT_CENTER
	comm_vbox.add_theme_constant_override("separation", 3)
	bot_hbox.add_child(comm_vbox)

	var feed_tag := Label.new()
	feed_tag.text = "LIVE COMMENTARY"
	feed_tag.add_theme_color_override("font_color", Color(0.12, 0.90, 0.55))
	feed_tag.add_theme_font_size_override("font_size", 11)
	comm_vbox.add_child(feed_tag)

	_commentary_label = Label.new()
	_commentary_label.text = ""
	_commentary_label.clip_text = true
	_commentary_label.add_theme_color_override("font_color", Color(0.94, 0.96, 1.0))
	_commentary_label.add_theme_font_size_override("font_size", 15)
	comm_vbox.add_child(_commentary_label)

	_skip_button = Button.new()
	_skip_button.text = "Skip (1)"
	_skip_button.custom_minimum_size = Vector2(104, 48)
	_skip_button.focus_mode = Control.FOCUS_NONE
	_skip_button.pressed.connect(func() -> void: emit_signal("skip_requested"))
	bot_hbox.add_child(_skip_button)

	_tactic_button = Button.new()
	_tactic_button.text = "TACTIC"
	_tactic_button.custom_minimum_size = Vector2(132, 48)
	_tactic_button.focus_mode = Control.FOCUS_NONE
	var tac_btn_style := StyleBoxFlat.new()
	tac_btn_style.bg_color = Color(0.08, 0.62, 0.38)
	tac_btn_style.set_corner_radius_all(8)
	_tactic_button.add_theme_stylebox_override("normal", tac_btn_style)
	_tactic_button.add_theme_color_override("font_color", Color.WHITE)
	_tactic_button.add_theme_font_size_override("font_size", 16)
	_tactic_button.pressed.connect(open_tactical_screen)
	bot_hbox.add_child(_tactic_button)

	# 4. FLOATING NOTIFICATION TOAST
	_toast_label = Label.new()
	_toast_label.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	_toast_label.offset_top = -124.0
	_toast_label.offset_bottom = -92.0
	_toast_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_toast_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_toast_label.add_theme_color_override("font_color", Color(1.0, 0.92, 0.42))
	_toast_label.add_theme_color_override("font_outline_color", Color.BLACK)
	_toast_label.add_theme_constant_override("outline_size", 4)
	_toast_label.add_theme_font_size_override("font_size", 16)
	_toast_label.visible = false
	add_child(_toast_label)

	# 5. POST-GOAL KICKOFF RESUME OVERLAY (Click anywhere to resume)
	_kickoff_overlay = Control.new()
	_kickoff_overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_kickoff_overlay.mouse_filter = Control.MOUSE_FILTER_STOP
	_kickoff_overlay.visible = false
	_kickoff_overlay.gui_input.connect(_on_kickoff_overlay_gui_input)
	add_child(_kickoff_overlay)

	var ko_center := CenterContainer.new()
	ko_center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	ko_center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_kickoff_overlay.add_child(ko_center)

	var ko_card := PanelContainer.new()
	ko_card.custom_minimum_size = Vector2(340, 88)
	ko_card.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var ko_style := StyleBoxFlat.new()
	ko_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	ko_style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	ko_style.set_border_width_all(2)
	ko_style.set_corner_radius_all(12)
	ko_style.set_content_margin_all(16)
	ko_card.add_theme_stylebox_override("panel", ko_style)
	ko_center.add_child(ko_card)

	_kickoff_label = Label.new()
	_kickoff_label.text = "KICKOFF\nCLICK TO RESUME"
	_kickoff_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_kickoff_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_kickoff_label.add_theme_color_override("font_color", Color(0.95, 0.98, 1.0))
	_kickoff_label.add_theme_font_size_override("font_size", 18)
	ko_card.add_child(_kickoff_label)

	# 6. FULL-TIME REPORT CARD
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	_fulltime_panel = PanelContainer.new()
	_fulltime_panel.custom_minimum_size = Vector2(460, 330)
	_fulltime_panel.visible = false

	var ft_style := StyleBoxFlat.new()
	ft_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	ft_style.border_color = Color(0.12, 0.20, 0.32, 0.85)
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
	ft_title.add_theme_color_override("font_color", Color(0.12, 0.90, 0.55))
	ft_title.add_theme_font_size_override("font_size", 22)
	vbox.add_child(ft_title)

	_fulltime_score_label = Label.new()
	_fulltime_score_label.text = "HOME 0 - 0 AWAY"
	_fulltime_score_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fulltime_score_label.add_theme_font_size_override("font_size", 22)
	vbox.add_child(_fulltime_score_label)

	_fulltime_stats_label = Label.new()
	_fulltime_stats_label.text = ""
	_fulltime_stats_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fulltime_stats_label.add_theme_color_override("font_color", Color(0.88, 0.92, 0.98))
	_fulltime_stats_label.add_theme_font_size_override("font_size", 14)
	vbox.add_child(_fulltime_stats_label)

	_fulltime_progress_label = Label.new()
	_fulltime_progress_label.text = "Skip Available: 0 | Manual Matches: 0/5"
	_fulltime_progress_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_fulltime_progress_label.add_theme_color_override("font_color", Color(0.75, 0.82, 0.92))
	_fulltime_progress_label.add_theme_font_size_override("font_size", 13)
	vbox.add_child(_fulltime_progress_label)

	_next_match_button = Button.new()
	_next_match_button.text = "Play Next Match [R]"
	_next_match_button.custom_minimum_size = Vector2(200, 44)
	_next_match_button.focus_mode = Control.FOCUS_NONE
	_next_match_button.pressed.connect(func() -> void: emit_signal("next_match_requested"))
	vbox.add_child(_next_match_button)

	var sec_bg := Color(0.07, 0.12, 0.20, 0.95)
	var sec_bd := Color(0.18, 0.30, 0.48, 0.85)
	var pri_bg := Color(0.06, 0.45, 0.32, 0.95)
	var pri_bd := Color(0.15, 0.92, 0.58, 0.90)

	for b in [_pause_btn, _speed_1x_btn, _speed_2x_btn, _speed_4x_btn, _nearest_def_button, _slide_tackle_btn, _skip_button]:
		_style_hud_btn(b, sec_bg, sec_bd)

	_style_hud_btn(_gk_rush_btn, Color(0.65, 0.44, 0.10, 0.95), Color(0.95, 0.72, 0.22, 0.90))
	_style_hud_btn(_tactic_button, pri_bg, pri_bd)
	_style_hud_btn(_next_match_button, pri_bg, pri_bd)
	_set_sim_speed(1.0)


func _add_stat_bar(parent: VBoxContainer, title_text: String, initial_val: String) -> Array:
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 4)
	parent.add_child(box)

	var top := HBoxContainer.new()
	box.add_child(top)

	var title := Label.new()
	title.text = title_text
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title.add_theme_color_override("font_color", Color(0.68, 0.78, 0.90))
	title.add_theme_font_size_override("font_size", 11)
	top.add_child(title)

	var val_lbl := Label.new()
	val_lbl.text = initial_val
	val_lbl.add_theme_color_override("font_color", Color.WHITE)
	val_lbl.add_theme_font_size_override("font_size", 12)
	top.add_child(val_lbl)

	var track := ColorRect.new()
	track.custom_minimum_size = Vector2(0, 7)
	track.color = Color(0.90, 0.26, 0.32, 0.88)
	box.add_child(track)

	var home_fill := ColorRect.new()
	home_fill.anchor_left = 0.0
	home_fill.anchor_top = 0.0
	home_fill.anchor_bottom = 1.0
	home_fill.anchor_right = 0.50
	home_fill.color = Color(0.10, 0.82, 0.50, 0.95)
	track.add_child(home_fill)

	return [val_lbl, home_fill]

func _add_stat_text_row(parent: VBoxContainer, title_text: String, initial_val: String) -> Label:
	var row := HBoxContainer.new()
	parent.add_child(row)

	var title := Label.new()
	title.text = title_text
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title.add_theme_color_override("font_color", Color(0.68, 0.78, 0.90))
	title.add_theme_font_size_override("font_size", 11)
	row.add_child(title)

	var val_lbl := Label.new()
	val_lbl.text = initial_val
	val_lbl.add_theme_color_override("font_color", Color.WHITE)
	val_lbl.add_theme_font_size_override("font_size", 12)
	row.add_child(val_lbl)
	return val_lbl

func _can_use_defensive_controls() -> bool:
	if not is_instance_valid(match_engine):
		return false
	var restart_type: int = int(match_engine.call("get_restart_state")) if match_engine.has_method("get_restart_state") else _active_restart_type
	return match_engine.get_chance_state() == _user_defend_chance() \
		and restart_type == 0 \
		and not match_engine.is_match_paused() \
		and not match_engine.is_waiting_for_kickoff_resume() \
		and not _manual_paused \
		and not is_tactical_screen_open()

func _update_chance_controls() -> void:
	if not is_instance_valid(match_engine):
		return
	if _manual_paused and not match_engine.is_match_paused() and not is_tactical_screen_open():
		_manual_paused = false

	var c_state: int = match_engine.get_chance_state()
	var restart_type: int = int(match_engine.call("get_restart_state")) if match_engine.has_method("get_restart_state") else _active_restart_type
	var is_paused: bool = is_tactical_screen_open() or _manual_paused or match_engine.is_match_paused()
	var pitch_node: Node = _get_match_pitch()
	var decision_paused: bool = pitch_node != null and pitch_node.has_method("is_attack_decision_paused") and bool(pitch_node.is_attack_decision_paused())
	var show_def_controls: bool = _can_use_defensive_controls()

	if _nearest_def_button:
		_nearest_def_button.visible = show_def_controls
	if _slide_tackle_btn:
		_slide_tackle_btn.visible = show_def_controls
	if _gk_rush_btn:
		_gk_rush_btn.visible = show_def_controls

	if _chance_status_label:
		if match_engine.is_waiting_for_kickoff_resume():
			_chance_status_label.text = "Click anywhere to resume."
			_chance_status_label.add_theme_color_override("font_color", Color(0.20, 0.95, 0.55))
		elif restart_type != 0:
			var side_name: String = _home_club_name if _active_restart_is_home else _away_club_name
			_chance_status_label.text = "%s — %s" % [side_name, _format_restart_label(restart_type)]
			_chance_status_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.35))
		elif is_paused:
			_chance_status_label.text = "Match paused."
			_chance_status_label.add_theme_color_override("font_color", Color(1.0, 0.82, 0.30))
		elif c_state == _user_attack_chance():
			if decision_paused:
				_chance_status_label.text = "Tap to pass, drag slow for through ball, tap pitch to dribble, or swipe fast to shoot."
				_chance_status_label.add_theme_color_override("font_color", Color(1.0, 0.88, 0.30))
			else:
				_chance_status_label.text = ""
				_chance_status_label.add_theme_color_override("font_color", Color(0.20, 0.95, 0.55))
		elif c_state == _user_defend_chance():
			_chance_status_label.text = "Swipe to slide tackle, tap to block, or rush GK out."
			_chance_status_label.add_theme_color_override("font_color", Color(1.0, 0.50, 0.42))
		else:
			_chance_status_label.text = ""
			_chance_status_label.add_theme_color_override("font_color", Color(0.68, 0.78, 0.90))

func open_tactical_screen() -> void:
	if not is_instance_valid(match_engine) or not _tactical_screen:
		return
	if _tactical_screen.visible:
		return

	_paused_by_tactics = not match_engine.is_match_paused()
	match_engine.pause_match(true)

	if _bottom_bar:
		_bottom_bar.visible = false
	if _right_panel:
		_right_panel.visible = false
	if _toast_label:
		_toast_label.visible = false

	if _tactical_screen.has_method("sync_squad_from_engine"):
		_tactical_screen.sync_squad_from_engine(match_engine)
	_tactical_screen.open_tactics()
	refresh_all()
	emit_signal("tactic_screen_opened")

func close_tactical_screen() -> void:
	if _tactical_screen:
		_tactical_screen.visible = false

	if _bottom_bar:
		_bottom_bar.visible = true
	if _right_panel:
		_right_panel.visible = true

	var pitch_node: Node = _get_match_pitch()
	var keep_decision_paused: bool = pitch_node != null and pitch_node.has_method("is_attack_decision_paused") and bool(pitch_node.is_attack_decision_paused())

	if is_instance_valid(match_engine) and _paused_by_tactics and not keep_decision_paused and not _manual_paused:
		match_engine.pause_match(false)
	_paused_by_tactics = false
	refresh_all()
	emit_signal("tactic_screen_closed")

func _on_substitution_applied(_is_home: bool, _pitch_slot: int, _bench_slot: int) -> void:
	refresh_all()
	var pitch_node: Node = _get_match_pitch()
	if pitch_node != null and pitch_node.has_method("queue_redraw"):
		pitch_node.queue_redraw()

func _on_squad_updated() -> void:
	refresh_all()
	if _tactical_screen != null and _tactical_screen.visible and _tactical_screen.has_method("sync_squad_from_engine"):
		_tactical_screen.sync_squad_from_engine(match_engine)

func is_tactical_screen_open() -> bool:
	return _tactical_screen != null and _tactical_screen.visible

func is_manual_pause_active() -> bool:
	return _manual_paused

func _on_pause_btn_pressed() -> void:
	if not is_instance_valid(match_engine) or not match_engine.is_match_active() or is_tactical_screen_open() or match_engine.is_waiting_for_kickoff_resume():
		return
	_manual_paused = not match_engine.is_match_paused()
	match_engine.pause_match(_manual_paused)
	refresh_all()

func _set_sim_speed(scale: float) -> void:
	Engine.time_scale = scale
	var pairs: Array = [[_speed_1x_btn, 1.0], [_speed_2x_btn, 2.0], [_speed_4x_btn, 4.0]]
	for pair in pairs:
		var btn: Button = pair[0]
		var target_scale: float = pair[1]
		if btn:
			var active: bool = is_equal_approx(scale, target_scale)
			btn.disabled = active
			_style_hud_btn(
				btn,
				Color(0.08, 0.35, 0.22, 0.95) if active else Color(0.07, 0.12, 0.20, 0.95),
				Color(0.18, 0.95, 0.58, 0.95) if active else Color(0.18, 0.30, 0.48, 0.85),
				Color(0.35, 1.0, 0.70) if active else Color.WHITE
			)
			if active:
				btn.add_theme_color_override("font_disabled_color", Color(0.35, 1.0, 0.70))

func refresh_all() -> void:
	if not is_instance_valid(match_engine):
		return

	_home_club_name = match_engine.get_home_club_name() if match_engine.has_method("get_home_club_name") else _home_club_name
	_away_club_name = match_engine.get_away_club_name() if match_engine.has_method("get_away_club_name") else _away_club_name
	_home_info_label.text = "%s  %s · OVR %d" % [_home_club_name, match_engine.get_home_formation(), match_engine.get_home_team_ovr()]
	_away_info_label.text = "%s · OVR %d  %s" % [match_engine.get_away_formation(), match_engine.get_away_team_ovr(), _away_club_name]
	_score_label.text = "%d  :  %d" % [match_engine.get_home_score(), match_engine.get_away_score()]
	_minute_label.text = "%02d'" % match_engine.get_current_minute()

	var c_state: int = match_engine.get_chance_state()
	var restart_type: int = int(match_engine.call("get_restart_state")) if match_engine.has_method("get_restart_state") else _active_restart_type
	if match_engine.is_waiting_for_kickoff_resume():
		_status_pill.text = "HALF TIME" if match_engine.get_current_minute() == 45 else "KICKOFF"
		_status_pill.add_theme_color_override("font_color", Color(0.20, 0.95, 0.55))
	elif is_tactical_screen_open() or _manual_paused or match_engine.is_match_paused():
		_status_pill.text = "PAUSED"
		_status_pill.add_theme_color_override("font_color", Color(1.0, 0.82, 0.30))
	elif restart_type != 0:
		_status_pill.text = _format_restart_label(restart_type)
		_status_pill.add_theme_color_override("font_color", Color(1.0, 0.85, 0.35))
	elif c_state == _user_attack_chance():
		_status_pill.text = "ATTACK CHANCE"
		_status_pill.add_theme_color_override("font_color", Color(0.20, 0.95, 0.55))
	elif c_state == _user_defend_chance():
		_status_pill.text = "DEFEND CHANCE"
		_status_pill.add_theme_color_override("font_color", Color(1.0, 0.45, 0.35))
	else:
		_status_pill.text = "1ST HALF" if match_engine.get_current_minute() < 45 else "2ND HALF"
		_status_pill.add_theme_color_override("font_color", Color(0.75, 0.84, 0.95))

	var any_paused: bool = is_tactical_screen_open() or _manual_paused or match_engine.is_match_paused()
	if _pause_btn:
		_pause_btn.text = ">" if any_paused else "||"

	var st: Dictionary = match_engine.get_match_stats()
	var h_poss: int = int(st.get("home_possession", 50))
	var a_poss: int = int(st.get("away_possession", 50))
	if _poss_val_label:
		_poss_val_label.text = "%d%%  -  %d%%" % [h_poss, a_poss]
	if _poss_bar_home:
		_poss_bar_home.anchor_right = clampf(float(h_poss) / 100.0, 0.05, 0.95)

	var h_xg: float = float(st.get("home_xg", 0.0))
	var a_xg: float = float(st.get("away_xg", 0.0))
	var tot_xg: float = h_xg + a_xg
	if _xg_val_label:
		_xg_val_label.text = "%.2f  -  %.2f" % [h_xg, a_xg]
	if _xg_bar_home:
		_xg_bar_home.anchor_right = clampf(h_xg / tot_xg, 0.08, 0.92) if tot_xg > 0.01 else 0.50

	var h_sh: int = int(st.get("home_shots", 0))
	var h_sot: int = int(st.get("home_shots_on_target", 0))
	var a_sh: int = int(st.get("away_shots", 0))
	var a_sot: int = int(st.get("away_shots_on_target", 0))
	var tot_sh: int = h_sh + a_sh
	if _shots_val_label:
		_shots_val_label.text = "%d (%d)  -  %d (%d)" % [h_sh, h_sot, a_sh, a_sot]
	if _shots_bar_home:
		_shots_bar_home.anchor_right = clampf(float(h_sh) / float(tot_sh), 0.08, 0.92) if tot_sh > 0 else 0.50

	if _passes_val_label:
		_passes_val_label.text = "%d/%d  -  %d/%d" % [
			int(st.get("home_passes_completed", 0)), int(st.get("home_passes", 0)),
			int(st.get("away_passes_completed", 0)), int(st.get("away_passes", 0))
		]

	if _def_actions_label:
		_def_actions_label.text = "%d/%d/%d  -  %d/%d/%d" % [
			int(st.get("home_tackles", 0)), int(st.get("home_interceptions", 0)), int(st.get("home_saves", 0)),
			int(st.get("away_tackles", 0)), int(st.get("away_interceptions", 0)), int(st.get("away_saves", 0))
		]

	_update_skip_button()
	_update_chance_controls()

func _update_skip_button() -> void:
	if not is_instance_valid(match_engine) or not _skip_button:
		return
	var tokens: int = match_engine.get_skip_tokens()
	_skip_button.text = "Skip (%d)" % tokens
	_skip_button.disabled = not match_engine.can_skip_match()

func show_toast(message: String, duration: float = 2.5) -> void:
	if not _toast_label or is_tactical_screen_open():
		return
	_toast_label.text = message
	_toast_label.visible = true
	_toast_timer.start(duration)

func hide_fulltime_summary() -> void:
	_manual_paused = false
	if _fulltime_panel:
		_fulltime_panel.visible = false
	if _kickoff_overlay:
		_kickoff_overlay.visible = false
	if _tactical_screen and _tactical_screen.has_method("reset_match_substitutions"):
		_tactical_screen.reset_match_substitutions()
	_commentary_history.clear()
	if _commentary_label:
		_commentary_label.text = ""
	refresh_all()

func _on_toast_timeout() -> void:
	if _toast_label:
		_toast_label.visible = false

func _on_minute_changed(_minute: int) -> void:
	refresh_all()

func _on_chance_state_changed(new_state: int, _ctrl_idx: int) -> void:
	refresh_all()
	if new_state == _user_attack_chance():
		show_toast("ATTACK CHANCE", 3.0)
	elif new_state == _user_defend_chance():
		show_toast("DEFEND CHANCE", 3.0)

func _on_goal_scored(home: int, away: int, is_home: bool) -> void:
	refresh_all()
	var scorer_side: String = _home_club_name if is_home else _away_club_name
	show_toast("GOAL! %s SCORES (%d : %d)" % [scorer_side, home, away], 3.0)

func _on_goal_kickoff_ready(kickoff_team: int, prompt_text: String) -> void:
	var team_name: String = _home_club_name if kickoff_team == 0 else _away_club_name
	var team_tag: String = "%s KICKOFF" % team_name
	if _kickoff_label:
		_kickoff_label.text = "%s\n%s" % [team_tag, prompt_text]
	if _kickoff_overlay:
		_kickoff_overlay.visible = true
	var cam := _get_camera_2d()
	if cam and is_instance_valid(match_engine):
		cam.position = match_engine.get_field_size() * 0.5
		cam.zoom = Vector2.ONE
	refresh_all()

func _on_goal_kickoff_resumed(_kickoff_team: int) -> void:
	if _kickoff_overlay:
		_kickoff_overlay.visible = false
	refresh_all()

func _on_kickoff_overlay_gui_input(event: InputEvent) -> void:
	if is_instance_valid(match_engine) and match_engine.is_waiting_for_kickoff_resume():
		if (event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT) \
		or (event is InputEventScreenTouch and event.pressed):
			_resume_kickoff_from_hud()
			accept_event()

func _on_match_event_logged(minute: int, event_type: String, is_home_team: bool, player_name: String, description: String) -> void:
	var side_tag: String = "[REF]" if player_name == "Referee" else ("[HOME]" if is_home_team else "[AWAY]")
	var entry: String = "%02d'  %s %s" % [minute, side_tag, description]
	_commentary_history.append(entry)
	if _commentary_history.size() > 60:
		_commentary_history.remove_at(0)
	if _commentary_label:
		_commentary_label.text = entry

	var upper_type: String = event_type.strip_edges().to_upper()
	if upper_type == "OFFSIDE":
		show_toast("OFFSIDE! Indirect Free Kick", 2.5)
	elif upper_type == "PENALTY":
		show_toast("PENALTY KICK! %s" % description, 3.0)
	elif upper_type == "RED_CARD" or upper_type == "DISMISSAL":
		show_toast("RED CARD! %s" % description, 3.0)
	elif upper_type == "YELLOW_CARD":
		show_toast("YELLOW CARD — %s" % description, 2.2)
	elif upper_type == "INJURY":
		show_toast("INJURY — %s" % description, 2.8)
	refresh_all()

func show_fulltime_summary(final_home: int, final_away: int) -> void:
	_manual_paused = false
	_set_sim_speed(1.0)
	if _kickoff_overlay:
		_kickoff_overlay.visible = false
	if is_tactical_screen_open():
		close_tactical_screen()
	refresh_all()
	if _fulltime_score_label:
		_fulltime_score_label.text = "%s  %d : %d  %s" % [_home_club_name, final_home, final_away, _away_club_name]

	if _fulltime_stats_label and is_instance_valid(match_engine):
		var st: Dictionary = match_engine.get_match_stats()
		_fulltime_stats_label.text = (
			"Possession: %d%%  -  %d%%\n" +
			"Shots (On Target): %d (%d)  -  %d (%d)\n" +
			"Expected Goals (xG): %.2f  -  %.2f\n" +
			"Passes Completed: %d/%d  -  %d/%d\n" +
			"Tackles / Interceptions / Saves: %d/%d/%d  -  %d/%d/%d"
		) % [
			st.get("home_possession", 50), st.get("away_possession", 50),
			st.get("home_shots", 0), st.get("home_shots_on_target", 0),
			st.get("away_shots", 0), st.get("away_shots_on_target", 0),
			st.get("home_xg", 0.0), st.get("away_xg", 0.0),
			st.get("home_passes_completed", 0), st.get("home_passes", 0),
			st.get("away_passes_completed", 0), st.get("away_passes", 0),
			st.get("home_tackles", 0), st.get("home_interceptions", 0), st.get("home_saves", 0),
			st.get("away_tackles", 0), st.get("away_interceptions", 0), st.get("away_saves", 0)
		]

	if _fulltime_progress_label and is_instance_valid(match_engine):
		var played: int = match_engine.get_manual_matches_played()
		var tokens: int = match_engine.get_skip_tokens()
		_fulltime_progress_label.text = "Skip Tokens: %d  |  Token Progress: %d / 5 Manual Matches" % [tokens, played % 5]

	if _fulltime_panel:
		_fulltime_panel.visible = true
