class_name YouthView
extends Control

signal player_profile_requested(player_id: int, context_mode: String)

var career_state: CareerState
var _ui_built: bool = false

var _fac_lvl_lbl: Label
var _acad_lvl_lbl: Label
var _coach_skill_lbl: Label
var _weekly_cost_lbl: Label
var _upgrade_fac_btn: Button
var _upgrade_acad_btn: Button
var _scout_btn: Button
var _coaches_vbox: VBoxContainer
var _prospects_vbox: VBoxContainer

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_flags_vertical = Control.SIZE_EXPAND_FILL
	_ensure_ui_built()
	visibility_changed.connect(func() -> void:
		if visible:
			refresh_youth()
	)
	refresh_youth()

func _ensure_ui_built() -> void:
	if _ui_built:
		return
	_ui_built = true

	var root := PanelContainer.new()
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var bg := StyleBoxFlat.new()
	bg.bg_color = Color(0.024, 0.035, 0.055, 1.0)
	bg.content_margin_left = 22
	bg.content_margin_right = 22
	bg.content_margin_top = 18
	bg.content_margin_bottom = 18
	root.add_theme_stylebox_override("panel", bg)
	add_child(root)

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(scroll)

	var vbox := VBoxContainer.new()
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.add_theme_constant_override("separation", 14)
	scroll.add_child(vbox)

	# Header
	var hdr_hbox := HBoxContainer.new()
	vbox.add_child(hdr_hbox)

	var title := Label.new()
	title.text = "YOUTH ACADEMY & DEVELOPMENT"
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title.add_theme_font_size_override("font_size", 20)
	hdr_hbox.add_child(title)

	_scout_btn = Button.new()
	_scout_btn.text = "🔭 Scout New Prospect"
	_scout_btn.custom_minimum_size = Vector2(180, 36)
	_scout_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_scout_btn, Color(0.06, 0.45, 0.32, 0.95), Color(0.11, 0.85, 0.63, 0.90))
	_scout_btn.pressed.connect(func() -> void:
		if is_instance_valid(career_state):
			career_state.scout_youth_prospect()
			refresh_youth()
	)
	hdr_hbox.add_child(_scout_btn)

	# Development Cycle Clarification Banner
	var cycle_lbl := Label.new()
	cycle_lbl.text = "Development Schedule: Weekly training ticks award attribute XP scaled by Coach Skill & Training Facility Level. Annual aging (+1 yr) occurs on July 1."
	cycle_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	cycle_lbl.add_theme_color_override("font_color", Color(0.68, 0.80, 0.94))
	cycle_lbl.add_theme_font_size_override("font_size", 12)
	vbox.add_child(cycle_lbl)

	# Infrastructure & Coaching Split
	var top_grid := HBoxContainer.new()
	top_grid.add_theme_constant_override("separation", 14)
	vbox.add_child(top_grid)

	# Left: Facilities Card
	var fac_card := _create_card()
	top_grid.add_child(fac_card)
	var fac_vb := VBoxContainer.new()
	fac_vb.add_theme_constant_override("separation", 8)
	fac_card.add_child(fac_vb)

	var fac_title := Label.new()
	fac_title.text = "ACADEMY INFRASTRUCTURE"
	fac_title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	fac_title.add_theme_font_size_override("font_size", 14)
	fac_vb.add_child(fac_title)

	_fac_lvl_lbl = Label.new()
	_fac_lvl_lbl.add_theme_color_override("font_color", Color.WHITE)
	fac_vb.add_child(_fac_lvl_lbl)

	_upgrade_fac_btn = Button.new()
	_upgrade_fac_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_upgrade_fac_btn, Color(0.08, 0.25, 0.42, 0.95), Color(0.22, 0.55, 0.88, 0.85))
	_upgrade_fac_btn.pressed.connect(func() -> void:
		if is_instance_valid(career_state) and career_state.upgrade_youth_facility():
			refresh_youth()
	)
	fac_vb.add_child(_upgrade_fac_btn)

	_acad_lvl_lbl = Label.new()
	_acad_lvl_lbl.add_theme_color_override("font_color", Color.WHITE)
	fac_vb.add_child(_acad_lvl_lbl)

	_upgrade_acad_btn = Button.new()
	_upgrade_acad_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(_upgrade_acad_btn, Color(0.08, 0.25, 0.42, 0.95), Color(0.22, 0.55, 0.88, 0.85))
	_upgrade_acad_btn.pressed.connect(func() -> void:
		if is_instance_valid(career_state) and career_state.upgrade_youth_academy():
			refresh_youth()
	)
	fac_vb.add_child(_upgrade_acad_btn)

	# Right: Coaching Staff Card
	var coach_card := _create_card()
	top_grid.add_child(coach_card)
	var coach_vb := VBoxContainer.new()
	coach_vb.add_theme_constant_override("separation", 8)
	coach_card.add_child(coach_vb)

	var coach_hdr := HBoxContainer.new()
	coach_vb.add_child(coach_hdr)

	var c_title := Label.new()
	c_title.text = "YOUTH COACHING STAFF"
	c_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	c_title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	c_title.add_theme_font_size_override("font_size", 14)
	coach_hdr.add_child(c_title)

	var hire_btn := Button.new()
	hire_btn.text = "+ Hire Coach ($4K/wk)"
	hire_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(hire_btn, Color(0.06, 0.42, 0.30, 0.95), Color(0.11, 0.82, 0.63, 0.85))
	hire_btn.pressed.connect(func() -> void:
		if is_instance_valid(career_state):
			career_state.hire_youth_coach("Le Huynh Duc", 78, 4000.0, "Technical")
			refresh_youth()
	)
	coach_hdr.add_child(hire_btn)

	_coach_skill_lbl = Label.new()
	_coach_skill_lbl.add_theme_color_override("font_color", Color(0.82, 0.90, 1.0))
	coach_vb.add_child(_coach_skill_lbl)

	_weekly_cost_lbl = Label.new()
	_weekly_cost_lbl.add_theme_color_override("font_color", Color(0.95, 0.80, 0.35))
	coach_vb.add_child(_weekly_cost_lbl)

	_coaches_vbox = VBoxContainer.new()
	_coaches_vbox.add_theme_constant_override("separation", 4)
	coach_vb.add_child(_coaches_vbox)

	# Bottom: Academy Prospects Roster
	var pros_card := _create_card()
	vbox.add_child(pros_card)
	var pros_vb := VBoxContainer.new()
	pros_vb.add_theme_constant_override("separation", 8)
	pros_card.add_child(pros_vb)

	var p_title := Label.new()
	p_title.text = "ACADEMY PROSPECTS ROSTER"
	p_title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	p_title.add_theme_font_size_override("font_size", 14)
	pros_vb.add_child(p_title)

	_prospects_vbox = VBoxContainer.new()
	_prospects_vbox.add_theme_constant_override("separation", 6)
	pros_vb.add_child(_prospects_vbox)

func refresh_youth() -> void:
	_ensure_ui_built()
	if not is_instance_valid(career_state) or not career_state.is_career_active():
		return

	var info: Dictionary = career_state.get_youth_academy_info()
	var prospects: Array = career_state.get_youth_prospects()
	var bal: int = career_state.get_total_balance()

	var fac_lvl: int = int(info.get("facility_level", 1))
	var acad_lvl: int = int(info.get("academy_level", 1))
	var fac_cost: int = int(info.get("facility_upgrade_cost", 0))
	var acad_cost: int = int(info.get("academy_upgrade_cost", 0))

	_fac_lvl_lbl.text = "Training Facilities: Level %d / 5" % fac_lvl
	if fac_lvl >= 5:
		_upgrade_fac_btn.text = "MAX LEVEL REACHED"
		_upgrade_fac_btn.disabled = true
	else:
		_upgrade_fac_btn.text = "Upgrade Facilities ($%.1fM)" % (float(fac_cost) / 1_000_000.0)
		_upgrade_fac_btn.disabled = (bal < fac_cost)

	_acad_lvl_lbl.text = "Scouting Network: Level %d / 5" % acad_lvl
	if acad_lvl >= 5:
		_upgrade_acad_btn.text = "MAX LEVEL REACHED"
		_upgrade_acad_btn.disabled = true
	else:
		_upgrade_acad_btn.text = "Upgrade Scouting ($%.1fM)" % (float(acad_cost) / 1_000_000.0)
		_upgrade_acad_btn.disabled = (bal < acad_cost)

	_coach_skill_lbl.text = "Average Coaching Skill: %d / 100" % int(info.get("average_coach_skill", 50))
	_weekly_cost_lbl.text = "Weekly Coach Salaries: $%d / wk" % int(info.get("total_weekly_salaries", 0))
	_scout_btn.disabled = (prospects.size() >= int(info.get("max_prospects", 12)))

	for c in _coaches_vbox.get_children():
		c.queue_free()
	var coaches: Array = info.get("coaches", [])
	for c_dict in coaches:
		var row := HBoxContainer.new()
		var lbl := Label.new()
		lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		lbl.text = "• %s (%s) — Skill: %d | $%d/wk" % [
			String(c_dict.get("name", "Coach")),
			String(c_dict.get("specialty", "General")),
			int(c_dict.get("coaching_skill", 70)),
			int(c_dict.get("weekly_salary", 3000))
		]
		lbl.add_theme_color_override("font_color", Color(0.85, 0.92, 1.0))
		lbl.add_theme_font_size_override("font_size", 12)
		row.add_child(lbl)

		var idx: int = int(c_dict.get("index", -1))
		var fire_btn := Button.new()
		fire_btn.text = "Release"
		fire_btn.focus_mode = Control.FOCUS_NONE
		_style_btn(fire_btn, Color(0.35, 0.10, 0.14, 0.90), Color(0.80, 0.25, 0.30, 0.85))
		fire_btn.pressed.connect(func() -> void:
			if career_state.fire_youth_coach(idx):
				refresh_youth()
		)
		row.add_child(fire_btn)
		_coaches_vbox.add_child(row)

	for c in _prospects_vbox.get_children():
		c.queue_free()

	if prospects.is_empty():
		var empty_lbl := Label.new()
		empty_lbl.text = "No prospects currently in the Youth Academy. Use 'Scout New Prospect' to recruit."
		empty_lbl.add_theme_color_override("font_color", Color(0.60, 0.70, 0.82))
		_prospects_vbox.add_child(empty_lbl)
		return

	for p in prospects:
		var pid: int = int(p.get("id", -1))
		var row_pc := PanelContainer.new()
		var r_style := StyleBoxFlat.new()
		r_style.bg_color = Color(0.06, 0.09, 0.15, 0.92)
		r_style.border_color = Color(0.14, 0.22, 0.34, 0.75)
		r_style.set_border_width_all(1)
		r_style.set_corner_radius_all(6)
		r_style.content_margin_left = 12
		r_style.content_margin_right = 12
		r_style.content_margin_top = 6
		r_style.content_margin_bottom = 6
		row_pc.add_theme_stylebox_override("panel", r_style)

		var hb := HBoxContainer.new()
		hb.add_theme_constant_override("separation", 10)
		row_pc.add_child(hb)

		var info_lbl := Label.new()
		info_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		info_lbl.text = "[%s]  %s  ·  Age %d  ·  OVR %d  ·  POT %d  ·  Cond %d%%" % [
			String(p.get("position", "CM")),
			String(p.get("name", "Prospect")),
			int(p.get("age", 15)),
			int(p.get("ovr", 62)),
			int(p.get("potential", 85)),
			int(p.get("condition", 100))
		]
		info_lbl.add_theme_color_override("font_color", Color(0.90, 0.96, 1.0))
		info_lbl.add_theme_font_size_override("font_size", 12)
		hb.add_child(info_lbl)

		var inspect_btn := Button.new()
		inspect_btn.text = "Inspect"
		inspect_btn.focus_mode = Control.FOCUS_NONE
		_style_btn(inspect_btn, Color(0.09, 0.15, 0.25, 0.95), Color(0.22, 0.35, 0.55, 0.85))
		inspect_btn.pressed.connect(func() -> void:
			player_profile_requested.emit(pid, "YOUTH")
		)
		hb.add_child(inspect_btn)

		var prom_btn := Button.new()
		prom_btn.text = "⬆ Promote"
		prom_btn.focus_mode = Control.FOCUS_NONE
		_style_btn(prom_btn, Color(0.06, 0.48, 0.34, 0.95), Color(0.11, 0.85, 0.63, 0.95))
		prom_btn.pressed.connect(func() -> void:
			if career_state.promote_youth_player(pid):
				refresh_youth()
		)
		hb.add_child(prom_btn)

		_prospects_vbox.add_child(row_pc)

func _create_card() -> PanelContainer:
	var pc := PanelContainer.new()
	pc.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	var s := StyleBoxFlat.new()
	s.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	s.border_color = Color(0.12, 0.20, 0.32, 0.85)
	s.set_border_width_all(1)
	s.set_corner_radius_all(10)
	s.set_content_margin_all(14)
	pc.add_theme_stylebox_override("panel", s)
	return pc

func _style_btn(btn: Button, bg: Color, border: Color) -> void:
	var n := StyleBoxFlat.new()
	n.bg_color = bg
	n.border_color = border
	n.set_border_width_all(1)
	n.set_corner_radius_all(6)
	n.content_margin_left = 10
	n.content_margin_right = 10
	n.content_margin_top = 4
	n.content_margin_bottom = 4
	var h := n.duplicate()
	h.bg_color = bg.lightened(0.10)
	var d := n.duplicate()
	d.bg_color = Color(0.05, 0.07, 0.11, 0.60)
	btn.add_theme_stylebox_override("normal", n)
	btn.add_theme_stylebox_override("hover", h)
	btn.add_theme_stylebox_override("pressed", h)
	btn.add_theme_stylebox_override("disabled", d)
	btn.add_theme_color_override("font_color", Color.WHITE)