class_name RedeemCodeModal
extends Control

signal code_redeemed(result: Dictionary)
signal closed()

var career_state: CareerState

var _code_input: LineEdit
var _status_lbl: Label
var _reward_lbl: Label
var _redeem_btn: Button
var _temp_career_state: CareerState

func _ready() -> void:
	visible = false
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build_ui()

func open_modal() -> void:
	var cs := _resolve_career_state()
	var already_unlocked := false
	if is_instance_valid(cs) and cs.has_method("is_tuesday_league_unlocked"):
		already_unlocked = bool(cs.call("is_tuesday_league_unlocked"))

	if is_instance_valid(_status_lbl):
		if already_unlocked:
			_status_lbl.text = "Code NHHA72528 is active! Tuesday League & club 11A7 are unlocked."
			_status_lbl.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
		else:
			_status_lbl.text = "Enter a special promo code (e.g. NHHA72528) to unlock Tuesday League & 11A7."
			_status_lbl.add_theme_color_override("font_color", Color(0.68, 0.78, 0.90))

	if is_instance_valid(_code_input):
		_code_input.text = ""
		_code_input.grab_focus()
	visible = true

func close_modal() -> void:
	visible = false
	closed.emit()

func _resolve_career_state() -> CareerState:
	if is_instance_valid(career_state):
		return career_state
	var parent_cs := get_parent().get_node_or_null("CareerState") as CareerState if is_instance_valid(get_parent()) else null
	if is_instance_valid(parent_cs):
		career_state = parent_cs
		return career_state
	if not is_instance_valid(_temp_career_state):
		_temp_career_state = CareerState.new()
		_temp_career_state.name = "TempCareerState"
		add_child(_temp_career_state)
	return _temp_career_state

func _on_redeem_pressed() -> void:
	var raw := _code_input.text.strip_edges().to_upper()
	if raw.is_empty():
		_status_lbl.text = "Please enter a promo code first."
		_status_lbl.add_theme_color_override("font_color", Color(1.0, 0.45, 0.45))
		return

	var cs := _resolve_career_state()
	if not is_instance_valid(cs) or not cs.has_method("redeem_promo_code"):
		_status_lbl.text = "CareerState is unavailable."
		_status_lbl.add_theme_color_override("font_color", Color(1.0, 0.45, 0.45))
		return

	var res: Dictionary = cs.call("redeem_promo_code", raw)
	var ok := bool(res.get("ok", false))
	_status_lbl.text = String(res.get("message", "Unknown response."))
	_status_lbl.add_theme_color_override(
		"font_color",
		Color(0.11, 0.82, 0.63) if ok else Color(1.0, 0.45, 0.45)
	)
	if ok:
		code_redeemed.emit(res)

func _build_ui() -> void:
	var dim := ColorRect.new()
	dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	dim.color = Color(0.0, 0.0, 0.0, 0.68)
	dim.gui_input.connect(func(ev: InputEvent) -> void:
		if ev is InputEventMouseButton and ev.pressed and ev.button_index == MOUSE_BUTTON_LEFT:
			close_modal()
	)
	add_child(dim)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(440, 0)
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.043, 0.063, 0.106, 0.99)
	sb.border_color = Color(0.11, 0.82, 0.63, 0.92)
	sb.set_border_width_all(2)
	sb.set_corner_radius_all(8)
	sb.set_content_margin_all(24)
	panel.add_theme_stylebox_override("panel", sb)
	center.add_child(panel)

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 12)
	panel.add_child(vb)

	var title := Label.new()
	title.text = "🎟️ REDEEM PROMO CODE"
	title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title.add_theme_font_size_override("font_size", 18)
	vb.add_child(title)

	_reward_lbl = Label.new()
	_reward_lbl.text = "Unlocks hidden Tuesday League (11A7) + active career budget & token rewards."
	_reward_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_reward_lbl.add_theme_color_override("font_color", Color(0.62, 0.72, 0.86))
	_reward_lbl.add_theme_font_size_override("font_size", 12)
	vb.add_child(_reward_lbl)

	_code_input = LineEdit.new()
	_code_input.placeholder_text = "Enter code (e.g. NHHA72528)"
	_code_input.custom_minimum_size = Vector2(0, 40)
	_code_input.text_submitted.connect(func(_t: String) -> void: _on_redeem_pressed())
	vb.add_child(_code_input)

	_status_lbl = Label.new()
	_status_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_status_lbl.add_theme_font_size_override("font_size", 12)
	vb.add_child(_status_lbl)

	var btns := HBoxContainer.new()
	btns.add_theme_constant_override("separation", 10)
	vb.add_child(btns)

	var close_btn := Button.new()
	close_btn.text = "Close"
	close_btn.custom_minimum_size = Vector2(105, 38)
	close_btn.focus_mode = Control.FOCUS_NONE
	close_btn.pressed.connect(close_modal)
	btns.add_child(close_btn)

	_redeem_btn = Button.new()
	_redeem_btn.text = "Redeem Code"
	_redeem_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_redeem_btn.custom_minimum_size = Vector2(0, 38)
	_redeem_btn.focus_mode = Control.FOCUS_NONE
	_redeem_btn.pressed.connect(_on_redeem_pressed)
	btns.add_child(_redeem_btn)
