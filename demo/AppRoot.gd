class_name AppRoot
extends Control

const MATCH_SCENE_PATH := "res://match_scene.tscn"

var career_state: CareerState
var match_scene: Node
var career_shell: CareerShell
var main_menu: MainMenuView
var new_career_view: NewCareerView

var _active_slot_name: String = "career_autosave"
var _has_unsaved_changes: bool = false
var _unsaved_dialog: ConfirmationDialog

func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)

	career_state = CareerState.new()
	career_state.name = "CareerState"
	add_child(career_state)
	_connect_dirty_tracking()

	var packed_match := load(MATCH_SCENE_PATH) as PackedScene
	if packed_match:
		match_scene = packed_match.instantiate()
		match_scene.name = "MatchScene"
		add_child(match_scene)
		_set_match_scene_visible(false)
	else:
		push_error("AppRoot: Failed to load %s" % MATCH_SCENE_PATH)

	career_shell = CareerShell.new()
	career_shell.name = "CareerShell"
	career_shell.career_state = career_state
	career_shell.visible = false
	add_child(career_shell)

	if is_instance_valid(match_scene):
		if "match_scene" in career_shell:
			career_shell.match_scene = match_scene
		if "career_shell" in match_scene:
			match_scene.career_shell = career_shell

	if career_shell.has_signal("main_menu_requested"):
		career_shell.main_menu_requested.connect(_on_career_shell_main_menu_requested)

	main_menu = MainMenuView.new()
	main_menu.name = "MainMenuView"
	main_menu.career_state = career_state
	main_menu.new_career_requested.connect(_open_new_career_wizard)
	main_menu.continue_career_requested.connect(_load_and_enter_career)
	main_menu.load_career_requested.connect(_load_and_enter_career)
	main_menu.exit_requested.connect(func() -> void: get_tree().quit())
	add_child(main_menu)

	new_career_view = NewCareerView.new()
	new_career_view.name = "NewCareerView"
	new_career_view.career_state = career_state
	new_career_view.visible = false
	new_career_view.career_created.connect(_on_new_career_created)
	new_career_view.back_requested.connect(_show_main_menu)
	add_child(new_career_view)

	_build_unsaved_dialog()
	_show_main_menu()

func _connect_dirty_tracking() -> void:
	career_state.date_changed.connect(func(_d: String) -> void: _has_unsaved_changes = true)
	career_state.squad_updated.connect(func(_c: int) -> void: _has_unsaved_changes = true)
	career_state.tactics_updated.connect(func(_c: int) -> void: _has_unsaved_changes = true)
	career_state.youth_updated.connect(func() -> void: _has_unsaved_changes = true)
	career_state.career_saved.connect(func(_s: String) -> void: _has_unsaved_changes = false)
	career_state.career_loaded.connect(func(_s: String) -> void: _has_unsaved_changes = false)

func _build_unsaved_dialog() -> void:
	_unsaved_dialog = ConfirmationDialog.new()
	_unsaved_dialog.title = "Unsaved Career Progress"
	_unsaved_dialog.dialog_text = "You have unsaved progress. Save before returning to the Main Menu?"
	_unsaved_dialog.ok_button_text = "Save & Exit"
	_unsaved_dialog.cancel_button_text = "Cancel"
	_unsaved_dialog.add_button("Discard Changes", true, "discard")
	_unsaved_dialog.confirmed.connect(func() -> void:
		career_state.save_career(_active_slot_name)
		_show_main_menu()
	)
	_unsaved_dialog.custom_action.connect(func(action: StringName) -> void:
		if action == &"discard":
			_unsaved_dialog.hide()
			_show_main_menu()
	)
	add_child(_unsaved_dialog)

func _set_match_scene_visible(show_match: bool) -> void:
	if not is_instance_valid(match_scene):
		return
	var layer := match_scene.get_node_or_null("2DOverviewLayer") as CanvasLayer
	if is_instance_valid(layer):
		layer.visible = show_match

func _show_main_menu() -> void:
	_set_match_scene_visible(false)
	career_shell.visible = false
	new_career_view.visible = false
	main_menu.show_error("")
	main_menu.refresh_save_state()
	main_menu.visible = true

func _open_new_career_wizard() -> void:
	main_menu.visible = false
	career_shell.visible = false
	new_career_view.refresh_clubs()
	new_career_view.visible = true

func _on_new_career_created(club_id: int, manager_name: String, slot_name: String) -> void:
	var start_year := 2006 if career_state.has_method("get_leagues_and_clubs") else 2026
	if not career_state.start_new_career(club_id, manager_name, start_year):
		new_career_view._error_lbl.text = "Failed to initialize career for the selected club."
		new_career_view._error_lbl.visible = true
		return

	_active_slot_name = slot_name
	career_shell.active_save_slot = slot_name
	career_state.save_career(_active_slot_name)
	_has_unsaved_changes = false
	_enter_career_dashboard()

func _load_and_enter_career(slot_name: String) -> void:
	if not career_state.load_career(slot_name):
		main_menu.show_error("Could not load save '%s'. The file may be missing or incompatible." % slot_name)
		main_menu.refresh_save_state()
		return

	_active_slot_name = slot_name
	career_shell.active_save_slot = slot_name
	_has_unsaved_changes = false
	_enter_career_dashboard()

func _enter_career_dashboard() -> void:
	main_menu.visible = false
	new_career_view.visible = false
	_set_match_scene_visible(false)
	career_shell.career_state = career_state
	career_shell.visible = true
	career_shell.refresh_all_views()
	career_shell.navigate_to("dashboard")

func _on_career_shell_main_menu_requested() -> void:
	if _has_unsaved_changes and career_state.is_career_active():
		_unsaved_dialog.popup_centered()
	else:
		_show_main_menu()
