extends Node3D

signal highlight_finished(success: bool, is_home_team: bool)

@onready var camera_3d: Camera3D = $Camera3D
@onready var ball_3d: Node3D = $Ball3D
@onready var players_container: Node3D = $Players3D

var current_is_home_team: bool = true
var current_scenario: int = 1
var is_attacking_chance: bool = true

# Active controlled player 3D attributes (Loaded dynamically per highlight)
var active_player_stats = {
	"pace": 82,
	"shooting": 78,
	"passing": 75,
	"dribbling": 80,
	"defending": 48,
	"physical": 72
}

func setup_highlight(scenario_type: int, is_home_team: bool, ball_pos: Vector3) -> void:
	current_scenario = scenario_type
	current_is_home_team = is_home_team
	
	# Scenarios 1-3 = Attacking Chance | 4-5 = Defensive Chance
	is_attacking_chance = (scenario_type <= 3)

	# Activate 3D Viewport Camera
	if camera_3d:
		camera_3d.make_current()

	# Set 3D Ball Location from C++ Signal
	if ball_3d:
		ball_3d.global_position = ball_pos

	# Orient Camera and setup 3D roles
	_setup_camera_and_spawns(ball_pos, is_home_team)

	# Temporary auto-resolution placeholder (Replace with manual joystick/touch controls)
	get_tree().create_timer(3.0).timeout.connect(_simulate_highlight_result)

func _setup_camera_and_spawns(ball_pos: Vector3, is_home: bool) -> void:
	if not camera_3d:
		return

	var attack_dir: float = 1.0 if is_home else -1.0

	if is_attacking_chance:
		# Attacking Camera: Positioned behind controlled attacker looking toward goal
		var camera_offset: Vector3 = Vector3(0.0, 4.0, -7.0 * attack_dir)
		camera_3d.global_position = ball_pos + camera_offset
		camera_3d.look_at(ball_pos + Vector3(0.0, 0.5, 12.0 * attack_dir), Vector3.UP)
		print("3D HIGHLIGHT: ATTACKING CHANCE")
	else:
		# Defensive Camera: Positioned facing oncoming attacker to block/tackle
		var camera_offset: Vector3 = Vector3(0.0, 5.0, 8.0 * attack_dir)
		camera_3d.global_position = ball_pos + camera_offset
		camera_3d.look_at(ball_pos + Vector3(0.0, 0.2, -5.0 * attack_dir), Vector3.UP)
		print("3D HIGHLIGHT: DEFENSIVE CHANCE")

	# Clear previous 3D scenario nodes
	if players_container:
		for child in players_container.get_children():
			child.queue_free()

	# TODO: Spawn attacker, defender, and goalkeeper 3D meshes in players_container

# Attribute-based mechanic helpers for active 3D controls
func calculate_move_speed() -> float:
	return 5.0 + (active_player_stats["pace"] * 0.08)

func calculate_shot_power() -> float:
	return 15.0 + (active_player_stats["shooting"] * 0.25)

func perform_tackle_check(opponent_dribble_stat: int) -> bool:
	var tackle_rating: int = active_player_stats["defending"]
	var success_chance: float = float(tackle_rating) / float(tackle_rating + opponent_dribble_stat)
	return (success_chance + randf_range(-0.1, 0.1)) > 0.5

func _simulate_highlight_result() -> void:
	# Temporary outcome resolution for loop testing
	var success: bool = randf() < 0.65
	emit_signal("highlight_finished", success, current_is_home_team)