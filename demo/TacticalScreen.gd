class_name TacticalScreen
extends PanelContainer

signal resume_requested

var match_engine: MatchEngine

var _formation_option: OptionButton
var _mentality_option: OptionButton
var _pressing_option: OptionButton
var _def_line_option: OptionButton
var _passing_option: OptionButton
var _width_option: OptionButton
var _tempo_option: OptionButton
var _def_approach_option: OptionButton

var _pitch_frame: PanelContainer
var _pitch_preview: Control
var _xi_ovr_label: Label
var _subs_count_label: Label
var _subs_list_vbox: VBoxContainer
var _status_label: Label

var _initial_snapshot: Dictionary = {}
var _ui_built: bool = false

# Interactive Pitch, OVR & Substitution State
var _starter_card_rects: Array[Rect2] = []
var _starter_ovrs: Array[int] = []
var _selected_starter_idx: int = -1
var _selected_bench_idx: int = -1
var _subs_used: int = 0
const MAX_SUBS: int = 5

var _bench_squad: Array[Dictionary] = []
var _subbed_off_names: PackedStringArray = PackedStringArray()

func _ready() -> void:
	_ensure_ui_built()
	if not is_instance_valid(match_engine):
		match_engine = _resolve_match_engine()

func _ensure_ui_built() -> void:
	if _ui_built:
		return
	_ui_built = true

	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	visible = false

	var bg := StyleBoxFlat.new()
	bg.bg_color = Color(0.035, 0.052, 0.088, 0.98)
	bg.set_content_margin_all(18)
	add_theme_stylebox_override("panel", bg)

	_build_layout()

func _resolve_match_engine() -> MatchEngine:
	var candidate: Node = get_node_or_null("../../../MatchEngine")
	if candidate is MatchEngine:
		return candidate as MatchEngine
	if get_tree() and get_tree().root:
		var found: Node = get_tree().root.find_child("MatchEngine", true, false)
		if found is MatchEngine:
			return found as MatchEngine
	return null

func bind_match_engine(p_engine: MatchEngine) -> void:
	match_engine = p_engine
	_ensure_ui_built()

## Public API for Phase 3 Main Game UI & Team Management UI to inject managed squad OVRs
func set_managed_lineup_ovrs(ovrs: Array) -> void:
	_starter_ovrs.clear()
	for i in range(11):
		if i < ovrs.size():
			_starter_ovrs.append(clampi(int(ovrs[i]), 40, 99))
		else:
			var fallback: int = match_engine.get_home_team_ovr() if is_instance_valid(match_engine) else 75
			_starter_ovrs.append(fallback)
	_refresh_all_views()

func reset_match_substitutions() -> void:
	_subs_used = 0
	_subbed_off_names.clear()
	_selected_starter_idx = -1
	_selected_bench_idx = -1
	_starter_ovrs.clear()
	_initialize_default_bench()

func _ensure_starter_ovrs() -> void:
	if _starter_ovrs.size() == 11:
		return

	# Check if Phase 3 CareerState autoload exists with managed squad OVRs
	var career_state: Node = get_node_or_null("/root/CareerState")
	if career_state != null and career_state.has_method("get_starting_xi_ovrs"):
		var career_ovrs: Variant = career_state.get_starting_xi_ovrs()
		if career_ovrs is Array and (career_ovrs as Array).size() >= 11:
			set_managed_lineup_ovrs(career_ovrs as Array)
			return

	var base_ovr: int = match_engine.get_home_team_ovr() if is_instance_valid(match_engine) else 75
	var offsets: Array[int] = [0, -1, 0, 1, -1, 1, 0, 1, 0, 2, -1]
	_starter_ovrs.clear()
	for i in range(11):
		_starter_ovrs.append(clampi(base_ovr + offsets[i], 45, 99))

func _get_xi_average_ovr() -> int:
	if _starter_ovrs.is_empty():
		return match_engine.get_home_team_ovr() if is_instance_valid(match_engine) else 75
	var total: int = 0
	for val in _starter_ovrs:
		total += val
	return int(round(float(total) / float(_starter_ovrs.size())))

func _initialize_default_bench() -> void:
	var base_ovr: int = match_engine.get_home_team_ovr() if is_instance_valid(match_engine) else 75
	_bench_squad = [
		{"name": "Home #12", "position": "GK",  "ovr": clampi(base_ovr - 2, 50, 99), "stamina": 100.0, "used": false},
		{"name": "Home #13", "position": "CB",  "ovr": clampi(base_ovr - 1, 50, 99), "stamina": 100.0, "used": false},
		{"name": "Home #14", "position": "LB",  "ovr": clampi(base_ovr - 2, 50, 99), "stamina": 100.0, "used": false},
		{"name": "Home #15", "position": "CDM", "ovr": clampi(base_ovr,     50, 99), "stamina": 100.0, "used": false},
		{"name": "Home #16", "position": "CM",  "ovr": clampi(base_ovr + 1, 50, 99), "stamina": 100.0, "used": false},
		{"name": "Home #17", "position": "LW",  "ovr": clampi(base_ovr,     50, 99), "stamina": 100.0, "used": false},
		{"name": "Home #18", "position": "ST",  "ovr": clampi(base_ovr + 1, 50, 99), "stamina": 100.0, "used": false}
	]

func open_tactics() -> void:
	_ensure_ui_built()
	if not is_instance_valid(match_engine):
		match_engine = _resolve_match_engine()
	if not is_instance_valid(match_engine):
		push_error("TacticalScreen: Cannot open tactics — MatchEngine not found.")
		return

	_ensure_starter_ovrs()
	if _bench_squad.is_empty():
		_initialize_default_bench()

	_selected_starter_idx = -1
	_selected_bench_idx = -1

	_initial_snapshot = {
		"formation": match_engine.get_home_formation(),
		"mentality": match_engine.get_home_mentality(),
		"pressing": match_engine.get_home_pressing(),
		"def_line": match_engine.get_home_defensive_line(),
		"passing": match_engine.get_home_passing_style(),
		"width": match_engine.get_home_attacking_width(),
		"tempo": match_engine.get_home_tempo(),
		"def_approach": match_engine.get_home_defensive_approach()
	}

	_sync_controls_from_snapshot(_initial_snapshot)
	_refresh_all_views()
	if _status_label:
		_status_label.text = "Paused at %02d' · Temporary In-Match Instructions" % match_engine.get_current_minute()
	visible = true

func _build_layout() -> void:
	var root_vbox := VBoxContainer.new()
	root_vbox.add_theme_constant_override("separation", 12)
	add_child(root_vbox)

	# --- Top Header Bar with Top-Right 'X' Close Button ---
	var header := HBoxContainer.new()
	header.add_theme_constant_override("separation", 12)
	root_vbox.add_child(header)

	var badge := Label.new()
	badge.text = "TACTICS & LINEUP"
	badge.add_theme_color_override("font_color", Color(0.12, 0.90, 0.55))
	badge.add_theme_font_size_override("font_size", 18)
	header.add_child(badge)

	_status_label = Label.new()
	_status_label.text = "Paused · Temporary In-Match Instructions"
	_status_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_status_label.add_theme_color_override("font_color", Color(0.72, 0.80, 0.90))
	_status_label.add_theme_font_size_override("font_size", 13)
	header.add_child(_status_label)

	var close_x_btn := Button.new()
	close_x_btn.text = "✕"
	close_x_btn.tooltip_text = "Close Tactics & Resume Match"
	close_x_btn.custom_minimum_size = Vector2(44, 40)
	close_x_btn.focus_mode = Control.FOCUS_NONE
	var x_style := StyleBoxFlat.new()
	x_style.bg_color = Color(0.78, 0.20, 0.24, 0.92)
	x_style.set_corner_radius_all(8)
	close_x_btn.add_theme_stylebox_override("normal", x_style)
	var x_hover := x_style.duplicate()
	x_hover.bg_color = Color(0.92, 0.26, 0.30, 1.0)
	close_x_btn.add_theme_stylebox_override("hover", x_hover)
	close_x_btn.add_theme_color_override("font_color", Color.WHITE)
	close_x_btn.add_theme_font_size_override("font_size", 18)
	close_x_btn.pressed.connect(_on_close_x_pressed)
	header.add_child(close_x_btn)

	# --- Main 3-Column Content: Vertical Pitch | Substitutions | Instructions ---
	var body_hbox := HBoxContainer.new()
	body_hbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	body_hbox.add_theme_constant_override("separation", 14)
	root_vbox.add_child(body_hbox)

	# 1. LEFT CARD: Rounded Vertical Pitch Lineup with Player OVRs
	var pitch_card := PanelContainer.new()
	pitch_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	pitch_card.size_flags_stretch_ratio = 1.25
	pitch_card.add_theme_stylebox_override("panel", _make_card_style())
	body_hbox.add_child(pitch_card)

	var pitch_vbox := VBoxContainer.new()
	pitch_vbox.add_theme_constant_override("separation", 8)
	pitch_card.add_child(pitch_vbox)

	var pitch_hdr := HBoxContainer.new()
	pitch_vbox.add_child(pitch_hdr)

	var pitch_title := Label.new()
	pitch_title.text = "STARTING XI (VERTICAL SHAPE)"
	pitch_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	pitch_title.add_theme_color_override("font_color", Color(0.92, 0.96, 1.0))
	pitch_title.add_theme_font_size_override("font_size", 14)
	pitch_hdr.add_child(pitch_title)

	_xi_ovr_label = Label.new()
	_xi_ovr_label.text = "XI OVR 75"
	_xi_ovr_label.add_theme_color_override("font_color", Color(0.98, 0.85, 0.28))
	_xi_ovr_label.add_theme_font_size_override("font_size", 13)
	pitch_hdr.add_child(_xi_ovr_label)

	# Rounded-Corner Clip Container for the Vertical Pitch
	_pitch_frame = PanelContainer.new()
	_pitch_frame.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_pitch_frame.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_pitch_frame.clip_children = CanvasItem.CLIP_CHILDREN_AND_DRAW
	var pf_style := StyleBoxFlat.new()
	pf_style.bg_color = Color(0.08, 0.42, 0.24, 1.0)
	pf_style.border_color = Color(0.24, 0.34, 0.48, 0.92)
	pf_style.set_border_width_all(2)
	pf_style.set_corner_radius_all(12)
	_pitch_frame.add_theme_stylebox_override("panel", pf_style)
	pitch_vbox.add_child(_pitch_frame)

	_pitch_preview = Control.new()
	_pitch_preview.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_pitch_preview.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_pitch_preview.custom_minimum_size = Vector2(340, 320)
	_pitch_preview.mouse_filter = Control.MOUSE_FILTER_STOP
	_pitch_preview.draw.connect(_draw_vertical_pitch)
	_pitch_preview.gui_input.connect(_on_pitch_gui_input)
	_pitch_frame.add_child(_pitch_preview)

	# 2. MIDDLE CARD: Substitutions Bench Panel
	var subs_card := PanelContainer.new()
	subs_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	subs_card.size_flags_stretch_ratio = 0.72
	subs_card.add_theme_stylebox_override("panel", _make_card_style())
	body_hbox.add_child(subs_card)

	var subs_vbox := VBoxContainer.new()
	subs_vbox.add_theme_constant_override("separation", 8)
	subs_card.add_child(subs_vbox)

	var subs_hdr := HBoxContainer.new()
	subs_vbox.add_child(subs_hdr)

	var subs_title := Label.new()
	subs_title.text = "SUBSTITUTIONS"
	subs_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	subs_title.add_theme_color_override("font_color", Color(0.92, 0.96, 1.0))
	subs_title.add_theme_font_size_override("font_size", 14)
	subs_hdr.add_child(subs_title)

	_subs_count_label = Label.new()
	_subs_count_label.text = "0 / 5"
	_subs_count_label.add_theme_color_override("font_color", Color(0.15, 0.90, 0.55))
	_subs_count_label.add_theme_font_size_override("font_size", 13)
	subs_hdr.add_child(_subs_count_label)

	var subs_scroll := ScrollContainer.new()
	subs_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	subs_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	subs_vbox.add_child(subs_scroll)

	_subs_list_vbox = VBoxContainer.new()
	_subs_list_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_subs_list_vbox.add_theme_constant_override("separation", 8)
	subs_scroll.add_child(_subs_list_vbox)

	# 3. RIGHT CARD: Team Tactical Instructions
	var right_card := PanelContainer.new()
	right_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	right_card.size_flags_stretch_ratio = 0.95
	right_card.add_theme_stylebox_override("panel", _make_card_style())
	body_hbox.add_child(right_card)

	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	right_card.add_child(scroll)

	var ctrl_vbox := VBoxContainer.new()
	ctrl_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	ctrl_vbox.add_theme_constant_override("separation", 10)
	scroll.add_child(ctrl_vbox)

	var inst_title := Label.new()
	inst_title.text = "TEAM INSTRUCTIONS"
	inst_title.add_theme_color_override("font_color", Color(0.92, 0.96, 1.0))
	inst_title.add_theme_font_size_override("font_size", 14)
	ctrl_vbox.add_child(inst_title)

	_formation_option = _add_selector_row(
		ctrl_vbox, "Formation",
		"Repositions all 10 outfielders into structural roles.", []
	)
	_formation_option.item_selected.connect(func(_idx: int) -> void:
		if is_instance_valid(match_engine) and _formation_option.selected >= 0:
			match_engine.set_home_formation(_formation_option.get_item_text(_formation_option.selected))
			_refresh_all_views()
	)

	_mentality_option = _add_selector_row(
		ctrl_vbox, "Mentality",
		"Adjusts risk, forward runs, and chance creation rate.",
		["Very Defensive", "Defensive", "Balanced", "Attacking", "Very Attacking"]
	)
	_pressing_option = _add_selector_row(
		ctrl_vbox, "Pressing",
		"High pressing wins possession higher up but drains stamina faster.",
		["Low", "Normal", "High"]
	)
	_def_line_option = _add_selector_row(
		ctrl_vbox, "Defensive Line",
		"High line compresses space; Deep line guards against counters.",
		["Deep", "Normal", "High"]
	)
	_passing_option = _add_selector_row(
		ctrl_vbox, "Passing Style",
		"Short favors possession; Direct pushes vertical balls to forwards.",
		["Short", "Mixed", "Direct"]
	)
	_width_option = _add_selector_row(
		ctrl_vbox, "Attacking Width",
		"Wide stretches fullbacks/wingers; Narrow overloads central lanes.",
		["Narrow", "Balanced", "Wide"]
	)
	_tempo_option = _add_selector_row(
		ctrl_vbox, "Tempo",
		"Fast accelerates transitions; Slow controls match rhythm.",
		["Slow", "Normal", "Fast"]
	)
	_def_approach_option = _add_selector_row(
		ctrl_vbox, "Def. Approach",
		"Aggressive increases tackle commitment; Stand Off blocks lanes.",
		["Stand Off", "Balanced", "Aggressive"]
	)

	# --- Footer Action Bar ---
	var footer := HBoxContainer.new()
	footer.alignment = BoxContainer.ALIGNMENT_END
	footer.add_theme_constant_override("separation", 12)
	root_vbox.add_child(footer)

	var reset_btn := Button.new()
	reset_btn.text = "RESET TACTICS"
	reset_btn.custom_minimum_size = Vector2(150, 42)
	reset_btn.focus_mode = Control.FOCUS_NONE
	reset_btn.pressed.connect(_on_reset_pressed)
	footer.add_child(reset_btn)

	var apply_btn := Button.new()
	apply_btn.text = "APPLY"
	apply_btn.custom_minimum_size = Vector2(130, 42)
	apply_btn.focus_mode = Control.FOCUS_NONE
	apply_btn.pressed.connect(_on_apply_pressed)
	footer.add_child(apply_btn)

	var resume_btn := Button.new()
	resume_btn.text = "APPLY & RESUME"
	resume_btn.custom_minimum_size = Vector2(175, 42)
	resume_btn.focus_mode = Control.FOCUS_NONE
	var res_style := StyleBoxFlat.new()
	res_style.bg_color = Color(0.08, 0.62, 0.38)
	res_style.set_corner_radius_all(8)
	resume_btn.add_theme_stylebox_override("normal", res_style)
	resume_btn.add_theme_color_override("font_color", Color.WHITE)
	resume_btn.pressed.connect(_on_resume_pressed)
	footer.add_child(resume_btn)

func _make_card_style() -> StyleBoxFlat:
	var st := StyleBoxFlat.new()
	st.bg_color = Color(0.07, 0.10, 0.16, 0.96)
	st.border_color = Color(0.18, 0.25, 0.36, 0.85)
	st.set_border_width_all(1)
	st.set_corner_radius_all(12)
	st.set_content_margin_all(14)
	return st

func _add_selector_row(parent: VBoxContainer, label_text: String, hint_text: String, items: Array) -> OptionButton:
	var wrapper := VBoxContainer.new()
	wrapper.add_theme_constant_override("separation", 2)
	parent.add_child(wrapper)

	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 8)
	wrapper.add_child(row)

	var lbl := Label.new()
	lbl.text = label_text
	lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	lbl.add_theme_color_override("font_color", Color(0.88, 0.93, 1.0))
	lbl.add_theme_font_size_override("font_size", 13)
	row.add_child(lbl)

	var opt := OptionButton.new()
	opt.custom_minimum_size = Vector2(165, 34)
	opt.focus_mode = Control.FOCUS_NONE
	for i in range(items.size()):
		opt.add_item(str(items[i]), i)
	row.add_child(opt)

	var hint := Label.new()
	hint.text = hint_text
	hint.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	hint.add_theme_color_override("font_color", Color(0.56, 0.66, 0.80))
	hint.add_theme_font_size_override("font_size", 11)
	wrapper.add_child(hint)

	return opt

func _sync_controls_from_snapshot(snap: Dictionary) -> void:
	if not is_instance_valid(match_engine):
		return

	_formation_option.clear()
	var formations: PackedStringArray = match_engine.get_available_formations()
	var target_fmt: String = str(snap.get("formation", "4-3-3"))
	for i in range(formations.size()):
		_formation_option.add_item(formations[i], i)
		if formations[i] == target_fmt:
			_formation_option.select(i)

	_mentality_option.select(clampi(int(snap.get("mentality", 2)), 0, 4))
	_pressing_option.select(clampi(int(snap.get("pressing", 1)), 0, 2))
	_def_line_option.select(clampi(int(snap.get("def_line", 1)), 0, 2))
	_passing_option.select(clampi(int(snap.get("passing", 1)), 0, 2))
	_width_option.select(clampi(int(snap.get("width", 1)), 0, 2))
	_tempo_option.select(clampi(int(snap.get("tempo", 1)), 0, 2))
	_def_approach_option.select(clampi(int(snap.get("def_approach", 1)), 0, 2))

func _apply_selected_values() -> void:
	if not is_instance_valid(match_engine):
		return

	if _formation_option.selected >= 0:
		match_engine.set_home_formation(_formation_option.get_item_text(_formation_option.selected))
	match_engine.set_home_mentality(_mentality_option.selected)
	match_engine.set_home_pressing(_pressing_option.selected)
	match_engine.set_home_defensive_line(_def_line_option.selected)
	match_engine.set_home_passing_style(_passing_option.selected)
	match_engine.set_home_attacking_width(_width_option.selected)
	match_engine.set_home_tempo(_tempo_option.selected)
	match_engine.set_home_defensive_approach(_def_approach_option.selected)
	_refresh_all_views()

func _on_reset_pressed() -> void:
	_sync_controls_from_snapshot(_initial_snapshot)
	_apply_selected_values()
	if _status_label:
		_status_label.text = "Tactical instructions restored to pre-pause state."

func _on_apply_pressed() -> void:
	_apply_selected_values()
	if _status_label:
		_status_label.text = "Tactics applied to active match!"

func _on_close_x_pressed() -> void:
	visible = false
	emit_signal("resume_requested")

func _on_resume_pressed() -> void:
	_apply_selected_values()
	visible = false
	emit_signal("resume_requested")

# =========================================================================
# SUBSTITUTIONS & STARTER SWAPS
# =========================================================================

func _rebuild_bench_ui() -> void:
	if not _subs_list_vbox:
		return
	for child in _subs_list_vbox.get_children():
		child.queue_free()

	if _subs_count_label:
		_subs_count_label.text = "%d / %d" % [_subs_used, MAX_SUBS]

	for b_idx in range(_bench_squad.size()):
		var sub: Dictionary = _bench_squad[b_idx]
		var is_used: bool = bool(sub.get("used", false))
		var is_selected: bool = (_selected_bench_idx == b_idx)

		var btn := Button.new()
		btn.custom_minimum_size = Vector2(0, 48)
		btn.focus_mode = Control.FOCUS_NONE
		btn.disabled = is_used or (_subs_used >= MAX_SUBS)

		var card_st := StyleBoxFlat.new()
		if is_used:
			card_st.bg_color = Color(0.08, 0.10, 0.14, 0.55)
			card_st.border_color = Color(0.20, 0.24, 0.30, 0.5)
		elif is_selected:
			card_st.bg_color = Color(0.10, 0.34, 0.24, 0.98)
			card_st.border_color = Color(0.20, 0.95, 0.58, 1.0)
		else:
			card_st.bg_color = Color(0.10, 0.14, 0.22, 0.95)
			card_st.border_color = Color(0.22, 0.30, 0.42, 0.85)
		card_st.set_border_width_all(2 if is_selected else 1)
		card_st.set_corner_radius_all(8)
		card_st.set_content_margin_all(8)
		btn.add_theme_stylebox_override("normal", card_st)
		btn.add_theme_stylebox_override("hover", card_st)
		btn.add_theme_stylebox_override("disabled", card_st)

		var status_txt: String = "SUBBED IN" if is_used else ("OVR %d · %d%%" % [int(sub.get("ovr", 75)), int(sub.get("stamina", 100))])
		btn.text = "[%s]  %s\n%s" % [str(sub.get("position", "CM")), str(sub.get("name", "Sub")), status_txt]
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.add_theme_font_size_override("font_size", 12)
		btn.add_theme_color_override("font_color", Color(0.55, 0.62, 0.72) if is_used else Color.WHITE)

		var captured_idx: int = b_idx
		btn.pressed.connect(func() -> void: _on_bench_player_clicked(captured_idx))
		_subs_list_vbox.add_child(btn)

func _on_bench_player_clicked(b_idx: int) -> void:
	if b_idx < 0 or b_idx >= _bench_squad.size():
		return
	if bool(_bench_squad[b_idx].get("used", false)) or _subs_used >= MAX_SUBS:
		return

	if _selected_bench_idx == b_idx:
		_selected_bench_idx = -1
	else:
		_selected_bench_idx = b_idx

	if _selected_starter_idx >= 0 and _selected_bench_idx >= 0:
		_execute_substitution(_selected_starter_idx, _selected_bench_idx)
		return

	_rebuild_bench_ui()
	if _pitch_preview:
		_pitch_preview.queue_redraw()

func _on_pitch_gui_input(event: InputEvent) -> void:
	var click_pos: Vector2 = Vector2(-1.0, -1.0)
	if event is InputEventMouseButton:
		var mb := event as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_LEFT and mb.pressed:
			click_pos = mb.position
	elif event is InputEventScreenTouch:
		var touch := event as InputEventScreenTouch
		if touch.pressed:
			click_pos = touch.position

	if click_pos.x < 0.0:
		return

	for i in range(_starter_card_rects.size()):
		if _starter_card_rects[i].has_point(click_pos):
			_on_starter_card_clicked(i)
			_pitch_preview.accept_event()
			return

func _on_starter_card_clicked(starter_idx: int) -> void:
	if starter_idx < 0 or starter_idx >= 11:
		return

	if _selected_bench_idx >= 0:
		_execute_substitution(starter_idx, _selected_bench_idx)
		return

	if _selected_starter_idx >= 0 and _selected_starter_idx != starter_idx:
		_swap_starters(_selected_starter_idx, starter_idx)
		_selected_starter_idx = -1
		_refresh_all_views()
		return

	_selected_starter_idx = -1 if (_selected_starter_idx == starter_idx) else starter_idx
	if _pitch_preview:
		_pitch_preview.queue_redraw()

func _execute_substitution(starter_idx: int, bench_idx: int) -> void:
	if not is_instance_valid(match_engine) or _subs_used >= MAX_SUBS:
		return
	if starter_idx < 0 or starter_idx >= 11 or bench_idx < 0 or bench_idx >= _bench_squad.size():
		return

	var sub: Dictionary = _bench_squad[bench_idx]
	if bool(sub.get("used", false)):
		return

	_ensure_starter_ovrs()
	var names: PackedStringArray = match_engine.get_player_names()
	var roles: PackedStringArray = match_engine.get_player_roles()
	var outgoing_name: String = names[starter_idx] if starter_idx < names.size() else ("Home #" + str(starter_idx + 1))
	var slot_role: String = roles[starter_idx] if starter_idx < roles.size() else "CM"
	var incoming_name: String = str(sub.get("name", "Sub"))
	var sub_ovr: int = int(sub.get("ovr", match_engine.get_home_team_ovr()))

	var lineup_arr: Array = []
	for i in range(11):
		var p_name: String = names[i] if i < names.size() else ("Home #" + str(i + 1))
		var p_role: String = roles[i] if i < roles.size() else "CM"
		if i == starter_idx:
			lineup_arr.append({
				"name": incoming_name,
				"position": slot_role,
				"pace": sub_ovr,
				"shooting": sub_ovr,
				"passing": sub_ovr,
				"dribbling": sub_ovr,
				"defending": sub_ovr,
				"physical": sub_ovr
			})
		else:
			lineup_arr.append({"name": p_name, "position": p_role})

	match_engine.set_home_lineup(lineup_arr)
	_starter_ovrs[starter_idx] = sub_ovr

	sub["used"] = true
	_bench_squad[bench_idx] = sub
	_subbed_off_names.append(outgoing_name)
	_subs_used += 1
	_selected_starter_idx = -1
	_selected_bench_idx = -1

	if _status_label:
		_status_label.text = "SUB (%d/%d): %s ON for %s" % [_subs_used, MAX_SUBS, incoming_name, outgoing_name]

	_refresh_all_views()

func _swap_starters(idx_a: int, idx_b: int) -> void:
	if not is_instance_valid(match_engine):
		return
	_ensure_starter_ovrs()
	var names: PackedStringArray = match_engine.get_player_names()
	var roles: PackedStringArray = match_engine.get_player_roles()
	if idx_a >= names.size() or idx_b >= names.size():
		return

	var lineup_arr: Array = []
	for i in range(11):
		var src_idx: int = idx_b if i == idx_a else (idx_a if i == idx_b else i)
		lineup_arr.append({
			"name": names[src_idx],
			"position": roles[i] if i < roles.size() else "CM"
		})
	match_engine.set_home_lineup(lineup_arr)

	var tmp_ovr: int = _starter_ovrs[idx_a]
	_starter_ovrs[idx_a] = _starter_ovrs[idx_b]
	_starter_ovrs[idx_b] = tmp_ovr

	if _status_label:
		_status_label.text = "Swapped %s and %s positions" % [names[idx_a], names[idx_b]]

func _refresh_all_views() -> void:
	_ensure_starter_ovrs()
	if _xi_ovr_label:
		_xi_ovr_label.text = "XI OVR %d" % _get_xi_average_ovr()
	_rebuild_bench_ui()
	if _pitch_preview:
		_pitch_preview.queue_redraw()

# =========================================================================
# VERTICAL PITCH RENDERING (GK AT BOTTOM -> FORWARDS AT TOP)
# =========================================================================

func _get_role_vertical_depth(role: String) -> float:
	var r: String = role.strip_edges().to_upper()
	if r == "GK":
		return 0.89
	if r == "CB":
		return 0.74
	if r == "LB" or r == "RB" or r == "LWB" or r == "RWB":
		return 0.69
	if r == "CDM":
		return 0.56
	if r == "CM" or r == "LM" or r == "RM":
		return 0.43
	if r == "CAM":
		return 0.31
	if r == "LW" or r == "RW":
		return 0.21
	return 0.13 # ST / CF

func _compute_vertical_slots(roles: PackedStringArray, positions: Array, field_sz: Vector2) -> Array[Vector2]:
	var slots: Array[Vector2] = []
	slots.resize(11)

	var line_groups: Array = [[], [], [], []]
	for i in range(11):
		var r: String = roles[i].strip_edges().to_upper() if i < roles.size() else "CM"
		var vy: float = _get_role_vertical_depth(r)
		var vx: float = 0.50
		if i < positions.size() and field_sz.y > 1.0:
			vx = clampf(Vector2(positions[i]).y / field_sz.y, 0.14, 0.86)
		slots[i] = Vector2(vx, vy)

		if r == "GK":
			line_groups[0].append(i)
		elif r in ["CB", "LB", "RB", "LWB", "RWB"]:
			line_groups[1].append(i)
		elif r in ["CDM", "CM", "CAM", "LM", "RM"]:
			line_groups[2].append(i)
		else:
			line_groups[3].append(i)

	for group in line_groups:
		var count: int = group.size()
		if count <= 1:
			if count == 1:
				slots[group[0]].x = 0.50
			continue

		group.sort_custom(func(a: int, b: int) -> bool: return slots[a].x < slots[b].x)
		var min_x: float = 0.15 if count >= 4 else (0.20 if count == 3 else 0.34)
		var max_x: float = 1.0 - min_x
		for k in range(count):
			var t: float = float(k) / float(count - 1)
			slots[group[k]].x = lerpf(min_x, max_x, t)

	return slots

func _draw_vertical_pitch() -> void:
	if not is_instance_valid(match_engine) or not _pitch_preview:
		return

	var sz: Vector2 = _pitch_preview.size
	if sz.x <= 20.0 or sz.y <= 20.0:
		return

	_ensure_starter_ovrs()
	var font: Font = ThemeDB.fallback_font
	var line_col := Color(1.0, 1.0, 1.0, 0.52)

	# Mowed grass stripes (clipped automatically to rounded _pitch_frame)
	_pitch_preview.draw_rect(Rect2(Vector2.ZERO, sz), Color(0.08, 0.42, 0.24), true)
	var stripe_h: float = sz.y / 10.0
	for s in range(0, 10, 2):
		_pitch_preview.draw_rect(Rect2(Vector2(0.0, s * stripe_h), Vector2(sz.x, stripe_h)), Color(0.10, 0.48, 0.28), true)

	# Halfway line & center circle
	_pitch_preview.draw_line(Vector2(0.0, sz.y * 0.06), Vector2(sz.x, sz.y * 0.06), line_col, 1.5)
	_pitch_preview.draw_arc(Vector2(sz.x * 0.5, sz.y * 0.06), minf(sz.x, sz.y) * 0.16, 0.0, PI, 36, line_col, 1.5)

	# Bottom penalty box & 6-yard box
	var box_w: float = sz.x * 0.58
	var box_h: float = sz.y * 0.18
	_pitch_preview.draw_rect(Rect2(Vector2((sz.x - box_w) * 0.5, sz.y - box_h), Vector2(box_w, box_h)), line_col, false, 1.8)

	var small_w: float = sz.x * 0.28
	var small_h: float = sz.y * 0.07
	_pitch_preview.draw_rect(Rect2(Vector2((sz.x - small_w) * 0.5, sz.y - small_h), Vector2(small_w, small_h)), line_col, false, 1.8)

	var positions: Array = match_engine.get_player_positions()
	var roles: PackedStringArray = match_engine.get_player_roles()
	var names: PackedStringArray = match_engine.get_player_names()
	var staminas: PackedFloat32Array = match_engine.get_player_stamina()
	var field_sz: Vector2 = match_engine.get_field_size()

	var norm_slots: Array[Vector2] = _compute_vertical_slots(roles, positions, field_sz)
	_starter_card_rects.resize(11)

	var card_w: float = clampf(sz.x * 0.20, 68.0, 94.0)
	var card_h: float = 50.0

	for i in range(11):
		var center_pt := Vector2(norm_slots[i].x * sz.x, norm_slots[i].y * sz.y)
		var card_rect := Rect2(center_pt - Vector2(card_w * 0.5, card_h * 0.5), Vector2(card_w, card_h))
		_starter_card_rects[i] = card_rect

		var is_selected: bool = (_selected_starter_idx == i)
		var role_txt: String = roles[i] if i < roles.size() else "CM"
		var name_txt: String = names[i] if i < names.size() else ("#" + str(i + 1))
		var p_ovr: int = _starter_ovrs[i] if i < _starter_ovrs.size() else 75
		var stam_val: float = staminas[i] if i < staminas.size() else 100.0
		var stam_ratio: float = clampf(stam_val / 100.0, 0.0, 1.0)

		var card_bg: Color = Color(0.08, 0.28, 0.20, 0.96) if is_selected else Color(0.06, 0.09, 0.15, 0.93)
		var border_col: Color = Color(1.0, 0.88, 0.22, 1.0) if is_selected else (Color(0.95, 0.74, 0.16, 0.9) if i == 0 else Color(0.22, 0.85, 0.55, 0.85))
		_pitch_preview.draw_rect(card_rect, card_bg, true)
		_pitch_preview.draw_rect(card_rect, border_col, false, 2.2 if is_selected else 1.4)

		# Top Role + OVR Header Strip
		var pill_rect := Rect2(card_rect.position, Vector2(card_w, 17.0))
		var pill_col: Color = Color(0.85, 0.64, 0.12, 0.35) if i == 0 else Color(0.08, 0.72, 0.44, 0.35)
		_pitch_preview.draw_rect(pill_rect, pill_col, true)

		# Left: Role | Right: OVR Pill
		_pitch_preview.draw_string(font, card_rect.position + Vector2(6.0, 13.0), role_txt, HORIZONTAL_ALIGNMENT_LEFT, int(card_w * 0.55), 10, Color(0.96, 0.92, 0.38))

		var ovr_badge_w: float = 24.0
		var ovr_badge_rect := Rect2(Vector2(card_rect.end.x - ovr_badge_w - 3.0, card_rect.position.y + 2.0), Vector2(ovr_badge_w, 13.0))
		var ovr_bg: Color = Color(0.92, 0.72, 0.16, 0.92) if p_ovr >= 80 else (Color(0.14, 0.74, 0.48, 0.92) if p_ovr >= 72 else Color(0.32, 0.42, 0.56, 0.92))
		_pitch_preview.draw_rect(ovr_badge_rect, ovr_bg, true)
		_pitch_preview.draw_string(font, ovr_badge_rect.position + Vector2(0.0, 10.5), str(p_ovr), HORIZONTAL_ALIGNMENT_CENTER, int(ovr_badge_w), 10, Color(0.05, 0.08, 0.12))

		# Player Name
		_pitch_preview.draw_string(font, card_rect.position + Vector2(2.0, 32.0), name_txt, HORIZONTAL_ALIGNMENT_CENTER, int(card_w - 4.0), 11, Color.WHITE)

		# Stamina Bar
		var bar_pad: float = 6.0
		var bar_w: float = card_w - (bar_pad * 2.0)
		var bar_pos: Vector2 = card_rect.position + Vector2(bar_pad, card_h - 8.0)
		var stam_col: Color = Color(0.20, 0.95, 0.52) if stam_val > 60.0 else (Color(0.98, 0.76, 0.20) if stam_val > 35.0 else Color(0.95, 0.30, 0.30))
		_pitch_preview.draw_rect(Rect2(bar_pos, Vector2(bar_w, 4.0)), Color(0.12, 0.15, 0.20, 0.9), true)
		_pitch_preview.draw_rect(Rect2(bar_pos, Vector2(bar_w * stam_ratio, 4.0)), stam_col, true)