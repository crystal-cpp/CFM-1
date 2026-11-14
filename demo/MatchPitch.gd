extends Node2D

@onready var match_engine: MatchEngine = $"../../MatchEngine"

var _ball_trail: PackedVector2Array = PackedVector2Array()
const MAX_TRAIL_POINTS: int = 10

func _ready() -> void:
	if not match_engine:
		push_error("MatchPitch: MatchEngine node missing at path '../../MatchEngine'.")
		return

	if not match_engine.is_formations_loaded() and not match_engine.load_default_formations():
		push_error("MatchPitch: CRITICAL — formations.csv failed to load!")
		return

	var available: PackedStringArray = match_engine.get_available_formations()
	var home_fmt: String = "4-3-3" if match_engine.has_formation("4-3-3") else (available[0] if available.size() > 0 else "4-3-3")
	var away_fmt: String = "4-4-2" if match_engine.has_formation("4-4-2") else (available[0] if available.size() > 0 else "4-4-2")

	if not match_engine.set_home_formation(home_fmt):
		push_error("MatchPitch: Home formation '" + home_fmt + "' failed in " + match_engine.get_loaded_formations_path())
	if not match_engine.set_away_formation(away_fmt):
		push_error("MatchPitch: Away formation '" + away_fmt + "' failed in " + match_engine.get_loaded_formations_path())

	match_engine.minute_changed.connect(_on_minute_changed)
	match_engine.goal_scored.connect(_on_goal_scored)
	match_engine.match_ended.connect(_on_match_ended)

	match_engine.start_match()

func _process(_delta: float) -> void:
	if is_instance_valid(match_engine) and match_engine.is_match_active():
		var ball_pos: Vector2 = match_engine.get_ball_position()
		if _ball_trail.is_empty() or _ball_trail[_ball_trail.size() - 1].distance_to(ball_pos) > 3.0:
			_ball_trail.append(ball_pos)
			if _ball_trail.size() > MAX_TRAIL_POINTS:
				_ball_trail.remove_at(0)
	queue_redraw()

func _draw() -> void:
	if not is_instance_valid(match_engine):
		return

	var field_size: Vector2 = match_engine.get_field_size()
	if field_size.x <= 0.0 or field_size.y <= 0.0:
		return

	var viewport_size: Vector2 = get_viewport_rect().size
	var offset: Vector2 = ((viewport_size - field_size) * 0.5).max(Vector2.ZERO)
	offset.y = maxf(offset.y, 72.0)

	var default_font: Font = ThemeDB.fallback_font
	var line_color := Color(1.0, 1.0, 1.0, 0.88)

	# 1. Turf Base & Mowed Grass Stripes
	draw_rect(Rect2(offset, field_size), Color(0.15, 0.48, 0.19), true)
	var stripe_count: int = 12
	var stripe_w: float = field_size.x / float(stripe_count)
	for s in range(0, stripe_count, 2):
		draw_rect(Rect2(offset + Vector2(s * stripe_w, 0.0), Vector2(stripe_w, field_size.y)), Color(0.18, 0.54, 0.22), true)

	# 2. Outer Boundary, Halfway Line & Center Circle
	draw_rect(Rect2(offset, field_size), line_color, false, 3.0)
	var half_x: float = field_size.x * 0.5
	var center: Vector2 = offset + (field_size * 0.5)
	draw_line(offset + Vector2(half_x, 0.0), offset + Vector2(half_x, field_size.y), line_color, 2.0)
	draw_arc(center, 70.0, 0.0, TAU, 64, line_color, 2.0)
	draw_circle(center, 4.0, line_color)

	# 3. Penalty Boxes, 6-Yard Boxes, Penalty Spots & Goal Frames
	var box_w: float = field_size.x * 0.16
	var box_h: float = field_size.y * 0.55
	var box_y: float = (field_size.y - box_h) * 0.5
	draw_rect(Rect2(offset + Vector2(0.0, box_y), Vector2(box_w, box_h)), line_color, false, 2.0)
	draw_rect(Rect2(offset + Vector2(field_size.x - box_w, box_y), Vector2(box_w, box_h)), line_color, false, 2.0)

	var small_w: float = field_size.x * 0.055
	var small_h: float = field_size.y * 0.26
	var small_y: float = (field_size.y - small_h) * 0.5
	draw_rect(Rect2(offset + Vector2(0.0, small_y), Vector2(small_w, small_h)), line_color, false, 2.0)
	draw_rect(Rect2(offset + Vector2(field_size.x - small_w, small_y), Vector2(small_w, small_h)), line_color, false, 2.0)

	var pen_dist: float = field_size.x * 0.105
	draw_circle(offset + Vector2(pen_dist, field_size.y * 0.5), 3.5, line_color)
	draw_circle(offset + Vector2(field_size.x - pen_dist, field_size.y * 0.5), 3.5, line_color)

	# Goal Net Mouths
	var goal_h: float = 88.0
	var goal_y: float = (field_size.y - goal_h) * 0.5
	draw_rect(Rect2(offset + Vector2(-10.0, goal_y), Vector2(10.0, goal_h)), Color(0.95, 0.95, 0.95, 0.65), false, 2.5)
	draw_rect(Rect2(offset + Vector2(field_size.x, goal_y), Vector2(10.0, goal_h)), Color(0.95, 0.95, 0.95, 0.65), false, 2.5)

	# Corner Arcs
	draw_arc(offset, 14.0, 0.0, PI * 0.5, 16, line_color, 2.0)
	draw_arc(offset + Vector2(field_size.x, 0.0), 14.0, PI * 0.5, PI, 16, line_color, 2.0)
	draw_arc(offset + Vector2(0.0, field_size.y), 14.0, -PI * 0.5, 0.0, 16, line_color, 2.0)
	draw_arc(offset + field_size, 14.0, PI, PI * 1.5, 16, line_color, 2.0)

	# 4. Ball Trail
	for t in range(_ball_trail.size()):
		var alpha: float = (float(t + 1) / float(_ball_trail.size())) * 0.35
		draw_circle(offset + _ball_trail[t], 3.5, Color(1.0, 0.95, 0.4, alpha))

	# 5. 11 Home Players & 11 Away Players
	var positions: Array = match_engine.get_player_positions()
	var roles: PackedStringArray = match_engine.get_player_roles()
	var carrier_idx: int = match_engine.get_ball_carrier_index()

	for i in range(positions.size()):
		var pos: Vector2 = offset + Vector2(positions[i])
		var is_home: bool = i < 11
		var is_gk: bool = (i == 0 or i == 11)
		var color: Color
		if is_gk:
			color = Color(0.92, 0.72, 0.12) if is_home else Color(0.15, 0.78, 0.65)
		else:
			color = Color(0.88, 0.18, 0.20) if is_home else Color(0.18, 0.42, 0.92)

		if i == carrier_idx:
			draw_arc(pos, 14.0, 0.0, TAU, 32, Color(1.0, 0.92, 0.25), 2.2)

		draw_circle(pos, 9.5, color)
		draw_arc(pos, 9.5, 0.0, TAU, 32, Color.BLACK, 1.5)

		var label_text: String = roles[i] if i < roles.size() else ("H" + str(i + 1) if is_home else "A" + str(i - 10))
		var text_offset: Vector2 = pos + Vector2(-20.0, 20.0)
		draw_string(default_font, text_offset, label_text, HORIZONTAL_ALIGNMENT_CENTER, 40, 10, Color.WHITE)

	# 6. Ball
	var ball_pos: Vector2 = offset + match_engine.get_ball_position()
	draw_circle(ball_pos, 5.5, Color.WHITE)
	draw_arc(ball_pos, 5.5, 0.0, TAU, 16, Color.BLACK, 1.4)

func _on_minute_changed(minute: int) -> void:
	if minute % 15 == 0:
		print("Match Time: ", minute, "'")

func _on_goal_scored(home: int, away: int, is_home: bool) -> void:
	_ball_trail.clear()
	print("GOAL! Current Score: ", home, " - ", away, " | Scored By: ", "Home" if is_home else "Away")

func _on_match_ended(final_home: int, final_away: int) -> void:
	_ball_trail.clear()
	print("FULL TIME! Pitch Overview Ended. Score: ", final_home, " - ", final_away)