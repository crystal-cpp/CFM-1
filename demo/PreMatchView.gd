class_name PreMatchView
extends Control

signal start_match_requested(match_context: Dictionary)
signal back_requested

const FORMATIONS_CSV_PATH := "res://data/formations.csv"
const DEFAULT_FORMATIONS: PackedStringArray = [
	"4-3-3",
	"4-4-2",
	"4-2-3-1",
	"3-5-2",
	"5-3-2",
	"4-1-4-1",
]

var career_state: Node = null
var match_context: Dictionary = {}
var use_alternate_away_kit: bool = false
var _available_formations: PackedStringArray = PackedStringArray()
var _selected_swap_player_id: int = -1

var _importance_badge: Label
var _importance_desc: Label
var _home_title: Label
var _away_title: Label
var _home_tag_label: Label
var _away_tag_label: Label
var _opponent_title_label: Label
var _opponent_meta_label: Label
var _opponent_lineup_list: VBoxContainer
var _home_kit_swatch: ColorRect
var _away_kit_swatch: ColorRect
var _kit_status_lbl: Label
var _swap_kit_btn: Button
var _formation_opt: OptionButton
var _mentality_opt: OptionButton
var _pressing_opt: OptionButton
var _lineup_list: VBoxContainer
var _bench_header_lbl: Label
var _bench_list: VBoxContainer
var _swap_hint_lbl: Label
var _warning_lbl: Label


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_build_ui()


func _build_ui() -> void:
	var bg := ColorRect.new()
	bg.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.06, 0.08, 0.12)
	add_child(bg)

	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	margin.add_theme_constant_override("margin_left", 28)
	margin.add_theme_constant_override("margin_right", 28)
	margin.add_theme_constant_override("margin_top", 22)
	margin.add_theme_constant_override("margin_bottom", 22)
	add_child(margin)

	var root_vbox := VBoxContainer.new()
	root_vbox.add_theme_constant_override("separation", 14)
	margin.add_child(root_vbox)

	# Header Bar
	var top_bar := HBoxContainer.new()
	root_vbox.add_child(top_bar)

	var back_btn := Button.new()
	back_btn.text = "← Back to Hub"
	back_btn.pressed.connect(func() -> void: back_requested.emit())
	top_bar.add_child(back_btn)

	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	top_bar.add_child(spacer)

	_importance_badge = Label.new()
	_importance_badge.add_theme_font_size_override("font_size", 15)
	top_bar.add_child(_importance_badge)

	# Matchup & Kits Banner
	var matchup_panel := PanelContainer.new()
	root_vbox.add_child(matchup_panel)

	var match_vbox := VBoxContainer.new()
	match_vbox.add_theme_constant_override("separation", 8)
	matchup_panel.add_child(match_vbox)

	var teams_row := HBoxContainer.new()
	teams_row.alignment = BoxContainer.ALIGNMENT_CENTER
	teams_row.add_theme_constant_override("separation", 20)
	match_vbox.add_child(teams_row)

	_home_kit_swatch = ColorRect.new()
	_home_kit_swatch.custom_minimum_size = Vector2(36, 36)
	teams_row.add_child(_home_kit_swatch)

	_home_tag_label = Label.new()
	_home_tag_label.text = "HOME"
	_home_tag_label.add_theme_font_size_override("font_size", 11)
	teams_row.add_child(_home_tag_label)

	_home_title = Label.new()
	_home_title.add_theme_font_size_override("font_size", 22)
	teams_row.add_child(_home_title)

	var vs_lbl := Label.new()
	vs_lbl.text = "VS"
	vs_lbl.add_theme_font_size_override("font_size", 18)
	vs_lbl.add_theme_color_override("font_color", Color(0.65, 0.72, 0.82))
	teams_row.add_child(vs_lbl)

	_away_title = Label.new()
	_away_title.add_theme_font_size_override("font_size", 22)
	teams_row.add_child(_away_title)

	_away_tag_label = Label.new()
	_away_tag_label.text = "AWAY"
	_away_tag_label.add_theme_font_size_override("font_size", 11)
	teams_row.add_child(_away_tag_label)

	_away_kit_swatch = ColorRect.new()
	_away_kit_swatch.custom_minimum_size = Vector2(36, 36)
	teams_row.add_child(_away_kit_swatch)

	var kit_row := HBoxContainer.new()
	kit_row.alignment = BoxContainer.ALIGNMENT_CENTER
	kit_row.add_theme_constant_override("separation", 12)
	match_vbox.add_child(kit_row)

	_kit_status_lbl = Label.new()
	_kit_status_lbl.add_theme_font_size_override("font_size", 13)
	kit_row.add_child(_kit_status_lbl)

	_swap_kit_btn = Button.new()
	_swap_kit_btn.text = "Toggle Away Kit"
	_swap_kit_btn.pressed.connect(_on_toggle_away_kit)
	kit_row.add_child(_swap_kit_btn)

	_importance_desc = Label.new()
	_importance_desc.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_importance_desc.add_theme_color_override("font_color", Color(0.75, 0.80, 0.88))
	match_vbox.add_child(_importance_desc)

	# Body Split: Left = User Squad + Tactics, Right = Opponent Scouting.
	var body_hbox := HBoxContainer.new()
	body_hbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body_hbox.add_theme_constant_override("separation", 18)
	root_vbox.add_child(body_hbox)

	var user_panel := PanelContainer.new()
	user_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	user_panel.size_flags_stretch_ratio = 1.15
	body_hbox.add_child(user_panel)

	var user_scroll := ScrollContainer.new()
	user_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	user_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	user_panel.add_child(user_scroll)

	var user_vbox := VBoxContainer.new()
	user_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	user_vbox.add_theme_constant_override("separation", 7)
	user_scroll.add_child(user_vbox)

	var user_hdr := Label.new()
	user_hdr.text = "YOUR SQUAD"
	user_hdr.add_theme_font_size_override("font_size", 16)
	user_vbox.add_child(user_hdr)

	var tac_hdr := Label.new()
	tac_hdr.text = "TACTICAL INSTRUCTIONS"
	tac_hdr.add_theme_font_size_override("font_size", 13)
	tac_hdr.add_theme_color_override("font_color", Color(0.70, 0.82, 0.96))
	user_vbox.add_child(tac_hdr)

	_formation_opt = OptionButton.new()
	_populate_formation_options()
	_formation_opt.item_selected.connect(_on_tactics_changed)
	user_vbox.add_child(_labeled_row("Formation:", _formation_opt))

	_mentality_opt = OptionButton.new()
	for m: String in ["Very Defensive", "Defensive", "Balanced", "Attacking", "Very Attacking"]:
		_mentality_opt.add_item(m)
	_mentality_opt.selected = 2
	_mentality_opt.item_selected.connect(_on_tactics_changed)
	user_vbox.add_child(_labeled_row("Mentality:", _mentality_opt))

	_pressing_opt = OptionButton.new()
	for pr: String in ["Low Block", "Normal Press", "High Gegenpress"]:
		_pressing_opt.add_item(pr)
	_pressing_opt.selected = 1
	_pressing_opt.item_selected.connect(_on_tactics_changed)
	user_vbox.add_child(_labeled_row("Pressing:", _pressing_opt))

	_swap_hint_lbl = Label.new()
	_swap_hint_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_swap_hint_lbl.add_theme_font_size_override("font_size", 11)
	_swap_hint_lbl.add_theme_color_override("font_color", Color(0.65, 0.78, 0.92))
	_swap_hint_lbl.text = "Swap players to adjust your XI before kickoff."
	user_vbox.add_child(_swap_hint_lbl)

	var xi_hdr := Label.new()
	xi_hdr.text = "YOUR STARTING XI & FITNESS (11)"
	xi_hdr.add_theme_font_size_override("font_size", 14)
	user_vbox.add_child(xi_hdr)

	_lineup_list = VBoxContainer.new()
	_lineup_list.add_theme_constant_override("separation", 4)
	user_vbox.add_child(_lineup_list)

	var sep := HSeparator.new()
	user_vbox.add_child(sep)

	_bench_header_lbl = Label.new()
	_bench_header_lbl.text = "YOUR BENCH / SUBSTITUTES (0)"
	_bench_header_lbl.add_theme_font_size_override("font_size", 13)
	_bench_header_lbl.add_theme_color_override("font_color", Color(0.75, 0.84, 0.96))
	user_vbox.add_child(_bench_header_lbl)

	_bench_list = VBoxContainer.new()
	_bench_list.add_theme_constant_override("separation", 4)
	user_vbox.add_child(_bench_list)

	var opponent_panel := PanelContainer.new()
	opponent_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	opponent_panel.size_flags_stretch_ratio = 0.85
	body_hbox.add_child(opponent_panel)

	var opponent_scroll := ScrollContainer.new()
	opponent_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	opponent_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	opponent_panel.add_child(opponent_scroll)

	var opponent_vbox := VBoxContainer.new()
	opponent_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	opponent_vbox.add_theme_constant_override("separation", 8)
	opponent_scroll.add_child(opponent_vbox)

	_opponent_title_label = Label.new()
	_opponent_title_label.text = "OPPONENT SCOUTING"
	_opponent_title_label.add_theme_font_size_override("font_size", 16)
	opponent_vbox.add_child(_opponent_title_label)

	_opponent_meta_label = Label.new()
	_opponent_meta_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_opponent_meta_label.add_theme_color_override("font_color", Color(0.70, 0.82, 0.96))
	opponent_vbox.add_child(_opponent_meta_label)

	var opp_xi_hdr := Label.new()
	opp_xi_hdr.text = "EXPECTED OPPONENT LINEUP"
	opp_xi_hdr.add_theme_font_size_override("font_size", 13)
	opponent_vbox.add_child(opp_xi_hdr)

	_opponent_lineup_list = VBoxContainer.new()
	_opponent_lineup_list.add_theme_constant_override("separation", 5)
	opponent_vbox.add_child(_opponent_lineup_list)

	# Footer Action Bar
	var footer := HBoxContainer.new()
	root_vbox.add_child(footer)

	_warning_lbl = Label.new()
	_warning_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_warning_lbl.add_theme_color_override("font_color", Color(1.0, 0.78, 0.35))
	footer.add_child(_warning_lbl)

	var kickoff_btn := Button.new()
	kickoff_btn.text = "KICK OFF MATCH →"
	kickoff_btn.custom_minimum_size = Vector2(220, 44)
	kickoff_btn.pressed.connect(_on_kickoff_pressed)
	footer.add_child(kickoff_btn)


func _labeled_row(title: String, control: Control) -> HBoxContainer:
	var row := HBoxContainer.new()
	var lbl := Label.new()
	lbl.text = title
	lbl.custom_minimum_size = Vector2(120, 0)
	row.add_child(lbl)
	control.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(control)
	return row


func _load_available_formations() -> PackedStringArray:
	var loaded := PackedStringArray()
	if is_instance_valid(career_state) and career_state.has_method("get_available_formations"):
		var state_fmts: Variant = career_state.call("get_available_formations")
		if state_fmts is Array or state_fmts is PackedStringArray:
			for fmt: Variant in state_fmts:
				var f_str := String(fmt).strip_edges()
				if not f_str.is_empty() and not loaded.has(f_str):
					loaded.append(f_str)
	if loaded.is_empty() and FileAccess.file_exists(FORMATIONS_CSV_PATH):
		var file := FileAccess.open(FORMATIONS_CSV_PATH, FileAccess.READ)
		if file != null:
			var header := file.get_csv_line()
			var name_idx := 0
			for i: int in range(header.size()):
				var col := header[i].strip_edges().to_lower()
				if col == "formation" or col == "name" or col == "formation_name":
					name_idx = i
					break
			while not file.eof_reached():
				var row := file.get_csv_line()
				if row.size() > name_idx:
					var f_str := row[name_idx].strip_edges()
					if not f_str.is_empty() and f_str.to_lower() != "formation" and not loaded.has(f_str):
						loaded.append(f_str)
	if loaded.is_empty():
		return DEFAULT_FORMATIONS.duplicate()
	return loaded


func _populate_formation_options() -> void:
	if _formation_opt == null:
		return
	var current_sel := ""
	if _formation_opt.item_count > 0 and _formation_opt.selected >= 0:
		current_sel = _formation_opt.get_item_text(_formation_opt.selected)
	_available_formations = _load_available_formations()
	_formation_opt.clear()
	for fmt: String in _available_formations:
		_formation_opt.add_item(fmt)
	if not current_sel.is_empty():
		for i: int in range(_formation_opt.item_count):
			if _formation_opt.get_item_text(i) == current_sel:
				_formation_opt.select(i)
				break


func setup(p_career_state: Node, fixture: Dictionary, db: PlayerDatabase = null) -> void:
	career_state = p_career_state
	_selected_swap_player_id = -1
	_populate_formation_options()

	if is_instance_valid(career_state) and career_state.has_method("prepare_match_context"):
		match_context = career_state.call("prepare_match_context", fixture, db)
	else:
		match_context = _build_fallback_match_context(fixture)

	_sync_context_with_career_state()
	use_alternate_away_kit = bool(match_context.get("kits", {}).get("clash_detected", false))
	_refresh_view()


func _sync_context_with_career_state() -> void:
	if not is_instance_valid(career_state) or match_context.is_empty():
		return
	var managed_id: int = int(career_state.get_managed_club_id()) if career_state.has_method("get_managed_club_id") else int(match_context.get("home_club_id", 1))
	var user_is_home: bool = int(match_context.get("home_club_id", managed_id)) == managed_id
	match_context["user_is_home"] = user_is_home

	var tactics: Dictionary = match_context.get("tactics", {}).duplicate(true)
	if career_state.has_method("get_club_tactics"):
		var saved_tactics: Dictionary = career_state.get_club_tactics(managed_id)
		if not saved_tactics.is_empty():
			tactics = saved_tactics.duplicate(true)
	match_context["tactics"] = tactics

	var active_fmt := String(tactics.get("formation", "4-3-3"))
	if user_is_home:
		match_context["home_formation"] = active_fmt
	else:
		match_context["away_formation"] = active_fmt

	if career_state.has_method("get_club_squad"):
		var squad: Array = career_state.get_club_squad(managed_id)
		var split := _split_xi_and_bench(squad, active_fmt)
		var live_lineup: Array = split["xi"]
		var live_bench: Array = split["bench"]

		if user_is_home:
			match_context["home_lineup"] = live_lineup
			match_context["home_bench"] = live_bench
			match_context["home_squad"] = squad.duplicate(true)
		else:
			match_context["away_lineup"] = live_lineup
			match_context["away_bench"] = live_bench
			match_context["away_squad"] = squad.duplicate(true)

	match_context["user_club_id"] = match_context.get("home_club_id", managed_id) if user_is_home else match_context.get("away_club_id", managed_id)
	match_context["opponent_club_id"] = match_context.get("away_club_id", -1) if user_is_home else match_context.get("home_club_id", -1)
	match_context["user_club"] = match_context.get("home_club", "Home") if user_is_home else match_context.get("away_club", "Away")
	match_context["opponent_club"] = match_context.get("away_club", "Away") if user_is_home else match_context.get("home_club", "Home")
	match_context["user_ovr"] = match_context.get("home_ovr", 75) if user_is_home else match_context.get("away_ovr", 75)
	match_context["opponent_ovr"] = match_context.get("away_ovr", 75) if user_is_home else match_context.get("home_ovr", 75)
	match_context["user_formation"] = match_context.get("home_formation", "4-3-3") if user_is_home else match_context.get("away_formation", "4-3-3")
	match_context["opponent_formation"] = match_context.get("away_formation", "4-3-3") if user_is_home else match_context.get("home_formation", "4-3-3")
	match_context["user_lineup"] = match_context.get("home_lineup", []) if user_is_home else match_context.get("away_lineup", [])
	match_context["opponent_lineup"] = match_context.get("away_lineup", []) if user_is_home else match_context.get("home_lineup", [])
	match_context["user_bench"] = match_context.get("home_bench", []) if user_is_home else match_context.get("away_bench", [])
	match_context["opponent_bench"] = match_context.get("away_bench", []) if user_is_home else match_context.get("home_bench", [])


func _split_xi_and_bench(squad: Array, formation: String) -> Dictionary:
	if squad.size() <= 11:
		return {"xi": squad.duplicate(true), "bench": []}

	# Respect the exact Starting XI order maintained by CareerState (0..10 starters, 11..N-1 bench)
	var starters: Array = []
	var bench: Array = []
	for p: Variant in squad:
		if p is Dictionary and bool(p.get("is_starting_xi", p.get("is_starter", false))):
			starters.append(p)
		else:
			bench.append(p)

	if starters.size() == 11:
		return {"xi": starters, "bench": bench}

	if starters.is_empty():
		return {
			"xi": squad.slice(0, 11),
			"bench": squad.slice(11),
		}

	var picked_xi := _select_formation_aware_xi(squad, formation)
	var xi_ids: Dictionary = {}
	for p: Dictionary in picked_xi:
		xi_ids[p.get("id", p.get("name", ""))] = true

	var rem_bench: Array = []
	for p: Variant in squad:
		if p is Dictionary:
			var key: Variant = p.get("id", p.get("name", ""))
			if not xi_ids.has(key):
				rem_bench.append(p)

	return {"xi": picked_xi, "bench": rem_bench}


func _select_formation_aware_xi(squad: Array, formation: String) -> Array:
	if squad.size() <= 11:
		return squad.duplicate(true)

	var parts := formation.split("-")
	var def_req := int(parts[0]) if parts.size() >= 1 else 4
	var mid_req := 0
	for i: int in range(1, parts.size() - 1):
		mid_req += int(parts[i])
	if mid_req <= 0:
		mid_req = 3
	var fwd_req := int(parts[parts.size() - 1]) if parts.size() >= 2 else 3

	var pool: Array = squad.duplicate(true)
	pool.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		var a_fit := float(a.get("condition", a.get("stamina", 100.0)))
		var b_fit := float(b.get("condition", b.get("stamina", 100.0)))
		var a_score := float(a.get("ovr", 70)) * (0.5 + 0.5 * (a_fit / 100.0))
		var b_score := float(b.get("ovr", 70)) * (0.5 + 0.5 * (b_fit / 100.0))
		return a_score > b_score
	)

	var picked: Array = []
	var used_ids: Dictionary = {}
	var buckets := [
		{"group": "GK", "count": 1},
		{"group": "DEF", "count": def_req},
		{"group": "MID", "count": mid_req},
		{"group": "FWD", "count": fwd_req},
	]
	for b: Dictionary in buckets:
		var needed := int(b["count"])
		var target_group := String(b["group"])
		for p: Dictionary in pool:
			if needed <= 0:
				break
			var pid: Variant = p.get("id", p.get("player_id", p.get("name", "")))
			if used_ids.has(pid):
				continue
			if _position_group(String(p.get("position", "CM"))) == target_group:
				picked.append(p)
				used_ids[pid] = true
				needed -= 1

	for p: Dictionary in pool:
		if picked.size() >= 11:
			break
		var pid: Variant = p.get("id", p.get("player_id", p.get("name", "")))
		if not used_ids.has(pid):
			picked.append(p)
			used_ids[pid] = true

	return picked.slice(0, 11)


func _position_group(pos: String) -> String:
	var p := pos.strip_edges().to_upper()
	if p == "GK":
		return "GK"
	if p in ["CB", "LB", "RB", "LWB", "RWB", "SW"]:
		return "DEF"
	if p in ["ST", "CF", "LW", "RW", "LF", "RF", "SS"]:
		return "FWD"
	return "MID"


func _parse_color(raw: Variant, fallback: Color) -> Color:
	if raw is Color:
		return raw
	if raw is String:
		var s := String(raw).strip_edges()
		if not s.is_empty():
			return Color.html(s)
	return fallback


func _build_fallback_match_context(fixture: Dictionary) -> Dictionary:
	var home_id := int(fixture.get("home_club_id", 1))
	var away_id := int(fixture.get("away_club_id", 2))
	var managed_id := int(career_state.get_managed_club_id()) if is_instance_valid(career_state) and career_state.has_method("get_managed_club_id") else home_id

	var home_info: Dictionary = career_state.get_club_info(home_id) if is_instance_valid(career_state) and career_state.has_method("get_club_info") else {}
	var away_info: Dictionary = career_state.get_club_info(away_id) if is_instance_valid(career_state) and career_state.has_method("get_club_info") else {}

	var home_name := String(fixture.get("home_club", home_info.get("club_name", home_info.get("name", "Home"))))
	var away_name := String(fixture.get("away_club", away_info.get("club_name", away_info.get("name", "Away"))))
	var home_color: Color = _parse_color(home_info.get("primary_color", home_info.get("kit_primary", "")), Color(0.06, 0.78, 0.48))
	var away_color: Color = _parse_color(away_info.get("primary_color", away_info.get("kit_primary", "")), Color(0.92, 0.26, 0.30))
	var away_alt_color: Color = _parse_color(away_info.get("secondary_color", away_info.get("kit_secondary", "")), away_color.inverted())
	var clash := home_color.is_equal_approx(away_color)

	var tactics: Dictionary = {}
	if is_instance_valid(career_state) and career_state.has_method("get_club_tactics"):
		tactics = career_state.get_club_tactics(managed_id).duplicate(true)
	var active_fmt := String(tactics.get("formation", "4-3-3"))

	var home_squad: Array = career_state.get_club_squad(home_id) if is_instance_valid(career_state) and career_state.has_method("get_club_squad") else []
	var away_squad: Array = career_state.get_club_squad(away_id) if is_instance_valid(career_state) and career_state.has_method("get_club_squad") else []
	var home_split := _split_xi_and_bench(home_squad, active_fmt if home_id == managed_id else "4-3-3")
	var away_split := _split_xi_and_bench(away_squad, active_fmt if away_id == managed_id else "4-3-3")

	var stadium_name := String(fixture.get("stadium", home_info.get("stadium", "%s Stadium" % home_name)))
	var competition_name := String(fixture.get("competition", "League Match"))

	return {
		"fixture": fixture,
		"home_club_id": home_id,
		"away_club_id": away_id,
		"home_club": home_name,
		"away_club": away_name,
		"home_ovr": int(home_info.get("team_rating", home_info.get("reputation", 75))),
		"away_ovr": int(away_info.get("team_rating", away_info.get("reputation", 75))),
		"home_lineup": home_split["xi"],
		"away_lineup": away_split["xi"],
		"home_bench": home_split["bench"],
		"away_bench": away_split["bench"],
		"home_squad": home_squad,
		"away_squad": away_squad,
		"home_formation": active_fmt if home_id == managed_id else "4-3-3",
		"away_formation": active_fmt if away_id == managed_id else "4-3-3",
		"tactics": tactics,
		"user_is_home": home_id == managed_id,
		"stadium": stadium_name,
		"competition": competition_name,
		"importance": {
			"label": String(fixture.get("match_importance", competition_name.to_upper())),
			"text_color": Color(0.11, 0.82, 0.63),
			"description": "%s vs %s • %s" % [home_name, away_name, stadium_name],
		},
		"kits": {
			"home_primary": home_color,
			"away_primary": away_alt_color,
			"away_default_primary": away_color,
			"active_away_primary": away_alt_color if clash else away_color,
			"clash_detected": clash,
		},
	}


func _create_player_row(p: Dictionary, is_starter: bool) -> HBoxContainer:
	var pid := int(p.get("id", -1))
	var fitness: float = float(p.get("condition", p.get("stamina", 100.0)))
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 8)

	var name_lbl := Label.new()
	var tag := "" if is_starter else "SUB · "
	name_lbl.text = "%s[%s] %s (OVR %d)" % [tag, p.get("position", "CM"), p.get("name", "Player"), int(p.get("ovr", 75))]
	name_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	if pid == _selected_swap_player_id:
		name_lbl.add_theme_color_override("font_color", Color(0.25, 0.90, 1.0))
	elif not is_starter:
		name_lbl.add_theme_color_override("font_color", Color(0.80, 0.85, 0.92))
	row.add_child(name_lbl)

	var stam_lbl := Label.new()
	stam_lbl.text = "%d%%" % int(round(fitness))
	stam_lbl.add_theme_color_override(
		"font_color",
		Color(0.35, 0.95, 0.55) if fitness >= 75.0 else (Color(1.0, 0.80, 0.30) if fitness >= 55.0 else Color(1.0, 0.35, 0.35))
	)
	row.add_child(stam_lbl)

	if pid > 0 and is_instance_valid(career_state) and career_state.has_method("swap_squad_players"):
		var swap_btn := Button.new()
		swap_btn.custom_minimum_size = Vector2(78, 24)
		if pid == _selected_swap_player_id:
			swap_btn.text = "Cancel"
		elif _selected_swap_player_id > 0:
			swap_btn.text = "Swap In"
		else:
			swap_btn.text = "Swap"
		swap_btn.pressed.connect(func() -> void: _on_player_swap_clicked(pid))
		row.add_child(swap_btn)

	return row


func _on_player_swap_clicked(player_id: int) -> void:
	if _selected_swap_player_id == player_id:
		_selected_swap_player_id = -1
		_refresh_view()
		return

	if _selected_swap_player_id <= 0:
		_selected_swap_player_id = player_id
		_refresh_view()
		return

	var first_id := _selected_swap_player_id
	_selected_swap_player_id = -1

	if is_instance_valid(career_state) and career_state.has_method("swap_squad_players"):
		var managed_id: int = int(career_state.get_managed_club_id()) if career_state.has_method("get_managed_club_id") else int(match_context.get("home_club_id", 1))
		career_state.swap_squad_players(managed_id, first_id, player_id)
		_sync_context_with_career_state()

	_refresh_view()


func _refresh_view() -> void:
	if match_context.is_empty():
		return
	var imp: Dictionary = match_context.get("importance", {})
	_importance_badge.text = "[ %s ]" % String(imp.get("label", "LEAGUE MATCH"))
	_importance_badge.add_theme_color_override("font_color", imp.get("text_color", Color.WHITE))
	_importance_desc.text = String(imp.get("description", ""))

	_home_title.text = "%s (%d OVR)" % [match_context.get("home_club", "Home"), int(match_context.get("home_ovr", 75))]
	_home_tag_label.text = "HOME"
	_away_title.text = "%s (%d OVR)" % [match_context.get("away_club", "Away"), int(match_context.get("away_ovr", 75))]
	_away_tag_label.text = "AWAY"

	var kits: Dictionary = match_context.get("kits", {})
	_home_kit_swatch.color = kits.get("home_primary", Color.RED)
	var active_away: Color = kits.get("away_primary", Color.BLUE) if use_alternate_away_kit else kits.get("away_default_primary", Color.BLUE)
	_away_kit_swatch.color = active_away
	kits["active_away_primary"] = active_away
	_kit_status_lbl.text = "Kit Clash Auto-Resolved (Alternate Away Kit Active)" if bool(kits.get("clash_detected", false)) else ("Alternate Away Kit Active" if use_alternate_away_kit else "No Kit Clash Detected")

	var tactics: Dictionary = match_context.get("tactics", {})
	var active_fmt: String = String(tactics.get("formation", "4-3-3"))
	var found_fmt := false
	for i: int in range(_formation_opt.item_count):
		if _formation_opt.get_item_text(i) == active_fmt:
			_formation_opt.select(i)
			found_fmt = true
			break
	if not found_fmt and not active_fmt.is_empty():
		_formation_opt.add_item(active_fmt)
		_formation_opt.select(_formation_opt.item_count - 1)

	var saved_mentality := int(tactics.get("mentality", 2))
	if saved_mentality >= 0 and saved_mentality < _mentality_opt.item_count:
		_mentality_opt.select(saved_mentality)

	var saved_pressing := int(tactics.get("pressing", 1))
	if saved_pressing >= 0 and saved_pressing < _pressing_opt.item_count:
		_pressing_opt.select(saved_pressing)

	for child: Node in _lineup_list.get_children():
		child.queue_free()
	for child: Node in _bench_list.get_children():
		child.queue_free()

	var user_is_home: bool = bool(match_context.get("user_is_home", true))
	var xi: Array = match_context.get("user_lineup", match_context.get("home_lineup", []) if user_is_home else match_context.get("away_lineup", []))
	var bench: Array = match_context.get("user_bench", match_context.get("home_bench", []) if user_is_home else match_context.get("away_bench", []))
	var opponent_xi: Array = match_context.get("opponent_lineup", match_context.get("away_lineup", []) if user_is_home else match_context.get("home_lineup", []))
	var opponent_name: String = String(match_context.get("opponent_club", match_context.get("away_club", "Opponent") if user_is_home else match_context.get("home_club", "Opponent")))
	var opponent_ovr: int = int(match_context.get("opponent_ovr", match_context.get("away_ovr", 75) if user_is_home else match_context.get("home_ovr", 75)))
	var opponent_formation: String = String(match_context.get("opponent_formation", match_context.get("away_formation", "4-3-3") if user_is_home else match_context.get("home_formation", "4-3-3")))
	var tired_count: int = 0

	for p: Dictionary in xi:
		var fitness: float = float(p.get("condition", p.get("stamina", 100.0)))
		if fitness < 65.0:
			tired_count += 1
		_lineup_list.add_child(_create_player_row(p, true))

	_bench_header_lbl.text = "BENCH / SUBSTITUTES (%d)" % bench.size()
	if bench.is_empty():
		var empty_bench_lbl := Label.new()
		empty_bench_lbl.text = "No extra bench players available in squad."
		empty_bench_lbl.add_theme_color_override("font_color", Color(0.60, 0.66, 0.76))
		_bench_list.add_child(empty_bench_lbl)
	else:
		for bp: Dictionary in bench:
			_bench_list.add_child(_create_player_row(bp, false))

	if _selected_swap_player_id > 0:
		_swap_hint_lbl.text = "Select a second player to complete the lineup swap (or click Cancel)."
	else:
		_swap_hint_lbl.text = "Bench synced to squad (%d reserve(s)). Click Swap on any two players to adjust your XI." % bench.size()

	if _opponent_title_label:
		_opponent_title_label.text = "OPPONENT SCOUTING · %s" % opponent_name
	if _opponent_meta_label:
		_opponent_meta_label.text = "OVR %d  ·  Formation %s  ·  Expected starters %d" % [opponent_ovr, opponent_formation, opponent_xi.size()]
	if _opponent_lineup_list:
		for child in _opponent_lineup_list.get_children():
			child.queue_free()
		for i: int in range(opponent_xi.size()):
			var player: Dictionary = opponent_xi[i] if opponent_xi[i] is Dictionary else {}
			var scout_row := Label.new()
			scout_row.text = "%02d  [%s]  %s  ·  OVR %d" % [i + 1, player.get("position", "CM"), player.get("name", "Player"), int(player.get("ovr", 70))]
			scout_row.add_theme_color_override("font_color", Color(0.82, 0.87, 0.95))
			_opponent_lineup_list.add_child(scout_row)

	_warning_lbl.text = "Warning: %d starter(s) below 65%% fitness — higher injury risk." % tired_count if tired_count > 0 else "Squad condition optimal."


func _on_toggle_away_kit() -> void:
	use_alternate_away_kit = not use_alternate_away_kit
	var kits: Dictionary = match_context.get("kits", {})
	var swapped: Color = kits.get("away_primary", Color.BLUE) if use_alternate_away_kit else kits.get("away_default_primary", Color.BLUE)
	_away_kit_swatch.color = swapped
	kits["active_away_primary"] = swapped
	_kit_status_lbl.text = "Alternate Away Kit Active" if use_alternate_away_kit else "Standard Away Kit Active"


func _on_tactics_changed(_idx: int) -> void:
	if match_context.is_empty():
		return
	var fmt: String = _formation_opt.get_item_text(_formation_opt.selected)
	var tactics: Dictionary = match_context.get("tactics", {}).duplicate(true)

	if is_instance_valid(career_state):
		var club_id: int = int(career_state.get_managed_club_id()) if career_state.has_method("get_managed_club_id") else int(match_context.get("home_club_id", 1))
		if career_state.has_method("get_club_tactics"):
			tactics = career_state.get_club_tactics(club_id).duplicate(true)

		tactics["formation"] = fmt
		tactics["mentality"] = _mentality_opt.selected
		tactics["pressing"] = _pressing_opt.selected

		if career_state.has_method("set_club_tactics"):
			career_state.set_club_tactics(club_id, tactics)
	else:
		tactics["formation"] = fmt
		tactics["mentality"] = _mentality_opt.selected
		tactics["pressing"] = _pressing_opt.selected

	match_context["tactics"] = tactics.duplicate(true)
	if bool(match_context.get("user_is_home", true)):
		match_context["home_formation"] = fmt
	else:
		match_context["away_formation"] = fmt

	_sync_context_with_career_state()
	_refresh_view()


func _on_kickoff_pressed() -> void:
	_on_tactics_changed(0)
	start_match_requested.emit(match_context)
