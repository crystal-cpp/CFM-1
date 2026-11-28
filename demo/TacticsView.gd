class_name TacticsView
extends Control

signal tactics_saved(formation: String)

var career_state: CareerState
var _ui_built: bool = false

var _formation_opt: OptionButton
var _mentality_opt: OptionButton
var _pressing_opt: OptionButton
var _line_opt: OptionButton
var _passing_opt: OptionButton
var _width_opt: OptionButton
var _tempo_opt: OptionButton
var _approach_opt: OptionButton
var _impact_summary_lbl: Label
var _status_lbl: Label

const FORMATIONS: Array[String] = ["4-3-3", "4-4-2", "4-2-3-1", "3-5-2", "4-1-4-1", "4-3-2-1", "4-3-1-2", "4-1-2-1-2 Narrow", "4-1-2-1-2 Wide", "4-5-1", "3-4-3", "3-4-2-1", "3-4-1-2", "3-1-4-2", "5-3-2", "5-4-1", "5-2-3", "4-2-2-2", "4-2-4", "3-3-3-1", "4-1-3-2", "3-2-4-1", "5-2-1-2", "4-6-0"]

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_flags_vertical = Control.SIZE_EXPAND_FILL
	_ensure_ui_built()
	visibility_changed.connect(func() -> void:
		if visible:
			refresh_tactics()
	)
	refresh_tactics()

func _ensure_ui_built() -> void:
	if _ui_built:
		return
	_ui_built = true

	var root := PanelContainer.new()
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var bg := StyleBoxFlat.new()
	bg.bg_color = Color(0.024, 0.035, 0.055, 1.0)
	bg.content_margin_left = 20
	bg.content_margin_right = 20
	bg.content_margin_top = 16
	bg.content_margin_bottom = 16
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

	var title := Label.new()
	title.text = "TACTIC MANAGEMENT"
	title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title.add_theme_font_size_override("font_size", 20)
	vbox.add_child(title)

	var sub := Label.new()
	sub.text = "Configure your tactics."
	sub.add_theme_color_override("font_color", Color(0.65, 0.76, 0.90))
	sub.add_theme_font_size_override("font_size", 12)
	vbox.add_child(sub)

	# Grid Card for Tactical Controls
	var card := PanelContainer.new()
	var c_style := StyleBoxFlat.new()
	c_style.bg_color = Color(0.043, 0.063, 0.106, 0.96)
	c_style.border_color = Color(0.12, 0.22, 0.34, 0.85)
	c_style.set_border_width_all(1)
	c_style.set_corner_radius_all(10)
	c_style.set_content_margin_all(18)
	card.add_theme_stylebox_override("panel", c_style)
	vbox.add_child(card)

	var grid := GridContainer.new()
	grid.columns = 2
	grid.add_theme_constant_override("h_separation", 24)
	grid.add_theme_constant_override("v_separation", 12)
	card.add_child(grid)

	_formation_opt = _add_tactic_row(grid, "Base Formation", FORMATIONS)
	_mentality_opt = _add_tactic_row(grid, "Team Mentality", ["Ultra Defensive", "Defensive", "Balanced", "Attacking", "Ultra Attacking"])
	_pressing_opt = _add_tactic_row(grid, "Pressing Intensity", ["Low", "Balanced", "High"])
	_line_opt = _add_tactic_row(grid, "Defensive Line", ["Deep", "Standard", "Offside Trap"])
	_passing_opt = _add_tactic_row(grid, "Passing Style", ["Short Passing", "Mixed", "Direct"])
	_width_opt = _add_tactic_row(grid, "Attacking Width", ["Narrow", "Balanced", "Wide"])
	_tempo_opt = _add_tactic_row(grid, "Build-Up Tempo", ["Slow", "Normal", "High"])
	_approach_opt = _add_tactic_row(grid, "Defensive Approach", ["Cautious", "Balanced", "Aggressive"])

	# Tactical Impact Preview Box
	var impact_card := PanelContainer.new()
	var ic_style := StyleBoxFlat.new()
	ic_style.bg_color = Color(0.05, 0.10, 0.16, 0.95)
	ic_style.border_color = Color(0.11, 0.82, 0.63, 0.65)
	ic_style.set_border_width_all(1)
	ic_style.set_corner_radius_all(8)
	ic_style.set_content_margin_all(14)
	impact_card.add_theme_stylebox_override("panel", ic_style)
	vbox.add_child(impact_card)

	_impact_summary_lbl = Label.new()
	_impact_summary_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_impact_summary_lbl.add_theme_color_override("font_color", Color(0.82, 0.94, 0.90))
	_impact_summary_lbl.add_theme_font_size_override("font_size", 12)
	impact_card.add_child(_impact_summary_lbl)

	# Footer Actions
	var btn_hbox := HBoxContainer.new()
	btn_hbox.add_theme_constant_override("separation", 12)
	vbox.add_child(btn_hbox)

	var save_btn := Button.new()
	save_btn.text = "Apply"
	save_btn.custom_minimum_size = Vector2(180, 38)
	save_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(save_btn, Color(0.06, 0.45, 0.32, 0.95), Color(0.15, 0.92, 0.58, 0.90))
	save_btn.pressed.connect(_on_apply_pressed)
	btn_hbox.add_child(save_btn)

	var reset_btn := Button.new()
	reset_btn.text = "Reset"
	reset_btn.custom_minimum_size = Vector2(150, 38)
	reset_btn.focus_mode = Control.FOCUS_NONE
	_style_btn(reset_btn, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.30, 0.48, 0.85))
	reset_btn.pressed.connect(_on_reset_pressed)
	btn_hbox.add_child(reset_btn)

	_status_lbl = Label.new()
	_status_lbl.text = ""
	_status_lbl.add_theme_color_override("font_color", Color(0.25, 0.98, 0.65))
	_status_lbl.add_theme_font_size_override("font_size", 12)
	btn_hbox.add_child(_status_lbl)

func _add_tactic_row(grid: GridContainer, label_text: String, items: Array[String]) -> OptionButton:
	var lbl := Label.new()
	lbl.text = label_text
	lbl.custom_minimum_size = Vector2(200, 32)
	lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	lbl.add_theme_color_override("font_color", Color(0.85, 0.92, 1.0))
	lbl.add_theme_font_size_override("font_size", 13)
	grid.add_child(lbl)

	var opt := OptionButton.new()
	opt.custom_minimum_size = Vector2(260, 34)
	opt.focus_mode = Control.FOCUS_NONE
	_style_btn(opt, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.30, 0.48, 0.85))
	for item in items:
		opt.add_item(item)
	opt.item_selected.connect(func(_idx: int) -> void: _update_impact_preview())
	grid.add_child(opt)
	return opt

func refresh_tactics() -> void:
	_ensure_ui_built()
	if not is_instance_valid(career_state) or not career_state.is_career_active():
		return

	var club_id: int = career_state.get_managed_club_id()
	var t: Dictionary = career_state.get_club_tactics(club_id)
	var f_str: String = String(t.get("formation", "4-3-3"))
	var f_idx: int = FORMATIONS.find(f_str)
	_formation_opt.selected = f_idx if f_idx >= 0 else 0

	_mentality_opt.selected = clampi(int(t.get("mentality", 2)), 0, 4)
	_pressing_opt.selected = clampi(int(t.get("pressing", 1)), 0, 2)
	_line_opt.selected = clampi(int(t.get("defensive_line", 1)), 0, 2)
	_passing_opt.selected = clampi(int(t.get("passing_style", 1)), 0, 2)
	_width_opt.selected = clampi(int(t.get("width", 1)), 0, 2)
	_tempo_opt.selected = clampi(int(t.get("tempo", 1)), 0, 2)
	_approach_opt.selected = clampi(int(t.get("defensive_approach", 1)), 0, 2)
	_update_impact_preview()

func _update_impact_preview() -> void:
	var stamina_load: String = "Moderate"
	if _pressing_opt.selected == 2 and _tempo_opt.selected == 2:
		stamina_load = "Very High"
	elif _pressing_opt.selected == 0 and _tempo_opt.selected == 0:
		stamina_load = "Low"

	_impact_summary_lbl.text = "Tactical Summary:\n• System: %s (%s) — %s build-up at %s tempo.\n• Stamina Drain Prediction: %s." % [
		_formation_opt.get_item_text(_formation_opt.selected),
		_mentality_opt.get_item_text(_mentality_opt.selected),
		_passing_opt.get_item_text(_passing_opt.selected),
		_tempo_opt.get_item_text(_tempo_opt.selected),
		stamina_load
	]

func _on_apply_pressed() -> void:
	if not is_instance_valid(career_state):
		return
	var club_id: int = career_state.get_managed_club_id()
	var selected_form: String = _formation_opt.get_item_text(_formation_opt.selected)
	var payload := {
		"formation": selected_form,
		"mentality": _mentality_opt.selected,
		"pressing": _pressing_opt.selected,
		"defensive_line": _line_opt.selected,
		"passing_style": _passing_opt.selected,
		"width": _width_opt.selected,
		"tempo": _tempo_opt.selected,
		"defensive_approach": _approach_opt.selected
	}
	if career_state.set_club_tactics(club_id, payload):
		_status_lbl.text = "Saved tactics (%s)." % selected_form
		tactics_saved.emit(selected_form)

func _on_reset_pressed() -> void:
	_formation_opt.selected = 0
	_mentality_opt.selected = 2
	_pressing_opt.selected = 1
	_line_opt.selected = 1
	_passing_opt.selected = 1
	_width_opt.selected = 1
	_tempo_opt.selected = 1
	_approach_opt.selected = 1
	_update_impact_preview()
	_on_apply_pressed()

func _style_btn(btn: Button, bg: Color, border: Color) -> void:
	var n := StyleBoxFlat.new()
	n.bg_color = bg
	n.border_color = border
	n.set_border_width_all(1)
	n.set_corner_radius_all(6)
	n.content_margin_left = 12
	n.content_margin_right = 12
	n.content_margin_top = 6
	n.content_margin_bottom = 6

	var h := n.duplicate()
	h.bg_color = bg.lightened(0.10)
	h.border_color = border.lightened(0.12)

	var p := n.duplicate()
	p.bg_color = bg.darkened(0.10)

	var d := n.duplicate()
	d.bg_color = Color(0.05, 0.07, 0.11, 0.60)
	d.border_color = Color(0.12, 0.16, 0.24, 0.50)

	btn.add_theme_stylebox_override("normal", n)
	btn.add_theme_stylebox_override("hover", h)
	btn.add_theme_stylebox_override("pressed", p)
	btn.add_theme_stylebox_override("disabled", d)
	btn.add_theme_color_override("font_color", Color.WHITE)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)
	btn.add_theme_color_override("font_disabled_color", Color(0.45, 0.52, 0.62))
