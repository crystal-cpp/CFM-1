extends Node3D

@onready var match_engine: MatchEngine = $MatchEngine
@onready var overview_2d: CanvasLayer = $"2DOverviewLayer"
@onready var highlight_3d: Node3D = $"3DHighlightLayer"

func _ready() -> void:
	if not match_engine or not overview_2d or not highlight_3d:
		push_error("MatchScene: Missing child nodes! Check node hierarchy.")
		return

	# Ensure default view state
	overview_2d.show()
	highlight_3d.hide()

	# Connect C++ MatchEngine signals
	match_engine.interactive_event_triggered.connect(_on_interactive_event_triggered)
	match_engine.match_ended.connect(_on_match_ended)

	# Connect 3D Highlight Manager signals
	if highlight_3d.has_signal("highlight_finished"):
		highlight_3d.highlight_finished.connect(_on_highlight_finished)

func _unhandled_input(event: InputEvent) -> void:
	# Press 'S' key during match to trigger Skip Match AI system
	if event is InputEventKey and event.pressed and event.keycode == KEY_S:
		if is_instance_valid(match_engine) and match_engine.is_match_active():
			if match_engine.can_skip_match():
				print("KEY 'S' PRESSED: Skipping match auto-simulation...")
				match_engine.skip_match()
			else:
				var played: int = match_engine.get_manual_matches_played()
				var remaining: int = 5 - (played % 5)
				print("Cannot skip match! Tokens: ", match_engine.get_skip_tokens(), " | Play ", remaining, " more manual matches to earn a token.")

func _on_interactive_event_triggered(scenario_type: int, is_home_team: bool, ball_spawn_3d: Vector3) -> void:
	print("MatchScene: Pausing 2D simulation -> Launching 3D Highlight [Type: ", scenario_type, "]")
	
	# Switch Viewports
	overview_2d.hide()
	highlight_3d.show()
	
	# Delegate 3D setup to Highlight3DManager
	if highlight_3d.has_method("setup_highlight"):
		highlight_3d.setup_highlight(scenario_type, is_home_team, ball_spawn_3d)

func _on_highlight_finished(success: bool, is_home_team: bool) -> void:
	print("MatchScene: 3D Highlight Completed | Outcome Success: ", success)

	# Resume C++ Engine simulation & resolve scenario
	match_engine.resolve_manual_scenario(success, is_home_team)

	# Return to 2D Match Viewport
	highlight_3d.hide()
	overview_2d.show()

func _on_match_ended(final_home: int, final_away: int) -> void:
	print("FULL TIME! Final Scoreboard: Home ", final_home, " - ", final_away, " Away")
	print("Manual Matches Played: ", match_engine.get_manual_matches_played(), " | Available Skip Tokens: ", match_engine.get_skip_tokens())