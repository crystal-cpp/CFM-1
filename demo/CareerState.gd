# League -> Array[Dictionary] of fixtures
var league_fixtures: Dictionary = {}
var current_week: int = 1
var user_club: String = ""
var user_league: String = ""

func generate_all_league_fixtures(db: PlayerDatabase, clubs_csv_path: String = "res://data/clubs.csv") -> void:
	if db.get_club_count() == 0:
		db.load_clubs_from_csv(clubs_csv_path)
	db.sync_clubs_from_players()

	league_fixtures.clear()
	for league_name: String in db.get_all_leagues():
		var club_dicts: Array = db.get_clubs_by_league(league_name)
		var club_names: Array[String] = []
		for entry: Dictionary in club_dicts:
			club_names.append(String(entry.get("name", "")))
		club_names.sort()
		league_fixtures[league_name] = _build_double_round_robin(league_name, club_names)

func _build_double_round_robin(league_name: String, club_names: Array[String]) -> Array[Dictionary]:
	var teams: Array[String] = club_names.duplicate()
	if teams.size() < 2:
		return []
	if teams.size() % 2 == 1:
		teams.append("") # Bye slot for odd-team leagues

	var n: int = teams.size()
	var rounds_per_half: int = n - 1
	var half_size: int = n / 2
	var first_half: Array[Array] = []
	var slug: String = league_name.to_snake_case()

	for r: int in range(rounds_per_half):
		var round_pairs: Array = []
		for i: int in range(half_size):
			var home: String = teams[i]
			var away: String = teams[n - 1 - i]
			if i == 0 and (r % 2 == 1):
				var tmp := home
				home = away
				away = tmp
			if home.is_empty() or away.is_empty():
				continue
			round_pairs.append([home, away])
		first_half.append(round_pairs)

		# Rotate indices 1 .. n-1 clockwise
		var last: String = teams.pop_back()
		teams.insert(1, last)

	var all_fixtures: Array[Dictionary] = []
	for half: int in range(2):
		for r: int in range(rounds_per_half):
			var matchday: int = half * rounds_per_half + r + 1
			var pairs: Array = first_half[r]
			for idx: int in range(pairs.size()):
				var home: String = pairs[idx][0] if half == 0 else pairs[idx][1]
				var away: String = pairs[idx][1] if half == 0 else pairs[idx][0]
				all_fixtures.append({
					"id": "%s_md%02d_%d" % [slug, matchday, idx],
					"league": league_name,
					"matchday": matchday,
					"week": matchday,
					"home_club": home,
					"away_club": away,
					"home_goals": -1,
					"away_goals": -1,
					"played": false,
					"is_user_match": (home == user_club or away == user_club),
				})
	return all_fixtures

func get_league_fixtures(league_name: String, matchday: int = -1) -> Array[Dictionary]:
	var list: Array[Dictionary] = league_fixtures.get(league_name, [])
	if matchday < 0:
		return list
	var filtered: Array[Dictionary] = []
	for fx: Dictionary in list:
		if int(fx.get("matchday", -1)) == matchday:
			filtered.append(fx)
	return filtered

func get_club_fixtures(club_name: String = user_club, league_name: String = user_league) -> Array[Dictionary]:
	var target_league: String = league_name
	if target_league.is_empty():
		for lg: String in league_fixtures.keys():
			for fx: Dictionary in league_fixtures[lg]:
				if fx.get("home_club") == club_name or fx.get("away_club") == club_name:
					target_league = lg
					break
			if not target_league.is_empty():
				break
	var result: Array[Dictionary] = []
	for fx: Dictionary in league_fixtures.get(target_league, []):
		if fx.get("home_club") == club_name or fx.get("away_club") == club_name:
			result.append(fx)
	return result

func get_next_match(club_name: String = user_club) -> Dictionary:
	for fx: Dictionary in get_club_fixtures(club_name):
		if not bool(fx.get("played", false)):
			return fx
	return {}
# --- Phase 5C Match & Squad Condition State ---
var user_squad: Array[Dictionary] = []
var active_fixture: Dictionary = {}
var active_match_context: Dictionary = {}
var last_match_summary: Dictionary = {}

# Tactical defaults synced with MatchEngine enums
var user_tactics: Dictionary = {
	"formation": "4-3-3",
	"mentality": 2,          # 0..4 (Balanced = 2)
	"pressing": 1,           # 0..2 (Normal = 1)
	"defensive_line": 1,     # 0..2 (Normal = 1)
	"passing_style": 1,      # 0..2 (Normal = 1)
	"attacking_width": 1,    # 0..2 (Normal = 1)
	"tempo": 1,              # 0..2 (Normal = 1)
	"defensive_approach": 1, # 0..2 (Normal = 1)
}

const INJURY_TYPES: Array[Dictionary] = [
	{"name": "Hamstring Strain", "min_weeks": 2, "max_weeks": 4},
	{"name": "Ankle Sprain", "min_weeks": 1, "max_weeks": 3},
	{"name": "Groin Tightness", "min_weeks": 1, "max_weeks": 2},
	{"name": "Calf Bruise", "min_weeks": 1, "max_weeks": 2},
	{"name": "Knee Twist", "min_weeks": 3, "max_weeks": 5},
]


func get_match_importance(fixture: Dictionary) -> Dictionary:
	var comp: String = String(fixture.get("competition", fixture.get("league", ""))).to_lower()
	var md: int = int(fixture.get("matchday", 1))
	var home_name: String = String(fixture.get("home_club", ""))
	var away_name: String = String(fixture.get("away_club", ""))

	if comp.contains("friendly") or comp.contains("friendlies"):
		return {
			"tier": "FRIENDLY",
			"label": "PRE-SEASON FRIENDLY",
			"badge_color": Color(0.55, 0.40, 0.14),
			"text_color": Color(1.0, 0.88, 0.40),
			"stamina_drain_mult": 0.85,
			"injury_risk_mult": 0.65,
			"description": "Low-stakes fitness tune-up. Ideal for rotating fringe players."
		}

	if comp.contains("champions") or comp.contains("ucl"):
		return {
			"tier": "EUROPEAN_NIGHT",
			"label": "EUROPEAN NIGHT",
			"badge_color": Color(0.18, 0.30, 0.68),
			"text_color": Color(0.65, 0.86, 1.0),
			"stamina_drain_mult": 1.12,
			"injury_risk_mult": 1.15,
			"description": "High-tempo continental clash under the floodlights."
		}

	var table: Array[Dictionary] = get_league_standings(String(fixture.get("league", user_league)))
	var home_rank: int = _find_club_rank(table, home_name)
	var away_rank: int = _find_club_rank(table, away_name)

	if md >= 10 and home_rank > 0 and away_rank > 0 and home_rank <= 3 and away_rank <= 3:
		return {
			"tier": "TITLE_SIX_POINTER",
			"label": "TITLE SIX-POINTER",
			"badge_color": Color(0.62, 0.18, 0.22),
			"text_color": Color(1.0, 0.72, 0.72),
			"stamina_drain_mult": 1.10,
			"injury_risk_mult": 1.10,
			"description": "Top-three direct showdown with major title implications."
		}

	if md >= 28 and (home_rank >= 16 or away_rank >= 16):
		return {
			"tier": "RELEGATION_SCRAP",
			"label": "SURVIVAL BATTLE",
			"badge_color": Color(0.58, 0.32, 0.12),
			"text_color": Color(1.0, 0.80, 0.55),
			"stamina_drain_mult": 1.08,
			"injury_risk_mult": 1.08,
			"description": "Physical late-season battle where every point counts."
		}

	return {
		"tier": "STANDARD",
		"label": "LEAGUE MATCHDAY %d" % maxi(1, md),
		"badge_color": Color(0.10, 0.46, 0.36),
		"text_color": Color(0.40, 0.98, 0.72),
		"stamina_drain_mult": 1.00,
		"injury_risk_mult": 1.00,
		"description": "Standard domestic league fixture."
	}


func resolve_match_kits(home_club: String, away_club: String, db: PlayerDatabase = null) -> Dictionary:
	var home_colors: Dictionary = _get_club_kit_colors(home_club, db, true)
	var away_colors: Dictionary = _get_club_kit_colors(away_club, db, false)

	var home_primary: Color = home_colors["primary"]
	var home_secondary: Color = home_colors["secondary"]
	var away_primary: Color = away_colors["primary"]
	var away_secondary: Color = away_colors["secondary"]

	var clash_detected: bool = _colors_clash(home_primary, away_primary)
	var resolved_away_primary: Color = away_primary
	var resolved_away_secondary: Color = away_secondary

	if clash_detected:
		if not _colors_clash(home_primary, away_secondary):
			resolved_away_primary = away_secondary
			resolved_away_secondary = away_primary
		else:
			# High-contrast fallback kit
			var luma: float = home_primary.get_luminance()
			resolved_away_primary = Color(0.94, 0.95, 0.97) if luma < 0.45 else Color(0.10, 0.13, 0.20)
			resolved_away_secondary = Color(0.15, 0.20, 0.32) if luma < 0.45 else Color(0.92, 0.78, 0.24)

	return {
		"clash_detected": clash_detected,
		"home_primary": home_primary,
		"home_secondary": home_secondary,
		"away_primary": resolved_away_primary,
		"away_secondary": resolved_away_secondary,
		"away_default_primary": away_primary,
		"away_default_secondary": away_secondary,
	}


func prepare_match_context(fixture: Dictionary, db: PlayerDatabase = null) -> Dictionary:
	active_fixture = fixture.duplicate(true)
	var home_club: String = String(fixture.get("home_club", user_club))
	var away_club: String = String(fixture.get("away_club", "Opponent FC"))
	var user_is_home: bool = (home_club == user_club)

	var home_xi: Array[Dictionary] = _build_starting_xi(home_club, db, user_is_home)
	var away_xi: Array[Dictionary] = _build_starting_xi(away_club, db, not user_is_home)
	var kits: Dictionary = resolve_match_kits(home_club, away_club, db)
	var importance: Dictionary = get_match_importance(fixture)

	active_match_context = {
		"fixture": active_fixture,
		"home_club": home_club,
		"away_club": away_club,
		"user_is_home": user_is_home,
		"home_lineup": home_xi,
		"away_lineup": away_xi,
		"home_ovr": _compute_lineup_ovr(home_xi),
		"away_ovr": _compute_lineup_ovr(away_xi),
		"home_formation": String(user_tactics.get("formation", "4-3-3")) if user_is_home else _get_club_default_formation(home_club, db),
		"away_formation": _get_club_default_formation(away_club, db) if user_is_home else String(user_tactics.get("formation", "4-3-3")),
		"kits": kits,
		"importance": importance,
		"tactics": user_tactics.duplicate(true),
	}
	return active_match_context


func apply_context_to_match_engine(engine: MatchEngine, context: Dictionary = {}) -> void:
	var ctx: Dictionary = context if not context.is_empty() else active_match_context
	if ctx.is_empty() or engine == null:
		return

	var home_fmt: String = String(ctx.get("home_formation", "4-3-3"))
	var away_fmt: String = String(ctx.get("away_formation", "4-4-2"))
	if engine.has_formation(home_fmt):
		engine.set_home_formation(home_fmt)
	if engine.has_formation(away_fmt):
		engine.set_away_formation(away_fmt)

	engine.set_home_team_ovr(int(ctx.get("home_ovr", 75)))
	engine.set_away_team_ovr(int(ctx.get("away_ovr", 74)))
	engine.set_home_lineup(ctx.get("home_lineup", []))
	engine.set_away_lineup(ctx.get("away_lineup", []))

	var tac: Dictionary = ctx.get("tactics", user_tactics)
	var user_is_home: bool = bool(ctx.get("user_is_home", true))
	if user_is_home:
		engine.set_home_mentality(int(tac.get("mentality", 2)))
		engine.set_home_pressing(int(tac.get("pressing", 1)))
		engine.set_home_defensive_line(int(tac.get("defensive_line", 1)))
		engine.set_home_passing_style(int(tac.get("passing_style", 1)))
		engine.set_home_attacking_width(int(tac.get("attacking_width", 1)))
		engine.set_home_tempo(int(tac.get("tempo", 1)))
		engine.set_home_defensive_approach(int(tac.get("defensive_approach", 1)))
	else:
		engine.set_away_mentality(int(tac.get("mentality", 2)))
		engine.set_away_pressing(int(tac.get("pressing", 1)))
		engine.set_away_defensive_line(int(tac.get("defensive_line", 1)))
		engine.set_away_passing_style(int(tac.get("passing_style", 1)))
		engine.set_away_attacking_width(int(tac.get("attacking_width", 1)))
		engine.set_away_tempo(int(tac.get("tempo", 1)))
		engine.set_away_defensive_approach(int(tac.get("defensive_approach", 1)))


func commit_match_result(engine: MatchEngine) -> Dictionary:
	var home_goals: int = engine.get_home_score()
	var away_goals: int = engine.get_away_score()
	var stats: Dictionary = engine.get_match_stats()
	var staminas: PackedFloat32Array = engine.get_player_stamina()
	var names: PackedStringArray = engine.get_player_names()

	var home_club: String = String(active_match_context.get("home_club", user_club))
	var away_club: String = String(active_match_context.get("away_club", "Opponent FC"))
	var user_is_home: bool = bool(active_match_context.get("user_is_home", true))
	var importance: Dictionary = active_match_context.get("importance", {"stamina_drain_mult": 1.0, "injury_risk_mult": 1.0})
	var drain_mult: float = float(importance.get("stamina_drain_mult", 1.0))
	var risk_mult: float = float(importance.get("injury_risk_mult", 1.0))

	# 1. Record fixture result in league_fixtures
	var fix_id: String = String(active_fixture.get("id", ""))
	var fix_league: String = String(active_fixture.get("league", user_league))
	var fix_md: int = int(active_fixture.get("matchday", current_week))

	if league_fixtures.has(fix_league):
		for fx: Dictionary in league_fixtures[fix_league]:
			if String(fx.get("id", "")) == fix_id:
				fx["home_goals"] = home_goals
				fx["away_goals"] = away_goals
				fx["played"] = true
				break

	# 2. Simulate remaining AI matches in this matchday
	_simulate_ai_matchday(fix_md)

	# 3. Sync user squad stamina & roll post-match injury checks
	var offset: int = 0 if user_is_home else 11
	var played_names: Dictionary = {}
	var stamina_report: Array[Dictionary] = []
	var new_injuries: Array[Dictionary] = []

	for slot: int in range(11):
		var idx: int = offset + slot
		if idx >= names.size() or idx >= staminas.size():
			continue
		var p_name: String = names[idx]
		played_names[p_name] = true
		var raw_end_stam: float = staminas[idx]
		var adjusted_stam: float = clampf(100.0 - ((100.0 - raw_end_stam) * drain_mult), 10.0, 100.0)

		var p_dict: Dictionary = _find_user_player(p_name)
		var prev_stam: float = float(p_dict.get("stamina", 100.0))
		if not p_dict.is_empty():
			p_dict["stamina"] = adjusted_stam

		stamina_report.append({
			"name": p_name,
			"position": String(p_dict.get("position", "CM")),
			"before": prev_stam,
			"after": adjusted_stam,
		})

		# Injury roll increases sharply below 35% stamina
		var fatigue_factor: float = clampf((48.0 - adjusted_stam) / 38.0, 0.0, 1.0)
		var phys: float = float(p_dict.get("physical", 72))
		var base_prob: float = (0.012 + fatigue_factor * 0.095) * risk_mult * (1.15 - phys * 0.003)
		if randf() < base_prob:
			var inj_tpl: Dictionary = INJURY_TYPES[randi() % INJURY_TYPES.size()]
			var weeks: int = randi_range(int(inj_tpl["min_weeks"]), int(inj_tpl["max_weeks"]))
			if not p_dict.is_empty():
				p_dict["injury_name"] = String(inj_tpl["name"])
				p_dict["injury_weeks"] = weeks
			new_injuries.append({
				"name": p_name,
				"injury": String(inj_tpl["name"]),
				"weeks": weeks,
			})

	# 4. Recover rested / injured players & advance week
	for p: Dictionary in user_squad:
		var p_name: String = String(p.get("name", ""))
		if not played_names.has(p_name):
			p["stamina"] = minf(100.0, float(p.get("stamina", 100.0)) + 24.0)
			var inj_w: int = int(p.get("injury_weeks", 0))
			if inj_w > 0:
				inj_w -= 1
				p["injury_weeks"] = inj_w
				if inj_w <= 0:
					p["injury_name"] = ""

	current_week = maxi(current_week + 1, fix_md + 1)

	last_match_summary = {
		"home_club": home_club,
		"away_club": away_club,
		"home_goals": home_goals,
		"away_goals": away_goals,
		"user_is_home": user_is_home,
		"stats": stats,
		"stamina_report": stamina_report,
		"new_injuries": new_injuries,
		"skip_tokens": engine.get_skip_tokens(),
		"manual_matches_played": engine.get_manual_matches_played(),
	}
	return last_match_summary


func _simulate_ai_matchday(matchday: int) -> void:
	for lg: String in league_fixtures.keys():
		for fx: Dictionary in league_fixtures[lg]:
			if int(fx.get("matchday", -1)) != matchday or bool(fx.get("played", false)):
				continue
			if bool(fx.get("is_user_match", false)):
				continue
			fx["home_goals"] = maxi(0, int(round(1.45 + randf_range(-1.2, 1.8))))
			fx["away_goals"] = maxi(0, int(round(1.15 + randf_range(-1.1, 1.6))))
			fx["played"] = true


func get_league_standings(league_name: String) -> Array[Dictionary]:
	var table_map: Dictionary = {}
	for fx: Dictionary in league_fixtures.get(league_name, []):
		var h: String = String(fx.get("home_club", ""))
		var a: String = String(fx.get("away_club", ""))
		if not table_map.has(h):
			table_map[h] = {"club": h, "played": 0, "won": 0, "drawn": 0, "lost": 0, "gf": 0, "ga": 0, "gd": 0, "pts": 0}
		if not table_map.has(a):
			table_map[a] = {"club": a, "played": 0, "won": 0, "drawn": 0, "lost": 0, "gf": 0, "ga": 0, "gd": 0, "pts": 0}
		if not bool(fx.get("played", false)):
			continue
		var hg: int = int(fx.get("home_goals", 0))
		var ag: int = int(fx.get("away_goals", 0))
		table_map[h]["played"] += 1
		table_map[a]["played"] += 1
		table_map[h]["gf"] += hg
		table_map[h]["ga"] += ag
		table_map[a]["gf"] += ag
		table_map[a]["ga"] += hg
		if hg > ag:
			table_map[h]["won"] += 1
			table_map[h]["pts"] += 3
			table_map[a]["lost"] += 1
		elif ag > hg:
			table_map[a]["won"] += 1
			table_map[a]["pts"] += 3
			table_map[h]["lost"] += 1
		else:
			table_map[h]["drawn"] += 1
			table_map[a]["drawn"] += 1
			table_map[h]["pts"] += 1
			table_map[a]["pts"] += 1

	var rows: Array[Dictionary] = []
	for club_key: String in table_map.keys():
		var r: Dictionary = table_map[club_key]
		r["gd"] = int(r["gf"]) - int(r["ga"])
		rows.append(r)

	rows.sort_custom(func(x: Dictionary, y: Dictionary) -> bool:
		if int(x["pts"]) != int(y["pts"]):
			return int(x["pts"]) > int(y["pts"])
		if int(x["gd"]) != int(y["gd"]):
			return int(x["gd"]) > int(y["gd"])
		return int(x["gf"]) > int(y["gf"])
	)
	return rows


func _find_club_rank(standings: Array[Dictionary], club_name: String) -> int:
	for i: int in range(standings.size()):
		if String(standings[i].get("club", "")) == club_name:
			return i + 1
	return -1


func _find_user_player(player_name: String) -> Dictionary:
	for p: Dictionary in user_squad:
		if String(p.get("name", "")) == player_name:
			return p
	return {}


func _build_starting_xi(club_name: String, db: PlayerDatabase, use_user_squad: bool) -> Array[Dictionary]:
	var pool: Array[Dictionary] = []
	if use_user_squad and not user_squad.is_empty():
		pool = user_squad.duplicate(true)
	elif db != null and db.has_method("get_players_by_club"):
		for raw: Dictionary in db.get_players_by_club(club_name):
			pool.append(raw.duplicate(true))

	# Filter out injured players first when selecting fit XI
	var fit_pool: Array[Dictionary] = []
	for p: Dictionary in pool:
		if int(p.get("injury_weeks", 0)) <= 0:
			fit_pool.append(p)
	if fit_pool.size() >= 11:
		pool = fit_pool

	var xi: Array[Dictionary] = []
	for i: int in range(mini(11, pool.size())):
		var p: Dictionary = pool[i]
		xi.append({
			"name": String(p.get("name", "%s #%d" % [club_name, i + 1])),
			"position": String(p.get("position", "GK" if i == 0 else "CM")),
			"ovr": int(p.get("ovr", 75)),
			"pace": int(p.get("pace", 74)),
			"shooting": int(p.get("shooting", 72)),
			"passing": int(p.get("passing", 74)),
			"dribbling": int(p.get("dribbling", 74)),
			"defending": int(p.get("defending", 72)),
			"physical": int(p.get("physical", 74)),
			"stamina": float(p.get("stamina", 100.0)),
		})

	while xi.size() < 11:
		var slot: int = xi.size()
		xi.append({
			"name": "%s #%d" % [club_name, slot + 1],
			"position": "GK" if slot == 0 else "CM",
			"ovr": 73,
			"pace": 72, "shooting": 70, "passing": 73,
			"dribbling": 72, "defending": 72, "physical": 73,
			"stamina": 100.0,
		})
	return xi


func _compute_lineup_ovr(xi: Array[Dictionary]) -> int:
	if xi.is_empty():
		return 75
	var total: int = 0
	for p: Dictionary in xi:
		total += int(p.get("ovr", 75))
	return clampi(int(round(float(total) / float(xi.size()))), 40, 99)


func _get_club_default_formation(club_name: String, db: PlayerDatabase) -> String:
	if db != null and db.has_method("get_club_info"):
		var info: Dictionary = db.get_club_info(club_name)
		var fmt: String = String(info.get("formation", info.get("default_formation", "")))
		if not fmt.is_empty():
			return fmt
	return "4-3-3"


func _get_club_kit_colors(club_name: String, db: PlayerDatabase, is_home_fallback: bool) -> Dictionary:
	if db != null and db.has_method("get_club_info"):
		var info: Dictionary = db.get_club_info(club_name)
		var pri_str: String = String(info.get("kit_color", info.get("primary_color", "")))
		var sec_str: String = String(info.get("secondary_color", ""))
		if not pri_str.is_empty() and Color.html_is_valid(pri_str):
			var pri := Color.from_string(pri_str, Color(0.82, 0.14, 0.18))
			var sec := Color.from_string(sec_str, Color(0.95, 0.95, 0.96)) if Color.html_is_valid(sec_str) else Color(0.95, 0.95, 0.96)
			return {"primary": pri, "secondary": sec}
	if is_home_fallback:
		return {"primary": Color(0.82, 0.14, 0.18), "secondary": Color(0.96, 0.96, 0.97)}
	return {"primary": Color(0.14, 0.36, 0.84), "secondary": Color(0.96, 0.84, 0.22)}


func _colors_clash(a: Color, b: Color) -> bool:
	var dr: float = a.r - b.r
	var dg: float = a.g - b.g
	var db_val: float = a.b - b.b
	return sqrt(dr * dr + dg * dg + db_val * db_val) < 0.38