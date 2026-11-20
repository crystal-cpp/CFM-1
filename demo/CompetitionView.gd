class_name CompetitionsView
extends PanelContainer

signal route_requested(route_name: String)
signal player_profile_requested(player_id: int, context_mode: String)

var career_state: CareerState

# CSV Cache (parsed from clubs.csv)
var _csv_clubs: Array[Dictionary] = []
var _leagues_map: Dictionary = {} # league_name -> Array[Dictionary]
var _csv_loaded: bool = false

# UI State
var _active_tab: String = "league" # "league" | "ucl" | "stats" | "prizes"
var _selected_league: String = ""
var _selected_ucl_md: int = 1

# Header & Navigation Controls
var _season_badge: Label
var _tab_buttons: Dictionary = {}
var _league_selector_box: HBoxContainer
var _league_option: OptionButton

# Content Containers
var _league_tab_root: VBoxContainer
var _league_rows_vbox: VBoxContainer
var _ucl_tab_root: HBoxContainer
var _ucl_standings_vbox: VBoxContainer
var _ucl_fixtures_vbox: VBoxContainer
var _ucl_md_label: Label
var _stats_tab_root: HBoxContainer
var _scorers_vbox: VBoxContainer
var _assists_vbox: VBoxContainer
var _prizes_tab_root: VBoxContainer

const CLUBS_CSV_PATHS: Array[String] = [
	"res://data/clubs.csv",
	"res://clubs.csv",
	"res://assets/data/clubs.csv",
	"res://database/clubs.csv",
	"user://clubs.csv"
]

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_flags_vertical = Control.SIZE_EXPAND_FILL

	var root_style := StyleBoxFlat.new()
	root_style.bg_color = Color(0.030, 0.042, 0.068, 0.95)
	root_style.content_margin_left = 20
	root_style.content_margin_right = 20
	root_style.content_margin_top = 16
	root_style.content_margin_bottom = 16
	add_theme_stylebox_override("panel", root_style)

	_load_clubs_csv()
	_build_ui()
	_switch_tab("league")

func refresh_competitions() -> void:
	# Refresh from the live career registry after CareerShell injects CareerState.
	if is_instance_valid(career_state) and career_state.is_career_active():
		_load_clubs_csv()
	elif not _csv_loaded:
		_load_clubs_csv()

	if is_instance_valid(career_state) and career_state.is_career_active():
		var summary: Dictionary = career_state.get_header_summary()
		if is_instance_valid(_season_badge):
			_season_badge.text = "SEASON %s" % String(summary.get("season_label", "2026/27"))

		var managed_league: String = _get_managed_club_league()
		if _selected_league.is_empty() and not managed_league.is_empty():
			_selected_league = managed_league
			_sync_league_dropdown()

	_refresh_league_table()
	_refresh_ucl_view()
	_refresh_player_stats()

# ==============================================================================
# CSV LOADING & FALLBACK SEEDING (clubs.csv)
# ==============================================================================

func _load_clubs_csv() -> void:
	_csv_clubs.clear()
	_leagues_map.clear()
	_csv_loaded = false

	# Prefer CareerState's canonical club registry. This is the same registry
	# used by New Career, fixtures and club information, so the Competitions
	# screen cannot silently diverge from the active save.
	if is_instance_valid(career_state) and career_state.is_career_active() and career_state.has_method("get_leagues_and_clubs"):
		var registry: Variant = career_state.call("get_leagues_and_clubs")
		if registry is Array:
			for item in registry:
				if not (item is Dictionary):
					continue
				var club_name := String(item.get("club_name", item.get("club", item.get("name", "")))).strip_edges()
				var league_name := String(item.get("league", "")).strip_edges()
				if club_name.is_empty() or league_name.is_empty():
					continue
				var club_dict: Dictionary = {
					"id": int(item.get("club_id", item.get("id", 0))),
					"name": club_name,
					"league": league_name,
					"country": String(item.get("country", "")),
					"reputation": int(item.get("reputation", 50)),
					"formation": String(item.get("default_formation", item.get("formation", "4-3-3"))),
					"kit_color": _parse_kit_color(String(item.get("primary_color", "#22c55e")))
				}
				_add_competition_club(club_dict)
			if not _csv_clubs.is_empty():
				_csv_loaded = true
				print("CompetitionsView: loaded %d clubs across %d leagues from CareerState." % [_csv_clubs.size(), _leagues_map.size()])
				return

	# Standalone / pre-career fallback: read the same CSV using normalized headers.
	var found_path: String = ""
	for path in CLUBS_CSV_PATHS:
		if FileAccess.file_exists(path):
			found_path = path
			break
	if found_path.is_empty():
		push_warning("CompetitionsView: clubs.csv was not found in configured resource paths.")
		_csv_loaded = true
		return

	var file := FileAccess.open(found_path, FileAccess.READ)
	if file == null:
		push_warning("CompetitionsView: unable to open clubs CSV at %s." % found_path)
		_csv_loaded = true
		return

	var headers: PackedStringArray = file.get_csv_line()
	var col_idx: Dictionary = {}
	for i in range(headers.size()):
		var key := headers[i].strip_edges().to_lower().replace("_", "").replace(" ", "").replace("-", "")
		col_idx[key] = i

	var idx_id: int = int(col_idx.get("clubid", -1))
	var idx_club: int = int(col_idx.get("club", -1))
	var idx_league: int = int(col_idx.get("league", -1))
	var idx_country: int = int(col_idx.get("country", -1))
	var idx_rep: int = int(col_idx.get("reputation", -1))
	var idx_form: int = int(col_idx.get("defaultformation", col_idx.get("formation", -1)))
	var idx_kit: int = int(col_idx.get("primarycolor", col_idx.get("kitcolor", -1)))

	if idx_club < 0 or idx_league < 0:
		file.close()
		push_error("CompetitionsView: clubs.csv is missing required Club and League columns.")
		_csv_loaded = true
		return

	while not file.eof_reached():
		var row: PackedStringArray = file.get_csv_line()
		if row.size() <= max(idx_club, idx_league):
			continue
		var club_name := row[idx_club].strip_edges()
		var league_name := row[idx_league].strip_edges()
		if club_name.is_empty() or league_name.is_empty():
			continue
		var club_dict: Dictionary = {
			"id": int(row[idx_id].strip_edges()) if idx_id >= 0 and idx_id < row.size() else _csv_clubs.size() + 1,
			"name": club_name,
			"league": league_name,
			"country": row[idx_country].strip_edges() if idx_country >= 0 and idx_country < row.size() else "",
			"reputation": int(row[idx_rep].strip_edges()) if idx_rep >= 0 and idx_rep < row.size() else 70,
			"formation": row[idx_form].strip_edges() if idx_form >= 0 and idx_form < row.size() else "4-3-3",
			"kit_color": _parse_kit_color(row[idx_kit].strip_edges()) if idx_kit >= 0 and idx_kit < row.size() else Color(0.18, 0.85, 0.52)
		}
		_add_competition_club(club_dict)

	file.close()
	_csv_loaded = true
	print("CompetitionsView: loaded %d clubs across %d leagues from %s." % [_csv_clubs.size(), _leagues_map.size(), found_path])

func _add_competition_club(club_dict: Dictionary) -> void:
	_csv_clubs.append(club_dict)
	var league_name := String(club_dict.get("league", ""))
	if not _leagues_map.has(league_name):
		_leagues_map[league_name] = []
	_leagues_map[league_name].append(club_dict)

func _parse_kit_color(raw: String) -> Color:
	var s: String = raw.strip_edges()
	if s.begins_with("#") and (s.length() == 7 or s.length() == 9):
		return Color.from_string(s, Color(0.18, 0.85, 0.52))
	match s.to_lower():
		"red": return Color(0.88, 0.20, 0.24)
		"blue": return Color(0.20, 0.52, 0.92)
		"white": return Color(0.90, 0.92, 0.96)
		"yellow": return Color(0.95, 0.80, 0.18)
		"green": return Color(0.16, 0.78, 0.42)
		"black": return Color(0.22, 0.25, 0.30)
		"claret", "maroon": return Color(0.55, 0.14, 0.24)
		"sky blue", "lightblue": return Color(0.35, 0.75, 0.98)
		"orange": return Color(0.95, 0.48, 0.14)
		"purple": return Color(0.58, 0.28, 0.85)
		_: return Color(0.18, 0.85, 0.52)

func _get_managed_club_name() -> String:
	if is_instance_valid(career_state) and career_state.is_career_active():
		var summary: Dictionary = career_state.get_header_summary()
		return String(summary.get("managed_club_name", ""))
	return ""

func _get_managed_club_league() -> String:
	var managed_name: String = _get_managed_club_name()
	if not managed_name.is_empty():
		for c in _csv_clubs:
			if String(c["name"]).to_lower() == managed_name.to_lower():
				return String(c["league"])
	if not _leagues_map.is_empty():
		return String(_leagues_map.keys()[0])
	return "Premier League"

# ==============================================================================
# UI CONSTRUCTION
# ==============================================================================

func _build_ui() -> void:
	var main_vbox := VBoxContainer.new()
	main_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	main_vbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_theme_constant_override("separation", 14)
	add_child(main_vbox)

	# --- Top Header Bar ---
	var top_bar := HBoxContainer.new()
	top_bar.add_theme_constant_override("separation", 12)
	main_vbox.add_child(top_bar)

	var title_vbox := VBoxContainer.new()
	title_vbox.add_theme_constant_override("separation", 2)
	top_bar.add_child(title_vbox)

	var title_lbl := Label.new()
	title_lbl.text = "COMPETITIONS CENTER"
	title_lbl.add_theme_color_override("font_color", Color(0.15, 0.92, 0.58))
	title_lbl.add_theme_font_size_override("font_size", 20)
	title_vbox.add_child(title_lbl)

	var sub_lbl := Label.new()
	sub_lbl.text = "Domestic League Standings, UEFA Champions League, Player Leaders & Prize Structure"
	sub_lbl.add_theme_color_override("font_color", Color(0.55, 0.66, 0.80))
	sub_lbl.add_theme_font_size_override("font_size", 12)
	title_vbox.add_child(sub_lbl)

	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	top_bar.add_child(spacer)

	_season_badge = _make_badge("SEASON 2026/27", Color(0.16, 0.34, 0.58), Color(0.75, 0.88, 1.0))
	top_bar.add_child(_season_badge)

	# --- Sub-Navigation Tab Bar + League Filter ---
	var nav_bar := HBoxContainer.new()
	nav_bar.add_theme_constant_override("separation", 8)
	main_vbox.add_child(nav_bar)

	var tabs: Array[Dictionary] = [
		{ "id": "league", "label": "🏆 Domestic League" },
		{ "id": "ucl", "label": "⭐ UEFA Champions League" },
		{ "id": "stats", "label": "👟 Top Scorers & Assists" },
		{ "id": "prizes", "label": "💰 Prize Money" }
	]

	for t in tabs:
		var t_id: String = t["id"]
		var btn := Button.new()
		btn.text = t["label"]
		btn.custom_minimum_size = Vector2(165, 36)
		btn.focus_mode = Control.FOCUS_NONE
		btn.pressed.connect(func() -> void: _switch_tab(t_id))
		nav_bar.add_child(btn)
		_tab_buttons[t_id] = btn

	var nav_spacer := Control.new()
	nav_spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	nav_bar.add_child(nav_spacer)

	_league_selector_box = HBoxContainer.new()
	_league_selector_box.add_theme_constant_override("separation", 8)
	nav_bar.add_child(_league_selector_box)

	var filter_lbl := Label.new()
	filter_lbl.text = "League:"
	filter_lbl.add_theme_color_override("font_color", Color(0.68, 0.78, 0.90))
	filter_lbl.add_theme_font_size_override("font_size", 12)
	_league_selector_box.add_child(filter_lbl)

	_league_option = OptionButton.new()
	_league_option.custom_minimum_size = Vector2(210, 34)
	_league_option.focus_mode = Control.FOCUS_NONE
	_league_option.item_selected.connect(_on_league_selected)
	_league_selector_box.add_child(_league_option)
	_populate_league_dropdown()

	main_vbox.add_child(HSeparator.new())

	# --- Body Stack ---
	var body_stack := Control.new()
	body_stack.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body_stack.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_child(body_stack)

	_build_league_tab(body_stack)
	_build_ucl_tab(body_stack)
	_build_stats_tab(body_stack)
	_build_prizes_tab(body_stack)

func _populate_league_dropdown() -> void:
	_league_option.clear()
	var league_names: Array = _leagues_map.keys()
	league_names.sort()

	if league_names.is_empty():
		_league_option.add_item("Premier League")
		_selected_league = "Premier League"
		return

	var managed_league: String = _get_managed_club_league()
	if _selected_league.is_empty():
		_selected_league = managed_league

	for i in range(league_names.size()):
		var l_name: String = String(league_names[i])
		_league_option.add_item(l_name)
		if l_name == _selected_league:
			_league_option.select(i)

func _sync_league_dropdown() -> void:
	for i in range(_league_option.item_count):
		if _league_option.get_item_text(i) == _selected_league:
			_league_option.select(i)
			break

func _on_league_selected(idx: int) -> void:
	_selected_league = _league_option.get_item_text(idx)
	_refresh_league_table()

func _switch_tab(tab_id: String) -> void:
	_active_tab = tab_id

	_league_tab_root.visible = (tab_id == "league")
	_ucl_tab_root.visible = (tab_id == "ucl")
	_stats_tab_root.visible = (tab_id == "stats")
	_prizes_tab_root.visible = (tab_id == "prizes")
	_league_selector_box.visible = (tab_id == "league")

	for key in _tab_buttons.keys():
		var btn: Button = _tab_buttons[key]
		if key == tab_id:
			_style_btn(btn, Color(0.08, 0.36, 0.22, 0.95), Color(0.18, 0.95, 0.58, 0.95), Color(0.35, 1.0, 0.72))
		else:
			_style_btn(btn, Color(0.06, 0.09, 0.14, 0.85), Color(0.14, 0.20, 0.32, 0.75), Color(0.80, 0.88, 0.96))

	refresh_competitions()

# ==============================================================================
# TAB 1: DOMESTIC LEAGUE TABLE
# ==============================================================================

func _build_league_tab(parent: Control) -> void:
	_league_tab_root = VBoxContainer.new()
	_league_tab_root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_league_tab_root.add_theme_constant_override("separation", 10)
	parent.add_child(_league_tab_root)

	# Zone Legend
	var legend_bar := HBoxContainer.new()
	legend_bar.add_theme_constant_override("separation", 18)
	_league_tab_root.add_child(legend_bar)

	legend_bar.add_child(_make_legend_dot(Color(0.15, 0.88, 0.52), "UCL Qualification (1–4)"))
	legend_bar.add_child(_make_legend_dot(Color(0.25, 0.60, 0.98), "Europa League (5–6)"))
	legend_bar.add_child(_make_legend_dot(Color(0.92, 0.26, 0.30), "Relegation Zone (18–20)"))
	legend_bar.add_child(_make_legend_dot(Color(1.0, 0.85, 0.30), "Your Club"))

	# Table Card
	var card := _make_card_panel()
	card.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_league_tab_root.add_child(card)

	var card_vbox := VBoxContainer.new()
	card_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	card_vbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	card_vbox.add_theme_constant_override("separation", 6)
	card.add_child(card_vbox)

	# Column Headers
	card_vbox.add_child(_make_standings_header_row())
	card_vbox.add_child(HSeparator.new())

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	card_vbox.add_child(scroll)

	_league_rows_vbox = VBoxContainer.new()
	_league_rows_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_league_rows_vbox.add_theme_constant_override("separation", 4)
	scroll.add_child(_league_rows_vbox)

func _make_standings_header_row() -> HBoxContainer:
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 6)

	var cols: Array[Array] = [
		["#", 36, HORIZONTAL_ALIGNMENT_CENTER],
		["CLUB", 0, HORIZONTAL_ALIGNMENT_LEFT], # expand
		["MP", 42, HORIZONTAL_ALIGNMENT_CENTER],
		["W", 42, HORIZONTAL_ALIGNMENT_CENTER],
		["D", 42, HORIZONTAL_ALIGNMENT_CENTER],
		["L", 42, HORIZONTAL_ALIGNMENT_CENTER],
		["GF", 44, HORIZONTAL_ALIGNMENT_CENTER],
		["GA", 44, HORIZONTAL_ALIGNMENT_CENTER],
		["GD", 48, HORIZONTAL_ALIGNMENT_CENTER],
		["PTS", 52, HORIZONTAL_ALIGNMENT_CENTER],
		["FORM", 118, HORIZONTAL_ALIGNMENT_CENTER]
	]

	for c in cols:
		var lbl := Label.new()
		lbl.text = String(c[0])
		lbl.horizontal_alignment = int(c[2])
		lbl.add_theme_color_override("font_color", Color(0.52, 0.64, 0.78))
		lbl.add_theme_font_size_override("font_size", 11)
		if int(c[1]) == 0:
			lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		else:
			lbl.custom_minimum_size = Vector2(int(c[1]), 22)
		row.add_child(lbl)

	return row

func _refresh_league_table() -> void:
	for child in _league_rows_vbox.get_children():
		child.queue_free()

	var standings: Array[Dictionary] = _get_league_standings_data(_selected_league)
	var managed_name: String = _get_managed_club_name().to_lower()
	var total_teams: int = standings.size()

	if standings.is_empty():
		var empty_lbl := Label.new()
		empty_lbl.text = "No clubs found for '%s'. Verify clubs.csv is placed in res://data/clubs.csv or res://clubs.csv." % _selected_league
		empty_lbl.add_theme_color_override("font_color", Color(0.65, 0.75, 0.88))
		_league_rows_vbox.add_child(empty_lbl)
		return

	for i in range(total_teams):
		var entry: Dictionary = standings[i]
		var pos: int = i + 1
		var c_name: String = String(entry.get("name", "Club"))
		var is_managed: bool = (not managed_name.is_empty() and c_name.to_lower() == managed_name)

		var zone_col := Color(0.16, 0.22, 0.34, 0.4)
		if pos <= 4:
			zone_col = Color(0.15, 0.88, 0.52, 0.95)
		elif pos <= 6:
			zone_col = Color(0.25, 0.60, 0.98, 0.95)
		elif total_teams >= 10 and pos > total_teams - 3:
			zone_col = Color(0.92, 0.26, 0.30, 0.95)

		var row_panel := PanelContainer.new()
		var r_style := StyleBoxFlat.new()
		if is_managed:
			r_style.bg_color = Color(0.08, 0.28, 0.20, 0.85)
			r_style.border_color = Color(0.18, 0.92, 0.58, 0.90)
			r_style.set_border_width_all(1)
		else:
			r_style.bg_color = Color(0.042, 0.060, 0.095, 0.85) if (i % 2 == 0) else Color(0.036, 0.052, 0.082, 0.85)
		r_style.set_corner_radius_all(5)
		r_style.content_margin_left = 4
		r_style.content_margin_right = 6
		r_style.content_margin_top = 5
		r_style.content_margin_bottom = 5
		row_panel.add_theme_stylebox_override("panel", r_style)
		_league_rows_vbox.add_child(row_panel)

		var hbox := HBoxContainer.new()
		hbox.add_theme_constant_override("separation", 6)
		row_panel.add_child(hbox)

		# Zone Strip + Position
		var zone_strip := ColorRect.new()
		zone_strip.custom_minimum_size = Vector2(4, 22)
		zone_strip.color = zone_col
		hbox.add_child(zone_strip)

		var pos_lbl := _make_cell(str(pos), 28, HORIZONTAL_ALIGNMENT_CENTER, Color(1.0, 0.85, 0.30) if is_managed else Color.WHITE)
		hbox.add_child(pos_lbl)

		# Club Color Swatch + Name
		var club_box := HBoxContainer.new()
		club_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		club_box.add_theme_constant_override("separation", 8)
		hbox.add_child(club_box)

		var swatch := ColorRect.new()
		swatch.custom_minimum_size = Vector2(10, 18)
		swatch.color = entry.get("kit_color", Color(0.18, 0.85, 0.52))
		club_box.add_child(swatch)

		var name_lbl := Label.new()
		name_lbl.text = ("%s  ★" % c_name) if is_managed else c_name
		name_lbl.add_theme_color_override("font_color", Color(0.35, 1.0, 0.72) if is_managed else Color(0.90, 0.94, 0.99))
		name_lbl.add_theme_font_size_override("font_size", 13)
		club_box.add_child(name_lbl)

		var mp: int = int(entry.get("mp", 0))
		var w: int = int(entry.get("w", 0))
		var d: int = int(entry.get("d", 0))
		var l: int = int(entry.get("l", 0))
		var gf: int = int(entry.get("gf", 0))
		var ga: int = int(entry.get("ga", 0))
		var gd: int = gf - ga
		var pts: int = int(entry.get("pts", w * 3 + d))

		hbox.add_child(_make_cell(str(mp), 42, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hbox.add_child(_make_cell(str(w), 42, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hbox.add_child(_make_cell(str(d), 42, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hbox.add_child(_make_cell(str(l), 42, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hbox.add_child(_make_cell(str(gf), 44, HORIZONTAL_ALIGNMENT_CENTER, Color(0.68, 0.76, 0.88)))
		hbox.add_child(_make_cell(str(ga), 44, HORIZONTAL_ALIGNMENT_CENTER, Color(0.68, 0.76, 0.88)))

		var gd_str: String = ("+%d" % gd) if gd > 0 else str(gd)
		var gd_col := Color(0.30, 0.95, 0.62) if gd > 0 else (Color(0.95, 0.42, 0.45) if gd < 0 else Color(0.75, 0.82, 0.92))
		hbox.add_child(_make_cell(gd_str, 48, HORIZONTAL_ALIGNMENT_CENTER, gd_col))
		hbox.add_child(_make_cell(str(pts), 52, HORIZONTAL_ALIGNMENT_CENTER, Color(0.30, 0.98, 0.65) if is_managed else Color.WHITE))

		# Form Badges
		var form_box := HBoxContainer.new()
		form_box.custom_minimum_size = Vector2(118, 22)
		form_box.alignment = BoxContainer.ALIGNMENT_CENTER
		form_box.add_theme_constant_override("separation", 3)
		hbox.add_child(form_box)

		var form_arr: Array = entry.get("form", [])
		if form_arr.is_empty():
			var dash := Label.new()
			dash.text = "—"
			dash.add_theme_color_override("font_color", Color(0.45, 0.55, 0.68))
			dash.add_theme_font_size_override("font_size", 11)
			form_box.add_child(dash)
		else:
			for res in form_arr.slice(max(0, form_arr.size() - 5)):
				form_box.add_child(_make_form_pill(String(res)))

func _get_league_standings_data(league_name: String) -> Array[Dictionary]:
	# CareerState.get_league_standings() has no arguments and returns the
	# managed club's domestic league rows. Normalize its C++ field names to
	# the view's presentation schema, but only use it for that league.
	if is_instance_valid(career_state) and career_state.is_career_active() and career_state.has_method("get_league_standings"):
		var managed_league := _get_managed_club_league()
		if league_name == managed_league:
			var live_data: Variant = career_state.call("get_league_standings")
			if live_data is Array and not live_data.is_empty():
				var typed: Array[Dictionary] = []
				for item in live_data:
					if not (item is Dictionary):
						continue
					var club_name := String(item.get("club_name", item.get("name", "Club")))
					var source_club: Dictionary = {}
					for club in _csv_clubs:
						if String(club.get("name", "")).to_lower() == club_name.to_lower():
							source_club = club
							break
					var won := int(item.get("won", item.get("w", 0)))
					var drawn := int(item.get("drawn", item.get("d", 0)))
					typed.append({
						"id": int(item.get("club_id", source_club.get("id", 0))),
						"name": club_name,
						"league": league_name,
						"reputation": int(source_club.get("reputation", 50)),
						"kit_color": source_club.get("kit_color", Color(0.18, 0.85, 0.52)),
						"mp": int(item.get("played", item.get("mp", 0))),
						"w": won,
						"d": drawn,
						"l": int(item.get("lost", item.get("l", 0))),
						"gf": int(item.get("goals_for", item.get("gf", 0))),
						"ga": int(item.get("goals_against", item.get("ga", 0))),
						"pts": int(item.get("points", item.get("pts", won * 3 + drawn))),
						"form": item.get("form", [])
					})
				if not typed.is_empty():
					return typed

	# Pre-season / non-managed league fallback: seed every club in the selected
	# league from the canonical registry, sorted by reputation and name.
	var raw_clubs: Array = _leagues_map.get(league_name, [])
	var table: Array[Dictionary] = []
	for c in raw_clubs:
		table.append({
			"id": c.get("id", 0),
			"name": c.get("name", "Club"),
			"league": c.get("league", league_name),
			"reputation": c.get("reputation", 50),
			"kit_color": c.get("kit_color", Color(0.18, 0.85, 0.52)),
			"mp": 0, "w": 0, "d": 0, "l": 0,
			"gf": 0, "ga": 0, "pts": 0,
			"form": []
		})

	table.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		if int(a["pts"]) != int(b["pts"]):
			return int(a["pts"]) > int(b["pts"])
		var gd_a: int = int(a["gf"]) - int(a["ga"])
		var gd_b: int = int(b["gf"]) - int(b["ga"])
		if gd_a != gd_b:
			return gd_a > gd_b
		if int(a["reputation"]) != int(b["reputation"]):
			return int(a["reputation"]) > int(b["reputation"])
		return String(a["name"]) < String(b["name"])
	)
	return table

# ==============================================================================
# TAB 2: UEFA CHAMPIONS LEAGUE (STANDINGS + FIXTURES)
# ==============================================================================

func _build_ucl_tab(parent: Control) -> void:
	_ucl_tab_root = HBoxContainer.new()
	_ucl_tab_root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_ucl_tab_root.add_theme_constant_override("separation", 14)
	_ucl_tab_root.visible = false
	parent.add_child(_ucl_tab_root)

	# --- Left Panel: UCL League Phase Standings (Top 36 Clubs from clubs.csv) ---
	var left_card := _make_card_panel()
	left_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	left_card.size_flags_stretch_ratio = 1.15
	_ucl_tab_root.add_child(left_card)

	var left_vb := VBoxContainer.new()
	left_vb.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	left_vb.size_flags_vertical = Control.SIZE_EXPAND_FILL
	left_vb.add_theme_constant_override("separation", 8)
	left_card.add_child(left_vb)

	var ucl_hdr := HBoxContainer.new()
	left_vb.add_child(ucl_hdr)

	var ucl_title := Label.new()
	ucl_title.text = "⭐ UEFA CHAMPIONS LEAGUE — LEAGUE PHASE (36 CLUBS)"
	ucl_title.add_theme_color_override("font_color", Color(0.45, 0.78, 1.0))
	ucl_title.add_theme_font_size_override("font_size", 14)
	ucl_hdr.add_child(ucl_title)

	var ucl_legend := HBoxContainer.new()
	ucl_legend.add_theme_constant_override("separation", 14)
	left_vb.add_child(ucl_legend)
	ucl_legend.add_child(_make_legend_dot(Color(0.15, 0.88, 0.52), "1–8: Round of 16"))
	ucl_legend.add_child(_make_legend_dot(Color(0.28, 0.62, 0.98), "9–24: Knockout Play-offs"))
	ucl_legend.add_child(_make_legend_dot(Color(0.92, 0.26, 0.30), "25–36: Eliminated"))

	# UCL Header Row
	var hdr_row := HBoxContainer.new()
	hdr_row.add_theme_constant_override("separation", 4)
	hdr_row.add_child(_make_cell("#", 32, HORIZONTAL_ALIGNMENT_CENTER, Color(0.52, 0.64, 0.78)))
	var c_lbl := Label.new()
	c_lbl.text = "CLUB"
	c_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	c_lbl.add_theme_color_override("font_color", Color(0.52, 0.64, 0.78))
	c_lbl.add_theme_font_size_override("font_size", 11)
	hdr_row.add_child(c_lbl)
	hdr_row.add_child(_make_cell("MP", 34, HORIZONTAL_ALIGNMENT_CENTER, Color(0.52, 0.64, 0.78)))
	hdr_row.add_child(_make_cell("W", 32, HORIZONTAL_ALIGNMENT_CENTER, Color(0.52, 0.64, 0.78)))
	hdr_row.add_child(_make_cell("D", 32, HORIZONTAL_ALIGNMENT_CENTER, Color(0.52, 0.64, 0.78)))
	hdr_row.add_child(_make_cell("L", 32, HORIZONTAL_ALIGNMENT_CENTER, Color(0.52, 0.64, 0.78)))
	hdr_row.add_child(_make_cell("GD", 40, HORIZONTAL_ALIGNMENT_CENTER, Color(0.52, 0.64, 0.78)))
	hdr_row.add_child(_make_cell("PTS", 42, HORIZONTAL_ALIGNMENT_CENTER, Color(0.52, 0.64, 0.78)))
	left_vb.add_child(hdr_row)
	left_vb.add_child(HSeparator.new())

	var ucl_scroll := ScrollContainer.new()
	ucl_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	ucl_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	left_vb.add_child(ucl_scroll)

	_ucl_standings_vbox = VBoxContainer.new()
	_ucl_standings_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_ucl_standings_vbox.add_theme_constant_override("separation", 4)
	ucl_scroll.add_child(_ucl_standings_vbox)

	# --- Right Panel: UCL Matchday Fixtures & Results ---
	var right_card := _make_card_panel()
	right_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	right_card.size_flags_stretch_ratio = 0.85
	_ucl_tab_root.add_child(right_card)

	var right_vb := VBoxContainer.new()
	right_vb.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	right_vb.size_flags_vertical = Control.SIZE_EXPAND_FILL
	right_vb.add_theme_constant_override("separation", 10)
	right_card.add_child(right_vb)

	var fx_hdr := HBoxContainer.new()
	fx_hdr.add_theme_constant_override("separation", 8)
	right_vb.add_child(fx_hdr)

	var fx_title := Label.new()
	fx_title.text = "UCL FIXTURES"
	fx_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	fx_title.add_theme_color_override("font_color", Color(0.45, 0.78, 1.0))
	fx_title.add_theme_font_size_override("font_size", 14)
	fx_hdr.add_child(fx_title)

	var prev_btn := Button.new()
	prev_btn.text = "◀"
	prev_btn.custom_minimum_size = Vector2(30, 28)
	prev_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(prev_btn, Color(0.08, 0.14, 0.24, 0.9), Color(0.22, 0.42, 0.72, 0.8))
	prev_btn.pressed.connect(func() -> void:
		_selected_ucl_md = max(1, _selected_ucl_md - 1)
		_refresh_ucl_fixtures_only()
	)
	fx_hdr.add_child(prev_btn)

	_ucl_md_label = Label.new()
	_ucl_md_label.text = "Matchday 1"
	_ucl_md_label.custom_minimum_size = Vector2(96, 26)
	_ucl_md_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_ucl_md_label.add_theme_color_override("font_color", Color.WHITE)
	_ucl_md_label.add_theme_font_size_override("font_size", 12)
	fx_hdr.add_child(_ucl_md_label)

	var next_btn := Button.new()
	next_btn.text = "▶"
	next_btn.custom_minimum_size = Vector2(30, 28)
	next_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(next_btn, Color(0.08, 0.14, 0.24, 0.9), Color(0.22, 0.42, 0.72, 0.8))
	next_btn.pressed.connect(func() -> void:
		_selected_ucl_md = min(8, _selected_ucl_md + 1)
		_refresh_ucl_fixtures_only()
	)
	fx_hdr.add_child(next_btn)

	right_vb.add_child(HSeparator.new())

	var fx_scroll := ScrollContainer.new()
	fx_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	fx_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	right_vb.add_child(fx_scroll)

	_ucl_fixtures_vbox = VBoxContainer.new()
	_ucl_fixtures_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_ucl_fixtures_vbox.add_theme_constant_override("separation", 6)
	fx_scroll.add_child(_ucl_fixtures_vbox)

func _get_ucl_clubs() -> Array[Dictionary]:
	# 1. Check live CareerState UCL standings
	if is_instance_valid(career_state) and career_state.has_method("get_ucl_standings"):
		var live_ucl: Variant = career_state.call("get_ucl_standings")
		if live_ucl is Array and not live_ucl.is_empty():
			var typed: Array[Dictionary] = []
			for item in live_ucl:
				if item is Dictionary:
					typed.append(item)
			return typed

	# 2. Seed top 36 European clubs by reputation from clubs.csv
	var non_europe: Array[String] = ["argentina", "brazil", "usa", "united states", "saudi arabia", "japan", "south korea", "australia", "mexico", "vietnam"]
	var candidates: Array[Dictionary] = []
	for c in _csv_clubs:
		var country: String = String(c.get("country", "")).to_lower()
		if country in non_europe:
			continue
		candidates.append({
			"id": c["id"],
			"name": c["name"],
			"league": c["league"],
			"reputation": c["reputation"],
			"kit_color": c["kit_color"],
			"mp": 0, "w": 0, "d": 0, "l": 0,
			"gf": 0, "ga": 0, "pts": 0
		})

	candidates.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		if int(a["pts"]) != int(b["pts"]):
			return int(a["pts"]) > int(b["pts"])
		if int(a["reputation"]) != int(b["reputation"]):
			return int(a["reputation"]) > int(b["reputation"])
		return String(a["name"]) < String(b["name"])
	)

	if candidates.size() > 36:
		return candidates.slice(0, 36)
	return candidates

func _refresh_ucl_view() -> void:
	for child in _ucl_standings_vbox.get_children():
		child.queue_free()

	var ucl_clubs: Array[Dictionary] = _get_ucl_clubs()
	var managed_name: String = _get_managed_club_name().to_lower()

	for i in range(ucl_clubs.size()):
		var entry: Dictionary = ucl_clubs[i]
		var pos: int = i + 1
		var c_name: String = String(entry.get("name", "Club"))
		var is_managed: bool = (not managed_name.is_empty() and c_name.to_lower() == managed_name)

		var zone_col := Color(0.92, 0.26, 0.30, 0.95)
		if pos <= 8:
			zone_col = Color(0.15, 0.88, 0.52, 0.95)
		elif pos <= 24:
			zone_col = Color(0.28, 0.62, 0.98, 0.95)

		var row_pc := PanelContainer.new()
		var st := StyleBoxFlat.new()
		if is_managed:
			st.bg_color = Color(0.08, 0.28, 0.20, 0.88)
			st.border_color = Color(0.18, 0.92, 0.58, 0.95)
			st.set_border_width_all(1)
		else:
			st.bg_color = Color(0.042, 0.060, 0.098, 0.85) if (i % 2 == 0) else Color(0.036, 0.052, 0.084, 0.85)
		st.set_corner_radius_all(4)
		st.content_margin_left = 4
		st.content_margin_right = 6
		st.content_margin_top = 4
		st.content_margin_bottom = 4
		row_pc.add_theme_stylebox_override("panel", st)
		_ucl_standings_vbox.add_child(row_pc)

		var hb := HBoxContainer.new()
		hb.add_theme_constant_override("separation", 4)
		row_pc.add_child(hb)

		var strip := ColorRect.new()
		strip.custom_minimum_size = Vector2(4, 20)
		strip.color = zone_col
		hb.add_child(strip)

		hb.add_child(_make_cell(str(pos), 26, HORIZONTAL_ALIGNMENT_CENTER, Color(1.0, 0.85, 0.30) if is_managed else Color.WHITE))

		var name_box := HBoxContainer.new()
		name_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		name_box.add_theme_constant_override("separation", 6)
		hb.add_child(name_box)

		var sw := ColorRect.new()
		sw.custom_minimum_size = Vector2(8, 16)
		sw.color = entry.get("kit_color", Color(0.28, 0.62, 0.98))
		name_box.add_child(sw)

		var n_lbl := Label.new()
		n_lbl.text = "%s  (%s)" % [c_name, String(entry.get("league", ""))]
		n_lbl.clip_text = true
		n_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		n_lbl.add_theme_color_override("font_color", Color(0.35, 1.0, 0.72) if is_managed else Color(0.88, 0.93, 0.99))
		n_lbl.add_theme_font_size_override("font_size", 12)
		name_box.add_child(n_lbl)

		var mp: int = int(entry.get("mp", 0))
		var w: int = int(entry.get("w", 0))
		var d: int = int(entry.get("d", 0))
		var l: int = int(entry.get("l", 0))
		var gd: int = int(entry.get("gf", 0)) - int(entry.get("ga", 0))
		var pts: int = int(entry.get("pts", w * 3 + d))

		hb.add_child(_make_cell(str(mp), 34, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hb.add_child(_make_cell(str(w), 32, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hb.add_child(_make_cell(str(d), 32, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hb.add_child(_make_cell(str(l), 32, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hb.add_child(_make_cell(("+%d" % gd) if gd > 0 else str(gd), 40, HORIZONTAL_ALIGNMENT_CENTER, Color(0.75, 0.82, 0.92)))
		hb.add_child(_make_cell(str(pts), 42, HORIZONTAL_ALIGNMENT_CENTER, Color.WHITE))

	_refresh_ucl_fixtures_only()

func _refresh_ucl_fixtures_only() -> void:
	_ucl_md_label.text = "Matchday %d" % _selected_ucl_md
	for child in _ucl_fixtures_vbox.get_children():
		child.queue_free()

	var fixtures: Array[Dictionary] = _get_ucl_fixtures_for_md(_selected_ucl_md)
	var managed_name: String = _get_managed_club_name().to_lower()

	if fixtures.is_empty():
		var lbl := Label.new()
		lbl.text = "UCL fixtures will appear once clubs.csv is loaded."
		lbl.add_theme_color_override("font_color", Color(0.65, 0.75, 0.88))
		_ucl_fixtures_vbox.add_child(lbl)
		return

	for fx in fixtures:
		var home: String = String(fx.get("home", "Home FC"))
		var away: String = String(fx.get("away", "Away FC"))
		var played: bool = bool(fx.get("played", false))
		var date_str: String = String(fx.get("date", "2026-09-16"))
		var is_user_match: bool = (not managed_name.is_empty() and (home.to_lower() == managed_name or away.to_lower() == managed_name))

		var card := PanelContainer.new()
		var st := StyleBoxFlat.new()
		if is_user_match:
			st.bg_color = Color(0.08, 0.24, 0.36, 0.92)
			st.border_color = Color(0.25, 0.72, 1.0, 0.95)
			st.set_border_width_all(1)
		else:
			st.bg_color = Color(0.042, 0.062, 0.10, 0.88)
			st.border_color = Color(0.12, 0.18, 0.30, 0.65)
			st.set_border_width_all(1)
		st.set_corner_radius_all(6)
		st.content_margin_left = 10
		st.content_margin_right = 10
		st.content_margin_top = 7
		st.content_margin_bottom = 7
		card.add_theme_stylebox_override("panel", st)
		_ucl_fixtures_vbox.add_child(card)

		var vb := VBoxContainer.new()
		vb.add_theme_constant_override("separation", 4)
		card.add_child(vb)

		var meta := Label.new()
		meta.text = "UEFA CHAMPIONS LEAGUE  •  MD %d  •  %s" % [_selected_ucl_md, date_str]
		meta.add_theme_color_override("font_color", Color(0.45, 0.75, 1.0) if is_user_match else Color(0.48, 0.60, 0.75))
		meta.add_theme_font_size_override("font_size", 10)
		vb.add_child(meta)

		var match_row := HBoxContainer.new()
		match_row.add_theme_constant_override("separation", 8)
		vb.add_child(match_row)

		var home_lbl := Label.new()
		home_lbl.text = home
		home_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		home_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		home_lbl.add_theme_color_override("font_color", Color(0.35, 1.0, 0.72) if home.to_lower() == managed_name else Color.WHITE)
		home_lbl.add_theme_font_size_override("font_size", 12)
		match_row.add_child(home_lbl)

		var score_text: String = ("%d - %d" % [int(fx.get("home_goals", 0)), int(fx.get("away_goals", 0))]) if played else "VS"
		var score_pill := _make_badge(
			score_text,
			Color(0.14, 0.55, 0.35) if played else Color(0.16, 0.28, 0.48),
			Color.WHITE
		)
		score_pill.custom_minimum_size = Vector2(54, 22)
		score_pill.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		match_row.add_child(score_pill)

		var away_lbl := Label.new()
		away_lbl.text = away
		away_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		away_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT
		away_lbl.add_theme_color_override("font_color", Color(0.35, 1.0, 0.72) if away.to_lower() == managed_name else Color.WHITE)
		away_lbl.add_theme_font_size_override("font_size", 12)
		match_row.add_child(away_lbl)

func _get_ucl_fixtures_for_md(md: int) -> Array[Dictionary]:
	# 1. Check live CareerState UCL fixtures
	if is_instance_valid(career_state) and career_state.has_method("get_ucl_fixtures"):
		var live_fx: Variant = career_state.call("get_ucl_fixtures", md)
		if live_fx is Array and not live_fx.is_empty():
			var typed: Array[Dictionary] = []
			for item in live_fx:
				if item is Dictionary:
					typed.append(item)
			return typed

	# 2. Deterministic Swiss-style rotation pairings from the 36 UCL clubs in clubs.csv
	var clubs: Array[Dictionary] = _get_ucl_clubs()
	var n: int = clubs.size()
	if n < 2:
		return []

	var md_dates: Array[String] = [
		"2026-09-16", "2026-10-01", "2026-10-22", "2026-11-05",
		"2026-11-26", "2026-12-10", "2027-01-21", "2027-01-29"
	]
	var date_str: String = md_dates[clampi(md - 1, 0, md_dates.size() - 1)]

	var out: Array[Dictionary] = []
	var half: int = n / 2
	for i in range(half):
		var idx_a: int = (i + md - 1) % n
		var idx_b: int = (n - 1 - i + md - 1) % n
		if idx_a == idx_b:
			idx_b = (idx_b + 1) % n
		var home_first: bool = ((i + md) % 2 == 0)
		out.append({
			"matchday": md,
			"date": date_str,
			"home": clubs[idx_a if home_first else idx_b]["name"],
			"away": clubs[idx_b if home_first else idx_a]["name"],
			"played": false,
			"home_goals": 0,
			"away_goals": 0
		})
	return out

# ==============================================================================
# TAB 3: TOP SCORERS & TOP ASSISTS
# ==============================================================================

func _build_stats_tab(parent: Control) -> void:
	_stats_tab_root = HBoxContainer.new()
	_stats_tab_root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_stats_tab_root.add_theme_constant_override("separation", 16)
	_stats_tab_root.visible = false
	parent.add_child(_stats_tab_root)

	# Scorers Card
	var sc_card := _make_card_panel()
	sc_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_stats_tab_root.add_child(sc_card)

	var sc_vb := VBoxContainer.new()
	sc_vb.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	sc_vb.size_flags_vertical = Control.SIZE_EXPAND_FILL
	sc_vb.add_theme_constant_override("separation", 8)
	sc_card.add_child(sc_vb)

	var sc_title := Label.new()
	sc_title.text = "👟 GOLDEN BOOT — TOP SCORERS"
	sc_title.add_theme_color_override("font_color", Color(0.15, 0.92, 0.58))
	sc_title.add_theme_font_size_override("font_size", 15)
	sc_vb.add_child(sc_title)
	sc_vb.add_child(HSeparator.new())

	var sc_scroll := ScrollContainer.new()
	sc_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	sc_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	sc_vb.add_child(sc_scroll)

	_scorers_vbox = VBoxContainer.new()
	_scorers_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_scorers_vbox.add_theme_constant_override("separation", 6)
	sc_scroll.add_child(_scorers_vbox)

	# Assists Card
	var as_card := _make_card_panel()
	as_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_stats_tab_root.add_child(as_card)

	var as_vb := VBoxContainer.new()
	as_vb.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	as_vb.size_flags_vertical = Control.SIZE_EXPAND_FILL
	as_vb.add_theme_constant_override("separation", 8)
	as_card.add_child(as_vb)

	var as_title := Label.new()
	as_title.text = "🎯 PLAYMAKER AWARD — TOP ASSISTS"
	as_title.add_theme_color_override("font_color", Color(0.45, 0.78, 1.0))
	as_title.add_theme_font_size_override("font_size", 15)
	as_vb.add_child(as_title)
	as_vb.add_child(HSeparator.new())

	var as_scroll := ScrollContainer.new()
	as_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	as_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	as_vb.add_child(as_scroll)

	_assists_vbox = VBoxContainer.new()
	_assists_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_assists_vbox.add_theme_constant_override("separation", 6)
	as_scroll.add_child(_assists_vbox)

func _refresh_player_stats() -> void:
	for c in _scorers_vbox.get_children():
		c.queue_free()
	for c in _assists_vbox.get_children():
		c.queue_free()

	var scorers: Array = []
	var assists: Array = []

	if is_instance_valid(career_state):
		if career_state.has_method("get_competition_top_scorers"):
			scorers = career_state.call("get_competition_top_scorers", _selected_league)
		if career_state.has_method("get_competition_top_assists"):
			assists = career_state.call("get_competition_top_assists", _selected_league)

	_populate_stat_list(_scorers_vbox, scorers, "goals", "G")
	_populate_stat_list(_assists_vbox, assists, "assists", "A")

func _populate_stat_list(target: VBoxContainer, items: Array, stat_key: String, unit_suffix: String) -> void:
	if items.is_empty():
		var empty_lbl := Label.new()
		empty_lbl.text = "No match stats recorded yet — advance matches in the season to populate leaderboard."
		empty_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		empty_lbl.add_theme_color_override("font_color", Color(0.60, 0.72, 0.85))
		empty_lbl.add_theme_font_size_override("font_size", 12)
		target.add_child(empty_lbl)
		return

	for i in range(items.size()):
		var p: Dictionary = items[i]
		var p_id: int = int(p.get("player_id", -1))
		var p_name: String = String(p.get("name", "Player"))
		var club_name: String = String(p.get("club", ""))
		var apps: int = int(p.get("apps", 0))
		var val: int = int(p.get(stat_key, 0))

		var row_pc := PanelContainer.new()
		var st := StyleBoxFlat.new()
		st.bg_color = Color(0.042, 0.062, 0.10, 0.88)
		st.set_corner_radius_all(6)
		st.content_margin_left = 10
		st.content_margin_right = 10
		st.content_margin_top = 6
		st.content_margin_bottom = 6
		row_pc.add_theme_stylebox_override("panel", st)
		target.add_child(row_pc)

		var hb := HBoxContainer.new()
		hb.add_theme_constant_override("separation", 10)
		row_pc.add_child(hb)

		hb.add_child(_make_cell("#%d" % (i + 1), 32, HORIZONTAL_ALIGNMENT_CENTER, Color(1.0, 0.85, 0.30) if i == 0 else Color(0.75, 0.85, 0.95)))

		var name_btn := Button.new()
		name_btn.text = "%s  (%s)" % [p_name, club_name]
		name_btn.flat = true
		name_btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		name_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		name_btn.focus_mode = Control.FOCUS_NONE
		name_btn.add_theme_color_override("font_color", Color.WHITE)
		name_btn.add_theme_color_override("font_hover_color", Color(0.30, 0.98, 0.65))
		if p_id > 0:
			name_btn.pressed.connect(func() -> void: player_profile_requested.emit(p_id, "SCOUT"))
		hb.add_child(name_btn)

		hb.add_child(_make_cell("%d Apps" % apps, 64, HORIZONTAL_ALIGNMENT_RIGHT, Color(0.58, 0.70, 0.84)))
		hb.add_child(_make_badge("%d %s" % [val, unit_suffix], Color(0.12, 0.45, 0.28), Color(0.35, 1.0, 0.72)))

# ==============================================================================
# TAB 4: PRIZE MONEY & COMPETITION STRUCTURE
# ==============================================================================

func _build_prizes_tab(parent: Control) -> void:
	_prizes_tab_root = VBoxContainer.new()
	_prizes_tab_root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_prizes_tab_root.add_theme_constant_override("separation", 14)
	_prizes_tab_root.visible = false
	parent.add_child(_prizes_tab_root)

	var grid := HBoxContainer.new()
	grid.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	grid.size_flags_vertical = Control.SIZE_EXPAND_FILL
	grid.add_theme_constant_override("separation", 16)
	_prizes_tab_root.add_child(grid)

	# Domestic Prize Card
	var dom_card := _make_card_panel()
	dom_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	grid.add_child(dom_card)

	var dom_vb := VBoxContainer.new()
	dom_vb.add_theme_constant_override("separation", 10)
	dom_card.add_child(dom_vb)

	var dom_h := Label.new()
	dom_h.text = "🏆 DOMESTIC LEAGUE MERIT PAYMENTS"
	dom_h.add_theme_color_override("font_color", Color(0.15, 0.92, 0.58))
	dom_h.add_theme_font_size_override("font_size", 15)
	dom_vb.add_child(dom_h)
	dom_vb.add_child(HSeparator.new())

	var dom_rows: Array[Array] = [
		["1st Place (Champions)", "$55.0M + UCL Spot"],
		["2nd Place", "$48.0M + UCL Spot"],
		["3rd Place", "$42.0M + UCL Spot"],
		["4th Place", "$36.0M + UCL Spot"],
		["5th–6th Place", "$28.0M + UEL Spot"],
		["7th–10th Place", "$20.0M"],
		["11th–17th Place", "$14.0M"],
		["18th–20th (Relegated)", "$8.0M Parachute"]
	]
	for r in dom_rows:
		dom_vb.add_child(_make_prize_row(String(r[0]), String(r[1]), Color(0.30, 0.98, 0.65)))

	# UCL Prize Card
	var ucl_card := _make_card_panel()
	ucl_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	grid.add_child(ucl_card)

	var ucl_vb := VBoxContainer.new()
	ucl_vb.add_theme_constant_override("separation", 10)
	ucl_card.add_child(ucl_vb)

	var ucl_h := Label.new()
	ucl_h.text = "⭐ UEFA CHAMPIONS LEAGUE PRIZE POOL"
	ucl_h.add_theme_color_override("font_color", Color(0.45, 0.78, 1.0))
	ucl_h.add_theme_font_size_override("font_size", 15)
	ucl_vb.add_child(ucl_h)
	ucl_vb.add_child(HSeparator.new())

	var ucl_rows: Array[Array] = [
		["League Phase Qualification", "$18.6M Base Fee"],
		["League Phase Match Win", "$2.1M per Win"],
		["League Phase Match Draw", "$0.7M per Draw"],
		["Top 8 Direct R16 Bonus", "$2.0M + $11.0M R16"],
		["Quarter-Finals Qualification", "$12.5M"],
		["Semi-Finals Qualification", "$15.0M"],
		["Finalist (Runner-Up)", "$18.5M"],
		["UEFA Champions League Winner", "$25.0M Total Final"]
	]
	for r in ucl_rows:
		ucl_vb.add_child(_make_prize_row(String(r[0]), String(r[1]), Color(0.45, 0.80, 1.0)))

func _make_prize_row(stage: String, payout: String, val_col: Color) -> HBoxContainer:
	var hb := HBoxContainer.new()
	var l1 := Label.new()
	l1.text = stage
	l1.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	l1.add_theme_color_override("font_color", Color(0.84, 0.90, 0.98))
	l1.add_theme_font_size_override("font_size", 13)
	hb.add_child(l1)

	var l2 := Label.new()
	l2.text = payout
	l2.add_theme_color_override("font_color", val_col)
	l2.add_theme_font_size_override("font_size", 13)
	hb.add_child(l2)
	return hb

# ==============================================================================
# UI HELPERS
# ==============================================================================

func _make_card_panel() -> PanelContainer:
	var pc := PanelContainer.new()
	var st := StyleBoxFlat.new()
	st.bg_color = Color(0.045, 0.065, 0.10, 0.92)
	st.border_color = Color(0.14, 0.20, 0.32, 0.75)
	st.set_border_width_all(1)
	st.set_corner_radius_all(8)
	st.content_margin_left = 14
	st.content_margin_right = 14
	st.content_margin_top = 12
	st.content_margin_bottom = 12
	pc.add_theme_stylebox_override("panel", st)
	return pc

func _make_cell(txt: String, min_w: int, align: HorizontalAlignment, col: Color) -> Label:
	var lbl := Label.new()
	lbl.text = txt
	lbl.custom_minimum_size = Vector2(min_w, 22)
	lbl.horizontal_alignment = align
	lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	lbl.add_theme_color_override("font_color", col)
	lbl.add_theme_font_size_override("font_size", 12)
	return lbl

func _make_badge(txt: String, bg_col: Color, txt_col: Color) -> Label:
	var lbl := Label.new()
	lbl.text = txt
	var st := StyleBoxFlat.new()
	st.bg_color = bg_col.lerp(Color.BLACK, 0.30)
	st.border_color = bg_col
	st.set_border_width_all(1)
	st.set_corner_radius_all(5)
	st.content_margin_left = 8
	st.content_margin_right = 8
	st.content_margin_top = 3
	st.content_margin_bottom = 3
	lbl.add_theme_stylebox_override("normal", st)
	lbl.add_theme_color_override("font_color", txt_col)
	lbl.add_theme_font_size_override("font_size", 11)
	return lbl

func _make_form_pill(res: String) -> Label:
	var r: String = res.to_upper()
	var col := Color(0.45, 0.52, 0.62)
	if r == "W":
		col = Color(0.14, 0.72, 0.42)
	elif r == "L":
		col = Color(0.82, 0.22, 0.26)

	var lbl := Label.new()
	lbl.text = r
	lbl.custom_minimum_size = Vector2(20, 20)
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	var st := StyleBoxFlat.new()
	st.bg_color = col
	st.set_corner_radius_all(4)
	lbl.add_theme_stylebox_override("normal", st)
	lbl.add_theme_color_override("font_color", Color.WHITE)
	lbl.add_theme_font_size_override("font_size", 10)
	return lbl

func _make_legend_dot(col: Color, label_text: String) -> HBoxContainer:
	var hb := HBoxContainer.new()
	hb.add_theme_constant_override("separation", 6)
	var dot := ColorRect.new()
	dot.custom_minimum_size = Vector2(10, 10)
	dot.color = col
	hb.add_child(dot)
	var lbl := Label.new()
	lbl.text = label_text
	lbl.add_theme_color_override("font_color", Color(0.70, 0.80, 0.92))
	lbl.add_theme_font_size_override("font_size", 11)
	hb.add_child(lbl)
	return hb

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

	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("pressed", normal)
	btn.add_theme_color_override("font_color", text_col)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)