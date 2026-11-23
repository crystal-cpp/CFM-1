class_name PostMatchModal
extends Control

signal continue_requested(summary: Dictionary)

var _last_summary: Dictionary = {}
var _score_lbl: Label
var _result_tag: Label
var _stats_grid: GridContainer
var _fitness_list: VBoxContainer
var _injury_list: VBoxContainer
var _token_lbl: Label


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	visible = false
	_build_ui()


func _build_ui() -> void:
	var dim := ColorRect.new()
	dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	dim.color = Color(0.02, 0.03, 0.06, 0.84)
	add_child(dim)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var card := PanelContainer.new()
	card.custom_minimum_size = Vector2(780, 520)
	center.add_child(card)

	var margin := MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 24)
	margin.add_theme_constant_override("margin_right", 24)
	margin.add_theme_constant_override("margin_top", 20)
	margin.add_theme_constant_override("margin_bottom", 20)
	card.add_child(margin)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 12)
	margin.add_child(vbox)

	_result_tag = Label.new()
	_result_tag.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_result_tag.add_theme_font_size_override("font_size", 14)
	vbox.add_child(_result_tag)

	_score_lbl = Label.new()
	_score_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_score_lbl.add_theme_font_size_override("font_size", 26)
	vbox.add_child(_score_lbl)

	var split := HBoxContainer.new()
	split.size_flags_vertical = Control.SIZE_EXPAND_FILL
	split.add_theme_constant_override("separation", 20)
	vbox.add_child(split)

	# Left Column: Match Statistics
	var left_box := VBoxContainer.new()
	left_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	split.add_child(left_box)

	var stats_hdr := Label.new()
	stats_hdr.text = "MATCH STATISTICS"
	stats_hdr.add_theme_font_size_override("font_size", 15)
	left_box.add_child(stats_hdr)

	_stats_grid = GridContainer.new()
	_stats_grid.columns = 3
	_stats_grid.add_theme_constant_override("h_separation", 14)
	_stats_grid.add_theme_constant_override("v_separation", 6)
	left_box.add_child(_stats_grid)

	# Right Column: Fitness & Medical Report
	var right_box := VBoxContainer.new()
	right_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	right_box.add_theme_constant_override("separation", 8)
	split.add_child(right_box)

	var fit_hdr := Label.new()
	fit_hdr.text = "POST-MATCH STAMINA"
	fit_hdr.add_theme_font_size_override("font_size", 15)
	right_box.add_child(fit_hdr)

	_fitness_list = VBoxContainer.new()
	_fitness_list.add_theme_constant_override("separation", 3)
	right_box.add_child(_fitness_list)

	var inj_hdr := Label.new()
	inj_hdr.text = "MEDICAL ROOM REPORT"
	inj_hdr.add_theme_font_size_override("font_size", 15)
	right_box.add_child(inj_hdr)

	_injury_list = VBoxContainer.new()
	right_box.add_child(_injury_list)

	# Footer
	var footer := HBoxContainer.new()
	vbox.add_child(footer)

	_token_lbl = Label.new()
	_token_lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_token_lbl.add_theme_color_override("font_color", Color(0.55, 0.88, 1.0))
	footer.add_child(_token_lbl)

	var cont_btn := Button.new()
	cont_btn.text = "CONTINUE TO HUB →"
	cont_btn.custom_minimum_size = Vector2(200, 40)
	cont_btn.pressed.connect(func() -> void:
		visible = false
		continue_requested.emit(_last_summary)
	)
	footer.add_child(cont_btn)


func show_report(summary: Dictionary) -> void:
	_last_summary = summary
	var home_club: String = String(summary.get("home_club", "Home"))
	var away_club: String = String(summary.get("away_club", "Away"))
	var hg: int = int(summary.get("home_goals", 0))
	var ag: int = int(summary.get("away_goals", 0))
	var user_is_home: bool = bool(summary.get("user_is_home", true))

	var user_goals: int = hg if user_is_home else ag
	var opp_goals: int = ag if user_is_home else hg
	if user_goals > opp_goals:
		_result_tag.text = "FULL TIME — VICTORY"
		_result_tag.add_theme_color_override("font_color", Color(0.38, 0.96, 0.62))
	elif user_goals < opp_goals:
		_result_tag.text = "FULL TIME — DEFEAT"
		_result_tag.add_theme_color_override("font_color", Color(1.0, 0.42, 0.42))
	else:
		_result_tag.text = "FULL TIME — DRAW"
		_result_tag.add_theme_color_override("font_color", Color(0.95, 0.85, 0.45))

	_score_lbl.text = "%s  %d - %d  %s" % [home_club, hg, ag, away_club]

	# Populate Match Stats Grid
	for child: Node in _stats_grid.get_children():
		child.queue_free()

	var st: Dictionary = summary.get("stats", {})
	var stat_rows: Array[Array] = [
		["%d%%" % int(st.get("home_possession", 50)), "Possession", "%d%%" % int(st.get("away_possession", 50))],
		["%.2f" % float(st.get("home_xg", 0.0)), "Expected Goals (xG)", "%.2f" % float(st.get("away_xg", 0.0))],
		[str(int(st.get("home_shots", 0))), "Total Shots", str(int(st.get("away_shots", 0)))],
		[str(int(st.get("home_shots_on_target", 0))), "On Target", str(int(st.get("away_shots_on_target", 0)))],
		["%d/%d" % [int(st.get("home_passes_completed", 0)), int(st.get("home_passes", 0))], "Passes", "%d/%d" % [int(st.get("away_passes_completed", 0)), int(st.get("away_passes", 0))]],
		[str(int(st.get("home_tackles", 0))), "Tackles", str(int(st.get("away_tackles", 0)))],
		[str(int(st.get("home_saves", 0))), "GK Saves", str(int(st.get("away_saves", 0)))],
	]
	for r: Array in stat_rows:
		var l_val := Label.new()
		l_val.text = String(r[0])
		var mid_lbl := Label.new()
		mid_lbl.text = String(r[1])
		mid_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		mid_lbl.add_theme_color_override("font_color", Color(0.70, 0.76, 0.85))
		var r_val := Label.new()
		r_val.text = String(r[2])
		r_val.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		_stats_grid.add_child(l_val)
		_stats_grid.add_child(mid_lbl)
		_stats_grid.add_child(r_val)

	# Populate Stamina Drain Report
	for child: Node in _fitness_list.get_children():
		child.queue_free()

	for item: Dictionary in summary.get("stamina_report", []):
		var after_stam: float = float(item.get("after", 100.0))
		var lbl := Label.new()
		lbl.text = "[%s] %s: %d%% -> %d%%" % [
			item.get("position", "CM"),
			item.get("name", "Player"),
			int(round(float(item.get("before", 100.0)))),
			int(round(after_stam))
		]
		lbl.add_theme_font_size_override("font_size", 12)
		lbl.add_theme_color_override("font_color", Color(0.45, 0.94, 0.62) if after_stam >= 65.0 else Color(1.0, 0.72, 0.35))
		_fitness_list.add_child(lbl)

	# Populate Medical Report
	for child: Node in _injury_list.get_children():
		child.queue_free()

	var injuries: Array = summary.get("new_injuries", [])
	if injuries.is_empty():
		var clean_lbl := Label.new()
		clean_lbl.text = "No new injuries reported."
		clean_lbl.add_theme_color_override("font_color", Color(0.45, 0.94, 0.62))
		_injury_list.add_child(clean_lbl)
	else:
		for inj: Dictionary in injuries:
			var inj_lbl := Label.new()
			inj_lbl.text = "INJURED: %s — %s (%d wks)" % [inj.get("name", ""), inj.get("injury", ""), int(inj.get("weeks", 1))]
			inj_lbl.add_theme_color_override("font_color", Color(1.0, 0.38, 0.38))
			_injury_list.add_child(inj_lbl)

	_token_lbl.text = "Skip Tokens Available: %d (Matches Completed: %d)" % [
		int(summary.get("skip_tokens", 1)),
		int(summary.get("manual_matches_played", 0))
	]
	visible = true
