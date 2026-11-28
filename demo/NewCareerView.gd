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

const MIN_PLAYABLE_LEAGUE_CLUBS := 8

var _clubs_by_league: Dictionary = {}
var _all_registry_entries: Array[Dictionary] = []

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	_build_ui()
	refresh_clubs()

static func _normalize_league_name(raw_league: String) -> String:
	var trimmed := raw_league.strip_edges()
	if trimmed.to_lower() == "tuesday league" or trimmed.to_lower() == "premier league":
		return "Premier League"
	return trimmed

func get_all_registered_entries() -> Array[Dictionary]:
	return _all_registry_entries.duplicate(true)

func get_registered_league_club_count(league_name: String) -> int:
	var target := _normalize_league_name(league_name)
	var seen: Dictionary = {}
	for entry in _all_registry_entries:
		if String(entry.get("league", "")) != target:
			continue
		var cid := int(entry.get("club_id", 0))
		var cname := String(entry.get("club_name", "")).strip_edges().to_lower()
		var key := "id:%d" % cid if cid > 0 else "name:%s" % cname
		seen[key] = true
	return seen.size()

func refresh_clubs() -> void:
	_clubs_by_league.clear()
	_all_registry_entries.clear()

	var raw_entries: Array = []
	if is_instance_valid(career_state) and career_state.has_method("get_leagues_and_clubs"):
		raw_entries = career_state.call("get_leagues_and_clubs")
	else:
		for cid in range(1, 17):
			var info: Dictionary = career_state.get_club_info(cid) if is_instance_valid(career_state) else {}
			var cname := String(info.get("club_name", "Club %d" % cid))
			raw_entries.append({ "club_id": cid, "club_name": cname, "league": "Premier League" })

	# 1. Normalize league names and count distinct valid registered clubs per league
	#    while keeping small (< 8 club) leagues intact in _all_registry_entries.
	var seen_keys_by_league: Dictionary = {}
	var unlocked_by_league: Dictionary = {}

	for item in raw_entries:
		if not (item is Dictionary):
			continue
		var cid := int(item.get("club_id", item.get("id", 0)))
		var cname := String(item.get("club_name", item.get("club", item.get("name", "")))).strip_edges()
		var league := _normalize_league_name(String(item.get("league", "")))
		if (cid <= 0 and cname.is_empty()) or league.is_empty():
			continue

		var normalized_item: Dictionary = item.duplicate(true)
		normalized_item["club_id"] = cid
		normalized_item["club_name"] = cname if not cname.is_empty() else ("Club %d" % cid)
		normalized_item["league"] = league

		var dedup_key := "id:%d" % cid if cid > 0 else "name:%s" % cname.to_lower()
		if not seen_keys_by_league.has(league):
			seen_keys_by_league[league] = {}
			unlocked_by_league[league] = []
		if seen_keys_by_league[league].has(dedup_key):
			continue

		seen_keys_by_league[league][dedup_key] = true
		_all_registry_entries.append(normalized_item)

		if not bool(normalized_item.get("is_locked", false)):
			unlocked_by_league[league].append(normalized_item)

	# 2. Only expose leagues with >= 8 distinct valid registered clubs in the selector
	for league_key in seen_keys_by_league.keys():
		var valid_count: int = seen_keys_by_league[league_key].size()
		var selectable_clubs: Array = unlocked_by_league.get(league_key, [])
		if valid_count >= MIN_PLAYABLE_LEAGUE_CLUBS and not selectable_clubs.is_empty():
			_clubs_by_league[league_key] = selectable_clubs

	print("NewCareerView: loaded %d playable leagues (>= %d clubs) from %d total leagues: %s" % [
		_clubs_by_league.size(),
		MIN_PLAYABLE_LEAGUE_CLUBS,
		seen_keys_by_league.size(),
		", ".join(PackedStringArray(_clubs_by_league.keys()))
	])

	for league_key in _clubs_by_league.keys():
		var club_list: Array = _clubs_by_league[league_key]
		club_list.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
			var rep_a := int(a.get("reputation", 50))
			var rep_b := int(b.get("reputation", 50))
			if rep_a != rep_b:
				return rep_a > rep_b
			return String(a.get("club_name", "")) < String(b.get("club_name", ""))
		)

	_league_option.clear()
	_club_option.clear()
	var league_names: Array = _clubs_by_league.keys()
	league_names.sort()
	for league_name in league_names:
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
