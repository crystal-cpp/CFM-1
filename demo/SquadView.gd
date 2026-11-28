class_name SquadView
extends Control

signal player_profile_requested(player_id: int, context_mode: String)
signal route_requested(route_id: String)

var career_state: CareerState

var _ui_built: bool = false
var _selected_player_id: int = -1
var _captain_id: int = -1
var _current_formation: String = "4-3-3"
var _formations_cache: Dictionary = {}

# UI Containers & Widgets
var _header_title: Label
var _team_ovr_pill: Label
var _formation_pill: Label
var _avg_cond_pill: Label
var _selection_banner: Label
var _pitch_canvas: Control
var _pitch_slots_layer: Control
var _bench_vbox: VBoxContainer
var _reserves_vbox: VBoxContainer
var _captain_btn: Button
var _inspect_btn: Button
var _clear_sel_btn: Button

const DEFAULT_FORMATIONS: Dictionary = {
	"4-3-3": [
		{ "role": "GK",  "x": 0.50, "y": 0.90 },
		{ "role": "LB",  "x": 0.16, "y": 0.72 },
		{ "role": "CB",  "x": 0.38, "y": 0.75 },
		{ "role": "CB",  "x": 0.62, "y": 0.75 },
		{ "role": "RB",  "x": 0.84, "y": 0.72 },
		{ "role": "CM",  "x": 0.28, "y": 0.50 },
		{ "role": "CDM", "x": 0.50, "y": 0.56 },
		{ "role": "CM",  "x": 0.72, "y": 0.50 },
		{ "role": "LW",  "x": 0.20, "y": 0.24 },
		{ "role": "ST",  "x": 0.50, "y": 0.18 },
		{ "role": "RW",  "x": 0.80, "y": 0.24 }
	],
	"4-2-3-1": [
		{ "role": "GK",  "x": 0.50, "y": 0.90 },
		{ "role": "LB",  "x": 0.16, "y": 0.73 },
		{ "role": "CB",  "x": 0.38, "y": 0.76 },
		{ "role": "CB",  "x": 0.62, "y": 0.76 },
		{ "role": "RB",  "x": 0.84, "y": 0.73 },
		{ "role": "CDM", "x": 0.36, "y": 0.58 },
		{ "role": "CDM", "x": 0.64, "y": 0.58 },
		{ "role": "LW",  "x": 0.20, "y": 0.36 },
		{ "role": "CAM", "x": 0.50, "y": 0.38 },
		{ "role": "RW",  "x": 0.80, "y": 0.36 },
		{ "role": "ST",  "x": 0.50, "y": 0.17 }
	],
	"4-4-2": [
		{ "role": "GK",  "x": 0.50, "y": 0.90 },
		{ "role": "LB",  "x": 0.16, "y": 0.72 },
		{ "role": "CB",  "x": 0.38, "y": 0.75 },
		{ "role": "CB",  "x": 0.62, "y": 0.75 },
		{ "role": "RB",  "x": 0.84, "y": 0.72 },
		{ "role": "LW",  "x": 0.18, "y": 0.48 },
		{ "role": "CM",  "x": 0.38, "y": 0.50 },
		{ "role": "CM",  "x": 0.62, "y": 0.50 },
		{ "role": "RW",  "x": 0.82, "y": 0.48 },
		{ "role": "ST",  "x": 0.36, "y": 0.20 },
		{ "role": "ST",  "x": 0.64, "y": 0.20 }
	],
	"3-5-2": [
		{ "role": "GK",  "x": 0.50, "y": 0.90 },
		{ "role": "CB",  "x": 0.26, "y": 0.75 },
		{ "role": "CB",  "x": 0.50, "y": 0.77 },
		{ "role": "CB",  "x": 0.74, "y": 0.75 },
		{ "role": "LB",  "x": 0.14, "y": 0.48 },
		{ "role": "CDM", "x": 0.36, "y": 0.56 },
		{ "role": "CAM", "x": 0.50, "y": 0.40 },
		{ "role": "CDM", "x": 0.64, "y": 0.56 },
		{ "role": "RB",  "x": 0.86, "y": 0.48 },
		{ "role": "ST",  "x": 0.36, "y": 0.20 },
		{ "role": "ST",  "x": 0.64, "y": 0.20 }
	],
	"5-3-2": [
		{ "role": "GK",  "x": 0.50, "y": 0.90 },
		{ "role": "LB",  "x": 0.14, "y": 0.68 },
		{ "role": "CB",  "x": 0.32, "y": 0.75 },
		{ "role": "CB",  "x": 0.50, "y": 0.77 },
		{ "role": "CB",  "x": 0.68, "y": 0.75 },
		{ "role": "RB",  "x": 0.86, "y": 0.68 },
		{ "role": "CM",  "x": 0.28, "y": 0.48 },
		{ "role": "CDM", "x": 0.50, "y": 0.52 },
		{ "role": "CM",  "x": 0.72, "y": 0.48 },
		{ "role": "ST",  "x": 0.36, "y": 0.20 },
		{ "role": "ST",  "x": 0.64, "y": 0.20 }
	],
	"4-1-4-1": [
		{ "role": "GK",  "x": 0.50, "y": 0.90 },
		{ "role": "LB",  "x": 0.16, "y": 0.73 },
		{ "role": "CB",  "x": 0.38, "y": 0.76 },
		{ "role": "CB",  "x": 0.62, "y": 0.76 },
		{ "role": "RB",  "x": 0.84, "y": 0.73 },
		{ "role": "CDM", "x": 0.50, "y": 0.60 },
		{ "role": "LW",  "x": 0.18, "y": 0.42 },
		{ "role": "CM",  "x": 0.38, "y": 0.44 },
		{ "role": "CM",  "x": 0.62, "y": 0.44 },
		{ "role": "RW",  "x": 0.82, "y": 0.42 },
		{ "role": "ST",  "x": 0.50, "y": 0.18 }
	]
}

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_flags_vertical = Control.SIZE_EXPAND_FILL
	_load_formations_data()
	_ensure_ui_built()
	visibility_changed.connect(func() -> void:
		if visible:
			refresh_squad()
	)
	refresh_squad()

func _load_formations_data() -> void:
	_formations_cache = DEFAULT_FORMATIONS.duplicate(true)
	var paths: Array[String] = ["res://data/formations.csv", "res://formations.csv"]
	for path in paths:
		if FileAccess.file_exists(path):
			var f := FileAccess.open(path, FileAccess.READ)
			if f == null:
				continue
			var parsed: Dictionary = {}
			while not f.eof_reached():
				var line: String = f.get_line().strip_edges()
				if line.is_empty() or line.begins_with("#") or line.to_lower().begins_with("formation"):
					continue
				var cols: PackedStringArray = line.split(",")
				if cols.size() >= 4:
					var fname: String = cols[0].strip_edges()
					var role: String = cols[1].strip_edges().to_upper()
					var raw_depth: float = clampf(float(cols[2]), 0.06, 0.94)
					var raw_width: float = clampf(float(cols[3]), 0.10, 0.94)
					if not parsed.has(fname):
						parsed[fname] = []
					parsed[fname].append({ "role": role, "x": raw_width, "y": 1.0 - raw_depth })
			f.close()
			for k in parsed.keys():
				if parsed[k].size() == 11:
					_formations_cache[k] = parsed[k]
			break

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

	# Top Bar
	var top_hbox := HBoxContainer.new()
	top_hbox.add_theme_constant_override("separation", 7)
	main_vbox.add_child(top_hbox)

	_header_title = Label.new()
	_header_title.text = "SQUAD MANAGEMENT"
	_header_title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	_header_title.add_theme_font_size_override("font_size", 20)
	_header_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_header_title.clip_text = true
	top_hbox.add_child(_header_title)

	_formation_pill = _create_badge("Formation: 4-3-3", Color(0.16, 0.38, 0.68), Color(0.75, 0.88, 1.0))
	top_hbox.add_child(_formation_pill)

	_team_ovr_pill = _create_badge("XI OVR: 75", Color(0.06, 0.67, 0.52), Color(0.35, 1.0, 0.72))
	top_hbox.add_child(_team_ovr_pill)

	_avg_cond_pill = _create_badge("Fitness: 100%", Color(0.55, 0.42, 0.12), Color(1.0, 0.85, 0.35))
	top_hbox.add_child(_avg_cond_pill)

	var tactics_shortcut := Button.new()
	tactics_shortcut.text = "Tactics"
	tactics_shortcut.focus_mode = Control.FOCUS_NONE
	_style_btn(tactics_shortcut, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.28, 0.44, 0.85))
	tactics_shortcut.pressed.connect(func() -> void: route_requested.emit("tactics"))
	top_hbox.add_child(tactics_shortcut)

	# Interactive Action & Selection Toolbar
	var toolbar := PanelContainer.new()
	var tb_style := StyleBoxFlat.new()
	tb_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	tb_style.border_color = Color(0.12, 0.20, 0.32, 0.80)
	tb_style.set_border_width_all(1)
	tb_style.set_corner_radius_all(8)
	tb_style.content_margin_left = 12
	tb_style.content_margin_right = 12
	tb_style.content_margin_top = 8
	tb_style.content_margin_bottom = 8
	toolbar.add_theme_stylebox_override("panel", tb_style)
	main_vbox.add_child(toolbar)

	var tb_hbox := HBoxContainer.new()
	tb_hbox.add_theme_constant_override("separation", 10)
	toolbar.add_child(tb_hbox)

	_selection_banner = Label.new()
	_selection_banner.text = "Select any player on the pitch, bench, or reserves to swap positions or inspect attributes."
	_selection_banner.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_selection_banner.custom_minimum_size.x = 0
	_selection_banner.clip_text = true
	_selection_banner.add_theme_color_override("font_color", Color(0.75, 0.84, 0.95))
	_selection_banner.add_theme_font_size_override("font_size", 12)
	tb_hbox.add_child(_selection_banner)

	_captain_btn = Button.new()
	_captain_btn.text = "Make Captain"
	_captain_btn.disabled = true
	_captain_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_captain_btn, Color(0.45, 0.34, 0.08, 0.95), Color(0.85, 0.68, 0.20, 0.90), Color(1.0, 0.90, 0.45))
	_captain_btn.pressed.connect(_on_make_captain_pressed)
	tb_hbox.add_child(_captain_btn)

	_inspect_btn = Button.new()
	_inspect_btn.text = "Profile"
	_inspect_btn.disabled = true
	_inspect_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_inspect_btn, Color(0.06, 0.42, 0.30, 0.95), Color(0.11, 0.82, 0.63, 0.90))
	_inspect_btn.pressed.connect(_on_inspect_selected_pressed)
	tb_hbox.add_child(_inspect_btn)

	_clear_sel_btn = Button.new()
	_clear_sel_btn.text = "✕ Clear"
	_clear_sel_btn.disabled = true
	_clear_sel_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_clear_sel_btn, Color(0.10, 0.14, 0.22, 0.90), Color(0.22, 0.30, 0.42, 0.80))
	_clear_sel_btn.pressed.connect(func() -> void:
		_selected_player_id = -1
		refresh_squad()
	)
	tb_hbox.add_child(_clear_sel_btn)

	# Main Split: Left 2D Tactical Pitch | Right Bench & Reserves Lists
	var split := HBoxContainer.new()
	split.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	split.size_flags_vertical = Control.SIZE_EXPAND_FILL
	split.add_theme_constant_override("separation", 16)
	main_vbox.add_child(split)

	# Left Pitch Card
	var pitch_card := PanelContainer.new()
	pitch_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	pitch_card.size_flags_vertical = Control.SIZE_EXPAND_FILL
	pitch_card.size_flags_stretch_ratio = 1.65
	var pc_style := StyleBoxFlat.new()
	pc_style.bg_color = Color(0.035, 0.085, 0.065, 0.98)
	pc_style.border_color = Color(0.11, 0.82, 0.63, 0.70)
	pc_style.set_border_width_all(2)
	pc_style.set_corner_radius_all(10)
	pc_style.set_content_margin_all(10)
	pitch_card.add_theme_stylebox_override("panel", pc_style)
	pitch_card.custom_minimum_size = Vector2(280, 0)
	split.add_child(pitch_card)

	_pitch_canvas = Control.new()
	_pitch_canvas.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_pitch_canvas.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_pitch_canvas.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_pitch_canvas.draw.connect(_on_pitch_draw)
	_pitch_canvas.resized.connect(refresh_squad)
	pitch_card.add_child(_pitch_canvas)

	_pitch_slots_layer = Control.new()
	_pitch_slots_layer.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_pitch_canvas.add_child(_pitch_slots_layer)

	# Right Roster Column (Bench + Reserves)
	var right_card := PanelContainer.new()
	right_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	right_card.size_flags_vertical = Control.SIZE_EXPAND_FILL
	right_card.size_flags_stretch_ratio = 0.85
	var rc_style := StyleBoxFlat.new()
	rc_style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	rc_style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	rc_style.set_border_width_all(1)
	rc_style.set_corner_radius_all(10)
	rc_style.set_content_margin_all(12)
	right_card.add_theme_stylebox_override("panel", rc_style)
	split.add_child(right_card)

	var right_scroll := ScrollContainer.new()
	right_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	right_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	right_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	right_card.add_child(right_scroll)

	var lists_vbox := VBoxContainer.new()
	lists_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	lists_vbox.add_theme_constant_override("separation", 10)
	right_scroll.add_child(lists_vbox)

	var bench_hdr := Label.new()
	bench_hdr.text = "BENCH (7)"
	bench_hdr.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	bench_hdr.add_theme_font_size_override("font_size", 13)
	lists_vbox.add_child(bench_hdr)

	_bench_vbox = VBoxContainer.new()
	_bench_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_bench_vbox.add_theme_constant_override("separation", 5)
	lists_vbox.add_child(_bench_vbox)

	lists_vbox.add_child(HSeparator.new())

	var res_hdr := Label.new()
	res_hdr.text = "RESERVES"
	res_hdr.add_theme_color_override("font_color", Color(0.55, 0.68, 0.85))
	res_hdr.add_theme_font_size_override("font_size", 13)
	lists_vbox.add_child(res_hdr)

	_reserves_vbox = VBoxContainer.new()
	_reserves_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_reserves_vbox.add_theme_constant_override("separation", 5)
	lists_vbox.add_child(_reserves_vbox)

func _on_pitch_draw() -> void:
	var w: float = _pitch_canvas.size.x
	var h: float = _pitch_canvas.size.y
	if w <= 10.0 or h <= 10.0:
		return

	var line_col := Color(0.18, 0.78, 0.55, 0.42)
	var stripe_col := Color(0.05, 0.12, 0.09, 0.65)
	var stripe_h: float = h / 8.0
	for i in range(8):
		if i % 2 == 0:
			_pitch_canvas.draw_rect(Rect2(0, i * stripe_h, w, stripe_h), stripe_col, true)

	# Outer touchline, halfway line, center circle, penalty boxes
	_pitch_canvas.draw_rect(Rect2(12, 12, w - 24, h - 24), line_col, false, 2.0)
	_pitch_canvas.draw_line(Vector2(12, h * 0.5), Vector2(w - 12, h * 0.5), line_col, 2.0)
	_pitch_canvas.draw_arc(Vector2(w * 0.5, h * 0.5), minf(w, h) * 0.13, 0.0, TAU, 48, line_col, 2.0)
	_pitch_canvas.draw_rect(Rect2(w * 0.24, 12, w * 0.52, h * 0.16), line_col, false, 2.0)
	_pitch_canvas.draw_rect(Rect2(w * 0.24, h - 12 - h * 0.16, w * 0.52, h * 0.16), line_col, false, 2.0)

func refresh_squad() -> void:
	_ensure_ui_built()
	if not is_instance_valid(career_state) or not career_state.is_career_active():
		return

	var club_id: int = career_state.get_managed_club_id()
	var club_info: Dictionary = career_state.get_club_info(club_id)
	var squad: Array = career_state.get_club_squad(club_id)

	_current_formation = String(club_info.get("formation", "4-3-3"))
	_captain_id = int(club_info.get("captain_id", -1))

	_header_title.text = "SQUAD MANAGEMENT"
	_formation_pill.text = "Formation: %s" % _current_formation
	_team_ovr_pill.text = "XI OVR: %d" % int(club_info.get("team_rating", 75))

	var total_cond: int = 0
	var injured_active_names: PackedStringArray = PackedStringArray()
	for idx in range(squad.size()):
		var p: Dictionary = squad[idx]
		total_cond += int(p.get("condition", p.get("stamina", 100)))
		var p_inj: bool = bool(p.get("is_injured", false)) or int(p.get("injury_days_remaining", 0)) > 0
		if p_inj and idx < 18:
			injured_active_names.append("%s (%dd)" % [String(p.get("name", "Player")), int(p.get("injury_days_remaining", 1))])
	var avg_cond: int = int(total_cond / max(1, squad.size()))
	_avg_cond_pill.text = "Avg Fitness: %d%%" % avg_cond

	# Update selection toolbar state
	var has_sel: bool = (_selected_player_id != -1)
	_captain_btn.disabled = not has_sel
	_inspect_btn.disabled = not has_sel
	_clear_sel_btn.disabled = not has_sel
	if has_sel:
		var sel_prof: Dictionary = career_state.get_player_profile(_selected_player_id)
		_selection_banner.add_theme_color_override("font_color", Color(0.75, 0.84, 0.95))
		_selection_banner.text = "Selected: %s (%s · OVR %d)" % [
			String(sel_prof.get("name", "Player")),
			String(sel_prof.get("position", "CM")),
			int(sel_prof.get("ovr", 70))
		]
	elif not injured_active_names.is_empty():
		_selection_banner.add_theme_color_override("font_color", Color(1.0, 0.42, 0.45))
		_selection_banner.text = "Injured: %s" % ", ".join(injured_active_names)
	else:
		_selection_banner.add_theme_color_override("font_color", Color(0.75, 0.84, 0.95))
		_selection_banner.text = "Manage your squad here."

	# Clear old children
	for c in _pitch_slots_layer.get_children():
		c.queue_free()
	for c in _bench_vbox.get_children():
		c.queue_free()
	for c in _reserves_vbox.get_children():
		c.queue_free()

	var slots_def: Array = _formations_cache.get(_current_formation, DEFAULT_FORMATIONS["4-3-3"])
	var pw: float = maxf(1.0, _pitch_canvas.size.x)
	var ph: float = maxf(1.0, _pitch_canvas.size.y)

	for i in range(squad.size()):
		var p: Dictionary = squad[i]
		if i < 11 and i < slots_def.size():
			var slot_meta: Dictionary = slots_def[i]
			var card: Button = _create_pitch_player_node(p, String(slot_meta["role"]))
			var card_w: float = 122.0
			var card_h: float = 54.0
			var px: float = float(slot_meta["x"]) * pw - card_w * 0.5
			var py: float = float(slot_meta["y"]) * ph - card_h * 0.5
			card.custom_minimum_size = Vector2(card_w, card_h)
			card.position = Vector2(clampf(px, 8.0, maxf(8.0, pw - card_w - 8.0)), clampf(py, 8.0, maxf(8.0, ph - card_h - 8.0)))
			_pitch_slots_layer.add_child(card)
		elif i < 18:
			_bench_vbox.add_child(_create_list_player_row(p, "SUB %d" % (i - 10)))
		else:
			_reserves_vbox.add_child(_create_list_player_row(p, "RES"))

func _get_position_fit_color(player_pos: String, slot_role: String) -> Color:
	if player_pos == slot_role:
		return Color(0.11, 0.82, 0.63) # Natural fit (Pitch-Green)
	var defenders := ["CB", "LB", "RB"]
	var midfielders := ["CDM", "CM", "CAM"]
	var wingers := ["LW", "RW", "ST"]
	if (player_pos in defenders and slot_role in defenders) or \
	   (player_pos in midfielders and slot_role in midfielders) or \
	   (player_pos in wingers and slot_role in wingers):
		return Color(0.95, 0.68, 0.18) # Compatible secondary (Amber)
	return Color(0.92, 0.25, 0.30) # Out of position (Crimson)

func _create_pitch_player_node(p: Dictionary, slot_role: String) -> Button:
	var pid: int = int(p.get("id", -1))
	var p_pos: String = String(p.get("position", "CM"))
	var ovr: int = int(p.get("ovr", 70))
	var cond: int = int(p.get("condition", 100))
	var full_name: String = String(p.get("name", "Player"))
	var parts: PackedStringArray = full_name.split(" ")
	var short_name: String = parts[parts.size() - 1] if parts.size() > 0 else full_name
	if short_name.length() > 11:
		short_name = short_name.substr(0, 10) + "."

	var fit_col: Color = _get_position_fit_color(p_pos, slot_role)
	var is_selected: bool = (pid == _selected_player_id)
	var cap_prefix: String = "© " if pid == _captain_id else ""
	var is_injured: bool = bool(p.get("is_injured", false)) or int(p.get("injury_days_remaining", 0)) > 0
	var inj_days: int = int(p.get("injury_days_remaining", 0))
	var inj_tier: String = _format_injury_tier(inj_days, String(p.get("injury_label", "")))

	var btn := Button.new()
	btn.custom_minimum_size = Vector2(122, 54)
	btn.focus_mode = Control.FOCUS_NONE
	if is_injured:
		btn.text = "%s%s\n%s · INJ %dd" % [cap_prefix, short_name, slot_role, inj_days]
		btn.tooltip_text = "%s Injury (%d days left)" % [inj_tier, inj_days]
	else:
		btn.text = "%s%s\n%s/%s · %d (%d%%)" % [cap_prefix, short_name, slot_role, p_pos, ovr, cond]
	btn.add_theme_font_size_override("font_size", 10)

	var bg := Color(0.12, 0.38, 0.26, 0.98) if is_selected else (Color(0.24, 0.06, 0.08, 0.96) if is_injured else Color(0.043, 0.063, 0.106, 0.95))
	var border := Color(1.0, 0.90, 0.30, 1.0) if is_selected else (Color(0.95, 0.24, 0.28, 1.0) if is_injured else fit_col)
	_style_btn(btn, bg, border, Color.WHITE)
	btn.pressed.connect(func() -> void: _on_player_clicked(pid))
	return btn

func _create_list_player_row(p: Dictionary, tag: String) -> Button:
	var pid: int = int(p.get("id", -1))
	var p_pos: String = String(p.get("position", "CM"))
	var ovr: int = int(p.get("ovr", 70))
	var cond: int = int(p.get("condition", 100))
	var name_str: String = String(p.get("name", "Player"))
	var is_selected: bool = (pid == _selected_player_id)
	var cap_str: String = " [C]" if pid == _captain_id else ""
	var is_injured: bool = bool(p.get("is_injured", false)) or int(p.get("injury_days_remaining", 0)) > 0
	var inj_days: int = int(p.get("injury_days_remaining", 0))
	var inj_tier: String = _format_injury_tier(inj_days, String(p.get("injury_label", "")))
	var status_str: String = ("INJ %s (%dd)" % [inj_tier, inj_days]) if is_injured else ("Fit %d%%" % cond)
	var btn := Button.new()
	btn.custom_minimum_size = Vector2(0, 32)
	btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	btn.clip_text = true
	btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
	btn.focus_mode = Control.FOCUS_NONE
	var compact_name: String = name_str if name_str.length() <= 14 else name_str.substr(0, 13) + "…"
	btn.text = " %s | %s | %s%s | %d | %s" % [tag, p_pos, compact_name, cap_str, ovr, status_str]
	btn.add_theme_font_size_override("font_size", 10)
	if is_injured:
		btn.tooltip_text = "Injured (%s, %d day(s) remaining) — Unavailable for matchday XI/Bench" % [inj_tier, inj_days]
	var bg := Color(0.10, 0.34, 0.24, 0.95) if is_selected else (Color(0.20, 0.06, 0.08, 0.92) if is_injured else Color(0.06, 0.09, 0.15, 0.90))
	var border := Color(0.18, 0.95, 0.58, 0.95) if is_selected else (Color(0.92, 0.25, 0.30, 0.90) if is_injured else Color(0.14, 0.22, 0.34, 0.70))
	_style_btn(btn, bg, border, Color(0.90, 0.95, 1.0))
	btn.pressed.connect(func() -> void: _on_player_clicked(pid))
	return btn

func _on_player_clicked(player_id: int) -> void:
	if _selected_player_id == -1:
		_selected_player_id = player_id
		refresh_squad()
		return
	if _selected_player_id == player_id:
		_selected_player_id = -1
		refresh_squad()
		player_profile_requested.emit(player_id, "SQUAD")
		return
	if not is_instance_valid(career_state):
		_selected_player_id = -1
		refresh_squad()
		return
	var club_id: int = career_state.get_managed_club_id()
	var squad: Array = career_state.get_club_squad(club_id)
	var idx_a: int = -1
	var idx_b: int = -1
	var p_a: Dictionary = {}
	var p_b: Dictionary = {}
	for i in range(squad.size()):
		var entry: Dictionary = squad[i]
		var eid: int = int(entry.get("id", -1))
		if eid == _selected_player_id:
			idx_a = i
			p_a = entry
		elif eid == player_id:
			idx_b = i
			p_b = entry
	if idx_a != -1 and idx_b != -1:
		var pos_a: String = String(p_a.get("position", "CM")).to_upper()
		var pos_b: String = String(p_b.get("position", "CM")).to_upper()
		if (idx_a == 0 and pos_b != "GK") or (idx_b == 0 and pos_a != "GK") or \
		   (idx_a >= 1 and idx_a <= 10 and pos_b == "GK") or (idx_b >= 1 and idx_b <= 10 and pos_a == "GK"):
			_selected_player_id = -1
			refresh_squad()
			_selection_banner.add_theme_color_override("font_color", Color(1.0, 0.42, 0.45))
			_selection_banner.text = "Only Goalkeepers (GK) can play in the GK slot!"
			return
		var inj_a: bool = bool(p_a.get("is_injured", false)) or int(p_a.get("injury_days_remaining", 0)) > 0
		var inj_b: bool = bool(p_b.get("is_injured", false)) or int(p_b.get("injury_days_remaining", 0)) > 0
		if (inj_a and idx_b < 18) or (inj_b and idx_a < 18):
			var blocked_p: Dictionary = p_a if inj_a else p_b
			var rem_d: int = int(blocked_p.get("injury_days_remaining", 1))
			_selected_player_id = -1
			refresh_squad()
			_selection_banner.add_theme_color_override("font_color", Color(1.0, 0.42, 0.45))
			_selection_banner.text = "Blocked: %s is injured (%s · %dd left) and cannot enter XI or Bench." % [String(blocked_p.get("name", "Player")), _format_injury_tier(rem_d, String(blocked_p.get("injury_label", ""))), rem_d]
			return
	career_state.swap_squad_players(club_id, _selected_player_id, player_id)
	_selected_player_id = -1
	refresh_squad()

func _format_injury_tier(days_left: int, raw_severity: String) -> String:
	var severity: String = raw_severity.strip_edges()
	if not severity.is_empty():
		return severity
	if days_left <= 5:
		return "Minor"
	if days_left <= 14:
		return "Moderate"
	if days_left <= 60:
		return "Major"
	return "Severe"

func _on_make_captain_pressed() -> void:
	if _selected_player_id == -1 or not is_instance_valid(career_state):
		return
	var club_id: int = career_state.get_managed_club_id()
	career_state.set_club_captain(club_id, _selected_player_id)
	_selected_player_id = -1
	refresh_squad()

func _on_inspect_selected_pressed() -> void:
	if _selected_player_id == -1:
		return
	var target_id: int = _selected_player_id
	_selected_player_id = -1
	refresh_squad()
	player_profile_requested.emit(target_id, "SQUAD")

func _create_badge(text: String, bg: Color, fg: Color) -> Label:
	var lbl := Label.new()
	lbl.text = text
	var s := StyleBoxFlat.new()
	s.bg_color = bg.lerp(Color.BLACK, 0.40)
	s.border_color = bg
	s.set_border_width_all(1)
	s.set_corner_radius_all(6)
	s.content_margin_left = 10
	s.content_margin_right = 10
	s.content_margin_top = 4
	s.content_margin_bottom = 4
	lbl.add_theme_stylebox_override("normal", s)
	lbl.add_theme_color_override("font_color", fg)
	lbl.add_theme_font_size_override("font_size", 12)
	return lbl

func _style_btn(btn: Button, bg: Color, border: Color, fg: Color = Color.WHITE) -> void:
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
	var d := n.duplicate()
	d.bg_color = Color(0.05, 0.07, 0.11, 0.60)
	d.border_color = Color(0.12, 0.16, 0.24, 0.50)
	btn.add_theme_stylebox_override("normal", n)
	btn.add_theme_stylebox_override("hover", h)
	btn.add_theme_stylebox_override("pressed", h)
	btn.add_theme_stylebox_override("disabled", d)
	btn.add_theme_color_override("font_color", fg)
	btn.add_theme_color_override("font_disabled_color", Color(0.45, 0.52, 0.62))
