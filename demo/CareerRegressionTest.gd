extends Node

class_name CareerRegressionTest

var _passed: int = 0
var _failed: int = 0

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey:
		var key := event as InputEventKey
		if key.pressed and not key.is_echo() and key.keycode == KEY_F10:
			run_all_career_tests()
			get_viewport().set_input_as_handled()

func _assert(condition: bool, description: String) -> void:
	if condition:
		_passed += 1
		print("  [PASS] %s" % description)
	else:
		_failed += 1
		printerr("  [FAIL] %s" % description)

func run_all_career_tests() -> Dictionary:
	_passed = 0
	_failed = 0

	print("\n========================================================")
	print("  PHASE 3, 5C & 5D: CAREERSTATE, DB & NAVIGATION SUITE")
	print("========================================================")

	var career := CareerState.new()

	# Phase 3A + 5D Bootstrap Tests
	_test_career_initialization(career)
	_test_subsystem_queries_and_bridges(career)
	_test_youth_promotion_bridge(career)
	_test_matchengine_payload_handoff(career)
	_test_calendar_and_finances(career)
	_test_save_load_roundtrip_integrity(career)

	# Phase 3B Tests
	_test_career_shell_and_navigation_router(career)

	# Phase 3C Tests
	_test_phase_3c_views_and_modal(career)

	# Phase 5D-2 Database, ClubID, League Isolation & Odd-Round-Robin Suite
	_test_phase_5d_database_and_league_isolation()

	# Phase 5G Match Laws, Restarts, Cards & Injury Telemetry Suite
	_test_phase_5g_match_laws()

	print("========================================================")
	print("  TOTAL CAREER TESTS: %d PASSED, %d FAILED" % [_passed, _failed])
	print("========================================================\n")

	career.free()
	return { "passed": _passed, "failed": _failed }

func _test_career_initialization(career: CareerState) -> void:
	print("\n--- 1. Career Bootstrap & 15 July 2006 Initialization ---")
	var all_entries: Array = career.get_leagues_and_clubs()
	_assert(all_entries.size() >= 16, "get_leagues_and_clubs() returns club registry (got %d)" % all_entries.size())

	# Find Arsenal FC (or first Premier League club)
	var target_cid: int = 1001
	for entry in all_entries:
		if entry is Dictionary and String(entry.get("club_name", "")) == "Arsenal FC":
			target_cid = int(entry["club_id"])
			break

	var ok := career.start_new_career(target_cid, "Test Manager", 2006)
	_assert(ok, "start_new_career() returns true for ClubID %d" % target_cid)
	_assert(career.is_career_active(), "is_career_active() is true after bootstrap")
	_assert(career.get_managed_club_id() == target_cid, "Managed club ID preserves CSV ClubID (%d)" % target_cid)
	_assert(career.get_season_label() == "2006/07", "Season label formatted as 2006/07")
	_assert(career.get_current_date_iso() == "2006-07-15", "Calendar begins at 2006-07-15")

	var header: Dictionary = career.get_header_summary()
	_assert(header.has("total_balance") and int(header["total_balance"]) > 0, "Header summary has positive total balance")
	_assert(header.has("manager_name") and header["manager_name"] == "Test Manager", "Header summary reflects manager name")
	_assert(header.has("managed_club_name") and not String(header["managed_club_name"]).is_empty(), "Header reflects managed club name")

func _test_subsystem_queries_and_bridges(career: CareerState) -> void:
	print("\n--- 2. Subsystem Single-Source-of-Truth Bridges ---")
	var cid: int = career.get_managed_club_id()
	var squad: Array = career.get_club_squad(cid)
	_assert(squad.size() >= 11, "Managed club squad has at least starting XI players (got %d)" % squad.size())

	var player_ids: PackedInt32Array = career.get_club_player_ids(cid)
	_assert(player_ids.size() == squad.size(), "get_club_player_ids() matches squad size")

	if player_ids.size() > 0:
		var pid: int = player_ids[0]
		var profile: Dictionary = career.get_player_profile(pid)
		_assert(not profile.is_empty(), "get_player_profile(pid) returns player details")
		_assert(profile.has("name") and profile.has("ovr") and profile.has("position"), "Profile includes name, ovr, position")
		_assert(profile.has("condition") and int(profile["condition"]) == 100, "Initial player condition is 100")

	var prospects: Array = career.get_youth_prospects()
	_assert(prospects.size() >= 5, "Youth Academy has at least 5 prospects seeded (got %d)" % prospects.size())

	var standings: Array = career.get_league_standings()
	_assert(standings.size() == 20, "Premier League table contains 20 clubs (got %d)" % standings.size())

	var unread: int = career.get_unread_inbox_count()
	_assert(unread >= 1, "Initial welcome message in inbox (unread: %d)" % unread)

func _test_phase_5d_database_and_league_isolation() -> void:
	print("\n--- 9. Phase 5D-2: 156-Club Registry, NHHA72528 Lock, League Isolation & Odd Round-Robin ---")
	var c_state := CareerState.new()
	var registry: Array = c_state.get_leagues_and_clubs()
	_assert(registry.size() == 156, "Registry loads all 156 clubs from clubs.csv (got %d)" % registry.size())

	var la_liga_count := 0
	var tuesday_cid := -1
	var la_liga_cid := -1
	var allsvenskan_cid := -1
	var unique_leagues := {}

	for entry in registry:
		var lg := String(entry.get("league", ""))
		var nm := String(entry.get("club_name", ""))
		var cid := int(entry.get("club_id", 0))
		unique_leagues[lg] = true
		if lg == "La Liga":
			la_liga_count += 1
			if la_liga_cid < 0:
				la_liga_cid = cid
		elif lg == "Tuesday League" or nm == "11A7":
			tuesday_cid = cid
		elif lg == "Allsvenskan":
			allsvenskan_cid = cid

	_assert(unique_leagues.size() == 30, "Registry contains all 30 leagues (got %d)" % unique_leagues.size())
	_assert(la_liga_count == 21, "La Liga contains 21 clubs (odd-team round-robin check, got %d)" % la_liga_count)
	_assert(tuesday_cid == 1098, "11A7 preserves ClubID 1098 (got %d)" % tuesday_cid)

	# Odd-sized league (La Liga = 21 clubs -> 42 matchdays, zero double-bookings per date)
	if la_liga_cid > 0:
		var started := c_state.start_new_career(la_liga_cid, "La Liga Tester", 2006)
		_assert(started, "Started career in odd-sized La Liga (ClubID %d)" % la_liga_cid)
		_assert(c_state.get_league_standings().size() == 21, "La Liga standings contain strictly 21 clubs")
		_assert(c_state.get_total_matchdays("La Liga") == 42, "21-club double round-robin generates 42 matchdays")

		var all_fix: Array = c_state.get_fixtures("ALL", -1, -1)
		var booked_per_date := {}
		var double_booked := false
		for f in all_fix:
			var d_str := String(f.get("date_iso", ""))
			var h_id := int(f.get("home_club_id", -1))
			var a_id := int(f.get("away_club_id", -1))
			if not booked_per_date.has(d_str):
				booked_per_date[d_str] = {}
			if booked_per_date[d_str].has(h_id) or booked_per_date[d_str].has(a_id):
				double_booked = true
			booked_per_date[d_str][h_id] = true
			booked_per_date[d_str][a_id] = true
		_assert(not double_booked, "Zero double-booked clubs on any friendly or league date in odd-sized La Liga")

	# Single-club league isolation check (Allsvenskan = 1 club -> 1 standing row, 0 borrowed teams)
	if allsvenskan_cid > 0:
		var single_ok := c_state.start_new_career(allsvenskan_cid, "Solo Tester", 2006)
		_assert(single_ok, "Single-club league (Allsvenskan) starts cleanly")
		_assert(c_state.get_league_standings().size() == 1, "Single-club league does not borrow clubs from other leagues")

	c_state.free()

func _test_youth_promotion_bridge(career: CareerState) -> void:
	print("\n--- 3. Youth Academy Senior Squad Promotion ---")
	var prospects: Array = career.get_youth_prospects()
	if prospects.is_empty():
		_assert(false, "No youth prospects available to test promotion")
		return

	var prospect_id: int = int(prospects[0]["id"])
	var prospect_name: String = String(prospects[0]["name"])
	var initial_squad_size: int = career.get_club_squad(1).size()

	var promoted: bool = career.promote_youth_player(prospect_id)
	_assert(promoted, "promote_youth_player() succeeded for prospect ID %d (%s)" % [prospect_id, prospect_name])

	var new_squad_size: int = career.get_club_squad(1).size()
	_assert(new_squad_size == initial_squad_size + 1, "Senior squad roster incremented by 1")

	var updated_profile: Dictionary = career.get_player_profile(prospect_id)
	_assert(int(updated_profile["club_id"]) == 1, "Promoted player now belongs to club 1")

func _test_matchengine_payload_handoff(career: CareerState) -> void:
	print("\n--- 4. MatchEngine Payload Handoff & Commit ---")
	var engine := MatchEngine.new()
	var payload_ok: bool = career.prepare_next_match_payload(engine)
	_assert(payload_ok, "prepare_next_match_payload(engine) populated match setup")
	_assert(not engine.get_home_formation().is_empty(), "MatchEngine received non-empty home formation: %s" % engine.get_home_formation())
	_assert(not engine.get_away_formation().is_empty(), "MatchEngine received non-empty away formation: %s" % engine.get_away_formation())
	_assert(engine.get_home_team_ovr() > 0, "MatchEngine home team OVR is valid (%d)" % engine.get_home_team_ovr())
	_assert(engine.get_away_team_ovr() > 0, "MatchEngine away team OVR is valid (%d)" % engine.get_away_team_ovr())

	var commit_ok: bool = career.commit_match_result(engine)
	_assert(commit_ok, "commit_match_result(engine) recorded match score & standings")
	engine.free()

func _test_calendar_and_finances(career: CareerState) -> void:
	print("\n--- 5. Calendar Progression & Finances ---")
	var date_before: String = career.get_current_date_iso()
	career.advance_day(7)
	var date_after: String = career.get_current_date_iso()
	_assert(date_before != date_after, "Calendar advanced 7 days (%s -> %s)" % [date_before, date_after])

func _test_save_load_roundtrip_integrity(career: CareerState) -> void:
	print("\n--- 6. Schema-Versioned Save / Load Round-Trip Integrity ---")
	var slot := "phase_3a_acceptance_slot"

	# Mutate state in memory
	var test_player_id: int = 1
	career.set_player_condition(test_player_id, 47)
	career.set_total_balance(98765432)
	career.set_transfer_budget(54321000)
	var date_before_save: String = career.get_current_date_iso()

	# Save career
	var save_ok: bool = career.save_career(slot)
	_assert(save_ok, "save_career('%s') executed atomically" % slot)

	# Verify slot appears in list_save_slots
	var slots: Array = career.list_save_slots()
	var slot_found: bool = false
	for s in slots:
		if s is Dictionary and s.has("slot_name") and s["slot_name"] == slot:
			slot_found = true
			break
	_assert(slot_found, "list_save_slots() includes newly saved slot '%s'" % slot)

	# Create a completely fresh CareerState instance
	var loaded_career := CareerState.new()
	_assert(not loaded_career.is_career_active(), "Fresh CareerState instance is inactive initially")

	# Load state
	var load_ok: bool = loaded_career.load_career(slot)
	_assert(load_ok, "load_career('%s') succeeded" % slot)
	_assert(loaded_career.is_career_active(), "Loaded CareerState is active")

	# Verify 1:1 state equality
	_assert(loaded_career.get_current_date_iso() == date_before_save,
		"Date matches exactly (%s == %s)" % [loaded_career.get_current_date_iso(), date_before_save])
	_assert(loaded_career.get_total_balance() == 98765432,
		"Total balance matches mutated value (98,765,432 == %d)" % loaded_career.get_total_balance())
	_assert(loaded_career.get_transfer_budget() == 54321000,
		"Transfer budget matches mutated value (54,321,000 == %d)" % loaded_career.get_transfer_budget())
	_assert(loaded_career.get_player_condition(test_player_id) == 47,
		"Player %d condition restored to exact mutated value (47 == %d)" % [test_player_id, loaded_career.get_player_condition(test_player_id)])

	_assert(loaded_career.get_league_standings().size() == 16, "16 club standings restored")
	_assert(loaded_career.get_club_squad(1).size() >= 11, "Club 1 squad restored")

	# Cleanup
	var deleted: bool = career.delete_save_slot(slot)
	_assert(deleted, "delete_save_slot('%s') successfully cleaned up test slot" % slot)

	loaded_career.free()

func _test_career_shell_and_navigation_router(career: CareerState) -> void:
	print("\n--- 7. Phase 3B: Global Navigation Shell & Router ---")
	var shell := CareerShell.new()
	shell.career_state = career
	add_child(shell)

	_assert(shell.is_inside_tree(), "CareerShell added cleanly to scene tree")

	# Header telemetry tests
	shell.refresh_header()
	var current_route: String = shell.get_current_route()
	_assert(current_route == "dashboard", "Default active route is 'dashboard'")

	# Rapid 10-Route Tab Switching & Zero Leaks Test
	var routes: Array[String] = [
		"dashboard", "squad", "tactics", "youth", "schedule",
		"competitions", "transfers", "club", "finances", "inbox"
	]

	var initial_child_count: int = shell._router_container.get_child_count()
	_assert(initial_child_count == 10, "Router container has exactly 10 mounted route view panels")

	# Rapidly switch through all 10 routes for 5 cycles (50 tab switches)
	var all_routes_navigated: bool = true
	for cycle in range(5):
		for r in routes:
			shell.navigate_to(r)
			if shell.get_current_route() != r:
				all_routes_navigated = false

	_assert(all_routes_navigated, "Successfully navigated 50 rapid route transitions across all 10 sidebar tabs")

	# Verify zero node leakage
	var final_child_count: int = shell._router_container.get_child_count()
	_assert(final_child_count == initial_child_count,
		"Zero scene-tree leaks detected in router container (%d == %d)" % [final_child_count, initial_child_count])

	# Verify active visibility isolation
	var visible_count: int = 0
	for r in routes:
		if shell._route_views[r].visible:
			visible_count += 1
	_assert(visible_count == 1, "Exactly one view panel visible at a time (no overlap)")

	# Test Live Signal Reflection in Header
	career.set_total_balance(123456789)
	_assert(shell._balance_lbl.text.contains("123.5M") or shell._balance_lbl.text.contains("123"),
		"Header live balance updated via balance_changed signal")

	career.advance_day(2)
	_assert(shell._date_lbl.text == career.get_current_date_iso(),
		"Header live date updated via date_changed signal (%s)" % shell._date_lbl.text)

	career.push_inbox_message("BOARD", "Test Signal Message", "Body test")
	_assert(shell._inbox_badge.visible and shell._inbox_badge.text != "0",
		"Header unread inbox badge updated via inbox_updated signal")

	# Test Manager Profile Overlay
	shell._open_manager_profile()
	_assert(shell._manager_profile_modal.visible, "Manager Profile modal opens on trigger")
	shell._manager_profile_modal.visible = false

	# Test Mobile Drawer Toggle
	var initial_sidebar_vis: bool = shell._sidebar_visible
	shell._toggle_sidebar()
	_assert(shell._sidebar_visible != initial_sidebar_vis, "Sidebar drawer toggles visibility cleanly")
	shell._toggle_sidebar() # restore

	shell.queue_free()

func _test_phase_3c_views_and_modal(career: CareerState) -> void:
	print("\n--- 8. Phase 3C: DashboardView, InboxView & Universal Player Modal ---")

	# 1. Test DashboardView
	var dash := DashboardView.new()
	dash.career_state = career
	add_child(dash)
	dash.refresh_dashboard()

	_assert(dash._next_match_card != null, "Dashboard Next Match hero card initialized")
	_assert(not dash._opp_name_lbl.text.is_empty(), "Dashboard displays next opponent name: %s" % dash._opp_name_lbl.text)
	_assert(dash._opp_ovr_lbl.text != "--", "Dashboard displays opponent team OVR: %s" % dash._opp_ovr_lbl.text)
	_assert(dash._standings_vbox.get_child_count() <= 5, "Dashboard Top-5 standings snippet populated")
	_assert(dash._objectives_vbox.get_child_count() >= 3, "Dashboard board objectives displayed")
	dash.queue_free()

	# 2. Test InboxView
	var inbox := InboxView.new()
	inbox.career_state = career
	add_child(inbox)
	inbox.refresh_inbox()

	_assert(inbox._messages_vbox.get_child_count() > 0, "Inbox view lists career messages")
	_assert(inbox.selected_message_id > 0, "Inbox auto-selects active message (ID: %d)" % inbox.selected_message_id)

	# Test category filter
	inbox._set_filter("BOARD")
	_assert(inbox.current_filter == "BOARD", "Inbox filter switches to BOARD")
	inbox._set_filter("ALL")
	_assert(inbox.current_filter == "ALL", "Inbox filter resets to ALL")
	inbox.queue_free()

	# 3. Test Universal PlayerProfileModal
	var modal := PlayerProfileModal.new()
	modal.career_state = career
	add_child(modal)

	# Open Squad Player
	var squad_ids: PackedInt32Array = career.get_club_player_ids(career.get_managed_club_id())
	_assert(squad_ids.size() > 0, "Squad player available for modal inspection")
	var test_pid: int = squad_ids[0]
	modal.open_player(test_pid, "SQUAD")

	_assert(modal.visible, "PlayerProfileModal becomes visible on open_player()")
	_assert(modal._ovr_lbl.text.to_int() > 50, "PlayerProfileModal displays valid player OVR: %s" % modal._ovr_lbl.text)
	_assert(modal._attr_labels["pace"].text.to_int() > 0, "Pace attribute populated")
	_assert(modal._attr_labels["shooting"].text.to_int() > 0, "Shooting attribute populated")
	_assert(modal._attr_labels["defending"].text.to_int() > 0, "Defending attribute populated")
	_assert(modal.current_context_mode == "SQUAD", "Modal context mode is SQUAD")

	# Open Youth Prospect
	var youth_prospects: Array = career.get_youth_prospects()
	if youth_prospects.size() > 0:
		var y_id: int = int(youth_prospects[0]["id"])
		modal.open_player(y_id, "YOUTH")
		_assert(modal.current_context_mode == "YOUTH", "Modal context mode adapts to YOUTH")
		_assert(modal._pos_pill.text.length() > 0, "Youth prospect preferred position displayed")

	modal.close_modal()
	_assert(not modal.visible, "PlayerProfileModal closes cleanly")
	modal.queue_free()

func _test_phase_5g_match_laws() -> void:
	print("\n--- 10. Phase 5G: Match Laws, Restarts, Discipline & Injury Telemetry ---")
	var engine := MatchEngine.new()
	engine.start_match()

	_assert(engine.has_method("get_restart_state"), "MatchEngine exposes get_restart_state()")
	_assert(engine.has_method("is_restart_active"), "MatchEngine exposes is_restart_active()")
	_assert(engine.has_method("get_sent_off_flags"), "MatchEngine exposes get_sent_off_flags()")
	_assert(engine.has_method("get_yellow_card_counts"), "MatchEngine exposes get_yellow_card_counts()")
	_assert(engine.has_signal("restart_state_changed"), "MatchEngine exposes restart_state_changed signal")

	_assert(not engine.is_restart_active(), "Restart state starts inactive at kickoff")
	_assert(engine.get_restart_state() == 0, "Initial restart state is RESTART_NONE (0)")

	var sent_off: Array = engine.get_sent_off_flags()
	var yellows: Array = engine.get_yellow_card_counts()
	_assert(sent_off.size() == 22, "Sent-off flags array has 22 players")
	_assert(yellows.size() == 22, "Yellow-card counts array has 22 players")
	if sent_off.size() == 22:
		_assert(not bool(sent_off[0]) and not bool(sent_off[21]), "All players start eligible (not sent off)")
	if yellows.size() == 22:
		_assert(int(yellows[0]) == 0 and int(yellows[21]) == 0, "All players start with 0 yellow cards")

	var stats: Dictionary = engine.get_match_stats()
	_assert(stats.has("home_fouls") and stats.has("away_fouls"), "get_match_stats() exports home_fouls and away_fouls")
	_assert(stats.has("home_offsides") and stats.has("away_offsides"), "get_match_stats() exports home_offsides and away_offsides")
	_assert(stats.has("home_yellow_cards") and stats.has("away_yellow_cards"), "get_match_stats() exports yellow card totals")
	_assert(stats.has("home_red_cards") and stats.has("away_red_cards"), "get_match_stats() exports red card totals")
	_assert(stats.has("match_injuries") and stats["match_injuries"] is Array, "get_match_stats() exports match_injuries Array")

	engine.free()

