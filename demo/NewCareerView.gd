class_name NewCareerView
extends Control

signal career_created(club_id: int, manager_name: String, slot_name: String)
signal back_requested()

var career_state: CareerState

var _league_option: OptionButton
var _club_option: OptionButton
var _manager_input: LineEdit
var _slot_input: LineEdit
var _error_lbl: Label

var _clubs_by_league: Dictionary = {}

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build_ui()
	refresh_clubs()

func refresh_clubs() -> void:
	_clubs_by_league.clear()
	if is_instance_valid(career_state) and career_state.has_method("get_leagues_and_clubs"):
		var entries: Array = career_state.call("get_leagues_and_clubs")
		for item in entries:
			if item is Dictionary and not bool(item.get("is_locked", false)):
				var league := String(item.get("league", "Premier Division"))
				if not _clubs_by_league.has(league):
					_clubs_by_league[league] = []
				_clubs_by_league[league].append(item)
	else:
		# Never fabricate club IDs or clubs when the authoritative registry is unavailable.
		push_warning("NewCareerView: CareerState club registry is unavailable; no clubs can be selected.")

	_league_option.clear()
	for league_name in _clubs_by_league.keys():
		_league_option.add_item(String(league_name))

	if _league_option.item_count > 0:
		_league_option.select(0)
		_on_league_selected(0)

func _on_league_selected(index: int) -> void:
	_club_option.clear()
	if index < 0 or index >= _league_option.item_count:
		return
	var league_name := _league_option.get_item_text(index)
	var clubs: Array = _clubs_by_league.get(league_name, [])
	for c in clubs:
		if not (c is Dictionary):
			continue
		var cid := int(c.get("club_id", 0))
		if cid <= 0:
			continue
		var cname := String(c.get("club", c.get("club_name", "")))
		if cname.strip_edges().is_empty():
			cname = "Club %d" % cid
		_club_option.add_item(cname, cid)
	if _club_option.item_count > 0:
		_club_option.select(0)

func _on_create_pressed() -> void:
	_error_lbl.visible = false
	var mgr := _manager_input.text.strip_edges()
	if mgr.is_empty():
		mgr = "Manager"

	var slot := _slot_input.text.strip_edges().to_snake_case()
	if slot.is_empty():
		slot = "career_slot_1"

	if _club_option.item_count == 0:
		_error_lbl.text = "No playable club is selected."
		_error_lbl.visible = true
		return

	var club_id := _club_option.get_selected_id()
	career_created.emit(club_id, mgr, slot)

func _build_ui() -> void:
	var bg := ColorRect.new()
	bg.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.024, 0.035, 0.055, 1.0)
	add_child(bg)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var card := PanelContainer.new()
	card.custom_minimum_size = Vector2(480, 0)
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.043, 0.063, 0.106, 0.98)
	style.border_color = Color(0.11, 0.16, 0.25, 1.0)
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(26)
	card.add_theme_stylebox_override("panel", style)
	center.add_child(card)

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 12)
	card.add_child(vb)

	var title := Label.new()
	title.text = "NEW CAREER SETUP"
	title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title.add_theme_font_size_override("font_size", 20)
	vb.add_child(title)

	vb.add_child(_make_field_label("1. SELECT LEAGUE"))
	_league_option = OptionButton.new()
	_league_option.custom_minimum_size = Vector2(0, 38)
	_league_option.item_selected.connect(_on_league_selected)
	vb.add_child(_league_option)

	vb.add_child(_make_field_label("2. SELECT CLUB"))
	_club_option = OptionButton.new()
	_club_option.custom_minimum_size = Vector2(0, 38)
	vb.add_child(_club_option)

	vb.add_child(_make_field_label("3. MANAGER NAME"))
	_manager_input = LineEdit.new()
	_manager_input.placeholder_text = "Enter Manager Name"
	_manager_input.text = "Manager"
	_manager_input.custom_minimum_size = Vector2(0, 38)
	vb.add_child(_manager_input)

	vb.add_child(_make_field_label("4. SAVE SLOT NAME"))
	_slot_input = LineEdit.new()
	_slot_input.placeholder_text = "career_slot_1"
	_slot_input.text = "career_slot_1"
	_slot_input.custom_minimum_size = Vector2(0, 38)
	vb.add_child(_slot_input)

	_error_lbl = Label.new()
	_error_lbl.visible = false
	_error_lbl.add_theme_color_override("font_color", Color(1.0, 0.45, 0.45))
	_error_lbl.add_theme_font_size_override("font_size", 12)
	vb.add_child(_error_lbl)

	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 12)
	vb.add_child(row)

	var back_btn := Button.new()
	back_btn.text = "Back"
	back_btn.custom_minimum_size = Vector2(120, 42)
	back_btn.pressed.connect(func() -> void: back_requested.emit())
	row.add_child(back_btn)

	var start_btn := Button.new()
	start_btn.text = "Start Career"
	start_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	start_btn.custom_minimum_size = Vector2(0, 42)
	start_btn.pressed.connect(_on_create_pressed)
	row.add_child(start_btn)

func _make_field_label(text_str: String) -> Label:
	var lbl := Label.new()
	lbl.text = text_str
	lbl.add_theme_color_override("font_color", Color(0.62, 0.72, 0.86))
	lbl.add_theme_font_size_override("font_size", 11)
	return lbl