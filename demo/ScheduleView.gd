class_name ScheduleView
extends Control

signal match_launch_requested()
signal route_requested(route_id: String)

var career_state: CareerState

var _ui_built: bool = false
var _active_tab: String = "my_fixtures" # "my_fixtures" | "league_matchdays" | "results_archive"
var _my_filter: String = "ALL"          # "ALL" | "PRE_SEASON" | "LEAGUE" | "HOME" | "AWAY"
var _archive_scope: String = "MY_CLUB"  # "MY_CLUB" | "ALL_LEAGUE"
var _archive_outcome: String = "ALL"    # "ALL" | "W" | "D" | "L"
var _selected_round_key: int = 0        # 0 = Pre-Season (Weeks 1-2), 1..N = League Matchday

# Header & Timeline Widgets
var _season_pill: Label
var _phase_pill: Label
var _next_match_pill: Label
var _sim_next_btn: Button
var _play_next_btn: Button

# Tab Buttons & Containers
var _tab_buttons: Dictionary = {}
var _tab_panels: Dictionary = {}

# Tab 1: My Club Fixtures
var _my_filter_buttons: Dictionary = {}
var _my_fixtures_vbox: VBoxContainer

# Tab 2: League Matchdays
var _md_prev_btn: Button
var _md_option: OptionButton
var _md_next_btn: Button
var _md_current_btn: Button
var _md_date_lbl: Label
var _md_fixtures_vbox: VBoxContainer

# Tab 3: Results Archive
var _archive_scope_buttons: Dictionary = {}
var _archive_outcome_buttons: Dictionary = {}
var _archive_stats_lbl: Label
var _archive_vbox: VBoxContainer

# Centered Match Detail / Preview Modal
var _detail_modal: PanelContainer
var _detail_title_lbl: Label
var _detail_meta_lbl: Label
var _detail_score_lbl: Label
var _detail_events_vbox: VBoxContainer
var _detail_action_btn: Button

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_flags_vertical = Control.SIZE_EXPAND_FILL
	_ensure_ui_built()
	visibility_changed.connect(func() -> void:
		if visible:
			refresh_schedule()
	)
	refresh_schedule()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if (event as InputEventKey).keycode == KEY_ESCAPE:
			if is_instance_valid(_detail_modal) and _detail_modal.visible:
				_detail_modal.visible = false
				get_viewport().set_input_as_handled()

func _style_btn(btn: Button, bg: Color, border: Color, text_col: Color = Color.WHITE) -> void:
	var normal := StyleBoxFlat.new()
	normal.bg_color = bg
	normal.border_color = border
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(6)
	normal.content_margin_left = 10
	normal.content_margin_right = 10
	normal.content_margin_top = 5
	normal.content_margin_bottom = 5

	var hover := normal.duplicate()
	hover.bg_color = bg.lightened(0.10)
	hover.border_color = border.lightened(0.15)

	var pressed := normal.duplicate()
	pressed.bg_color = bg.darkened(0.10)

	var disabled := normal.duplicate()
	disabled.bg_color = Color(0.05, 0.07, 0.11, 0.60)
	disabled.border_color = Color(0.12, 0.16, 0.24, 0.50)

	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("pressed", pressed)
	btn.add_theme_stylebox_override("disabled", disabled)
	btn.add_theme_color_override("font_color", text_col)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)
	btn.add_theme_color_override("font_disabled_color", Color(0.45, 0.52, 0.62))

func _create_badge(text: String, bg_color: Color, text_color: Color) -> Label:
	var lbl := Label.new()
	lbl.text = text
	var style := StyleBoxFlat.new()
	style.bg_color = bg_color.lerp(Color.BLACK, 0.38)
	style.border_color = bg_color
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.content_margin_left = 8
	style.content_margin_right = 8
	style.content_margin_top = 4
	style.content_margin_bottom = 4
	lbl.add_theme_stylebox_override("normal", style)
	lbl.add_theme_color_override("font_color", text_color)
	lbl.add_theme_font_size_override("font_size", 11)
	return lbl

func _ensure_ui_built() -> void:
	if _ui_built:
		return
	_ui_built = true

	var root_panel := PanelContainer.new()
	root_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var bg_style := StyleBoxFlat.new()
	bg_style.bg_color = Color(0.024, 0.035, 0.055, 1.0)
	bg_style.content_margin_left = 20
	bg_style.content_margin_right = 20
	bg_style.content_margin_top = 16
	bg_style.content_margin_bottom = 16
	root_panel.add_theme_stylebox_override("panel", bg_style)
	add_child(root_panel)

	var main_vbox := VBoxContainer.new()
	main_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	main_vbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_theme_constant_override("separation", 12)
	root_panel.add_child(main_vbox)

	# 1. Top Header & Season Window Status
	var top_hbox := HFlowContainer.new()
	top_hbox.add_theme_constant_override("h_separation", 10)
	top_hbox.add_theme_constant_override("v_separation", 8)
	main_vbox.add_child(top_hbox)

	var title_lbl := Label.new()
	title_lbl.text = "SCHEDULE & SEASON CALENDAR"
	title_lbl.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title_lbl.add_theme_font_size_override("font_size", 20)
	top_hbox.add_child(title_lbl)

	_season_pill = _create_badge("2026/27 • Jul – May", Color(0.20, 0.45, 0.75), Color(0.75, 0.88, 1.0))
	top_hbox.add_child(_season_pill)

	_phase_pill = _create_badge("Weeks 1–2: Pre-Season Window", Color(0.65, 0.48, 0.12), Color(1.0, 0.88, 0.35))
	top_hbox.add_child(_phase_pill)

	_next_match_pill = _create_badge("Next: TBD", Color(0.06, 0.67, 0.52), Color(0.35, 1.0, 0.72))
	top_hbox.add_child(_next_match_pill)

	# 2. Season Timeline & Quick Actions Strip
	var strip_panel := PanelContainer.new()
	var strip_style := StyleBoxFlat.new()
	strip_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	strip_style.border_color = Color(0.12, 0.20, 0.32, 0.80)
	strip_style.set_border_width_all(1)
	strip_style.set_corner_radius_all(8)
	strip_style.content_margin_left = 12
	strip_style.content_margin_right = 12
	strip_style.content_margin_top = 8
	strip_style.content_margin_bottom = 8
	strip_panel.add_theme_stylebox_override("panel", strip_style)
	main_vbox.add_child(strip_panel)

	var strip_hbox := HBoxContainer.new()
	strip_hbox.add_theme_constant_override("separation", 10)
	strip_panel.add_child(strip_hbox)

	var timeline_lbl := Label.new()
	timeline_lbl.text = "Season: Jul Friendlies  →  Mid-Aug League  →  May Finale"
	timeline_lbl.tooltip_text = "Season Structure: Weeks 1–2 (July Friendlies) → Week 3 / Mid-Aug (Matchday 1) → May (Season Finale)"
	timeline_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	timeline_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	timeline_lbl.custom_minimum_size = Vector2(260, 0)
	timeline_lbl.add_theme_color_override("font_color", Color(0.75, 0.84, 0.95))
	timeline_lbl.add_theme_font_size_override("font_size", 12)
	strip_hbox.add_child(timeline_lbl)

	_sim_next_btn = Button.new()
	_sim_next_btn.text = "⏩ Advance to Next Match"
	_sim_next_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_sim_next_btn, Color(0.08, 0.22, 0.38, 0.95), Color(0.20, 0.48, 0.80, 0.85))
	_sim_next_btn.pressed.connect(_on_advance_to_next_pressed)
	strip_hbox.add_child(_sim_next_btn)

	_play_next_btn = Button.new()
	_play_next_btn.text = "⚽ Go to Matchday"
	_play_next_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_play_next_btn, Color(0.06, 0.45, 0.30, 0.95), Color(0.15, 0.92, 0.58, 0.90), Color(0.85, 1.0, 0.92))
	_play_next_btn.pressed.connect(func() -> void: match_launch_requested.emit())
	strip_hbox.add_child(_play_next_btn)

	# 3. Tabbed View Navigation: My Club Fixtures | League Matchdays | Results Archive
	var tabs_hbox := HFlowContainer.new()
	tabs_hbox.add_theme_constant_override("h_separation", 8)
	tabs_hbox.add_theme_constant_override("v_separation", 6)
	main_vbox.add_child(tabs_hbox)

	var tab_defs: Array[Dictionary] = [
		{ "id": "my_fixtures", "label": "Club Fixtures" },
		{ "id": "league_matchdays", "label": "League Matchdays" },
		{ "id": "results_archive", "label": "Results" }
	]
	for td in tab_defs:
		var tid: String = td["id"]
		var btn := Button.new()
		btn.text = td["label"]
		btn.custom_minimum_size = Vector2(185, 36)
		btn.focus_mode = Control.FOCUS_NONE
		btn.pressed.connect(func() -> void: _switch_tab(tid))
		tabs_hbox.add_child(btn)
		_tab_buttons[tid] = btn

	# Tab Content Host
	var content_host := Control.new()
	content_host.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	content_host.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_child(content_host)

	_build_my_fixtures_tab(content_host)
	_build_league_matchdays_tab(content_host)
	_build_results_archive_tab(content_host)
	_build_detail_modal()

	_switch_tab("my_fixtures")

func _build_my_fixtures_tab(parent: Control) -> void:
	var panel := VBoxContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.add_theme_constant_override("separation", 10)
	parent.add_child(panel)
	_tab_panels["my_fixtures"] = panel

	var filter_hbox := HFlowContainer.new()
	filter_hbox.add_theme_constant_override("h_separation", 6)
	filter_hbox.add_theme_constant_override("v_separation", 6)
	panel.add_child(filter_hbox)

	var filters: Array[Dictionary] = [
		{ "id": "ALL", "label": "All Season (Jul–May)" },
		{ "id": "PRE_SEASON", "label": "Weeks 1–2: Pre-Season (Jul)" },
		{ "id": "LEAGUE", "label": "Week 3+: League (Mid-Aug–May)" },
		{ "id": "HOME", "label": "Home (H)" },
		{ "id": "AWAY", "label": "Away (A)" }
	]
	for flt in filters:
		var fid: String = flt["id"]
		var btn := Button.new()
		btn.text = flt["label"]
		btn.focus_mode = Control.FOCUS_NONE
		btn.pressed.connect(func() -> void:
			_my_filter = fid
			_refresh_my_fixtures_tab()
		)
		filter_hbox.add_child(btn)
		_my_filter_buttons[fid] = btn

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	panel.add_child(scroll)

	_my_fixtures_vbox = VBoxContainer.new()
	_my_fixtures_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_my_fixtures_vbox.add_theme_constant_override("separation", 8)
	scroll.add_child(_my_fixtures_vbox)

func _build_league_matchdays_tab(parent: Control) -> void:
	var panel := VBoxContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.add_theme_constant_override("separation", 10)
	panel.visible = false
	parent.add_child(panel)
	_tab_panels["league_matchdays"] = panel

	var nav_hbox := HFlowContainer.new()
	nav_hbox.add_theme_constant_override("h_separation", 8)
	nav_hbox.add_theme_constant_override("v_separation", 6)
	panel.add_child(nav_hbox)

	_md_prev_btn = Button.new()
	_md_prev_btn.text = "◀ Prev"
	_md_prev_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_md_prev_btn, Color(0.07, 0.11, 0.18, 0.95), Color(0.18, 0.28, 0.44, 0.85))
	_md_prev_btn.pressed.connect(func() -> void:
		_selected_round_key = maxi(0, _selected_round_key - 1)
		_refresh_league_matchdays_tab()
	)
	nav_hbox.add_child(_md_prev_btn)

	_md_option = OptionButton.new()
	_md_option.custom_minimum_size = Vector2(290, 34)
	_md_option.focus_mode = Control.FOCUS_NONE
	_md_option.item_selected.connect(func(idx: int) -> void:
		_selected_round_key = _md_option.get_item_id(idx)
		_refresh_league_matchdays_tab()
	)
	_style_btn(_md_option, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.30, 0.48, 0.85))
	nav_hbox.add_child(_md_option)

	_md_next_btn = Button.new()
	_md_next_btn.text = "Next ▶"
	_md_next_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_md_next_btn, Color(0.07, 0.11, 0.18, 0.95), Color(0.18, 0.28, 0.44, 0.85))
	_md_next_btn.pressed.connect(func() -> void:
		var max_md: int = career_state.get_total_matchdays("Premier Division") if is_instance_valid(career_state) else 30
		_selected_round_key = mini(max_md, _selected_round_key + 1)
		_refresh_league_matchdays_tab()
	)
	nav_hbox.add_child(_md_next_btn)

	_md_current_btn = Button.new()
	_md_current_btn.text = "Current Round"
	_md_current_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_md_current_btn, Color(0.06, 0.38, 0.25, 0.95), Color(0.16, 0.88, 0.55, 0.90))
	_md_current_btn.pressed.connect(func() -> void:
		_sync_default_round_key()
		_refresh_league_matchdays_tab()
	)
	nav_hbox.add_child(_md_current_btn)

	_md_date_lbl = Label.new()
	_md_date_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_md_date_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_md_date_lbl.add_theme_color_override("font_color", Color(0.75, 0.86, 0.98))
	_md_date_lbl.add_theme_font_size_override("font_size", 12)
	nav_hbox.add_child(_md_date_lbl)

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	panel.add_child(scroll)

	_md_fixtures_vbox = VBoxContainer.new()
	_md_fixtures_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_md_fixtures_vbox.add_theme_constant_override("separation", 8)
	scroll.add_child(_md_fixtures_vbox)

func _build_results_archive_tab(parent: Control) -> void:
	var panel := VBoxContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.add_theme_constant_override("separation", 10)
	panel.visible = false
	parent.add_child(panel)
	_tab_panels["results_archive"] = panel

	var bar := HFlowContainer.new()
	bar.add_theme_constant_override("h_separation", 8)
	bar.add_theme_constant_override("v_separation", 6)
	panel.add_child(bar)

	for sc in [{ "id": "MY_CLUB", "label": "My Club Results" }, { "id": "ALL_LEAGUE", "label": "All League Results" }]:
		var sid: String = sc["id"]
		var btn := Button.new()
		btn.text = sc["label"]
		btn.focus_mode = Control.FOCUS_NONE
		btn.pressed.connect(func() -> void:
			_archive_scope = sid
			_refresh_results_archive_tab()
		)
		bar.add_child(btn)
		_archive_scope_buttons[sid] = btn

	bar.add_child(VSeparator.new())

	for oc in [{ "id": "ALL", "label": "All" }, { "id": "W", "label": "Wins" }, { "id": "D", "label": "Draws" }, { "id": "L", "label": "Losses" }]:
		var oid: String = oc["id"]
		var btn := Button.new()
		btn.text = oc["label"]
		btn.focus_mode = Control.FOCUS_NONE
		btn.pressed.connect(func() -> void:
			_archive_outcome = oid
			_refresh_results_archive_tab()
		)
		bar.add_child(btn)
		_archive_outcome_buttons[oid] = btn

	_archive_stats_lbl = Label.new()
	_archive_stats_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_archive_stats_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_archive_stats_lbl.add_theme_color_override("font_color", Color(0.35, 0.98, 0.65))
	_archive_stats_lbl.add_theme_font_size_override("font_size", 12)
	bar.add_child(_archive_stats_lbl)

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	panel.add_child(scroll)

	_archive_vbox = VBoxContainer.new()
	_archive_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_archive_vbox.add_theme_constant_override("separation", 8)
	scroll.add_child(_archive_vbox)

func _build_detail_modal() -> void:
	_detail_modal = PanelContainer.new()
	_detail_modal.anchor_left = 0.5
	_detail_modal.anchor_top = 0.5
	_detail_modal.anchor_right = 0.5
	_detail_modal.anchor_bottom = 0.5
	_detail_modal.offset_left = -260.0
	_detail_modal.offset_top = -200.0
	_detail_modal.offset_right = 260.0
	_detail_modal.offset_bottom = 200.0
	_detail_modal.grow_horizontal = Control.GROW_DIRECTION_BOTH
	_detail_modal.grow_vertical = Control.GROW_DIRECTION_BOTH
	_detail_modal.custom_minimum_size = Vector2(520, 400)
	_detail_modal.visible = false

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.040, 0.058, 0.092, 0.99)
	style.border_color = Color(0.15, 0.88, 0.52, 0.95)
	style.set_border_width_all(2)
	style.set_corner_radius_all(10)
	style.set_content_margin_all(20)
	_detail_modal.add_theme_stylebox_override("panel", style)
	add_child(_detail_modal)

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 10)
	_detail_modal.add_child(vb)

	_detail_title_lbl = Label.new()
	_detail_title_lbl.text = "MATCH REPORT"
	_detail_title_lbl.add_theme_color_override("font_color", Color(0.15, 0.92, 0.58))
	_detail_title_lbl.add_theme_font_size_override("font_size", 16)
	vb.add_child(_detail_title_lbl)

	_detail_meta_lbl = Label.new()
	_detail_meta_lbl.add_theme_color_override("font_color", Color(0.70, 0.80, 0.92))
	_detail_meta_lbl.add_theme_font_size_override("font_size", 12)
	vb.add_child(_detail_meta_lbl)

	_detail_score_lbl = Label.new()
	_detail_score_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_detail_score_lbl.add_theme_color_override("font_color", Color.WHITE)
	_detail_score_lbl.add_theme_font_size_override("font_size", 20)
	vb.add_child(_detail_score_lbl)

	vb.add_child(HSeparator.new())

	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	vb.add_child(scroll)

	_detail_events_vbox = VBoxContainer.new()
	_detail_events_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_detail_events_vbox.add_theme_constant_override("separation", 6)
	scroll.add_child(_detail_events_vbox)

	var btn_row := HBoxContainer.new()
	btn_row.alignment = BoxContainer.ALIGNMENT_CENTER
	btn_row.add_theme_constant_override("separation", 12)
	vb.add_child(btn_row)

	_detail_action_btn = Button.new()
	_detail_action_btn.text = "Go to Pre-Match"
	_detail_action_btn.custom_minimum_size = Vector2(160, 34)
	_detail_action_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_detail_action_btn, Color(0.06, 0.45, 0.30, 0.95), Color(0.15, 0.92, 0.58, 0.90))
	_detail_action_btn.pressed.connect(func() -> void:
		_detail_modal.visible = false
		match_launch_requested.emit()
	)
	btn_row.add_child(_detail_action_btn)

	var close_btn := Button.new()
	close_btn.text = "Close"
	close_btn.custom_minimum_size = Vector2(110, 34)
	close_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(close_btn, Color(0.09, 0.14, 0.22, 0.95), Color(0.20, 0.32, 0.48, 0.9))
	close_btn.pressed.connect(func() -> void: _detail_modal.visible = false)
	btn_row.add_child(close_btn)

func _switch_tab(tab_id: String) -> void:
	_active_tab = tab_id
	for tid in _tab_buttons.keys():
		var btn: Button = _tab_buttons[tid]
		if tid == tab_id:
			_style_btn(btn, Color(0.08, 0.35, 0.22, 0.95), Color(0.18, 0.95, 0.58, 0.95), Color(0.35, 1.0, 0.70))
		else:
			_style_btn(btn, Color(0.06, 0.09, 0.15, 0.85), Color(0.14, 0.20, 0.30, 0.70), Color(0.80, 0.88, 0.96))
	for tid in _tab_panels.keys():
		_tab_panels[tid].visible = (tid == tab_id)
	refresh_schedule()

func _sync_default_round_key() -> void:
	if not is_instance_valid(career_state):
		_selected_round_key = 0
		return
	var next_fix: Dictionary = career_state.get_next_fixture()
	if not next_fix.is_empty():
		_selected_round_key = int(next_fix.get("matchday", 0))
	else:
		_selected_round_key = career_state.get_current_matchday()

func refresh_schedule() -> void:
	if not _ui_built or not is_instance_valid(career_state) or not career_state.is_career_active():
		return
	var phase: Dictionary = career_state.get_season_phase_info()
	_season_pill.text = "%s • Jul – May" % String(phase.get("season_label", "2026/27"))
	var full_phase_label: String = String(phase.get("phase_label", "Pre-Season"))
	var compact_phase_label: String = "Pre-Season" if full_phase_label.to_lower().contains("pre-season") else ("League" if full_phase_label.to_lower().contains("league") else full_phase_label)
	_phase_pill.text = "Wk %d • %s" % [int(phase.get("week", 1)), compact_phase_label]
	_phase_pill.tooltip_text = full_phase_label
	var next_fix: Dictionary = career_state.get_next_fixture()
	if next_fix.is_empty():
		_next_match_pill.text = "Season Complete"
		_next_match_pill.tooltip_text = "Season Complete (May)"
		_sim_next_btn.disabled = true
		_play_next_btn.disabled = true
	else:
		var is_home: bool = bool(next_fix.get("is_home", true))
		var opp: String = String(next_fix.get("away_club_name", "Opponent")) if is_home else String(next_fix.get("home_club_name", "Opponent"))
		var tag: String = "Friendly" if bool(next_fix.get("is_friendly", false)) else ("MD %d" % int(next_fix.get("matchday", 1)))
		var proj: Dictionary = _project_squad_readiness_for_fixture(next_fix)
		_next_match_pill.text = "Next: %s %s %s · XI ~%d%%" % [tag, "vs" if is_home else "@", opp, int(proj["projected_xi_stamina"])]
		_next_match_pill.tooltip_text = "Next (%s): %s %s (%s)\nIn %d day(s) · Projected XI Fitness: %d%% · Cleared: %d · Still Out: %d" % [tag, "vs" if is_home else "@", opp, String(next_fix.get("date_iso", "")), int(proj["days_until"]), int(proj["projected_xi_stamina"]), (proj["cleared_names"] as Array).size(), (proj["ruled_out_names"] as Array).size()]
		_sim_next_btn.disabled = false
		_play_next_btn.disabled = false
	_rebuild_round_dropdown()
	match _active_tab:
		"my_fixtures":
			_refresh_my_fixtures_tab()
		"league_matchdays":
			_refresh_league_matchdays_tab()
		"results_archive":
			_refresh_results_archive_tab()

func _rebuild_round_dropdown() -> void:
	if not is_instance_valid(_md_option) or not is_instance_valid(career_state):
		return
	var prev_sel: int = _selected_round_key
	_md_option.clear()
	_md_option.add_item("Weeks 1–2 • Pre-Season Friendlies (July)", 0)
	var total_md: int = career_state.get_total_matchdays("Premier Division")
	for md in range(1, total_md + 1):
		var wk: int = md + 2
		var label_str: String = "Week %d • Matchday %d" % [wk, md]
		if md == 1:
			label_str += " (Mid-August Kickoff)"
		elif md == total_md:
			label_str += " (May Season Finale)"
		_md_option.add_item(label_str, md)

	for i in range(_md_option.item_count):
		if _md_option.get_item_id(i) == prev_sel:
			_md_option.select(i)
			break

func _refresh_my_fixtures_tab() -> void:
	for fid in _my_filter_buttons.keys():
		var btn: Button = _my_filter_buttons[fid]
		if fid == _my_filter:
			_style_btn(btn, Color(0.08, 0.35, 0.22, 0.95), Color(0.18, 0.95, 0.58, 0.95), Color(0.35, 1.0, 0.70))
		else:
			_style_btn(btn, Color(0.06, 0.09, 0.15, 0.85), Color(0.14, 0.20, 0.30, 0.70), Color(0.80, 0.88, 0.96))

	for child in _my_fixtures_vbox.get_children():
		child.queue_free()

	var club_id: int = career_state.get_managed_club_id()
	var fixtures: Array[Dictionary] = career_state.get_fixtures("", -1, club_id)
	var next_fix: Dictionary = career_state.get_next_fixture()
	var next_id: int = int(next_fix.get("fixture_id", -1))

	var shown_pre_hdr: bool = false
	var shown_league_hdr: bool = false

	for fix in fixtures:
		var is_friendly: bool = bool(fix.get("is_friendly", false))
		var is_home: bool = bool(fix.get("is_home", true))

		if _my_filter == "PRE_SEASON" and not is_friendly:
			continue
		if _my_filter == "LEAGUE" and is_friendly:
			continue
		if _my_filter == "HOME" and not is_home:
			continue
		if _my_filter == "AWAY" and is_home:
			continue

		if is_friendly and not shown_pre_hdr:
			shown_pre_hdr = true
			_my_fixtures_vbox.add_child(_create_section_header("PRE-SEASON WINDOW • WEEKS 1–2 (JULY FRIENDLIES)"))
		elif not is_friendly and not shown_league_hdr:
			shown_league_hdr = true
			_my_fixtures_vbox.add_child(_create_section_header("COMPETITIVE LEAGUE SEASON • WEEK 3+ (MID-AUGUST TO MAY)"))

		_my_fixtures_vbox.add_child(_create_fixture_card(fix, int(fix.get("fixture_id", -1)) == next_id))

func _refresh_league_matchdays_tab() -> void:
	for child in _md_fixtures_vbox.get_children():
		child.queue_free()

	var fixtures: Array[Dictionary] = career_state.get_fixtures("", _selected_round_key, -1)
	if fixtures.is_empty():
		_md_date_lbl.text = "No fixtures scheduled"
		return

	var first_date: String = String(fixtures[0].get("date_iso", ""))
	if _selected_round_key == 0:
		_md_date_lbl.text = "Weeks 1–2 • July Pre-Season Window (Starts %s)" % first_date
	else:
		_md_date_lbl.text = "Week %d • Matchday %d (%s)" % [int(fixtures[0].get("week", 3)), _selected_round_key, first_date]

	var next_fix: Dictionary = career_state.get_next_fixture()
	var next_id: int = int(next_fix.get("fixture_id", -1))

	for fix in fixtures:
		_md_fixtures_vbox.add_child(_create_fixture_card(fix, int(fix.get("fixture_id", -1)) == next_id))

func _refresh_results_archive_tab() -> void:
	for sid in _archive_scope_buttons.keys():
		var btn: Button = _archive_scope_buttons[sid]
		if sid == _archive_scope:
			_style_btn(btn, Color(0.08, 0.35, 0.22, 0.95), Color(0.18, 0.95, 0.58, 0.95), Color(0.35, 1.0, 0.70))
		else:
			_style_btn(btn, Color(0.06, 0.09, 0.15, 0.85), Color(0.14, 0.20, 0.30, 0.70), Color(0.80, 0.88, 0.96))

	for oid in _archive_outcome_buttons.keys():
		var btn: Button = _archive_outcome_buttons[oid]
		btn.disabled = (_archive_scope != "MY_CLUB")
		if oid == _archive_outcome and _archive_scope == "MY_CLUB":
			_style_btn(btn, Color(0.08, 0.35, 0.22, 0.95), Color(0.18, 0.95, 0.58, 0.95), Color(0.35, 1.0, 0.70))
		else:
			_style_btn(btn, Color(0.06, 0.09, 0.15, 0.85), Color(0.14, 0.20, 0.30, 0.70), Color(0.80, 0.88, 0.96))

	for child in _archive_vbox.get_children():
		child.queue_free()

	var filter_club: int = career_state.get_managed_club_id() if _archive_scope == "MY_CLUB" else -1
	var all_fixtures: Array[Dictionary] = career_state.get_fixtures("", -1, filter_club)

	var completed: Array[Dictionary] = []
	var wins: int = 0
	var draws: int = 0
	var losses: int = 0
	var gf: int = 0
	var ga: int = 0

	for fix in all_fixtures:
		if not bool(fix.get("played", false)):
			continue
		if _archive_scope == "MY_CLUB":
			var res: String = String(fix.get("user_result", ""))
			var is_home: bool = bool(fix.get("is_home", true))
			var hg: int = int(fix.get("home_goals", 0))
			var ag: int = int(fix.get("away_goals", 0))
			gf += hg if is_home else ag
			ga += ag if is_home else hg
			if res == "W": wins += 1
			elif res == "D": draws += 1
			elif res == "L": losses += 1

			if _archive_outcome != "ALL" and res != _archive_outcome:
				continue
		completed.append(fix)

	completed.reverse()
	if _archive_scope == "MY_CLUB":
		_archive_stats_lbl.text = "Played: %d  |  %dW - %dD - %dL  |  GF %d / GA %d" % [completed.size(), wins, draws, losses, gf, ga]
	else:
		_archive_stats_lbl.text = "Total Completed Matches: %d" % completed.size()

	if completed.is_empty():
		var empty_lbl := Label.new()
		empty_lbl.text = "No completed matches in the archive yet. Advance the calendar or play your next match."
		empty_lbl.add_theme_color_override("font_color", Color(0.60, 0.70, 0.84))
		_archive_vbox.add_child(empty_lbl)
		return

	for fix in completed:
		_archive_vbox.add_child(_create_fixture_card(fix, false))

func _create_section_header(text: String) -> Label:
	var lbl := Label.new()
	lbl.text = text
	lbl.add_theme_color_override("font_color", Color(0.55, 0.72, 0.90))
	lbl.add_theme_font_size_override("font_size", 11)
	return lbl

func _create_fixture_card(fix: Dictionary, is_next_match: bool) -> PanelContainer:
	var card := PanelContainer.new()
	card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	var is_user: bool = bool(fix.get("is_user_match", false))
	var is_friendly: bool = bool(fix.get("is_friendly", false))

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.050, 0.080, 0.125, 0.95) if is_user else Color(0.042, 0.060, 0.098, 0.90)
	style.border_color = Color(0.18, 0.95, 0.58, 0.95) if is_next_match else (Color(0.16, 0.45, 0.35, 0.85) if is_user else Color(0.13, 0.19, 0.30, 0.75))
	style.set_border_width_all(2 if is_next_match else 1)
	style.set_corner_radius_all(8)
	style.content_margin_left = 14
	style.content_margin_right = 14
	style.content_margin_top = 10
	style.content_margin_bottom = 10
	card.add_theme_stylebox_override("panel", style)

	var hbox := HFlowContainer.new()
	hbox.add_theme_constant_override("h_separation", 12)
	hbox.add_theme_constant_override("v_separation", 6)
	card.add_child(hbox)

	var wk: int = int(fix.get("week", 1))
	var md: int = int(fix.get("matchday", 0))
	var round_tag: String = ("WK %d • FRIENDLY" % wk) if is_friendly else ("WK %d • MD %d" % [wk, md])
	var round_badge := _create_badge(
		round_tag,
		Color(0.65, 0.48, 0.12) if is_friendly else Color(0.18, 0.38, 0.68),
		Color(1.0, 0.88, 0.35) if is_friendly else Color(0.78, 0.90, 1.0)
	)
	hbox.add_child(round_badge)

	var date_lbl := Label.new()
	date_lbl.text = String(fix.get("date_iso", ""))
	date_lbl.custom_minimum_size = Vector2(92, 0)
	date_lbl.add_theme_color_override("font_color", Color(0.68, 0.78, 0.90))
	date_lbl.add_theme_font_size_override("font_size", 12)
	hbox.add_child(date_lbl)

	var comp_raw: String = String(fix.get("competition", ""))
	var comp_lower: String = comp_raw.to_lower()
	var is_ucl: bool = comp_lower.contains("champions") or comp_lower.contains("ucl")
	var comp_tag_text: String = "Friendlies" if is_friendly else ("UCL" if is_ucl else ("League Match %d" % maxi(1, md)))
	var comp_bg: Color = Color(0.55, 0.40, 0.14) if is_friendly else (Color(0.22, 0.34, 0.72) if is_ucl else Color(0.10, 0.46, 0.36))
	var comp_fg: Color = Color(1.0, 0.88, 0.40) if is_friendly else (Color(0.65, 0.85, 1.0) if is_ucl else Color(0.40, 0.98, 0.72))
	hbox.add_child(_create_badge(comp_tag_text, comp_bg, comp_fg))

	var match_lbl := Label.new()
	match_lbl.text = "%s  vs  %s" % [String(fix.get("home_club_name", "Home")), String(fix.get("away_club_name", "Away"))]
	match_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	match_lbl.custom_minimum_size = Vector2(180, 0)
	match_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	match_lbl.add_theme_color_override("font_color", Color.WHITE if is_user else Color(0.85, 0.90, 0.96))
	match_lbl.add_theme_font_size_override("font_size", 13)
	hbox.add_child(match_lbl)

	var stad_lbl := Label.new()
	stad_lbl.text = String(fix.get("stadium", ""))
	stad_lbl.custom_minimum_size = Vector2(100, 0)
	stad_lbl.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	stad_lbl.tooltip_text = stad_lbl.text
	stad_lbl.add_theme_color_override("font_color", Color(0.55, 0.66, 0.80))
	stad_lbl.add_theme_font_size_override("font_size", 11)
	hbox.add_child(stad_lbl)

	var played: bool = bool(fix.get("played", false))
	if played:
		var score_str: String = "%d - %d" % [int(fix.get("home_goals", 0)), int(fix.get("away_goals", 0))]
		var ures: String = String(fix.get("user_result", ""))
		var bg_c := Color(0.22, 0.32, 0.48)
		var txt_c := Color.WHITE
		if ures == "W":
			bg_c = Color(0.08, 0.65, 0.38)
			txt_c = Color(0.30, 1.0, 0.68)
		elif ures == "D":
			bg_c = Color(0.65, 0.48, 0.12)
			txt_c = Color(1.0, 0.88, 0.35)
		elif ures == "L":
			bg_c = Color(0.75, 0.22, 0.28)
			txt_c = Color(1.0, 0.70, 0.75)
		hbox.add_child(_create_badge(score_str if ures.is_empty() else ("%s (%s)" % [score_str, ures]), bg_c, txt_c))
	elif is_next_match:
		hbox.add_child(_create_badge("NEXT MATCH", Color(0.08, 0.68, 0.42), Color(0.35, 1.0, 0.72)))
	else:
		hbox.add_child(_create_badge("UPCOMING", Color(0.16, 0.24, 0.38), Color(0.65, 0.78, 0.92)))

	if not played and is_user:
		var proj: Dictionary = _project_squad_readiness_for_fixture(fix)
		var out_cnt: int = (proj["ruled_out_names"] as Array).size()
		var proj_fit: int = int(proj["projected_xi_stamina"])
		var rec_label: String = "XI ~%d%%" % proj_fit if out_cnt == 0 else "XI ~%d%% · %d OUT" % [proj_fit, out_cnt]
		var rec_bg := Color(0.62, 0.22, 0.26) if out_cnt > 0 else (Color(0.55, 0.42, 0.12) if proj_fit < 85 else Color(0.08, 0.42, 0.32))
		var rec_fg := Color(1.0, 0.78, 0.80) if out_cnt > 0 else (Color(1.0, 0.88, 0.40) if proj_fit < 85 else Color(0.45, 1.0, 0.75))
		hbox.add_child(_create_badge(rec_label, rec_bg, rec_fg))

	var inspect_btn := Button.new()
	inspect_btn.text = "Report" if played else "Details"
	inspect_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(inspect_btn, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.30, 0.48, 0.85))
	var fix_id: int = int(fix.get("fixture_id", 0))
	inspect_btn.pressed.connect(func() -> void: _open_fixture_detail_modal(fix_id))
	hbox.add_child(inspect_btn)

	return card

func _open_fixture_detail_modal(fixture_id: int) -> void:
	if not is_instance_valid(career_state):
		return
	var det: Dictionary = career_state.get_fixture_details(fixture_id)
	if det.is_empty():
		return

	var played: bool = bool(det.get("played", false))
	var is_friendly: bool = bool(det.get("is_friendly", false))
	var wk: int = int(det.get("week", 1))
	var md: int = int(det.get("matchday", 0))

	_detail_title_lbl.text = "%s • %s" % [
		String(det.get("competition", "Match")).to_upper(),
		("WEEK %d (JULY PRE-SEASON)" % wk) if is_friendly else ("WEEK %d • MATCHDAY %d" % [wk, md])
	]
	_detail_meta_lbl.text = "Date: %s   |   Venue: %s   |   Importance: %s" % [
		String(det.get("date_iso", "")),
		String(det.get("stadium", "")),
		String(det.get("match_importance", "STANDARD"))
	]

	var home_name: String = String(det.get("home_club_name", "Home"))
	var away_name: String = String(det.get("away_club_name", "Away"))
	if played:
		_detail_score_lbl.text = "%s   %d  -  %d   %s" % [
			home_name, int(det.get("home_goals", 0)), int(det.get("away_goals", 0)), away_name
		]
	else:
		_detail_score_lbl.text = "%s (OVR %d)   vs   %s (OVR %d)" % [
			home_name, int(det.get("home_ovr", 75)), away_name, int(det.get("away_ovr", 75))
		]

	for child in _detail_events_vbox.get_children():
		child.queue_free()

	if played:
		var att_lbl := Label.new()
		att_lbl.text = "Attendance: %d" % int(det.get("attendance", 0))
		att_lbl.add_theme_color_override("font_color", Color(0.65, 0.78, 0.92))
		att_lbl.add_theme_font_size_override("font_size", 12)
		_detail_events_vbox.add_child(att_lbl)

		var events: Array = det.get("events", [])
		if events.is_empty():
			var no_ev := Label.new()
			no_ev.text = "No goals recorded in this match."
			no_ev.add_theme_color_override("font_color", Color(0.65, 0.75, 0.88))
			_detail_events_vbox.add_child(no_ev)
		else:
			for ev in events:
				var ev_lbl := Label.new()
				var assist_name: String = String(ev.get("assist_player_name", ""))
				var assist_str: String = (" (ast. %s)" % assist_name) if not assist_name.is_empty() else ""
				ev_lbl.text = "%d'  ⚽ %s (%s)%s" % [
					int(ev.get("minute", 1)),
					String(ev.get("player_name", "Scorer")),
					String(ev.get("club_name", "")),
					assist_str
				]
				ev_lbl.add_theme_color_override("font_color", Color(0.88, 0.94, 1.0))
				ev_lbl.add_theme_font_size_override("font_size", 12)
				_detail_events_vbox.add_child(ev_lbl)
	else:
		var preview_lbl := Label.new()
		preview_lbl.text = "Formations: %s (%s) vs %s (%s)\nStatus: Scheduled — advance calendar or launch matchday to play." % [
			home_name, String(det.get("home_formation", "4-3-3")),
			away_name, String(det.get("away_formation", "4-3-3"))
		]
		preview_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		preview_lbl.add_theme_color_override("font_color", Color(0.80, 0.88, 0.96))
		preview_lbl.add_theme_font_size_override("font_size", 12)
		_detail_events_vbox.add_child(preview_lbl)

		var proj: Dictionary = _project_squad_readiness_for_fixture(det)
		var rec_lbl := Label.new()
		var cleared: Array = proj["cleared_names"]
		var ruled_out: Array = proj["ruled_out_names"]
		var cleared_str: String = ", ".join(PackedStringArray(cleared)) if not cleared.is_empty() else "None"
		var out_str: String = ", ".join(PackedStringArray(ruled_out)) if not ruled_out.is_empty() else "None"
		rec_lbl.text = "\nSquad Recovery Projection (%d day(s) out · ~%d%%/day):\n• Projected Starting XI Stamina: %d%%\n• Cleared by Kickoff: %s\n• Still Ruled Out: %s" % [int(proj["days_until"]), int(round(float(proj["recovery_rate"]) * 100.0)), int(proj["projected_xi_stamina"]), cleared_str, out_str]
		rec_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		rec_lbl.add_theme_color_override("font_color", Color(0.55, 0.92, 0.76) if ruled_out.is_empty() else Color(1.0, 0.78, 0.42))
		rec_lbl.add_theme_font_size_override("font_size", 12)
		_detail_events_vbox.add_child(rec_lbl)

	var next_fix: Dictionary = career_state.get_next_fixture()
	_detail_action_btn.visible = (not played and int(next_fix.get("fixture_id", -1)) == fixture_id)
	_detail_modal.visible = true

func _on_advance_to_next_pressed() -> void:
	if not is_instance_valid(career_state):
		return
	career_state.advance_to_next_matchday()
	_sync_default_round_key()
	refresh_schedule()

func _project_squad_readiness_for_fixture(fix: Dictionary) -> Dictionary:
	var rec_rate: float = 0.30
	if is_instance_valid(career_state) and career_state.has_method("get_stamina_recovery_rate_per_day"):
		rec_rate = clampf(float(career_state.call("get_stamina_recovery_rate_per_day")), 0.05, 1.0)
	var phase: Dictionary = career_state.get_season_phase_info() if is_instance_valid(career_state) else {}
	var days_until: int = int(fix.get("days_until", -1))
	if days_until < 0:
		var current_date: String = String(phase.get("current_date", ""))
		var fixture_date: String = String(fix.get("date_iso", ""))
		if not current_date.is_empty() and not fixture_date.is_empty():
			var current_ts: int = int(Time.get_unix_time_from_datetime_string(current_date + "T00:00:00"))
			var fixture_ts: int = int(Time.get_unix_time_from_datetime_string(fixture_date + "T00:00:00"))
			days_until = maxi(0, int(floor(float(fixture_ts - current_ts) / 86400.0)))
		else:
			var cur_wk: int = int(phase.get("week", 1))
			var fix_wk: int = int(fix.get("week", cur_wk))
			days_until = maxi(0, (fix_wk - cur_wk) * 7)
	var club_id: int = career_state.get_managed_club_id() if is_instance_valid(career_state) else -1
	var squad: Array = career_state.get_club_squad(club_id) if club_id != -1 else []
	var xi_sum: float = 0.0
	var xi_count: int = 0
	var cleared_names: Array[String] = []
	var ruled_out_names: Array[String] = []
	for i in range(squad.size()):
		var p: Dictionary = squad[i]
		var cur_stam: float = float(p.get("condition", p.get("stamina", 100.0)))
		var proj_stam: float = minf(100.0, cur_stam + float(days_until) * rec_rate * 100.0)
		if i < 11:
			xi_sum += proj_stam
			xi_count += 1
		var is_inj: bool = bool(p.get("is_injured", false)) or int(p.get("injury_days_remaining", 0)) > 0
		var rem_days: int = int(p.get("injury_days_remaining", 0))
		if is_inj and rem_days > 0:
			var pname: String = String(p.get("name", "Player"))
			if rem_days <= days_until:
				cleared_names.append("%s (clears in %dd)" % [pname, rem_days])
			else:
				ruled_out_names.append("%s (%dd left at kickoff)" % [pname, rem_days - days_until])
	return {"days_until": days_until, "recovery_rate": rec_rate, "projected_xi_stamina": int(round(xi_sum / maxf(1.0, float(xi_count)))), "cleared_names": cleared_names, "ruled_out_names": ruled_out_names}
