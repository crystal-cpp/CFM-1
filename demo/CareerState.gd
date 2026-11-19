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