class_name InboxView
extends PanelContainer

signal route_requested(route_name: String)
signal player_profile_requested(player_id: int, context_mode: String)

var career_state: CareerState
var current_filter: String = "ALL"
var selected_message_id: int = -1

# UI Components
var _filters_hbox: HBoxContainer
var _filter_buttons: Dictionary = {}
var _split_container: BoxContainer
var _left_panel: PanelContainer
var _messages_vbox: VBoxContainer
var _detail_title_lbl: Label
var _detail_meta_lbl: Label
var _detail_body_lbl: Label
var _action_btn: Button
var _mark_read_btn: Button
var _current_action_callable: Callable

const CATEGORIES: Array[String] = ["ALL", "MATCH", "TRANSFER", "BOARD", "YOUTH", "INJURY"]

func _ready() -> void:
	_ensure_ui_built()
	resized.connect(_update_responsive_layout)
	visibility_changed.connect(func() -> void:
		if visible:
			refresh_inbox()
	)
	refresh_inbox()

func _ensure_ui_built() -> void:
	if _filters_hbox != null:
		return

	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	size_flags_horizontal = Control.SIZE_EXPAND_FILL
	size_flags_vertical = Control.SIZE_EXPAND_FILL

	var root_style := StyleBoxFlat.new()
	root_style.bg_color = Color(0.028, 0.040, 0.065, 0.98)
	root_style.set_content_margin_all(20)
	add_theme_stylebox_override("panel", root_style)

	var main_vbox := VBoxContainer.new()
	main_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	main_vbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_theme_constant_override("separation", 14)
	add_child(main_vbox)

	# 1. TOP TOOLBAR: Category Filter Chips + Mark All Read
	var top_bar := HBoxContainer.new()
	top_bar.add_theme_constant_override("separation", 8)
	main_vbox.add_child(top_bar)

	_filters_hbox = HBoxContainer.new()
	_filters_hbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_filters_hbox.add_theme_constant_override("separation", 6)
	top_bar.add_child(_filters_hbox)

	for cat in CATEGORIES:
		var chip := Button.new()
		chip.text = cat
		chip.custom_minimum_size = Vector2(72, 32)
		chip.focus_mode = Control.FOCUS_NONE
		chip.add_theme_font_size_override("font_size", 11)
		chip.pressed.connect(func() -> void: _set_filter(cat))
		_filters_hbox.add_child(chip)
		_filter_buttons[cat] = chip

	_update_filter_chip_styles()

	_mark_read_btn = Button.new()
	_mark_read_btn.text = "✓ Mark All Read"
	_mark_read_btn.custom_minimum_size = Vector2(124, 32)
	_mark_read_btn.focus_mode = Control.FOCUS_NONE
	_apply_btn_style(_mark_read_btn, Color(0.07, 0.12, 0.20, 0.9), Color(0.18, 0.30, 0.48, 0.8), Color(0.85, 0.94, 1.0))
	_mark_read_btn.add_theme_font_size_override("font_size", 11)
	_mark_read_btn.pressed.connect(_on_mark_all_read_pressed)
	top_bar.add_child(_mark_read_btn)

	main_vbox.add_child(HSeparator.new())

	# 2. SPLIT PANE: Left Message List | Right Reader
	_split_container = BoxContainer.new()
	_split_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_split_container.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_split_container.add_theme_constant_override("separation", 16)
	main_vbox.add_child(_split_container)

	# Left Pane: Scrollable Message List
	_left_panel = PanelContainer.new()
	_left_panel.custom_minimum_size = Vector2(340, 180)
	_left_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var lp_style := StyleBoxFlat.new()
	lp_style.bg_color = Color(0.035, 0.052, 0.082, 0.92)
	lp_style.border_color = Color(0.12, 0.18, 0.28, 0.78)
	lp_style.set_border_width_all(1)
	lp_style.set_corner_radius_all(8)
	lp_style.set_content_margin_all(10)
	_left_panel.add_theme_stylebox_override("panel", lp_style)
	_split_container.add_child(_left_panel)

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	_left_panel.add_child(scroll)

	_messages_vbox = VBoxContainer.new()
	_messages_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_messages_vbox.add_theme_constant_override("separation", 6)
	scroll.add_child(_messages_vbox)

	# Right Pane: Message Detail Reader
	var right_panel := PanelContainer.new()
	right_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	right_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var rp_style := StyleBoxFlat.new()
	rp_style.bg_color = Color(0.040, 0.060, 0.095, 0.95)
	rp_style.border_color = Color(0.14, 0.22, 0.35, 0.85)
	rp_style.set_border_width_all(1)
	rp_style.set_corner_radius_all(10)
	rp_style.set_content_margin_all(20)
	right_panel.add_theme_stylebox_override("panel", rp_style)
	_split_container.add_child(right_panel)

	var reader_vbox := VBoxContainer.new()
	reader_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	reader_vbox.size_flags_vertical = Control.SIZE_EXPAND_FILL
	reader_vbox.add_theme_constant_override("separation", 14)
	right_panel.add_child(reader_vbox)

	_detail_title_lbl = Label.new()
	_detail_title_lbl.text = "Select a message from the list"
	_detail_title_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_detail_title_lbl.add_theme_color_override("font_color", Color.WHITE)
	_detail_title_lbl.add_theme_font_size_override("font_size", 18)
	reader_vbox.add_child(_detail_title_lbl)

	_detail_meta_lbl = Label.new()
	_detail_meta_lbl.text = "Category: --- · Date: ---"
	_detail_meta_lbl.add_theme_color_override("font_color", Color(0.55, 0.70, 0.85))
	_detail_meta_lbl.add_theme_font_size_override("font_size", 12)
	reader_vbox.add_child(_detail_meta_lbl)

	reader_vbox.add_child(HSeparator.new())

	_detail_body_lbl = Label.new()
	_detail_body_lbl.text = "Message contents will appear here."
	_detail_body_lbl.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_detail_body_lbl.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_detail_body_lbl.add_theme_color_override("font_color", Color(0.85, 0.92, 1.0))
	_detail_body_lbl.add_theme_font_size_override("font_size", 13)
	reader_vbox.add_child(_detail_body_lbl)

	_action_btn = Button.new()
	_action_btn.text = "⚡ Take Action"
	_action_btn.custom_minimum_size = Vector2(160, 38)
	_action_btn.size_flags_horizontal = Control.SIZE_SHRINK_BEGIN
	_action_btn.focus_mode = Control.FOCUS_NONE
	_apply_btn_style(_action_btn, Color(0.10, 0.56, 0.34, 0.95), Color(0.20, 0.95, 0.58, 0.95), Color.WHITE)
	_action_btn.visible = false
	reader_vbox.add_child(_action_btn)

	_update_responsive_layout()

func _update_responsive_layout() -> void:
	if not is_instance_valid(_split_container):
		return
	var is_narrow: bool = (size.x > 0.0 and size.x < 820.0)
	_split_container.vertical = is_narrow
	if is_instance_valid(_left_panel):
		_left_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL if is_narrow else Control.SIZE_FILL

func _apply_btn_style(btn: Button, bg: Color, border: Color, text_col: Color) -> void:
	var normal := StyleBoxFlat.new()
	normal.bg_color = bg
	normal.border_color = border
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(6)
	normal.content_margin_left = 10
	normal.content_margin_right = 10
	normal.content_margin_top = 6
	normal.content_margin_bottom = 6

	var hover := normal.duplicate()
	hover.bg_color = bg.lightened(0.09)
	hover.border_color = border.lightened(0.15)

	var pressed := normal.duplicate()
	pressed.bg_color = bg.darkened(0.10)

	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("pressed", pressed)
	btn.add_theme_color_override("font_color", text_col)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)

func _update_filter_chip_styles() -> void:
	for cat in _filter_buttons.keys():
		var chip: Button = _filter_buttons[cat]
		var active: bool = (cat == current_filter)
		if active:
			_apply_btn_style(chip, Color(0.10, 0.45, 0.28, 0.92), Color(0.20, 0.95, 0.58, 0.95), Color(0.40, 1.0, 0.72))
		else:
			_apply_btn_style(chip, Color(0.06, 0.09, 0.15, 0.85), Color(0.14, 0.22, 0.35, 0.75), Color(0.78, 0.86, 0.96))

func _set_filter(cat: String) -> void:
	current_filter = cat
	selected_message_id = -1
	_update_filter_chip_styles()
	refresh_inbox()

func _on_mark_all_read_pressed() -> void:
	if is_instance_valid(career_state):
		career_state.mark_all_inbox_read()
		refresh_inbox()

func _clear_reader_pane(empty_msg: String) -> void:
	_detail_title_lbl.text = empty_msg
	_detail_meta_lbl.text = "Category: %s · Date: ---" % current_filter
	_detail_body_lbl.text = "No messages available under this filter."
	if _current_action_callable.is_valid() and _action_btn.pressed.is_connected(_current_action_callable):
		_action_btn.pressed.disconnect(_current_action_callable)
	_current_action_callable = Callable()
	_action_btn.visible = false

# -----------------------------------------------------------------------------
# Data Binding & Inbox Refresh
# -----------------------------------------------------------------------------
func refresh_inbox() -> void:
	_ensure_ui_built()
	if not is_instance_valid(career_state):
		return

	for child in _messages_vbox.get_children():
		child.queue_free()

	var msgs: Array = career_state.get_inbox_messages(current_filter)
	if msgs.is_empty():
		var empty_lbl := Label.new()
		empty_lbl.text = "No messages in category '%s'" % current_filter
		empty_lbl.add_theme_color_override("font_color", Color(0.50, 0.60, 0.75))
		_messages_vbox.add_child(empty_lbl)
		_clear_reader_pane("No messages in '%s'" % current_filter)
		return

	var target_msg: Dictionary = {}
	for i in range(msgs.size()):
		var msg: Dictionary = msgs[i]
		var m_id: int = int(msg.get("id", 0))
		var is_read: bool = bool(msg.get("is_read", false))
		var title: String = String(msg.get("title", "Message"))
		var cat: String = String(msg.get("category", "INFO"))
		var date_iso: String = String(msg.get("date_iso", ""))

		if i == 0 or m_id == selected_message_id:
			if target_msg.is_empty() or m_id == selected_message_id:
				target_msg = msg

		var btn := Button.new()
		btn.custom_minimum_size = Vector2(0, 50)
		btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.clip_text = true
		btn.focus_mode = Control.FOCUS_NONE

		var is_selected: bool = (m_id == selected_message_id)
		var bg_col: Color = Color(0.08, 0.16, 0.28, 0.95) if is_selected else (Color(0.06, 0.12, 0.22, 0.90) if not is_read else Color(0.04, 0.06, 0.10, 0.78))
		var bd_col: Color = Color(0.20, 0.92, 0.56, 0.95) if (is_selected or not is_read) else Color(0.11, 0.16, 0.25, 0.65)
		_apply_btn_style(btn, bg_col, bd_col, Color.WHITE if not is_read else Color(0.75, 0.84, 0.94))

		var btn_text: String = ("● " if not is_read else "   ") + ("[%s] %s\n   %s" % [cat, title, date_iso])
		btn.text = btn_text
		btn.add_theme_font_size_override("font_size", 11)

		btn.pressed.connect(func() -> void: _display_message(msg))
		_messages_vbox.add_child(btn)

	if selected_message_id == -1 and not target_msg.is_empty():
		_display_message(target_msg)

func _display_message(msg: Dictionary) -> void:
	var m_id: int = int(msg.get("id", 0))
	selected_message_id = m_id

	_detail_title_lbl.text = String(msg.get("title", ""))
	_detail_meta_lbl.text = "Category: %s · Date: %s" % [
		String(msg.get("category", "")),
		String(msg.get("date_iso", ""))
	]
	_detail_body_lbl.text = String(msg.get("body", ""))

	# Mark read in backend (triggers inbox_updated -> refresh_inbox)
	if is_instance_valid(career_state):
		career_state.mark_inbox_read(m_id)

	# Normalize Action Route & Bind Button
	var raw_route: String = String(msg.get("action_route", "")).strip_edges().to_lower()
	var act_route: String = "club" if raw_route == "board" else raw_route
	var target_id: int = int(msg.get("target_entity_id", -1))

	if _current_action_callable.is_valid() and _action_btn.pressed.is_connected(_current_action_callable):
		_action_btn.pressed.disconnect(_current_action_callable)
	_current_action_callable = Callable()

	if not act_route.is_empty():
		_action_btn.visible = true
		_action_btn.text = "⚡ Open %s" % act_route.capitalize()
		_current_action_callable = func() -> void:
			if act_route == "squad" and target_id > 0:
				player_profile_requested.emit(target_id, "SQUAD")
			elif act_route == "youth" and target_id > 0:
				player_profile_requested.emit(target_id, "YOUTH")
			elif act_route == "transfers" and target_id > 0:
				player_profile_requested.emit(target_id, "TRANSFER")
			else:
				route_requested.emit(act_route)
		_action_btn.pressed.connect(_current_action_callable)
	else:
		_action_btn.visible = false
