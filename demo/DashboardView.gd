class_name DashboardView
extends PanelContainer

signal route_requested(route_name: String)
signal player_profile_requested(player_id: int, context_mode: String)
signal match_launch_requested

var career_state: CareerState

# Next Match Hero Widgets
var _next_match_card: PanelContainer
var _opp_name_lbl: Label
var _opp_badge_lbl: Label
var _match_meta_lbl: Label
var _opp_form_lbl: Label
var _opp_ovr_lbl: Label
var _pre_match_btn: Button

# Responsive Split Container
var _split_container: BoxContainer

# League & Club Overview Widgets
var _standings_vbox: VBoxContainer
var _club_rank_lbl: Label
var _club_pts_lbl: Label
var _club_record_lbl: Label
var _club_balance_lbl: Label
var _fitness_summary_lbl: Label

# Objectives & News Feed Containers
var _objectives_vbox: VBoxContainer
var _news_vbox: VBoxContainer

func _ready() -> void:
	_ensure_ui_built()
	resized.connect(_update_responsive_layout)
	visibility_changed.connect(func() -> void:
		if visible:
			refresh_dashboard()
	)
	refresh_dashboard()

func _ensure_ui_built() -> void:
	if _next_match_card != null:
		return

	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_flags_vertical = Control.SIZE_EXPAND_FILL

	var root_style := StyleBoxFlat.new()
	root_style.bg_color = Color(0.024, 0.035, 0.055, 1.0)
	root_style.content_margin_left = 20
	root_style.content_margin_right = 20
	root_style.content_margin_top = 16
	root_style.content_margin_bottom = 16
	add_theme_stylebox_override("panel", root_style)

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	add_child(scroll)

	var main_vbox := VBoxContainer.new()
	main_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	main_vbox.add_theme_constant_override("separation", 14)
	scroll.add_child(main_vbox)

	var page_title := Label.new()
	page_title.text = "CLUB DASHBOARD"
	page_title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	page_title.add_theme_font_size_override("font_size", 20)
	main_vbox.add_child(page_title)
	# 1. NEXT MATCH HERO CARD
	_build_next_match_hero(main_vbox)

	# 2. QUICK ACTIONS BAR
	_build_quick_actions(main_vbox)

	# 3. SPLIT ROW: LEAGUE & CLUB OVERVIEW (LEFT) + BOARD & NEWS (RIGHT)
	_split_container = BoxContainer.new()
	_split_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_split_container.add_theme_constant_override("separation", 16)
	main_vbox.add_child(_split_container)

	_build_league_overview(_split_container)
	_build_board_and_news(_split_container)
	_update_responsive_layout()

func _update_responsive_layout() -> void:
	if not is_instance_valid(_split_container):
		return
	_split_container.vertical = (size.x > 0.0 and size.x < 860.0)

func _apply_button_theme(btn: Button, bg: Color, border: Color, text_col: Color = Color.WHITE) -> void:
	var normal := StyleBoxFlat.new()
	normal.bg_color = bg
	normal.border_color = border
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(6)
	normal.content_margin_left = 10
	normal.content_margin_right = 10
	normal.content_margin_top = 6
	normal.content_margin_bottom = 6

	var hover := normal.duplicate()
	hover.bg_color = bg.lightened(0.10)
	hover.border_color = border.lightened(0.15)

	var pressed := normal.duplicate()
	pressed.bg_color = bg.darkened(0.12)

	var disabled := normal.duplicate()
	disabled.bg_color = Color(0.05, 0.07, 0.10, 0.55)
	disabled.border_color = Color(0.12, 0.16, 0.22, 0.45)

	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("pressed", pressed)
	btn.add_theme_stylebox_override("disabled", disabled)
	btn.add_theme_color_override("font_color", text_col)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)
	btn.add_theme_color_override("font_pressed_color", Color(0.85, 0.98, 0.90))
	btn.add_theme_color_override("font_disabled_color", Color(0.45, 0.52, 0.62))

func _build_next_match_hero(parent: Control) -> void:
	_next_match_card = PanelContainer.new()
	_next_match_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	var card_style := StyleBoxFlat.new()
	card_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	card_style.border_color = Color(0.18, 0.95, 0.58, 0.95)
	card_style.set_border_width_all(2)
	card_style.set_corner_radius_all(10)
	card_style.set_content_margin_all(16)
	_next_match_card.add_theme_stylebox_override("panel", card_style)
	parent.add_child(_next_match_card)

	var hbox := HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 16)
	_next_match_card.add_child(hbox)

	# Opponent Badge
	var badge_box := PanelContainer.new()
	badge_box.custom_minimum_size = Vector2(58, 58)
	var bb_style := StyleBoxFlat.new()
	bb_style.bg_color = Color(0.09, 0.14, 0.24, 0.95)
	bb_style.border_color = Color(0.20, 0.40, 0.68, 0.85)
	bb_style.set_border_width_all(1)
	bb_style.set_corner_radius_all(8)
	badge_box.add_theme_stylebox_override("panel", bb_style)

	_opp_badge_lbl = Label.new()
	_opp_badge_lbl.text = "---"
	_opp_badge_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_opp_badge_lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_opp_badge_lbl.add_theme_color_override("font_color", Color(0.40, 0.85, 1.0))
	_opp_badge_lbl.add_theme_font_size_override("font_size", 15)
	badge_box.add_child(_opp_badge_lbl)
	hbox.add_child(badge_box)

	# Fixture Info VBox
	var info_vbox := VBoxContainer.new()
	info_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	info_vbox.add_theme_constant_override("separation", 4)
	hbox.add_child(info_vbox)

	var title_lbl := Label.new()
	title_lbl.text = "UPCOMING MATCHDAY FIXTURE"
	title_lbl.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title_lbl.add_theme_font_size_override("font_size", 11)
	info_vbox.add_child(title_lbl)

	_opp_name_lbl = Label.new()
	_opp_name_lbl.text = "Awaiting Career Initialization..."
	_opp_name_lbl.add_theme_color_override("font_color", Color.WHITE)
	_opp_name_lbl.add_theme_font_size_override("font_size", 18)
	info_vbox.add_child(_opp_name_lbl)

	_match_meta_lbl = Label.new()
	_match_meta_lbl.text = "Competition · Kickoff Date"
	_match_meta_lbl.add_theme_color_override("font_color", Color(0.65, 0.75, 0.88))
	_match_meta_lbl.add_theme_font_size_override("font_size", 12)
	info_vbox.add_child(_match_meta_lbl)

	_opp_form_lbl = Label.new()
	_opp_form_lbl.text = "Opponent Rank: -- · Record: 0W - 0D - 0L"
	_opp_form_lbl.add_theme_color_override("font_color", Color(0.55, 0.78, 0.92))
	_opp_form_lbl.add_theme_font_size_override("font_size", 11)
	info_vbox.add_child(_opp_form_lbl)

	# Opponent OVR
	var ovr_vbox := VBoxContainer.new()
	ovr_vbox.alignment = BoxContainer.ALIGNMENT_CENTER
	hbox.add_child(ovr_vbox)

	var ovr_title := Label.new()
	ovr_title.text = "OPPONENT OVR"
	ovr_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	ovr_title.add_theme_color_override("font_color", Color(0.50, 0.65, 0.80))
	ovr_title.add_theme_font_size_override("font_size", 10)
	ovr_vbox.add_child(ovr_title)

	_opp_ovr_lbl = Label.new()
	_opp_ovr_lbl.text = "--"
	_opp_ovr_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_opp_ovr_lbl.add_theme_color_override("font_color", Color(0.95, 0.80, 0.25))
	_opp_ovr_lbl.add_theme_font_size_override("font_size", 20)
	ovr_vbox.add_child(_opp_ovr_lbl)

	# Action Button: PRE-MATCH
	_pre_match_btn = Button.new()
	_pre_match_btn.text = "PRE-MATCH"
	_pre_match_btn.custom_minimum_size = Vector2(148, 44)
	_pre_match_btn.focus_mode = Control.FOCUS_NONE
	_apply_button_theme(_pre_match_btn, Color(0.06, 0.45, 0.32, 0.95), Color(0.15, 0.92, 0.58, 0.90))
	_pre_match_btn.add_theme_font_size_override("font_size", 13)
	_pre_match_btn.pressed.connect(func() -> void: match_launch_requested.emit())
	hbox.add_child(_pre_match_btn)

func _build_quick_actions(parent: Control) -> void:
	var bar := HFlowContainer.new()
	bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bar.add_theme_constant_override("h_separation", 8)
	bar.add_theme_constant_override("v_separation", 8)
	parent.add_child(bar)

	var actions: Array[Dictionary] = [
		{ "label": "Squad", "route": "squad" },
		{ "label": "Tactics", "route": "tactics" },
		{ "label": "Youth Academy", "route": "youth" },
		{ "label": "Transfers", "route": "transfers" },
		{ "label": "Schedule", "route": "schedule" },
		{ "label": "Standings", "route": "competitions" }
	]

	for act in actions:
		var btn := Button.new()
		btn.text = act["label"]
		btn.custom_minimum_size = Vector2(124, 36)
		btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		btn.focus_mode = Control.FOCUS_NONE
		_apply_button_theme(btn, Color(0.06, 0.09, 0.15, 0.90), Color(0.16, 0.24, 0.38, 0.85), Color(0.88, 0.94, 1.0))
		btn.add_theme_font_size_override("font_size", 12)

		var r: String = act["route"]
		btn.pressed.connect(func() -> void: route_requested.emit(r))
		bar.add_child(btn)

func _build_league_overview(parent: Control) -> void:
	var pc := PanelContainer.new()
	pc.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	pc.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	style.set_border_width_all(1)
	style.set_corner_radius_all(10)
	style.set_content_margin_all(14)
	pc.add_theme_stylebox_override("panel", style)
	parent.add_child(pc)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 10)
	pc.add_child(vbox)

	# Your Club Snapshot Header
	var club_hdr := Label.new()
	club_hdr.text = "CLUB OVERVIEW"
	club_hdr.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	club_hdr.add_theme_font_size_override("font_size", 14)
	vbox.add_child(club_hdr)

	var stats_strip := HBoxContainer.new()
	stats_strip.add_theme_constant_override("separation", 12)
	vbox.add_child(stats_strip)

	_club_rank_lbl = Label.new()
	_club_rank_lbl.text = "Pos: #1"
	_club_rank_lbl.add_theme_color_override("font_color", Color(0.95, 0.82, 0.30))
	_club_rank_lbl.add_theme_font_size_override("font_size", 12)
	stats_strip.add_child(_club_rank_lbl)

	_club_pts_lbl = Label.new()
	_club_pts_lbl.text = "0 Pts"
	_club_pts_lbl.add_theme_color_override("font_color", Color(0.25, 0.96, 0.62))
	_club_pts_lbl.add_theme_font_size_override("font_size", 12)
	stats_strip.add_child(_club_pts_lbl)

	_club_record_lbl = Label.new()
	_club_record_lbl.text = "0W - 0D - 0L (GD 0)"
	_club_record_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_club_record_lbl.add_theme_color_override("font_color", Color(0.75, 0.85, 0.96))
	_club_record_lbl.add_theme_font_size_override("font_size", 12)
	stats_strip.add_child(_club_record_lbl)

	_club_balance_lbl = Label.new()
	_club_balance_lbl.text = "$50.0M"
	_club_balance_lbl.add_theme_color_override("font_color", Color(0.35, 0.92, 0.68))
	_club_balance_lbl.add_theme_font_size_override("font_size", 12)
	stats_strip.add_child(_club_balance_lbl)

	vbox.add_child(HSeparator.new())

	var title := Label.new()
	title.text = "LEAGUE STANDINGS (TOP 5)"
	title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title.add_theme_font_size_override("font_size", 14)
	vbox.add_child(title)

	# Compact Table Headers
	var hdr := HBoxContainer.new()
	hdr.add_theme_constant_override("separation", 6)
	var r_h := Label.new(); r_h.text = "#"; r_h.custom_minimum_size = Vector2(24, 0); hdr.add_child(r_h)
	var c_h := Label.new(); c_h.text = "Club"; c_h.size_flags_horizontal = Control.SIZE_EXPAND_FILL; hdr.add_child(c_h)
	var p_h := Label.new(); p_h.text = "P"; p_h.custom_minimum_size = Vector2(26, 0); hdr.add_child(p_h)
	var gd_h := Label.new(); gd_h.text = "GD"; gd_h.custom_minimum_size = Vector2(30, 0); hdr.add_child(gd_h)
	var pts_h := Label.new(); pts_h.text = "Pts"; pts_h.custom_minimum_size = Vector2(32, 0); hdr.add_child(pts_h)
	for h_lbl in [r_h, c_h, p_h, gd_h, pts_h]:
		(h_lbl as Label).add_theme_color_override("font_color", Color(0.55, 0.68, 0.82))
		(h_lbl as Label).add_theme_font_size_override("font_size", 11)
	vbox.add_child(hdr)

	_standings_vbox = VBoxContainer.new()
	_standings_vbox.add_theme_constant_override("separation", 4)
	vbox.add_child(_standings_vbox)

	vbox.add_child(HSeparator.new())

	# Fitness & Morale Overview Snippet
	var fit_title := Label.new()
	fit_title.text = "SQUAD CONDITION & FITNESS"
	fit_title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	fit_title.add_theme_font_size_override("font_size", 14)
	vbox.add_child(fit_title)

	_fitness_summary_lbl = Label.new()
	_fitness_summary_lbl.text = "Average Squad Fitness: 100% · Fully Match-Fit"
	_fitness_summary_lbl.add_theme_color_override("font_color", Color(0.70, 0.85, 0.95))
	_fitness_summary_lbl.add_theme_font_size_override("font_size", 12)
	vbox.add_child(_fitness_summary_lbl)

func _build_board_and_news(parent: Control) -> void:
	var pc := PanelContainer.new()
	pc.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	pc.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.040, 0.060, 0.095, 0.92)
	style.border_color = Color(0.12, 0.18, 0.28, 0.78)
	style.set_border_width_all(1)
	style.set_corner_radius_all(8)
	style.set_content_margin_all(14)
	pc.add_theme_stylebox_override("panel", style)
	parent.add_child(pc)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 10)
	pc.add_child(vbox)

	var obj_title := Label.new()
	obj_title.text = "BOARD OBJECTIVES PROGRESS"
	obj_title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	obj_title.add_theme_font_size_override("font_size", 14)
	vbox.add_child(obj_title)

	vbox.add_child(HSeparator.new())

	_objectives_vbox = VBoxContainer.new()
	_objectives_vbox.add_theme_constant_override("separation", 8)
	vbox.add_child(_objectives_vbox)

	vbox.add_child(HSeparator.new())

	var news_title := Label.new()
	news_title.text = "LATEST CLUB INBOX ALERTS"
	news_title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	news_title.add_theme_font_size_override("font_size", 14)
	vbox.add_child(news_title)

	_news_vbox = VBoxContainer.new()
	_news_vbox.add_theme_constant_override("separation", 6)
	vbox.add_child(_news_vbox)

# -----------------------------------------------------------------------------
# Data Binding & Telemetry Refresh
# -----------------------------------------------------------------------------
func refresh_dashboard() -> void:
	_ensure_ui_built()
	if not is_instance_valid(career_state) or not career_state.is_career_active():
		return

	_refresh_next_match()
	_refresh_standings_snippet()
	_refresh_objectives()
	_refresh_news_snippet()

func _refresh_next_match() -> void:
	var fix: Dictionary = career_state.get_next_fixture()
	if fix.is_empty():
		_opp_name_lbl.text = "No upcoming matches"
		_match_meta_lbl.text = "Season schedule completed."
		_opp_form_lbl.text = "All fixtures resolved"
		_opp_badge_lbl.text = "---"
		_opp_ovr_lbl.text = "--"
		_pre_match_btn.disabled = true
		return

	var user_club_id: int = career_state.get_managed_club_id()
	var is_home: bool = bool(fix.get("is_home", int(fix.get("home_club_id", -1)) == user_club_id))
	var opp_name: String = String(fix.get("away_club_name" if is_home else "home_club_name", "Opponent"))
	var opp_id: int = int(fix.get("away_club_id" if is_home else "home_club_id", -1))
	var comp: String = String(fix.get("competition", "Premier Division"))
	var date_iso: String = String(fix.get("date_iso", ""))
	var matchday: int = int(fix.get("matchday", 1))

	_opp_name_lbl.text = "vs %s" % opp_name
	_opp_badge_lbl.text = opp_name.substr(0, 3).to_upper()
	_match_meta_lbl.text = "%s · MD %d · %s (%s)" % [comp, matchday, date_iso, "HOME" if is_home else "AWAY"]

	var opp_info: Dictionary = career_state.get_club_info(opp_id)
	_opp_ovr_lbl.text = str(int(opp_info.get("team_rating", 75)))

	# Lookup Opponent Standing & Record
	var opp_rank: int = 0
	var opp_rec: String = "0W - 0D - 0L"
	for row in career_state.get_league_standings():
		if int(row.get("club_id", -1)) == opp_id:
			opp_rank = int(row.get("rank", 0))
			opp_rec = "%dW - %dD - %dL (%d Pts)" % [
				int(row.get("won", 0)),
				int(row.get("drawn", 0)),
				int(row.get("lost", 0)),
				int(row.get("points", 0))
			]
			break
	_opp_form_lbl.text = "Opponent Rank: #%d · Record: %s" % [max(1, opp_rank), opp_rec]
	_pre_match_btn.disabled = false

func _refresh_standings_snippet() -> void:
	for child in _standings_vbox.get_children():
		child.queue_free()

	var standings: Array = career_state.get_league_standings()
	var user_club_id: int = career_state.get_managed_club_id()
	var fin: Dictionary = career_state.get_finances()
	var balance: int = int(fin.get("total_balance", 0))
	_club_balance_lbl.text = "$%.1fM" % (float(balance) / 1_000_000.0) if abs(balance) >= 1_000_000 else "$%d" % balance

	# Update User Club Overview Strip
	for i in range(standings.size()):
		var r_dict: Dictionary = standings[i]
		if int(r_dict.get("club_id", -1)) == user_club_id:
			var u_rank: int = int(r_dict.get("rank", i + 1))
			var u_pts: int = int(r_dict.get("points", 0))
			var u_w: int = int(r_dict.get("won", 0))
			var u_d: int = int(r_dict.get("drawn", 0))
			var u_l: int = int(r_dict.get("lost", 0))
			var u_gd: int = int(r_dict.get("goal_diff", int(r_dict.get("goals_for", 0)) - int(r_dict.get("goals_against", 0))))
			_club_rank_lbl.text = "Pos: #%d" % u_rank
			_club_pts_lbl.text = "%d Pts" % u_pts
			_club_record_lbl.text = "%dW - %dD - %dL (GD %+d)" % [u_w, u_d, u_l, u_gd]
			break

	# Display Top 5
	for i in range(min(5, standings.size())):
		var row_dict: Dictionary = standings[i]
		var rank: int = int(row_dict.get("rank", i + 1))
		var c_name: String = String(row_dict.get("club_name", "Club"))
		var cid: int = int(row_dict.get("club_id", -1))
		var played: int = int(row_dict.get("played", 0))
		var gd: int = int(row_dict.get("goal_diff", int(row_dict.get("goals_for", 0)) - int(row_dict.get("goals_against", 0))))
		var pts: int = int(row_dict.get("points", 0))

		var row := HBoxContainer.new()
		row.add_theme_constant_override("separation", 6)

		var r_lbl := Label.new(); r_lbl.text = str(rank); r_lbl.custom_minimum_size = Vector2(24, 0); row.add_child(r_lbl)
		var c_lbl := Label.new(); c_lbl.text = c_name; c_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL; c_lbl.clip_text = true; row.add_child(c_lbl)
		var p_lbl := Label.new(); p_lbl.text = str(played); p_lbl.custom_minimum_size = Vector2(26, 0); row.add_child(p_lbl)
		var gd_lbl := Label.new(); gd_lbl.text = str(gd); gd_lbl.custom_minimum_size = Vector2(30, 0); row.add_child(gd_lbl)
		var pts_lbl := Label.new(); pts_lbl.text = str(pts); pts_lbl.custom_minimum_size = Vector2(32, 0); row.add_child(pts_lbl)

		if cid == user_club_id:
			c_lbl.add_theme_color_override("font_color", Color(0.20, 0.95, 0.60))
			pts_lbl.add_theme_color_override("font_color", Color(0.20, 0.95, 0.60))

		_standings_vbox.add_child(row)

	# Calculate Squad Average Condition
	var squad: Array = career_state.get_club_squad(user_club_id)
	if squad.size() > 0:
		var total_cond: int = 0
		var fatigued_count: int = 0
		for p in squad:
			var c: int = int(p.get("condition", 100))
			total_cond += c
			if c < 75:
				fatigued_count += 1
		var avg_cond: int = total_cond / squad.size()
		_fitness_summary_lbl.text = "Average Squad Fitness: %d%% · %d Player(s) Fatigued" % [avg_cond, fatigued_count]

func _refresh_objectives() -> void:
	for child in _objectives_vbox.get_children():
		child.queue_free()

	var objs: Array = career_state.get_board_objectives()
	for obj in objs:
		var title: String = String(obj.get("title", "Objective"))
		var completed: bool = bool(obj.get("is_completed", false))
		var reward: int = int(obj.get("reward_amount", 0))

		var row := HBoxContainer.new()
		var t_lbl := Label.new()
		t_lbl.text = ("✓ " if completed else "• ") + title
		t_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		t_lbl.clip_text = true
		t_lbl.add_theme_font_size_override("font_size", 12)
		if completed:
			t_lbl.add_theme_color_override("font_color", Color(0.30, 0.95, 0.60))
		row.add_child(t_lbl)

		var r_lbl := Label.new()
		r_lbl.text = "+$%.1fM" % (float(reward) / 1_000_000.0)
		r_lbl.add_theme_color_override("font_color", Color(0.95, 0.80, 0.25))
		r_lbl.add_theme_font_size_override("font_size", 11)
		row.add_child(r_lbl)

		_objectives_vbox.add_child(row)

func _refresh_news_snippet() -> void:
	for child in _news_vbox.get_children():
		child.queue_free()

	var msgs: Array = career_state.get_inbox_messages("")
	var count: int = min(3, msgs.size())

	if count == 0:
		var empty_lbl := Label.new()
		empty_lbl.text = "No club inbox messages."
		empty_lbl.add_theme_color_override("font_color", Color(0.50, 0.60, 0.75))
		_news_vbox.add_child(empty_lbl)
		return

	for i in range(count):
		var msg: Dictionary = msgs[i]
		var title: String = String(msg.get("title", "Message"))
		var cat: String = String(msg.get("category", "INFO"))
		var is_read: bool = bool(msg.get("is_read", false))

		var btn := Button.new()
		btn.text = ("%s[%s] %s" % ["" if is_read else "● ", cat, title])
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.clip_text = true
		btn.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
		btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		btn.focus_mode = Control.FOCUS_NONE
		_apply_button_theme(
			btn,
			Color(0.055, 0.095, 0.17, 0.90) if not is_read else Color(0.045, 0.068, 0.11, 0.75),
			Color(0.18, 0.80, 0.48, 0.80) if not is_read else Color(0.12, 0.18, 0.28, 0.65),
			Color(0.88, 0.95, 1.0)
		)
		btn.add_theme_font_size_override("font_size", 11)
		btn.pressed.connect(func() -> void: route_requested.emit("inbox"))
		_news_vbox.add_child(btn)
