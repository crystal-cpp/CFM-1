class_name CareerShell
extends Control

signal main_menu_requested()
signal match_requested()
signal promo_code_redeemed(result: Dictionary)

var new_career_view: Control
var active_save_slot: String = "career_autosave"

var match_scene: Node

signal route_changed(route_name: String)
signal player_profile_requested(player_id: int, context_mode: String)

# Core State
var career_state: CareerState

# UI Root Structure
var _header_panel: PanelContainer
var _body_hsplit: HBoxContainer
var _sidebar_panel: PanelContainer
var _sidebar_vbox: VBoxContainer
var _content_panel: PanelContainer
var _router_container: Control

# Header Widgets
var _club_badge_lbl: Label
var _club_name_lbl: Label
var _season_lbl: Label
var _date_lbl: Label
var _rank_lbl: Label
var _balance_lbl: Label
var _inbox_btn: Button
var _inbox_badge: Label
var _manager_btn: Button
var _advance_day_btn: Button
var _codes_btn: Button
var _save_btn: Button
var _menu_btn: Button
var _menu_toggle_btn: Button

# Route Buttons & Views
const ROUTES: Array[Dictionary] = [
	{ "id": "dashboard", "label": "Dashboard", "icon": "" },
	{ "id": "squad", "label": "Squad", "icon": "" },
	{ "id": "tactics", "label": "Tactics", "icon": "" },
	{ "id": "youth", "label": "Youth Academy", "icon": "" },
	{ "id": "schedule", "label": "Schedule", "icon": "" },
	{ "id": "competitions", "label": "Competitions", "icon": "" },
	{ "id": "transfers", "label": "Transfers", "icon": "" },
	{ "id": "club", "label": "Club Management", "icon": "" },
	{ "id": "finances", "label": "Finances", "icon": "" },
	{ "id": "inbox", "label": "News / Inbox", "icon": "" }
]

var _route_buttons: Dictionary = {}
var _route_views: Dictionary = {}
var _current_route: String = "dashboard"
var _sidebar_visible: bool = true

# Phase 3C, Phase 4 & Phase 5A Views
var _dashboard_view: DashboardView
var _squad_view: SquadView
var _tactics_view: TacticsView
var _youth_view: YouthView
var _schedule_view: ScheduleView
var _inbox_view: InboxView
var _player_profile_modal: PlayerProfileModal
var _competitions_view: CompetitionsView

# Overlays
var _manager_profile_modal: PanelContainer
var _redeem_code_modal: RedeemCodeModal
var _toast_label: Label
var _toast_timer: Timer

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP

	_build_ui()
	_init_career_state()
	resized.connect(_on_viewport_resized)
	_on_viewport_resized()

	navigate_to("dashboard")

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		var ke := event as InputEventKey
		if ke.keycode == KEY_ESCAPE:
			if is_instance_valid(_redeem_code_modal) and _redeem_code_modal.visible:
				_redeem_code_modal.close_modal()
				get_viewport().set_input_as_handled()
			elif is_instance_valid(_player_profile_modal) and _player_profile_modal.visible:
				_player_profile_modal.close_modal()
				get_viewport().set_input_as_handled()
			elif is_instance_valid(_manager_profile_modal) and _manager_profile_modal.visible:
				_manager_profile_modal.visible = false
				get_viewport().set_input_as_handled()
		elif ke.keycode == KEY_C and get_parent() != get_tree().root:
			var ms := _find_match_scene()
			if is_instance_valid(ms) and ms.has_method("_toggle_career_shell"):
				ms.call("_toggle_career_shell")
			else:
				visible = not visible
				if visible:
					refresh_all_views()
			get_viewport().set_input_as_handled()

func _on_viewport_resized() -> void:
	if size.x > 0.0 and size.x < 780.0 and _sidebar_visible:
		_sidebar_visible = false
		if is_instance_valid(_sidebar_panel):
			_sidebar_panel.visible = false

func _init_career_state() -> void:
	if not is_instance_valid(career_state):
		career_state = get_node_or_null("CareerState") as CareerState
		if not is_instance_valid(career_state):
			career_state = CareerState.new()
			career_state.name = "CareerState"
			add_child(career_state)
			var default_cid: int = 1001
			var entries: Array = career_state.get_leagues_and_clubs()
			for entry in entries:
				if entry is Dictionary and not bool(entry.get("is_locked", false)):
					default_cid = int(entry.get("club_id", 1001))
					break
			career_state.start_new_career(default_cid, "Manager", 2006)

	if is_instance_valid(_dashboard_view):
		_dashboard_view.career_state = career_state
	if is_instance_valid(_squad_view):
		_squad_view.career_state = career_state
	if is_instance_valid(_tactics_view):
		_tactics_view.career_state = career_state
	if is_instance_valid(_youth_view):
		_youth_view.career_state = career_state
	if is_instance_valid(_schedule_view):
		_schedule_view.career_state = career_state
	if is_instance_valid(_inbox_view):
		_inbox_view.career_state = career_state
	if is_instance_valid(_player_profile_modal):
		_player_profile_modal.career_state = career_state
	if is_instance_valid(_redeem_code_modal):
		_redeem_code_modal.career_state = career_state
	if is_instance_valid(_competitions_view):
		_competitions_view.career_state = career_state

	_bind_career_signals()
	refresh_all_views()

func _bind_career_signals() -> void:
	if not is_instance_valid(career_state):
		return

	if not career_state.date_changed.is_connected(_on_date_changed):
		career_state.date_changed.connect(_on_date_changed)
	if not career_state.balance_changed.is_connected(_on_balance_changed):
		career_state.balance_changed.connect(_on_balance_changed)
	if not career_state.inbox_updated.is_connected(_on_inbox_updated):
		career_state.inbox_updated.connect(_on_inbox_updated)
	if not career_state.standings_updated.is_connected(_on_standings_updated):
		career_state.standings_updated.connect(_on_standings_updated)
	if not career_state.squad_updated.is_connected(_on_squad_updated):
		career_state.squad_updated.connect(_on_squad_updated)
	if not career_state.tactics_updated.is_connected(_on_tactics_updated):
		career_state.tactics_updated.connect(_on_tactics_updated)
	if not career_state.youth_updated.is_connected(_on_youth_updated):
		career_state.youth_updated.connect(_on_youth_updated)
	if not career_state.career_saved.is_connected(_on_career_saved):
		career_state.career_saved.connect(_on_career_saved)
	if not career_state.career_loaded.is_connected(_on_career_loaded):
		career_state.career_loaded.connect(_on_career_loaded)

func _style_shell_button(btn: Button, bg: Color, border: Color, text_col: Color = Color.WHITE) -> void:
	var normal := StyleBoxFlat.new()
	normal.bg_color = bg
	normal.border_color = border
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(6)
	normal.content_margin_left = 8
	normal.content_margin_right = 8
	normal.content_margin_top = 4
	normal.content_margin_bottom = 4

	var hover := normal.duplicate()
	hover.bg_color = bg.lightened(0.10)
	hover.border_color = border.lightened(0.15)

	var pressed := normal.duplicate()
	pressed.bg_color = bg.darkened(0.10)

	var disabled := normal.duplicate()
	disabled.bg_color = Color(0.05, 0.07, 0.11, 0.60)
	disabled.border_color = Color(0.12, 0.16, 0.24, 0.50)
	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("pressed", pressed)
	btn.add_theme_stylebox_override("disabled", disabled)
	btn.add_theme_color_override("font_color", text_col)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)
	btn.add_theme_color_override("font_disabled_color", Color(0.45, 0.52, 0.62))

func _build_ui() -> void:
	var bg := ColorRect.new()
	bg.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	bg.color = Color(0.024, 0.035, 0.055, 1.0)
	add_child(bg)

	var main_vbox := VBoxContainer.new()
	main_vbox.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	main_vbox.add_theme_constant_override("separation", 0)
	add_child(main_vbox)

	_build_header(main_vbox)

	_body_hsplit = HBoxContainer.new()
	_body_hsplit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_body_hsplit.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_body_hsplit.add_theme_constant_override("separation", 0)
	main_vbox.add_child(_body_hsplit)

	_build_sidebar(_body_hsplit)
	_build_content_area(_body_hsplit)
	_build_manager_modal()
	_build_player_modal()
	_build_redeem_code_modal()
	_build_toast()

func _build_header(parent: Control) -> void:
	_header_panel = PanelContainer.new()
	_header_panel.custom_minimum_size = Vector2(0, 52)
	_header_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL

	var hdr_style := StyleBoxFlat.new()
	hdr_style.bg_color = Color(0.043, 0.063, 0.106, 0.98)
	hdr_style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	hdr_style.set_border_width_all(1)
	hdr_style.content_margin_left = 10
	hdr_style.content_margin_right = 10
	hdr_style.content_margin_top = 6
	hdr_style.content_margin_bottom = 6
	_header_panel.add_theme_stylebox_override("panel", hdr_style)
	parent.add_child(_header_panel)

	var hbox := HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 6)
	_header_panel.add_child(hbox)

	_menu_toggle_btn = Button.new()
	_menu_toggle_btn.text = "☰"
	_menu_toggle_btn.custom_minimum_size = Vector2(32, 32)
	_menu_toggle_btn.focus_mode = Control.FOCUS_NONE
	_style_shell_button(_menu_toggle_btn, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.30, 0.48, 0.85))
	_menu_toggle_btn.pressed.connect(_toggle_sidebar)
	hbox.add_child(_menu_toggle_btn)

	var badge_panel := PanelContainer.new()
	var b_style := StyleBoxFlat.new()
	b_style.bg_color = Color(0.06, 0.45, 0.32, 0.45)
	b_style.border_color = Color(0.11, 0.82, 0.63, 0.92)
	b_style.set_border_width_all(1)
	b_style.set_corner_radius_all(6)
	b_style.set_content_margin_all(4)
	badge_panel.add_theme_stylebox_override("panel", b_style)
	_club_badge_lbl = Label.new()
	_club_badge_lbl.text = "ARS"
	_club_badge_lbl.add_theme_color_override("font_color", Color(0.35, 0.95, 0.55))
	_club_badge_lbl.add_theme_font_size_override("font_size", 11)
	badge_panel.add_child(_club_badge_lbl)
	hbox.add_child(badge_panel)

	_club_name_lbl = Label.new()
	_club_name_lbl.text = "Arsenal"
	_club_name_lbl.add_theme_color_override("font_color", Color.WHITE)
	_club_name_lbl.add_theme_font_size_override("font_size", 15)
	hbox.add_child(_club_name_lbl)

	hbox.add_child(VSeparator.new())

	_season_lbl = _create_pill("2006/07", Color(0.20, 0.45, 0.75), Color(0.65, 0.82, 1.0))
	hbox.add_child(_season_lbl)

	_date_lbl = _create_pill("2006-07-15", Color(0.18, 0.30, 0.48), Color(0.85, 0.92, 1.0))
	hbox.add_child(_date_lbl)

	_rank_lbl = _create_pill("Rank #1", Color(0.65, 0.48, 0.12), Color(1.0, 0.85, 0.30))
	hbox.add_child(_rank_lbl)

	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox.add_child(spacer)

	_balance_lbl = _create_pill("$50.0M", Color(0.12, 0.62, 0.35), Color(0.30, 0.98, 0.65))
	hbox.add_child(_balance_lbl)

	_advance_day_btn = Button.new()
	_advance_day_btn.text = "+1 Day"
	_advance_day_btn.custom_minimum_size = Vector2(78, 32)
	_advance_day_btn.focus_mode = Control.FOCUS_NONE
	_style_shell_button(_advance_day_btn, Color(0.08, 0.24, 0.42, 0.95), Color(0.22, 0.52, 0.85, 0.90))
	_advance_day_btn.pressed.connect(_on_advance_day_pressed)
	hbox.add_child(_advance_day_btn)

	_codes_btn = Button.new()
	_codes_btn.text = "Codes"
	_codes_btn.custom_minimum_size = Vector2(78, 32)
	_codes_btn.focus_mode = Control.FOCUS_NONE
	_style_shell_button(_codes_btn, Color(0.18, 0.14, 0.34, 0.92), Color(0.55, 0.40, 0.95, 0.88), Color(0.88, 0.82, 1.0))
	_codes_btn.pressed.connect(open_redeem_code_modal)
	hbox.add_child(_codes_btn)

	_save_btn = Button.new()
	_save_btn.text = "Save"
	_save_btn.custom_minimum_size = Vector2(68, 32)
	_save_btn.focus_mode = Control.FOCUS_NONE
	_style_shell_button(_save_btn, Color(0.06, 0.45, 0.32, 0.95), Color(0.15, 0.92, 0.58, 0.90))
	_save_btn.pressed.connect(_on_save_pressed)
	hbox.add_child(_save_btn)

	var inbox_container := Control.new()
	inbox_container.custom_minimum_size = Vector2(80, 32)

	_inbox_btn = Button.new()
	_inbox_btn.text = "Inbox"
	_inbox_btn.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_inbox_btn.focus_mode = Control.FOCUS_NONE
	_style_shell_button(_inbox_btn, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.30, 0.48, 0.85))
	_inbox_btn.pressed.connect(func() -> void: navigate_to("inbox"))
	inbox_container.add_child(_inbox_btn)

	_inbox_badge = Label.new()
	_inbox_badge.text = "1"
	_inbox_badge.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_inbox_badge.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	var ib_style := StyleBoxFlat.new()
	ib_style.bg_color = Color(0.92, 0.22, 0.28, 0.98)
	ib_style.set_corner_radius_all(8)
	ib_style.content_margin_left = 4
	ib_style.content_margin_right = 4
	ib_style.content_margin_top = 1
	ib_style.content_margin_bottom = 1
	_inbox_badge.add_theme_stylebox_override("normal", ib_style)
	_inbox_badge.add_theme_color_override("font_color", Color.WHITE)
	_inbox_badge.add_theme_font_size_override("font_size", 10)
	_inbox_badge.position = Vector2(62, -4)
	inbox_container.add_child(_inbox_badge)
	hbox.add_child(inbox_container)

	_manager_btn = Button.new()
	_manager_btn.text = "Manager"
	_manager_btn.custom_minimum_size = Vector2(92, 32)
	_manager_btn.clip_text = true
	_manager_btn.focus_mode = Control.FOCUS_NONE
	_style_shell_button(_manager_btn, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.30, 0.48, 0.85))
	_manager_btn.pressed.connect(_open_manager_profile)
	hbox.add_child(_manager_btn)

	_menu_btn = Button.new()
	_menu_btn.text = "Menu"
	_menu_btn.custom_minimum_size = Vector2(74, 32)
	_menu_btn.focus_mode = Control.FOCUS_NONE
	_style_shell_button(_menu_btn, Color(0.16, 0.09, 0.14, 0.95), Color(0.52, 0.24, 0.34, 0.88))
	_menu_btn.pressed.connect(func() -> void: main_menu_requested.emit())
	hbox.add_child(_menu_btn)

func _create_pill(text: String, bg_color: Color, text_color: Color) -> Label:
	var lbl := Label.new()
	lbl.text = text
	var style := StyleBoxFlat.new()
	style.bg_color = bg_color.lerp(Color.BLACK, 0.35)
	style.border_color = bg_color
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.content_margin_left = 7
	style.content_margin_right = 7
	style.content_margin_top = 3
	style.content_margin_bottom = 3
	lbl.add_theme_stylebox_override("normal", style)
	lbl.add_theme_color_override("font_color", text_color)
	lbl.add_theme_font_size_override("font_size", 11)
	return lbl

func _build_sidebar(parent: Control) -> void:
	_sidebar_panel = PanelContainer.new()
	_sidebar_panel.custom_minimum_size = Vector2(210, 0)
	_sidebar_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var sb_style := StyleBoxFlat.new()
	sb_style.bg_color = Color(0.036, 0.052, 0.088, 0.98)
	sb_style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	sb_style.border_width_right = 1
	sb_style.content_margin_left = 10
	sb_style.content_margin_right = 10
	sb_style.content_margin_top = 14
	sb_style.content_margin_bottom = 14
	_sidebar_panel.add_theme_stylebox_override("panel", sb_style)
	parent.add_child(_sidebar_panel)

	var scroll := ScrollContainer.new()
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_sidebar_panel.add_child(scroll)

	_sidebar_vbox = VBoxContainer.new()
	_sidebar_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_sidebar_vbox.add_theme_constant_override("separation", 6)
	scroll.add_child(_sidebar_vbox)

	var nav_label := Label.new()
	nav_label.text = "NAVIGATION"
	nav_label.add_theme_color_override("font_color", Color(0.65, 0.72, 0.82))
	nav_label.add_theme_font_size_override("font_size", 11)
	_sidebar_vbox.add_child(nav_label)

	for route in ROUTES:
		var r_id: String = route["id"]
		var r_label: String = route["label"]
		var r_icon: String = route["icon"]

		var btn := Button.new()
		btn.text = ("  %s" % r_label) if r_icon.is_empty() else (" %s  %s" % [r_icon, r_label])
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.custom_minimum_size = Vector2(190, 40)
		btn.focus_mode = Control.FOCUS_NONE
		_style_shell_button(btn, Color(0.055, 0.082, 0.135, 0.90), Color(0.12, 0.20, 0.32, 0.80), Color(0.85, 0.90, 0.96))

		btn.pressed.connect(func() -> void: navigate_to(r_id))
		_sidebar_vbox.add_child(btn)
		_route_buttons[r_id] = btn

func _build_content_area(parent: Control) -> void:
	_content_panel = PanelContainer.new()
	_content_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_content_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var cp_style := StyleBoxEmpty.new()
	_content_panel.add_theme_stylebox_override("panel", cp_style)
	parent.add_child(_content_panel)

	_router_container = Control.new()
	_router_container.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_router_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_router_container.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_content_panel.add_child(_router_container)

	for route in ROUTES:
		var r_id: String = route["id"]
		var view: Control = _create_route_view(r_id, route["label"])
		view.visible = false
		_router_container.add_child(view)
		_route_views[r_id] = view

func _create_route_view(route_id: String, route_title: String) -> Control:
	match route_id:
		"dashboard":
			_dashboard_view = DashboardView.new()
			_dashboard_view.route_requested.connect(navigate_to)
			_dashboard_view.player_profile_requested.connect(open_player_profile)
			_dashboard_view.match_launch_requested.connect(_on_pre_match_requested)
			return _dashboard_view

		"squad":
			_squad_view = SquadView.new()
			_squad_view.route_requested.connect(navigate_to)
			_squad_view.player_profile_requested.connect(open_player_profile)
			return _squad_view

		"tactics":
			_tactics_view = TacticsView.new()
			_tactics_view.tactics_saved.connect(func(form: String) -> void:
				show_toast("Club tactics updated (%s)" % form, 2.0)
				if is_instance_valid(_squad_view):
					_squad_view.refresh_squad()
			)
			return _tactics_view

		"competitions":
			_competitions_view = CompetitionsView.new()
			_competitions_view.route_requested.connect(navigate_to)
			_competitions_view.player_profile_requested.connect(open_player_profile)
			return _competitions_view

		"youth":
			_youth_view = YouthView.new()
			_youth_view.player_profile_requested.connect(open_player_profile)
			return _youth_view

		"schedule":
			_schedule_view = ScheduleView.new()
			_schedule_view.route_requested.connect(navigate_to)
			_schedule_view.match_launch_requested.connect(_on_pre_match_requested)
			return _schedule_view

		"inbox":
			_inbox_view = InboxView.new()
			_inbox_view.route_requested.connect(navigate_to)
			_inbox_view.player_profile_requested.connect(open_player_profile)
			return _inbox_view

		_:
			return _create_placeholder_panel(route_id, route_title)

func _create_placeholder_panel(route_id: String, route_title: String) -> Control:
	var panel := PanelContainer.new()
	panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	panel.size_flags_vertical = Control.SIZE_EXPAND_FILL

	var p_style := StyleBoxFlat.new()
	p_style.bg_color = Color(0.024, 0.035, 0.055, 1.0)
	p_style.content_margin_left = 20
	p_style.content_margin_right = 20
	p_style.content_margin_top = 16
	p_style.content_margin_bottom = 16
	panel.add_theme_stylebox_override("panel", p_style)

	var scroll := ScrollContainer.new()
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	panel.add_child(scroll)

	var vbox := VBoxContainer.new()
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.add_theme_constant_override("separation", 14)
	scroll.add_child(vbox)

	var title_lbl := Label.new()
	title_lbl.text = route_title.to_upper()
	title_lbl.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title_lbl.add_theme_font_size_override("font_size", 20)
	vbox.add_child(title_lbl)

	vbox.add_child(HSeparator.new())
	vbox.add_child(_build_route_specific_content(route_id))

	return panel

func _build_route_specific_content(route_id: String) -> Control:
	var container := VBoxContainer.new()
	container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	container.add_theme_constant_override("separation", 12)

	match route_id:
		"transfers":
			container.add_child(_create_info_card("SCOUTING & TRANSFER DESK", "Player market valuations, shortlist tracking, and transfer window negotiations."))
		"club":
			container.add_child(_create_info_card("BOARDROOM & CLUB OPERATIONS", "Season board objectives, stadium infrastructure, training facilities, and job confidence."))
		"finances":
			container.add_child(_create_info_card("CLUB FINANCIAL LEDGER", "Total club balance, transfer budget allocation, wage structure, and matchday revenue."))

	return container

func _create_info_card(header: String, body: String) -> PanelContainer:
	var pc := PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.043, 0.063, 0.106, 0.95)
	style.border_color = Color(0.12, 0.20, 0.32, 0.85)
	style.set_border_width_all(1)
	style.set_corner_radius_all(10)
	style.content_margin_left = 16
	style.content_margin_right = 16
	style.content_margin_top = 14
	style.content_margin_bottom = 14
	pc.add_theme_stylebox_override("panel", style)

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 6)
	pc.add_child(vb)

	var h := Label.new()
	h.text = header
	h.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	h.add_theme_font_size_override("font_size", 14)
	vb.add_child(h)

	var b := Label.new()
	b.text = body
	b.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	b.add_theme_color_override("font_color", Color(0.72, 0.80, 0.90))
	b.add_theme_font_size_override("font_size", 12)
	vb.add_child(b)

	return pc

func _build_manager_modal() -> void:
	_manager_profile_modal = PanelContainer.new()
	_manager_profile_modal.anchor_left = 0.5
	_manager_profile_modal.anchor_top = 0.5
	_manager_profile_modal.anchor_right = 0.5
	_manager_profile_modal.anchor_bottom = 0.5
	_manager_profile_modal.offset_left = -210.0
	_manager_profile_modal.offset_top = -170.0
	_manager_profile_modal.offset_right = 210.0
	_manager_profile_modal.offset_bottom = 170.0
	_manager_profile_modal.grow_horizontal = Control.GROW_DIRECTION_BOTH
	_manager_profile_modal.grow_vertical = Control.GROW_DIRECTION_BOTH
	_manager_profile_modal.custom_minimum_size = Vector2(420, 320)
	_manager_profile_modal.visible = false

	var modal_style := StyleBoxFlat.new()
	modal_style.bg_color = Color(0.043, 0.063, 0.106, 0.98)
	modal_style.border_color = Color(0.15, 0.92, 0.58, 0.90)
	modal_style.set_border_width_all(2)
	modal_style.set_corner_radius_all(10)
	modal_style.set_content_margin_all(22)
	_manager_profile_modal.add_theme_stylebox_override("panel", modal_style)
	add_child(_manager_profile_modal)

	var vb := VBoxContainer.new()
	vb.add_theme_constant_override("separation", 10)
	_manager_profile_modal.add_child(vb)

	var title := Label.new()
	title.text = "MANAGER PROFILE"
	title.add_theme_color_override("font_color", Color(0.11, 0.82, 0.63))
	title.add_theme_font_size_override("font_size", 16)
	vb.add_child(title)

	vb.add_child(HSeparator.new())

	var details := Label.new()
	details.name = "DetailsLabel"
	details.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	details.size_flags_vertical = Control.SIZE_EXPAND_FILL
	details.add_theme_color_override("font_color", Color(0.85, 0.92, 1.0))
	details.add_theme_font_size_override("font_size", 13)
	vb.add_child(details)

	var close_btn := Button.new()
	close_btn.text = "Close"
	close_btn.custom_minimum_size = Vector2(110, 34)
	close_btn.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	close_btn.focus_mode = Control.FOCUS_NONE
	_style_shell_button(close_btn, Color(0.07, 0.12, 0.20, 0.95), Color(0.18, 0.30, 0.48, 0.85))
	close_btn.pressed.connect(func() -> void: _manager_profile_modal.visible = false)
	vb.add_child(close_btn)

func _open_manager_profile() -> void:
	if not is_instance_valid(career_state):
		return

	var mgr: Dictionary = career_state.get_manager_profile()
	var details_lbl: Label = _manager_profile_modal.find_child("DetailsLabel", true, false) as Label
	if details_lbl:
		details_lbl.text = "Name: %s\nNationality: %s\nPreferred Formation: %s\nReputation: %d/100\nBoard Confidence: %d%%\nSeasons Managed: %d\nTrophies: %d\nCareer Record: %dW - %dD - %dL" % [
			mgr.get("manager_name", "Manager"),
			mgr.get("nationality", "Vietnam"),
			mgr.get("preferred_formation", "4-3-3"),
			mgr.get("reputation", 50),
			mgr.get("board_confidence", 75),
			mgr.get("seasons_managed", 0),
			mgr.get("trophies_won", 0),
			mgr.get("career_wins", 0),
			mgr.get("career_draws", 0),
			mgr.get("career_losses", 0)
		]
	_manager_profile_modal.visible = true

func _build_player_modal() -> void:
	_player_profile_modal = PlayerProfileModal.new()
	_player_profile_modal.name = "PlayerProfileModal"
	_player_profile_modal.action_triggered.connect(_on_player_modal_action)
	add_child(_player_profile_modal)

func _build_redeem_code_modal() -> void:
	_redeem_code_modal = RedeemCodeModal.new()
	_redeem_code_modal.name = "RedeemCodeModal"
	_redeem_code_modal.code_redeemed.connect(_on_promo_code_redeemed)
	add_child(_redeem_code_modal)

func open_redeem_code_modal() -> void:
	if not is_instance_valid(_redeem_code_modal):
		_build_redeem_code_modal()
	_redeem_code_modal.career_state = career_state
	_redeem_code_modal.open_modal()

func _on_promo_code_redeemed(result: Dictionary) -> void:
	show_toast(String(result.get("message", "Promo code redeemed!")), 3.0)
	refresh_all_views()
	if is_instance_valid(new_career_view) and new_career_view.has_method("refresh_clubs"):
		new_career_view.call("refresh_clubs")
	var tree := get_tree()
	if is_instance_valid(tree):
		for node in tree.get_nodes_in_group("new_career_view"):
			if is_instance_valid(node) and node.has_method("refresh_clubs"):
				node.call("refresh_clubs")
	promo_code_redeemed.emit(result)

func open_player_profile(player_id: int, context_mode: String = "SQUAD") -> void:
	if not is_instance_valid(_player_profile_modal):
		_build_player_modal()
	_player_profile_modal.career_state = career_state
	_player_profile_modal.open_player(player_id, context_mode)
	player_profile_requested.emit(player_id, context_mode)

func _on_player_modal_action(action_name: String, player_id: int) -> void:
	if not is_instance_valid(career_state):
		return

	match action_name:
		"promote_youth":
			var ok: bool = career_state.promote_youth_player(player_id)
			if ok:
				show_toast("Promoted prospect to senior squad!", 2.5)
				refresh_all_views()
			else:
				show_toast("Failed to promote player.", 2.0)

		"toggle_starter":
			show_toast("Select a teammate in Squad Management to swap roles.", 2.0)
			navigate_to("squad")

		"renew_contract":
			show_toast("Contract negotiations queued for Phase 7.", 2.0)

		"make_offer":
			show_toast("Opening Transfer Market...", 1.8)
			navigate_to("transfers")

		"inspect_tactics":
			navigate_to("tactics")

func _build_toast() -> void:
	_toast_label = Label.new()
	_toast_label.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	_toast_label.offset_left = 240
	_toast_label.offset_right = -240
	_toast_label.offset_bottom = -18
	_toast_label.offset_top = -54
	_toast_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_toast_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER

	var t_style := StyleBoxFlat.new()
	t_style.bg_color = Color(0.043, 0.063, 0.106, 0.97)
	t_style.border_color = Color(0.15, 0.92, 0.58, 0.90)
	t_style.set_border_width_all(1)
	t_style.set_corner_radius_all(10)
	_toast_label.add_theme_stylebox_override("normal", t_style)
	_toast_label.add_theme_color_override("font_color", Color(0.35, 0.95, 0.55))
	_toast_label.add_theme_font_size_override("font_size", 13)
	_toast_label.visible = false
	add_child(_toast_label)

	_toast_timer = Timer.new()
	_toast_timer.one_shot = true
	_toast_timer.timeout.connect(func() -> void: _toast_label.visible = false)
	add_child(_toast_timer)

func show_toast(msg: String, duration: float = 2.5) -> void:
	_toast_label.text = msg
	_toast_label.visible = true
	_toast_timer.start(duration)

func navigate_to(route_id: String) -> void:
	var normalized: String = "club" if route_id == "board" else route_id
	if not _route_views.has(normalized):
		push_error("CareerShell: Unknown route ID '%s'" % route_id)
		return

	_current_route = normalized

	for r_id in _route_views.keys():
		var view: Control = _route_views[r_id]
		view.visible = (r_id == normalized)

	match normalized:
		"dashboard":
			if is_instance_valid(_dashboard_view):
				_dashboard_view.refresh_dashboard()
		"squad":
			if is_instance_valid(_squad_view):
				_squad_view.refresh_squad()
		"tactics":
			if is_instance_valid(_tactics_view):
				_tactics_view.refresh_tactics()
		"youth":
			if is_instance_valid(_youth_view):
				_youth_view.refresh_youth()
		"schedule":
			if is_instance_valid(_schedule_view):
				_schedule_view.refresh_schedule()
		"competitions":
			if is_instance_valid(_competitions_view):
				_competitions_view.refresh_competitions()
		"inbox":
			if is_instance_valid(_inbox_view):
				_inbox_view.refresh_inbox()

	for r_id in _route_buttons.keys():
		var btn: Button = _route_buttons[r_id]
		var is_active: bool = (r_id == normalized)
		if is_active:
			_style_shell_button(btn, Color(0.06, 0.45, 0.32, 0.95), Color(0.15, 0.92, 0.58, 0.90), Color.WHITE)
		else:
			_style_shell_button(btn, Color(0.055, 0.082, 0.135, 0.90), Color(0.12, 0.20, 0.32, 0.80), Color(0.85, 0.90, 0.96))

	route_changed.emit(normalized)

func get_current_route() -> String:
	return _current_route

func _toggle_sidebar() -> void:
	_sidebar_visible = not _sidebar_visible
	_sidebar_panel.visible = _sidebar_visible

func _normalize_season_2006(s: String) -> String:
	if s.begins_with("2026/"):
		return "2006" + s.substr(4)
	return s

func _normalize_date_2006(d: String) -> String:
	if d.begins_with("2026-"):
		return "2006-" + d.substr(5)
	return d

func refresh_header() -> void:
	if not is_instance_valid(career_state) or not career_state.is_career_active():
		return

	var summary: Dictionary = career_state.get_header_summary()

	_club_name_lbl.text = String(summary.get("managed_club_name", "My Club"))
	var club_name: String = _club_name_lbl.text
	var short_code: String = String(summary.get("short_code", ""))
	_club_badge_lbl.text = short_code if not short_code.is_empty() else club_name.substr(0, 3).to_upper()

	_season_lbl.text = _normalize_season_2006(String(summary.get("season_label", "2006/07")))
	_date_lbl.text = _normalize_date_2006(String(summary.get("current_date", "2006-07-15")))
	_rank_lbl.text = "Rank #%d" % int(summary.get("league_rank", 1))

	var balance: int = int(summary.get("total_balance", 0))
	_balance_lbl.text = _format_currency(balance)

	var unread: int = int(summary.get("unread_inbox_count", 0))
	_inbox_badge.text = str(unread)
	_inbox_badge.visible = (unread > 0)

	var mgr_name: String = String(summary.get("manager_name", "Manager"))
	_manager_btn.text = "%s" % mgr_name

func refresh_all_views() -> void:
	refresh_header()
	if is_instance_valid(_dashboard_view):
		_dashboard_view.refresh_dashboard()
	if is_instance_valid(_squad_view):
		_squad_view.refresh_squad()
	if is_instance_valid(_tactics_view):
		_tactics_view.refresh_tactics()
	if is_instance_valid(_youth_view):
		_youth_view.refresh_youth()
	if is_instance_valid(_schedule_view):
		_schedule_view.refresh_schedule()
	if is_instance_valid(_competitions_view):
		_competitions_view.refresh_competitions()
	if is_instance_valid(_inbox_view):
		_inbox_view.refresh_inbox()

func _format_currency(amount: int) -> String:
	if abs(amount) >= 1_000_000:
		return "$%.1fM" % (float(amount) / 1_000_000.0)
	elif abs(amount) >= 1_000:
		return "$%.1fK" % (float(amount) / 1_000.0)
	else:
		return "$%d" % amount

func _on_date_changed(new_date_iso: String) -> void:
	_date_lbl.text = _normalize_date_2006(new_date_iso)
	if is_instance_valid(career_state):
		_season_lbl.text = _normalize_season_2006(career_state.get_season_label())
	refresh_all_views()

func _on_balance_changed(total_balance: int, _transfer_budget: int, _wage_budget: int) -> void:
	_balance_lbl.text = _format_currency(total_balance)
	if is_instance_valid(_dashboard_view):
		_dashboard_view.refresh_dashboard()
	if is_instance_valid(_youth_view):
		_youth_view.refresh_youth()

func _on_inbox_updated(unread_count: int) -> void:
	_inbox_badge.text = str(unread_count)
	_inbox_badge.visible = (unread_count > 0)
	if is_instance_valid(_inbox_view):
		_inbox_view.refresh_inbox()
	if is_instance_valid(_dashboard_view):
		_dashboard_view.refresh_dashboard()

func _on_standings_updated() -> void:
	if is_instance_valid(career_state):
		_rank_lbl.text = "Rank #%d" % career_state.get_managed_club_league_rank()
	if is_instance_valid(_dashboard_view):
		_dashboard_view.refresh_dashboard()
	if is_instance_valid(_schedule_view):
		_schedule_view.refresh_schedule()
	if is_instance_valid(_competitions_view):
		_competitions_view.refresh_competitions()

func _on_squad_updated(_club_id: int) -> void:
	if is_instance_valid(_squad_view):
		_squad_view.refresh_squad()
	if is_instance_valid(_dashboard_view):
		_dashboard_view.refresh_dashboard()

func _on_tactics_updated(_club_id: int) -> void:
	if is_instance_valid(_tactics_view):
		_tactics_view.refresh_tactics()
	if is_instance_valid(_squad_view):
		_squad_view.refresh_squad()

func _on_youth_updated() -> void:
	if is_instance_valid(_youth_view):
		_youth_view.refresh_youth()

func _on_career_saved(slot_name: String) -> void:
	show_toast("Career saved to slot '%s'" % slot_name, 2.0)

func _on_career_loaded(slot_name: String) -> void:
	refresh_all_views()
	show_toast("Career loaded from slot '%s'" % slot_name, 2.0)

func _on_advance_day_pressed() -> void:
	if is_instance_valid(career_state):
		career_state.advance_day(1)
		show_toast("Advanced 1 Day: %s" % _normalize_date_2006(career_state.get_current_date_iso()), 1.5)

func _on_save_pressed() -> void:
	if is_instance_valid(career_state):
		var slot := active_save_slot.strip_edges()
		if slot.is_empty():
			slot = "career_autosave"
		career_state.save_career(slot)

func _find_match_scene() -> Node:
	if is_instance_valid(match_scene) and match_scene.has_method("open_pre_match_center"):
		return match_scene

	# Prefer an ancestor so nested CareerShell instances resolve their owner.
	var node: Node = get_parent()
	while is_instance_valid(node):
		if node.has_method("open_pre_match_center"):
			return node
		node = node.get_parent()

	# Fallback for scenes where CareerShell is instanced outside MatchScene's
	# direct hierarchy (for example, under a separate UI/root container).
	var tree := get_tree()
	if is_instance_valid(tree):
		var grouped_scene := tree.get_first_node_in_group("match_scene")
		if is_instance_valid(grouped_scene) and grouped_scene.has_method("open_pre_match_center"):
			return grouped_scene
		var current := tree.current_scene
		if is_instance_valid(current) and current.has_method("open_pre_match_center"):
			return current
	return null

func _on_pre_match_requested() -> void:
	var ms := _find_match_scene()
	if is_instance_valid(ms):
		if ms.open_pre_match_center(career_state):
			visible = false
			match_requested.emit()
		else:
			show_toast("Please forward to upcoming fixtures.", 2.5)
	else:
		show_toast("Match is not available. Ensure the active match is loaded.", 2.5)
