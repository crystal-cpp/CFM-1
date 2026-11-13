extends Node2D

@onready var match_engine: MatchEngine = $"../../MatchEngine"

func _ready() -> void:
	if not match_engine:
		push_error("MatchPitch: MatchEngine node missing at path '../../MatchEngine'.")
		return

	# Ensure formations.csv was loaded (never ignore a missing/empty formation file)
	if not match_engine.is_formations_loaded() and not match_engine.load_default_formations():
		push_error("MatchPitch: CRITICAL — formations.csv failed to load! Ensure demo/data/formations.csv or Players_Database/formations.csv is valid and non-empty.")
		return

	# Set Tactical Formations (validated against loaded CSV)
	if not match_engine.set_home_formation("4-3-3"):
		push_error("MatchPitch: Home formation '4-3-3' not found in " + match_engine.get_loaded_formations_path())
	if not match_engine.set_away_formation("4-4-2"):
		push_error("MatchPitch: Away formation '4-4-2' not found in " + match_engine.get_loaded_formations_path())

	# Connect non-event feedback signals
	match_engine.minute_changed.connect(_on_minute_changed)
	match_engine.goal_scored.connect(_on_goal_scored)
	match_engine.match_ended.connect(_on_match_ended)
	
	# Start match simulation
	match_engine.start_match()

func _process(_delta: float) -> void:
	queue_redraw()

func _draw() -> void:
	if not is_instance_valid(match_engine):
		return

	var field_size: Vector2 = match_engine.get_field_size()
	if field_size.x <= 0.0 or field_size.y <= 0.0:
		return

	var viewport_size: Vector2 = get_viewport_rect().size
	
	# Dynamic centering offset
	var offset: Vector2 = (viewport_size - field_size) * 0.5
	offset.x = max(offset.x, 0.0)
	offset.y = max(offset.y, 0.0)

	var default_font: Font = ThemeDB.fallback_font

	# Pitch Green Background & White Lines (Centered)
	draw_rect(Rect2(offset, field_size), Color(0.18, 0.55, 0.22), true)
	draw_rect(Rect2(offset, field_size), Color.WHITE, false, 3.0)
	
	var half_x: float = field_size.x * 0.5
	draw_line(offset + Vector2(half_x, 0), offset + Vector2(half_x, field_size.y), Color.WHITE, 2.0)
	draw_arc(offset + (field_size * 0.5), 70.0, 0, TAU, 64, Color.WHITE, 2.0)

	var box_w: float = field_size.x * 0.16
	var box_h: float = field_size.y * 0.55
	var box_y: float = (field_size.y - box_h) * 0.5
	draw_rect(Rect2(offset + Vector2(0, box_y), Vector2(box_w, box_h)), Color.WHITE, false, 2.0)
	draw_rect(Rect2(offset + Vector2(field_size.x - box_w, box_y), Vector2(box_w, box_h)), Color.WHITE, false, 2.0)

	# 11 Home Players (Red) & 11 Away Players (Blue)
	var positions: Array = match_engine.get_player_positions()
	for i in range(positions.size()):
		var pos: Vector2 = offset + Vector2(positions[i])
		var is_home: bool = i < 11
		var color: Color = Color.RED if is_home else Color.BLUE
		
		draw_circle(pos, 9.0, color)
		draw_arc(pos, 9.0, 0, TAU, 32, Color.BLACK, 1.5)

		var label_text: String = "H" + str(i + 1) if is_home else "A" + str(i - 10)
		var text_offset: Vector2 = pos + Vector2(-20.0, 20.0)
		draw_string(default_font, text_offset, label_text, HORIZONTAL_ALIGNMENT_CENTER, 40, 10, Color.WHITE)

	# Draw Ball (Yellow)
	var ball_pos: Vector2 = offset + match_engine.get_ball_position()
	draw_circle(ball_pos, 5.0, Color.YELLOW)
	draw_arc(ball_pos, 5.0, 0, TAU, 16, Color.BLACK, 1.0)

func _on_minute_changed(minute: int) -> void:
	print("Match Time: ", minute, "'")

func _on_goal_scored(home: int, away: int, is_home: bool) -> void:
	print("GOAL! Current Score: ", home, " - ", away, " | Scored By: ", "Home" if is_home else "Away")

func _on_match_ended(final_home: int, final_away: int) -> void:
	print("FULL TIME! Pitch Overview Ended. Score: ", final_home, " - ", final_away)