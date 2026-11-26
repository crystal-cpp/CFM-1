class_name PlayerProfileModal
extends PanelContainer

signal closed
signal action_triggered(action_name: String, player_id: int)

var career_state: CareerState
var current_player_id: int = -1
var current_context_mode: String = "SQUAD" # "SQUAD", "YOUTH", "TRANSFER", "MATCH"

# UI Nodes
var _name_lbl: Label
var _meta_lbl: Label
var _ovr_lbl: Label
var _pot_lbl: Label
var _pos_pill: Label
var _club_lbl: Label

# Attributes Grid
var _attr_labels: Dictionary = {}

# Status Bars / Info
var _cond_bar: ProgressBar
var _cond_val_lbl: Label
var _morale_bar: ProgressBar
var _morale_val_lbl: Label
var _wage_key_lbl: Label
var _wage_val_lbl: Label
var _contract_key_lbl: Label
var _contract_val_lbl: Label
var _value_val_lbl: Label
var _stats_val_lbl: Label
var _injury_key_lbl: Label
var _injury_val_lbl: Label

# Context Actions Container
var _actions_hbox: HBoxContainer
var _close_btn: Button

func _ready() -> void:
	_ensure_ui_built()
	visible = false

func _ensure_ui_built() -> void:
	if _name_lbl != null:
		return

	mouse_filter = Control.MOUSE_FILTER_STOP
	anchor_left = 0.5
	anchor_top = 0.5
	anchor_right = 0.5
	anchor_bottom = 0.5
	offset_left = -290.0
	offset_top = -250.0
	offset_right = 290.0
	offset_bottom = 250.0
	grow_horizontal = Control.GROW_DIRECTION_BOTH
	grow_vertical = Control.GROW_DIRECTION_BOTH
	custom_minimum_size = Vector2(560, 480)

	var modal_style := StyleBoxFlat.new()
	modal_style.bg_color = Color(0.035, 0.052, 0.088, 0.99)
	modal_style.border_color = Color(0.12, 0.88, 0.52, 0.95)
	modal_style.set_border_width_all(2)
	modal_style.set_corner_radius_all(12)
	modal_style.set_content_margin_all(20)
	add_theme_stylebox_override("panel", modal_style)

	var root_vbox := VBoxContainer.new()
	root_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	root_vbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root_vbox.add_theme_constant_override("separation", 14)
	add_child(root_vbox)

	# 1. HEADER ROW
	var header_hbox := HBoxContainer.new()
	header_hbox.add_theme_constant_override("separation", 14)
	root_vbox.add_child(header_hbox)

	var portrait_box := PanelContainer.new()
	portrait_box.custom_minimum_size = Vector2(60, 60)
	var pb_style := StyleBoxFlat.new()
	pb_style.bg_color = Color(0.08, 0.12, 0.20, 0.95)
	pb_style.border_color = Color(0.20, 0.32, 0.50, 0.85)
	pb_style.set_border_width_all(1)
	pb_style.set_corner_radius_all(8)
	portrait_box.add_theme_stylebox_override("panel", pb_style)

	var port_icon := Label.new()
	port_icon.text = "👤"
	port_icon.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	port_icon.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	port_icon.add_theme_font_size_override("font_size", 28)
	portrait_box.add_child(port_icon)
	header_hbox.add_child(portrait_box)

	var name_vbox := VBoxContainer.new()
	name_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	name_vbox.add_theme_constant_override("separation", 4)
	header_hbox.add_child(name_vbox)

	_name_lbl = Label.new()
	_name_lbl.text = "Player Name"
	_name_lbl.add_theme_color_override("font_color", Color.WHITE)
	_name_lbl.add_theme_font_size_override("font_size", 18)
	name_vbox.add_child(_name_lbl)

	var sub_hbox := HBoxContainer.new()
	sub_hbox.add_theme_constant_override("separation", 8)
	name_vbox.add_child(sub_hbox)

	_pos_pill = Label.new()
	_pos_pill.text = "ST"
	var pos_style := StyleBoxFlat.new()
	pos_style.bg_color = Color(0.08, 0.45, 0.28, 0.90)
	pos_style.border_color = Color(0.20, 0.90, 0.55, 0.85)
	pos_style.set_border_width_all(1)
	pos_style.set_corner_radius_all(4)
	pos_style.content_margin_left = 7
	pos_style.content_margin_right = 7
	pos_style.content_margin_top = 2
	pos_style.content_margin_bottom = 2
	_pos_pill.add_theme_stylebox_override("normal", pos_style)
	_pos_pill.add_theme_color_override("font_color", Color(0.35, 1.0, 0.70))
	_pos_pill.add_theme_font_size_override("font_size", 11)
	sub_hbox.add_child(_pos_pill)

	_meta_lbl = Label.new()
	_meta_lbl.text = "Age: 24"
	_meta_lbl.add_theme_color_override("font_color", Color(0.70, 0.80, 0.92))
	_meta_lbl.add_theme_font_size_override("font_size", 12)
	sub_hbox.add_child(_meta_lbl)

	_club_lbl = Label.new()
	_club_lbl.text = "Club"
	_club_lbl.add_theme_color_override("font_color", Color(0.55, 0.70, 0.85))
	_club_lbl.add_theme_font_size_override("font_size", 12)
	sub_hbox.add_child(_club_lbl)

	# OVR & Potential Badges
	var ovr_vbox := VBoxContainer.new()
	ovr_vbox.alignment = BoxContainer.ALIGNMENT_CENTER
	header_hbox.add_child(ovr_vbox)

	var ovr_panel := PanelContainer.new()
	var op_style := StyleBoxFlat.new()
	op_style.bg_color = Color(0.12, 0.85, 0.48, 0.25)
	op_style.border_color = Color(0.15, 0.95, 0.58, 0.95)
	op_style.set_border_width_all(2)
	op_style.set_corner_radius_all(8)
	op_style.set_content_margin_all(6)
	ovr_panel.add_theme_stylebox_override("panel", op_style)

	_ovr_lbl = Label.new()
	_ovr_lbl.text = "88"
	_ovr_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_ovr_lbl.add_theme_color_override("font_color", Color(0.25, 1.0, 0.65))
	_ovr_lbl.add_theme_font_size_override("font_size", 20)
	ovr_panel.add_child(_ovr_lbl)
	ovr_vbox.add_child(ovr_panel)

	_pot_lbl = Label.new()
	_pot_lbl.text = "POT 92"
	_pot_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_pot_lbl.add_theme_color_override("font_color", Color(0.95, 0.80, 0.25))
	_pot_lbl.add_theme_font_size_override("font_size", 11)
	ovr_vbox.add_child(_pot_lbl)

	_close_btn = Button.new()
	_close_btn.text = "✕"
	_close_btn.custom_minimum_size = Vector2(32, 32)
	_close_btn.focus_mode = Control.FOCUS_NONE
	_apply_modal_button_style(_close_btn, Color(0.10, 0.14, 0.22, 0.9), Color(0.24, 0.34, 0.50, 0.85))
	_close_btn.pressed.connect(close_modal)
	header_hbox.add_child(_close_btn)

	root_vbox.add_child(HSeparator.new())

	# 2. BODY SPLIT: Attributes on Left, Fitness/Contract on Right
	var body_hbox := HBoxContainer.new()
	body_hbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body_hbox.add_theme_constant_override("separation", 16)
	root_vbox.add_child(body_hbox)

	# Left: 6 Core Football Attributes
	var attr_card := _create_section_card("CORE ATTRIBUTES", 250)
	body_hbox.add_child(attr_card)
	var attr_vbox: VBoxContainer = attr_card.get_node("VBox")

	var attr_keys: Array[String] = ["pace", "shooting", "passing", "dribbling", "defending", "physical"]
	var attr_names: Array[String] = ["Pace (PAC)", "Shooting (SHO)", "Passing (PAS)", "Dribbling (DRI)", "Defending (DEF)", "Physical (PHY)"]

	for i in range(attr_keys.size()):
		var row := HBoxContainer.new()
		var n_lbl := Label.new()
		n_lbl.text = attr_names[i]
		n_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		n_lbl.add_theme_color_override("font_color", Color(0.80, 0.88, 0.96))
		n_lbl.add_theme_font_size_override("font_size", 12)
		row.add_child(n_lbl)

		var v_lbl := Label.new()
		v_lbl.text = "75"
		v_lbl.add_theme_color_override("font_color", Color(0.20, 0.90, 0.55))
		v_lbl.add_theme_font_size_override("font_size", 13)
		row.add_child(v_lbl)
		attr_vbox.add_child(row)

		_attr_labels[attr_keys[i]] = v_lbl

	# Right: Condition, Morale, Contract, Valuation, Season Telemetry
	var status_card := _create_section_card("CONDITION & STATUS", 250)
	body_hbox.add_child(status_card)
	var status_vbox: VBoxContainer = status_card.get_node("VBox")

	# Condition Bar
	var cond_header := HBoxContainer.new()
	var ch_lbl := Label.new()
	ch_lbl.text = "Condition / Stamina"
	ch_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	ch_lbl.add_theme_font_size_override("font_size", 12)
	cond_header.add_child(ch_lbl)
	_cond_val_lbl = Label.new()
	_cond_val_lbl.text = "100%"
	_cond_val_lbl.add_theme_color_override("font_color", Color(0.25, 0.95, 0.60))
	_cond_val_lbl.add_theme_font_size_override("font_size", 12)
	cond_header.add_child(_cond_val_lbl)
	status_vbox.add_child(cond_header)

	_cond_bar = _create_styled_progress_bar(Color(0.15, 0.88, 0.52))
	status_vbox.add_child(_cond_bar)

	# Morale Bar
	var mor_header := HBoxContainer.new()
	var mh_lbl := Label.new()
	mh_lbl.text = "Morale"
	mh_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	mh_lbl.add_theme_font_size_override("font_size", 12)
	mor_header.add_child(mh_lbl)
	_morale_val_lbl = Label.new()
	_morale_val_lbl.text = "Happy (85%)"
	_morale_val_lbl.add_theme_color_override("font_color", Color(0.35, 0.85, 1.0))
	_morale_val_lbl.add_theme_font_size_override("font_size", 12)
	mor_header.add_child(_morale_val_lbl)
	status_vbox.add_child(mor_header)

	_morale_bar = _create_styled_progress_bar(Color(0.25, 0.72, 0.98))
	status_vbox.add_child(_morale_bar)

	status_vbox.add_child(HSeparator.new())

	var wage_row: Array[Label] = _add_stat_row_pair(status_vbox, "Weekly Wage:", "$12,500 / wk")
	_wage_key_lbl = wage_row[0]
	_wage_val_lbl = wage_row[1]

	var contract_row: Array[Label] = _add_stat_row_pair(status_vbox, "Contract:", "3 Years")
	_contract_key_lbl = contract_row[0]
	_contract_val_lbl = contract_row[1]

	_value_val_lbl = _add_stat_row_pair(status_vbox, "Market Value:", "$24.5M")[1]
	_stats_val_lbl = _add_stat_row_pair(status_vbox, "Season Stats:", "0 Apps · 0 Goals · 0 Ast")[1]
	var inj_row: Array[Label] = _add_stat_row_pair(status_vbox, "Medical Status:", "Fit & Available")
	_injury_key_lbl = inj_row[0]
	_injury_val_lbl = inj_row[1]

	root_vbox.add_child(HSeparator.new())

	# 3. CONTEXT ACTION BUTTONS
	_actions_hbox = HBoxContainer.new()
	_actions_hbox.alignment = BoxContainer.ALIGNMENT_END
	_actions_hbox.add_theme_constant_override("separation", 10)
	root_vbox.add_child(_actions_hbox)

func _create_styled_progress_bar(fill_color: Color) -> ProgressBar:
	var bar := ProgressBar.new()
	bar.custom_minimum_size = Vector2(0, 10)
	bar.show_percentage = false
	bar.max_value = 100
	bar.value = 100

	var bg_sb := StyleBoxFlat.new()
	bg_sb.bg_color = Color(0.08, 0.12, 0.18, 0.95)
	bg_sb.set_corner_radius_all(4)

	var fill_sb := StyleBoxFlat.new()
	fill_sb.bg_color = fill_color
	fill_sb.set_corner_radius_all(4)

	bar.add_theme_stylebox_override("background", bg_sb)
	bar.add_theme_stylebox_override("fill", fill_sb)
	return bar

func _apply_modal_button_style(btn: Button, bg: Color, border: Color) -> void:
	var normal := StyleBoxFlat.new()
	normal.bg_color = bg
	normal.border_color = border
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(6)
	normal.content_margin_left = 12
	normal.content_margin_right = 12
	normal.content_margin_top = 6
	normal.content_margin_bottom = 6

	var hover := normal.duplicate()
	hover.bg_color = bg.lightened(0.10)
	hover.border_color = border.lightened(0.15)

	var pressed := normal.duplicate()
	pressed.bg_color = bg.darkened(0.10)

	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("pressed", pressed)
	btn.add_theme_color_override("font_color", Color.WHITE)

func _create_section_card(title: String, min_w: float) -> PanelContainer:
	var pc := PanelContainer.new()
	pc.custom_minimum_size = Vector2(min_w, 0)
	pc.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	pc.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.045, 0.065, 0.10, 0.88)
	style.border_color = Color(0.12, 0.18, 0.28, 0.85)
	style.set_border_width_all(1)
	style.set_corner_radius_all(8)
	style.set_content_margin_all(12)
	pc.add_theme_stylebox_override("panel", style)

	var vb := VBoxContainer.new()
	vb.name = "VBox"
	vb.add_theme_constant_override("separation", 8)
	pc.add_child(vb)

	var t := Label.new()
	t.text = title
	t.add_theme_color_override("font_color", Color(0.15, 0.88, 0.55))
	t.add_theme_font_size_override("font_size", 12)
	vb.add_child(t)
	vb.add_child(HSeparator.new())

	return pc

func _add_stat_row_pair(parent: VBoxContainer, key: String, val: String) -> Array[Label]:
	var hbox := HBoxContainer.new()
	var k_lbl := Label.new()
	k_lbl.text = key
	k_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	k_lbl.add_theme_color_override("font_color", Color(0.65, 0.75, 0.88))
	k_lbl.add_theme_font_size_override("font_size", 12)
	hbox.add_child(k_lbl)

	var v_lbl := Label.new()
	v_lbl.text = val
	v_lbl.add_theme_color_override("font_color", Color.WHITE)
	v_lbl.add_theme_font_size_override("font_size", 12)
	hbox.add_child(v_lbl)
	parent.add_child(hbox)
	return [k_lbl, v_lbl]

# -----------------------------------------------------------------------------
# Population & Context Modes
# -----------------------------------------------------------------------------
func open_player(player_id: int, context_mode: String = "SQUAD") -> void:
	_ensure_ui_built()
	current_player_id = player_id
	current_context_mode = context_mode.to_upper()

	if not is_instance_valid(career_state):
		push_error("PlayerProfileModal: CareerState reference not provided!")
		return

	var profile: Dictionary = career_state.get_player_profile(player_id)
	if profile.is_empty():
		push_error("PlayerProfileModal: Player ID %d not found in CareerState!" % player_id)
		return

	_name_lbl.text = String(profile.get("name", "Unknown Player"))
	_pos_pill.text = String(profile.get("position", "CM"))
	var age: int = int(profile.get("age", 24))
	var debut_age: int = int(profile.get("debut_age", 16))
	_meta_lbl.text = "Age: %d · Debut: %d" % [age, debut_age]
	_club_lbl.text = String(profile.get("club_name", "Free Agent"))

	var ovr: int = int(profile.get("ovr", 60))
	var pot: int = int(profile.get("potential", 75))
	_ovr_lbl.text = str(ovr)
	_pot_lbl.text = "POT %d" % pot

	for key in _attr_labels.keys():
		var label: Label = _attr_labels[key]
		label.text = str(int(profile.get(key, 60)))

	var cond: int = int(profile.get("condition", 100))
	_cond_bar.value = cond
	var days_to_full: int = int(ceili(maxf(0.0, 100.0 - float(cond)) / 30.0))
	_cond_val_lbl.text = ("%d%% (~%dd to 100%%)" % [cond, days_to_full]) if days_to_full > 0 else "100%"

	var mor: int = int(profile.get("morale", 80))
	_morale_bar.value = mor
	_morale_val_lbl.text = "%d%% (%s)" % [mor, "Happy" if mor >= 75 else "Content"]

	if current_context_mode == "YOUTH" or int(profile.get("club_id", 0)) == -1:
		_wage_key_lbl.text = "Academy Status:"
		_wage_val_lbl.text = "Youth Prospect"
		_contract_key_lbl.text = "Promotion:"
		_contract_val_lbl.text = "Eligible for Senior Squad" if age >= 15 else "In Training"
	else:
		_wage_key_lbl.text = "Weekly Wage:"
		var wage: int = int(profile.get("wage", 5000))
		_wage_val_lbl.text = "$%s / wk" % _format_number(wage)
		_contract_key_lbl.text = "Contract:"
		var c_years: int = int(profile.get("contract_years", 3))
		_contract_val_lbl.text = "%d Years" % c_years

	var val: float = float(profile.get("market_value", 0.0))
	_value_val_lbl.text = "$%.1fM" % (val / 1_000_000.0) if val >= 1_000_000 else "$%s" % _format_number(int(val))

	var apps: int = int(profile.get("appearances", 0))
	var goals: int = int(profile.get("goals", 0))
	var assists: int = int(profile.get("assists", 0))
	_stats_val_lbl.text = "%d Apps · %d Goals · %d Ast" % [apps, goals, assists]

	var is_injured: bool = bool(profile.get("is_injured", false)) or int(profile.get("injury_days_remaining", 0)) > 0
	var inj_days: int = int(profile.get("injury_days_remaining", 0))
	if is_injured:
		var tier: String = String(profile.get("injury_label", "")).strip_edges()
		if tier.is_empty():
			tier = "Minor" if inj_days <= 5 else ("Moderate" if inj_days <= 14 else ("Major" if inj_days <= 60 else "Severe"))
		var val_note: String = " (-5% to -10% value)" if tier.to_lower().begins_with("severe") else ""
		_injury_val_lbl.text = "%s · %d day(s) left%s" % [tier, inj_days, val_note]
		_injury_val_lbl.add_theme_color_override("font_color", Color(1.0, 0.35, 0.38))
	else:
		_injury_val_lbl.text = "Fit & Available"
		_injury_val_lbl.add_theme_color_override("font_color", Color(0.25, 0.95, 0.60))

	_build_context_actions(profile)
	visible = true

func _build_context_actions(profile: Dictionary) -> void:
	for child in _actions_hbox.get_children():
		child.queue_free()

	match current_context_mode:
		"YOUTH":
			var promote_btn := Button.new()
			promote_btn.text = "⭐ Promote to Senior Squad"
			promote_btn.focus_mode = Control.FOCUS_NONE
			_apply_modal_button_style(promote_btn, Color(0.10, 0.56, 0.34, 0.95), Color(0.20, 0.95, 0.58, 0.95))
			promote_btn.pressed.connect(func() -> void:
				action_triggered.emit("promote_youth", current_player_id)
				close_modal()
			)
			_actions_hbox.add_child(promote_btn)

		"SQUAD":
			var is_starter: bool = bool(profile.get("is_starting_xi", false))
			var role_btn := Button.new()
			role_btn.text = "Move to Bench" if is_starter else "Promote to Starting XI"
			role_btn.focus_mode = Control.FOCUS_NONE
			_apply_modal_button_style(role_btn, Color(0.08, 0.25, 0.42, 0.92), Color(0.22, 0.52, 0.85, 0.90))
			role_btn.pressed.connect(func() -> void:
				action_triggered.emit("toggle_starter", current_player_id)
				close_modal()
			)
			_actions_hbox.add_child(role_btn)

			var contract_btn := Button.new()
			contract_btn.text = "Negotiate Contract"
			contract_btn.focus_mode = Control.FOCUS_NONE
			_apply_modal_button_style(contract_btn, Color(0.10, 0.48, 0.30, 0.92), Color(0.20, 0.90, 0.55, 0.90))
			contract_btn.pressed.connect(func() -> void:
				action_triggered.emit("renew_contract", current_player_id)
			)
			_actions_hbox.add_child(contract_btn)

		"TRANSFER":
			var offer_btn := Button.new()
			offer_btn.text = "💼 Make Transfer Offer"
			offer_btn.focus_mode = Control.FOCUS_NONE
			_apply_modal_button_style(offer_btn, Color(0.10, 0.56, 0.34, 0.95), Color(0.20, 0.95, 0.58, 0.95))
			offer_btn.pressed.connect(func() -> void:
				action_triggered.emit("make_offer", current_player_id)
			)
			_actions_hbox.add_child(offer_btn)

		"MATCH":
			var inspect_btn := Button.new()
			inspect_btn.text = "View Tactical Role"
			inspect_btn.focus_mode = Control.FOCUS_NONE
			_apply_modal_button_style(inspect_btn, Color(0.08, 0.25, 0.42, 0.92), Color(0.22, 0.52, 0.85, 0.90))
			inspect_btn.pressed.connect(func() -> void:
				action_triggered.emit("inspect_tactics", current_player_id)
			)
			_actions_hbox.add_child(inspect_btn)

	var close_act_btn := Button.new()
	close_act_btn.text = "Close"
	close_act_btn.focus_mode = Control.FOCUS_NONE
	_apply_modal_button_style(close_act_btn, Color(0.09, 0.13, 0.20, 0.92), Color(0.20, 0.28, 0.42, 0.85))
	close_act_btn.pressed.connect(close_modal)
	_actions_hbox.add_child(close_act_btn)

func close_modal() -> void:
	visible = false
	closed.emit()

func _format_number(n: int) -> String:
	var s := str(n)
	var res := ""
	var count := 0
	for i in range(s.length() - 1, -1, -1):
		res = s[i] + res
		count += 1
		if count % 3 == 0 and i > 0:
			res = "," + res
	return res
