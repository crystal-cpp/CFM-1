extends Camera2D

@onready var match_engine: MatchEngine = $"../../../MatchEngine"

const CHANCE_NONE: int = 0
const CHANCE_HOME_ATTACK: int = 1
const CHANCE_AWAY_ATTACK: int = 2

func _ready() -> void:
	make_current()
	if match_engine:
		position = match_engine.get_field_size() * 0.5

func _process(delta: float) -> void:
	if not match_engine:
		return

	var target_pos: Vector2 = match_engine.get_chance_camera_focus()
	var target_zoom_val: float = match_engine.get_recommended_camera_zoom()

	if match_engine.get_chance_state() == CHANCE_NONE or match_engine.is_waiting_for_kickoff_resume():
		target_pos = match_engine.get_field_size() * 0.5
		target_zoom_val = 1.0

	var smooth: float = 1.0 - exp(-7.5 * delta)
	position = position.lerp(target_pos, smooth)
	zoom = zoom.lerp(Vector2(target_zoom_val, target_zoom_val), smooth)

func screen_to_pitch(screen_pos: Vector2) -> Vector2:
	return get_canvas_transform().affine_inverse() * screen_pos