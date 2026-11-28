class_name MainMenuView
extends Control

signal new_career_requested()
signal continue_career_requested(slot_name: String)
signal load_career_requested(slot_name: String)
signal exit_requested()

var career_state: CareerState

var _continue_btn: Button
var _continue_hint_lbl: Label
var _status_banner: Label
var _load_modal: PanelContainer
var _slots_vbox: VBoxContainer
var _settings_modal: PanelContainer

var _latest_valid_slot: String = ""

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build_ui()
	refresh_save_state()

func refresh_save_state() -> void:
	_latest_valid_slot = ""
	if not is_instance_valid(career_state):
		_update_continue_button(false, "Save system unavailable")
		return

	var slots: Array[Dictionary] = career_state.list_save_slots()
	var best_slot: Dictionary = {}
	for slot in slots:
		if _is_slot_valid(slot):
			if best_slot.is_empty() or String(slot.get("saved_at", "")) > String(best_slot.get("saved_at", "")):
				best_slot = slot

	if best_slot.is_empty():
		_update_continue_button(false, "No saved career found — start a New Career")
	else:
		_latest_valid_slot = String(best_slot.get("slot_name", ""))
		var club := String(best_slot.get("club_name", "Club"))
		var date_str := String(best_slot.get("current_date", ""))
		_update_continue_button(true, "Resume %s (%s)" % [club, date_str])

func _is_slot_valid(slot: Dictionary) -> bool:
	if slot.is_empty() or bool(slot.get("corrupt", false)):
		return false
	var slot_name := String(slot.get("slot_name", "")).strip_edges()
	if slot_name.is_empty():
		return false
	var schema_ver := int(slot.get("schema_version", 1))
	return schema_ver == 1

func _update_continue_button(enabled: bool, hint: String) -> void:
	if is_instance_valid(_continue_btn):
		_continue_btn.disabled = not enabled
	if is_instance_valid(_continue_hint_lbl):
		_continue_hint_lbl.text = hint

func show_error(msg: String) -> void:
	if is_instance_valid(_status_banner):
		_status_banner.text = msg
		_status_banner.visible = not msg.is_empty()

func _build_ui() -> void:
	var bg := ColorRect.new()
	bg.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.06, 0.08, 0.12, 1.0)
	add_child(bg)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var card := PanelContainer.new()
	card.custom_minimum_size = Vector2(460, 0)
	var card_style := StyleBoxFlat.new()
	card_style.bg_color = Color(0.085, 0.115, 0.17, 0.98)
	card_style.border_color = Color(0.18, 0.25, 0.36, 0.90)
	card_style.set_border_width_all(1)
	card_style.set_corner_radius_all(8)
	card_style.set_content_margin_all(28)
	card.add_theme_stylebox_override("panel", card_style)
	center.add_child(card)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	card.add_child(vbox)

	var badge := Label.new()
	badge.text = "CRYSTAL FOOTBALL MANAGER 1 - CFM1"
	badge.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	badge.add_theme_font_size_override("font_size", 12)
	vbox.add_child(badge)

	var title := Label.new()
	title.text = "CAREER SELECTION"
	title.add_theme_color_override("font_color", Color.WHITE)
	title.add_theme_font_size_override("font_size", 24)
	vbox.add_child(title)

	_status_banner = Label.new()
	_status_banner.visible = false
	_status_banner.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	var err_style := StyleBoxFlat.new()
	err_style.bg_color = Color(0.32, 0.10, 0.12, 0.95)
	err_style.border_color = Color(1.0, 0.35, 0.38, 0.92)
	err_style.set_border_width_all(1)
	err_style.set_corner_radius_all(6)
	err_style.set_content_margin_all(8)
	_status_banner.add_theme_stylebox_override("normal", err_style)
	_status_banner.add_theme_color_override("font_color", Color(1.0, 0.85, 0.86))
	_status_banner.add_theme_font_size_override("font_size", 12)
	vbox.add_child(_status_banner)

	vbox.add_child(HSeparator.new())

	_continue_btn = _make_menu_button("Continue Career", true)
	_continue_btn.pressed.connect(func() -> void:
		if not _latest_valid_slot.is_empty():
			continue_career_requested.emit(_latest_valid_slot)
	)
	vbox.add_child(_continue_btn)

	_continue_hint_lbl = Label.new()
	_continue_hint_lbl.text = "Checking saved careers..."
	_continue_hint_lbl.add_theme_color_override("font_color", Color(0.65, 0.72, 0.82))
	_continue_hint_lbl.add_theme_font_size_override("font_size", 11)
	vbox.add_child(_continue_hint_lbl)

	var new_btn := _make_menu_button("New Career", false)
	new_btn.pressed.connect(func() -> void: new_career_requested.emit())
	vbox.add_child(new_btn)

	var load_btn := _make_menu_button("Load Career", false)
	load_btn.pressed.connect(_open_load_modal)
	vbox.add_child(load_btn)

	var settings_btn := _make_menu_button("Settings", false)
	settings_btn.pressed.connect(func() -> void: _settings_modal.visible = true)
	vbox.add_child(settings_btn)

	var exit_btn := _make_menu_button("Exit", false)
	exit_btn.pressed.connect(func() -> void: exit_requested.emit())
	vbox.add_child(exit_btn)

	_build_load_modal()
	_build_settings_modal()

func _make_menu_button(text_label: String, primary: bool) -> Button:
	var btn := Button.new()
	btn.text = text_label
	btn.custom_minimum_size = Vector2(0, 44)
	btn.focus_mode = Control.FOCUS_NONE

	var normal := StyleBoxFlat.new()
	normal.bg_color = Color(0.08, 0.36, 0.26, 0.95) if primary else Color(0.095, 0.13, 0.195, 0.95)
	normal.border_color = Color(0.11, 0.82, 0.63, 0.95) if primary else Color(0.20, 0.28, 0.40, 0.90)
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(8)

	var hover := normal.duplicate()
	hover.bg_color = normal.bg_color.lightened(0.10)
	hover.border_color = normal.border_color.lightened(0.12)

	var disabled := normal.duplicate()
	disabled.bg_color = Color(0.065, 0.088, 0.132, 0.70)
	disabled.border_color = Color(0.14, 0.19, 0.28, 0.55)

	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("disabled", disabled)
	btn.add_theme_color_override("font_color", Color(0.35, 0.95, 0.55) if primary else Color.WHITE)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)
	btn.add_theme_color_override("font_disabled_color", Color(0.48, 0.56, 0.68))
	return btn

func _build_load_modal() -> void:
	_load_modal = _create_centered_modal(540, 400)
	add_child(_load_modal)

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 10)
	_load_modal.add_child(vb)

	var hdr := Label.new()
	hdr.text = "LOAD CAREER"
	hdr.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	hdr.add_theme_font_size_override("font_size", 16)
	vb.add_child(hdr)

	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	vb.add_child(scroll)

	_slots_vbox = VBoxContainer.new()
	_slots_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_slots_vbox.add_theme_constant_override("separation", 8)
	scroll.add_child(_slots_vbox)

	var close_btn := _make_menu_button("Back", false)
	close_btn.custom_minimum_size = Vector2(120, 36)
	close_btn.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	close_btn.pressed.connect(func() -> void: _load_modal.visible = false)
	vb.add_child(close_btn)

func _open_load_modal() -> void:
	for child in _slots_vbox.get_children():
		child.queue_free()

	var slots: Array[Dictionary] = career_state.list_save_slots() if is_instance_valid(career_state) else []
	if slots.is_empty():
		var empty_lbl := Label.new()
		empty_lbl.text = "No save slots found."
		empty_lbl.add_theme_color_override("font_color", Color(0.65, 0.75, 0.88))
		_slots_vbox.add_child(empty_lbl)
	else:
		for slot in slots:
			var valid := _is_slot_valid(slot)
			var slot_name := String(slot.get("slot_name", "unknown"))
			var club := String(slot.get("club_name", "Corrupt Save"))
			var mgr := String(slot.get("manager_name", "Unknown"))
			var date_str := String(slot.get("current_date", "N/A"))
			var season := String(slot.get("season_label", ""))

			var row_btn := _make_menu_button(
				"%s — %s (%s) • %s [%s]" % [slot_name, club, mgr, date_str, season] if valid
				else "%s — Incompatible or Corrupt Save" % slot_name,
				false
			)
			row_btn.disabled = not valid
			row_btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
			row_btn.pressed.connect(func() -> void:
				_load_modal.visible = false
				load_career_requested.emit(slot_name)
			)
			_slots_vbox.add_child(row_btn)

	_load_modal.visible = true

func _build_settings_modal() -> void:
	_settings_modal = _create_centered_modal(440, 280)
	add_child(_settings_modal)

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 12)
	_settings_modal.add_child(vb)

	var hdr := Label.new()
	hdr.text = "SETTINGS"
	hdr.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	hdr.add_theme_font_size_override("font_size", 16)
	vb.add_child(hdr)

	var fullscreen_chk := CheckBox.new()
	fullscreen_chk.text = "Fullscreen Display"
	fullscreen_chk.button_pressed = DisplayServer.window_get_mode() == DisplayServer.WINDOW_MODE_FULLSCREEN
	fullscreen_chk.toggled.connect(func(on: bool) -> void:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN if on else DisplayServer.WINDOW_MODE_WINDOWED)
	)
	vb.add_child(fullscreen_chk)

	var note := Label.new()
	note.text = ""
	note.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	note.add_theme_color_override("font_color", Color(0.65, 0.72, 0.82))
	note.add_theme_font_size_override("font_size", 12)
	vb.add_child(note)

	var spacer := Control.new()
	spacer.size_flags_vertical = Control.SIZE_EXPAND_FILL
	vb.add_child(spacer)

	var close_btn := _make_menu_button("Close", false)
	close_btn.custom_minimum_size = Vector2(120, 36)
	close_btn.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	close_btn.pressed.connect(func() -> void: _settings_modal.visible = false)
	vb.add_child(close_btn)

func _create_centered_modal(w: float, h: float) -> PanelContainer:
	var modal := PanelContainer.new()
	modal.anchor_left = 0.5
	modal.anchor_top = 0.5
	modal.anchor_right = 0.5
	modal.anchor_bottom = 0.5
	modal.offset_left = -w * 0.5
	modal.offset_top = -h * 0.5
	modal.offset_right = w * 0.5
	modal.offset_bottom = h * 0.5
	modal.grow_horizontal = Control.GROW_DIRECTION_BOTH
	modal.grow_vertical = Control.GROW_DIRECTION_BOTH
	modal.visible = false

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.072, 0.098, 0.148, 0.99)
	style.border_color = Color(0.11, 0.82, 0.63, 0.95)
	style.set_border_width_all(2)
	style.set_corner_radius_all(8)
	style.set_content_margin_all(22)
	modal.add_theme_stylebox_override("panel", style)
	return modal
