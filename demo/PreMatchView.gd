class_name PreMatchView
extends Control

signal start_match_requested(match_context: Dictionary)
signal back_requested

var career_state: Node = null
var match_context: Dictionary = {}
var use_alternate_away_kit: bool = false

var _importance_badge: Label
var _importance_desc: Label
var _home_title: Label
var _away_title: Label
var _home_kit_swatch: ColorRect
var _away_kit_swatch: ColorRect
var _kit_status_lbl: Label
var _swap_kit_btn: Button
var _formation_opt: OptionButton
var _mentality_opt: OptionButton
var _pressing_opt: OptionButton
var _lineup_list: VBoxContainer
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

	# Body Split: Left = Tactical Setup, Right = Starting XI Fitness & Injury Check
	var body_hbox := HBoxContainer.new()
	body_hbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body_hbox.add_theme_constant_override("separation", 18)
	root_vbox.add_child(body_hbox)

	var tac_panel := PanelContainer.new()
	tac_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body_hbox.add_child(tac_panel)

	var tac_vbox := VBoxContainer.new()
	tac_vbox.add_theme_constant_override("separation", 12)
	tac_panel.add_child(tac_vbox)

	var tac_hdr := Label.new()
	tac_hdr.text = "TACTICAL INSTRUCTIONS"
	tac_hdr.add_theme_font_size_override("font_size", 16)
	tac_vbox.add_child(tac_hdr)

	_formation_opt = OptionButton.new()
	for fmt: String in ["4-3-3", "4-4-2", "4-2-3-1", "3-5-2", "5-3-2"]:
		_formation_opt.add_item(fmt)
	_formation_opt.item_selected.connect(_on_tactics_changed)
	tac_vbox.add_child(_labeled_row("Formation:", _formation_opt))

	_mentality_opt = OptionButton.new()
	for m: String in ["Very Defensive", "Defensive", "Balanced", "Attacking", "Very Attacking"]:
		_mentality_opt.add_item(m)
	_mentality_opt.selected = 2
	_mentality_opt.item_selected.connect(_on_tactics_changed)
	tac_vbox.add_child(_labeled_row("Mentality:", _mentality_opt))

	_pressing_opt = OptionButton.new()
	for pr: String in ["Low Block", "Normal Press", "High Gegenpress"]:
		_pressing_opt.add_item(pr)
	_pressing_opt.selected = 1
	_pressing_opt.item_selected.connect(_on_tactics_changed)
	tac_vbox.add_child(_labeled_row("Pressing:", _pressing_opt))

	var squad_panel := PanelContainer.new()
	squad_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body_hbox.add_child(squad_panel)

	var squad_vbox := VBoxContainer.new()
	squad_vbox.add_theme_constant_override("separation", 6)
	squad_panel.add_child(squad_vbox)

	var xi_hdr := Label.new()
	xi_hdr.text = "STARTING XI & STAMINA READINESS"
	xi_hdr.add_theme_font_size_override("font_size", 16)
	squad_vbox.add_child(xi_hdr)

	_lineup_list = VBoxContainer.new()
	_lineup_list.add_theme_constant_override("separation", 4)
	squad_vbox.add_child(_lineup_list)

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


func setup(p_career_state: Node, fixture: Dictionary, db: PlayerDatabase = null) -> void:
	career_state = p_career_state
	if is_instance_valid(career_state) and career_state.has_method("prepare_match_context"):
		match_context = career_state.call("prepare_match_context", fixture, db)
	else:
		match_context = _build_fallback_match_context(fixture)

	use_alternate_away_kit = bool(match_context.get("kits", {}).get("clash_detected", false))
	_refresh_view()

func _build_fallback_match_context(fixture: Dictionary) -> Dictionary:
	var home_id := int(fixture.get("home_club_id", 1))
	var away_id := int(fixture.get("away_club_id", 2))
	var managed_id := int(career_state.get_managed_club_id()) if is_instance_valid(career_state) and career_state.has_method("get_managed_club_id") else home_id

	var home_info: Dictionary = career_state.get_club_info(home_id) if is_instance_valid(career_state) and career_state.has_method("get_club_info") else {}
	var away_info: Dictionary = career_state.get_club_info(away_id) if is_instance_valid(career_state) and career_state.has_method("get_club_info") else {}

	var home_name := String(fixture.get("home_club", home_info.get("club_name", "Home")))
	var away_name := String(fixture.get("away_club", away_info.get("club_name", "Away")))
	var home_color: Color = home_info.get("primary_color", Color(0.06, 0.78, 0.48))
	var away_color: Color = away_info.get("primary_color", Color(0.92, 0.26, 0.30))

	return {
		"fixture": fixture,
		"home_club_id": home_id,
		"away_club_id": away_id,
		"home_club": home_name,
		"away_club": away_name,
		"user_is_home": home_id == managed_id,
		"stadium": String(fixture.get("stadium", "%s Stadium" % home_name)),
		"competition": String(fixture.get("competition", "League Match")),
		"importance": {
			"label": String(fixture.get("match_importance", "LEAGUE MATCH")),
			"text_color": Color(0.11, 0.82, 0.63),
			"description": "%s vs %s" % [home_name, away_name],
		},
		"kits": {
			"home_primary": home_color,
			"away_primary": away_color,
			"away_default_primary": away_color,
			"clash_detected": false,
		},
	}


func _refresh_view() -> void:
	if match_context.is_empty():
		return
	var imp: Dictionary = match_context.get("importance", {})
	_importance_badge.text = "[ %s ]" % String(imp.get("label", "LEAGUE MATCH"))
	_importance_badge.add_theme_color_override("font_color", imp.get("text_color", Color.WHITE))
	_importance_desc.text = String(imp.get("description", ""))

	_home_title.text = "%s (%d OVR)" % [match_context.get("home_club", "Home"), int(match_context.get("home_ovr", 75))]
	_away_title.text = "%s (%d OVR)" % [match_context.get("away_club", "Away"), int(match_context.get("away_ovr", 75))]

	var kits: Dictionary = match_context.get("kits", {})
	_home_kit_swatch.color = kits.get("home_primary", Color.RED)
	_away_kit_swatch.color = kits.get("away_primary", Color.BLUE) if use_alternate_away_kit else kits.get("away_default_primary", Color.BLUE)
	_kit_status_lbl.text = "Kit Clash Auto-Resolved (Alternate Away Kit Active)" if bool(kits.get("clash_detected", false)) else "No Kit Clash Detected"

	for child: Node in _lineup_list.get_children():
		child.queue_free()

	var user_is_home: bool = bool(match_context.get("user_is_home", true))
	var xi: Array = match_context.get("home_lineup", []) if user_is_home else match_context.get("away_lineup", [])
	var tired_count: int = 0

	for p: Dictionary in xi:
		var stam: float = float(p.get("stamina", 100.0))
		if stam < 65.0:
			tired_count += 1
		var row := HBoxContainer.new()
		var name_lbl := Label.new()
		name_lbl.text = "[%s] %s (OVR %d)" % [p.get("position", "CM"), p.get("name", "Player"), int(p.get("ovr", 75))]
		name_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		row.add_child(name_lbl)

		var stam_lbl := Label.new()
		stam_lbl.text = "Stamina: %d%%" % int(round(stam))
		stam_lbl.add_theme_color_override("font_color", Color(0.35, 0.95, 0.55) if stam >= 75.0 else (Color(1.0, 0.80, 0.30) if stam >= 55.0 else Color(1.0, 0.35, 0.35)))
		row.add_child(stam_lbl)
		_lineup_list.add_child(row)

	_warning_lbl.text = "Warning: %d starter(s) below 65%% stamina — higher injury risk." % tired_count if tired_count > 0 else "Squad condition optimal."


func _on_toggle_away_kit() -> void:
	use_alternate_away_kit = not use_alternate_away_kit
	var kits: Dictionary = match_context.get("kits", {})
	var swapped: Color = kits.get("away_primary", Color.BLUE) if use_alternate_away_kit else kits.get("away_default_primary", Color.BLUE)
	_away_kit_swatch.color = swapped
	kits["active_away_primary"] = swapped


func _on_tactics_changed(_idx: int) -> void:
	if career_state == null or match_context.is_empty():
		return
	var fmt: String = _formation_opt.get_item_text(_formation_opt.selected)
	var club_id: int = int(career_state.get_managed_club_id()) if career_state.has_method("get_managed_club_id") else 1

	var tactics: Dictionary = {}
	if career_state.has_method("get_club_tactics"):
		tactics = career_state.get_club_tactics(club_id).duplicate(true)

	tactics["formation"] = fmt
	tactics["mentality"] = _mentality_opt.selected
	tactics["pressing"] = _pressing_opt.selected

	if career_state.has_method("set_club_tactics"):
		career_state.set_club_tactics(club_id, tactics)

	match_context["tactics"] = tactics.duplicate(true)
	if bool(match_context.get("user_is_home", true)):
		match_context["home_formation"] = fmt
	else:
		match_context["away_formation"] = fmt
	if bool(match_context.get("user_is_home", true)):
		match_context["home_formation"] = fmt
	else:
		match_context["away_formation"] = fmt


func _on_kickoff_pressed() -> void:
	_on_tactics_changed(0)
	start_match_requested.emit(match_context)
