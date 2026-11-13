extends Node3D

signal highlight_finished(success: bool, is_home_team: bool)

const BALL_RADIUS: float = 0.11
const PITCH_HALF_X: float = 45.0
const PITCH_HALF_Z: float = 26.0

@export var highlight_duration: float = 3.0
@export var attack_along_x: bool = false

@onready var camera_3d: Camera3D = $Camera3D
@onready var ball_3d: RigidBody3D = $Ball3D
@onready var ball_visual: Node3D = $Ball3D/Ball
@onready var players_container: Node3D = $Players3D

var current_is_home_team: bool = true
var current_scenario: int = 1
var is_attacking_chance: bool = true
var is_highlight_active: bool = false

var _highlight_timer: Timer

var active_player_stats = {
	"pace": 82,
	"shooting": 78,
	"passing": 75,
	"dribbling": 80,
	"defending": 48,
	"physical": 72
}

func _ready() -> void:
	_highlight_timer = Timer.new()
	_highlight_timer.name = "HighlightTimer"
	_highlight_timer.one_shot = true
	_highlight_timer.timeout.connect(_on_timer_timeout)
	add_child(_highlight_timer)

	if camera_3d:
		camera_3d.current = false

	_calibrate_ball_visual_center()
	_reset_ball_physics(Vector3(0.0, BALL_RADIUS, 0.0))

func _exit_tree() -> void:
	cleanup_highlight()

func setup_highlight(scenario_type: int, is_home_team: bool, ball_pos: Vector3) -> void:
	if is_highlight_active:
		return

	is_highlight_active = true
	current_scenario = scenario_type
	current_is_home_team = is_home_team
	is_attacking_chance = (scenario_type <= 3)

	# Clamp 3D spawn position safely inside stadium pitch bounds
	var safe_ball_pos := Vector3(
		clampf(ball_pos.x, -PITCH_HALF_X, PITCH_HALF_X),
		BALL_RADIUS,
		clampf(ball_pos.z, -PITCH_HALF_Z, PITCH_HALF_Z)
	)

	# Reset RigidBody3D state and place ball flush on pitch surface
	_reset_ball_physics(safe_ball_pos)

	# Activate 3D Viewport Camera
	if camera_3d:
		camera_3d.make_current()

	_setup_camera_and_spawns(safe_ball_pos, is_home_team)

	# Start managed highlight timer
	_highlight_timer.start(highlight_duration)

func _setup_camera_and_spawns(ball_pos: Vector3, is_home: bool) -> void:
	var dir_sign: float = 1.0 if is_home else -1.0
	var forward_axis: Vector3 = Vector3(dir_sign, 0.0, 0.0) if attack_along_x else Vector3(0.0, 0.0, dir_sign)

	if camera_3d:
		var cam_pos: Vector3
		var look_target: Vector3

		if is_attacking_chance:
			# Camera behind controlled attacker looking toward target goal
			cam_pos = ball_pos - (forward_axis * 7.0) + Vector3(0.0, 4.0, 0.0)
			look_target = ball_pos + (forward_axis * 12.0) + Vector3(0.0, 0.5, 0.0)
			print("3D HIGHLIGHT: ATTACKING CHANCE [Scenario ", current_scenario, "]")
		else:
			# Camera facing oncoming attacker to block/tackle
			cam_pos = ball_pos + (forward_axis * 8.0) + Vector3(0.0, 5.0, 0.0)
			look_target = ball_pos - (forward_axis * 5.0) + Vector3(0.0, 0.2, 0.0)
			print("3D HIGHLIGHT: DEFENSIVE CHANCE [Scenario ", current_scenario, "]")

		camera_3d.global_position = cam_pos
		if cam_pos.distance_squared_to(look_target) > 0.001:
			var view_dir: Vector3 = (look_target - cam_pos).normalized()
			var up_vec: Vector3 = Vector3.UP if absf(view_dir.dot(Vector3.UP)) < 0.99 else Vector3.FORWARD
			camera_3d.look_at(look_target, up_vec)

	_clear_spawned_players()
	_spawn_scenario_actors(ball_pos, forward_axis, is_home)

func _spawn_scenario_actors(ball_pos: Vector3, forward_axis: Vector3, is_home: bool) -> void:
	if not players_container:
		return

	var home_color := Color(0.85, 0.15, 0.15)
	var away_color := Color(0.15, 0.35, 0.90)
	var gk_color := Color(0.95, 0.75, 0.10)

	var attacker_color: Color = home_color if is_home else away_color
	var defender_color: Color = away_color if is_home else home_color

	# 1. Attacker right behind the ball
	var attacker_pos: Vector3 = ball_pos - (forward_axis * 0.8)
	attacker_pos.y = 0.0
	_create_player_marker("Attacker3D", attacker_pos, attacker_color)

	# 2. Primary Defender ahead of the ball
	var defender_pos: Vector3 = ball_pos + (forward_axis * 3.5)
	defender_pos.y = 0.0
	_create_player_marker("Defender3D", defender_pos, defender_color)

	# 3. Goalkeeper further downfield toward goal
	var gk_pos: Vector3 = ball_pos + (forward_axis * 14.0)
	gk_pos.x = clampf(gk_pos.x, -PITCH_HALF_X, PITCH_HALF_X)
	gk_pos.z = clampf(gk_pos.z, -PITCH_HALF_Z, PITCH_HALF_Z)
	gk_pos.y = 0.0
	_create_player_marker("Goalkeeper3D", gk_pos, gk_color)

func _create_player_marker(node_name: String, world_pos: Vector3, jersey_color: Color) -> void:
	var mesh_instance := MeshInstance3D.new()
	mesh_instance.name = node_name

	var capsule := CapsuleMesh.new()
	capsule.radius = 0.32
	capsule.height = 1.75
	mesh_instance.mesh = capsule

	var mat := StandardMaterial3D.new()
	mat.albedo_color = jersey_color
	mat.roughness = 0.6
	mesh_instance.material_override = mat

	players_container.add_child(mesh_instance)
	mesh_instance.global_position = world_pos + Vector3(0.0, capsule.height * 0.5, 0.0)

func _clear_spawned_players() -> void:
	if not players_container:
		return
	for child in players_container.get_children():
		child.queue_free()

func _reset_ball_physics(world_pos: Vector3) -> void:
	if not ball_3d:
		return
	ball_3d.freeze = true
	ball_3d.linear_velocity = Vector3.ZERO
	ball_3d.angular_velocity = Vector3.ZERO
	ball_3d.global_position = world_pos

func _calibrate_ball_visual_center() -> void:
	if not ball_visual:
		return

	# Zero out the stray scene translation first so AABB is measured cleanly
	ball_visual.position = Vector3.ZERO

	var combined_aabb := AABB()
	var has_bounds := false

	for node in ball_visual.find_children("*", "VisualInstance3D", true, false):
		var vis := node as VisualInstance3D
		if not vis:
			continue
		var local_aabb: AABB = vis.get_aabb()
		# Transform AABB corners into ball_visual's parent (Ball3D) space
		var xform: Transform3D = ball_visual.transform * _get_relative_transform(ball_visual, vis)
		for i in range(8):
			var corner: Vector3 = xform * local_aabb.get_endpoint(i)
			if not has_bounds:
				combined_aabb = AABB(corner, Vector3.ZERO)
				has_bounds = true
			else:
				combined_aabb = combined_aabb.expand(corner)

	if has_bounds:
		# Center the visual mesh directly on Ball3D's origin
		ball_visual.position = -combined_aabb.get_center()

func _get_relative_transform(ancestor: Node3D, descendant: Node3D) -> Transform3D:
	var result := Transform3D.IDENTITY
	var current: Node = descendant
	while current != null and current != ancestor:
		if current is Node3D:
			result = (current as Node3D).transform * result
		current = current.get_parent()
	return result

# Attribute-based mechanic helpers for active 3D controls
func calculate_move_speed() -> float:
	return 5.0 + (active_player_stats["pace"] * 0.08)

func calculate_shot_power() -> float:
	return 15.0 + (active_player_stats["shooting"] * 0.25)

func perform_tackle_check(opponent_dribble_stat: int) -> bool:
	var tackle_rating: int = active_player_stats["defending"]
	var success_chance: float = float(tackle_rating) / float(tackle_rating + opponent_dribble_stat)
	return (success_chance + randf_range(-0.1, 0.1)) > 0.5

func _on_timer_timeout() -> void:
	if not is_highlight_active:
		return

	var success: bool = randf() < 0.65
	var finished_home: bool = current_is_home_team
	cleanup_highlight()
	emit_signal("highlight_finished", success, finished_home)

func cleanup_highlight() -> void:
	is_highlight_active = false
	if _highlight_timer and not _highlight_timer.is_stopped():
		_highlight_timer.stop()
	if camera_3d:
		camera_3d.current = false
	_reset_ball_physics(Vector3(0.0, BALL_RADIUS, 0.0))
	_clear_spawned_players()