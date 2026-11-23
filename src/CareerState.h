#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "mainplayer.h"
#include "team.h"
#include "youth_academy.h"
#include "player_database.h"

namespace godot {

class MatchEngine;

struct ManagerProfile {
	std::string manager_name = "Manager";
	std::string nationality = "Vietnam";
	std::string preferred_formation = "4-3-3";
	int32_t reputation = 50;        // 1..100
	int32_t board_confidence = 75;  // 0..100 (job security)
	int32_t seasons_managed = 0;
	int32_t trophies_won = 0;
	int32_t career_wins = 0;
	int32_t career_draws = 0;
	int32_t career_losses = 0;
};

struct ClubFinances {
	int64_t total_balance = 0;
	int64_t transfer_budget = 0;
	int64_t wage_budget = 0;       // Weekly wage cap
	int64_t weekly_wage_bill = 0;  // Current weekly outlay
};

struct BoardObjective {
	int32_t id = 0;
	std::string category;          // "LEAGUE", "CUP", "FINANCE", "YOUTH"
	std::string title;
	std::string description;
	int32_t target_value = 0;
	int32_t current_value = 0;
	int64_t reward_amount = 0;
	bool is_mandatory = true;
	bool is_completed = false;
};

struct InboxMessage {
	int32_t id = 0;
	std::string date_iso;          // "YYYY-MM-DD"
	std::string category;          // "MATCH", "TRANSFER", "BOARD", "YOUTH", "INJURY"
	std::string title;
	std::string body;
	bool is_read = false;
	std::string action_route;      // e.g., "squad", "youth", "transfers", "club"
	int32_t target_entity_id = -1; // player_id, club_id, or fixture_id
};

struct MatchEventRecord {
	int32_t minute = 1;
	int32_t club_id = -1;
	int32_t player_id = -1;
	std::string player_name;
	int32_t assist_player_id = -1;
	std::string assist_player_name;
	std::string type = "GOAL";     // "GOAL", "PENALTY", "YELLOW", "RED"
};

struct FixtureRecord {
	int32_t fixture_id = 0;
	int32_t week = 1;              // Week 1-2: Pre-Season Window; Week 3+: Competitive Season
	int32_t matchday = 1;          // 0 for Pre-Season Friendlies, 1..N for League Matchdays
	std::string date_iso;
	std::string competition;       // e.g., "Pre-Season Friendly", "Premier Division"
	int32_t home_club_id = -1;
	int32_t away_club_id = -1;
	int32_t home_goals = -1;
	int32_t away_goals = -1;
	bool played = false;
	bool is_friendly = false;      // Friendlies do not award league standings points
	std::string stadium;
	int32_t attendance = 0;
	std::string match_importance = "STANDARD"; // "FRIENDLY", "STANDARD", "RIVALRY_DERBY", "TOP_CLASH", "TITLE_DECIDER", "CUP_FINAL"
	std::vector<MatchEventRecord> events;
};

struct LeagueStandingRow {
	int32_t club_id = -1;
	std::string club_name;
	int32_t played = 0;
	int32_t won = 0;
	int32_t drawn = 0;
	int32_t lost = 0;
	int32_t goals_for = 0;
	int32_t goals_against = 0;
	int32_t points = 0;
};

class CareerState : public Node {
	GDCLASS(CareerState, Node)

public:
	static constexpr uint32_t SAVE_SCHEMA_VERSION = 2;
	Dictionary prepare_match_context(const Dictionary &p_fixture = Dictionary(), const Ref<PlayerDatabase> &p_db = Ref<PlayerDatabase>()) const;
	static constexpr double DEFAULT_STAMINA_RECOVERY_PER_DAY = 0.30; // Tunable 0.25..0.30

private:
	bool _career_active = false;
	int32_t _managed_club_id = -1;

	// Calendar & Season Clock (Season runs July 1 -> May 31; June off-season)
	int32_t _season_start_year = 2026;
	int32_t _year = 2026;
	int32_t _month = 7;
	int32_t _day = 1;
	int32_t _current_matchday = 1;

	// Manager Career State
	ManagerProfile _manager;
	ClubFinances _finances;
	std::vector<BoardObjective> _board_objectives;
	std::vector<InboxMessage> _inbox;
	int32_t _next_inbox_id = 1;

	// League & Fixtures
	std::vector<FixtureRecord> _fixtures;
	std::vector<LeagueStandingRow> _standings;

	// Owned C++23 Subsystems (Single Source of Truth)
	Ref<PlayerDatabase> _player_db;
	std::unique_ptr<YouthAcademy> _youth_academy;
	std::vector<std::shared_ptr<Player>> _hidden_youth_prospects;
	std::unordered_map<int32_t, std::unique_ptr<Team>> _clubs;
	std::unordered_map<int32_t, int32_t> _captain_by_club;
	std::unordered_map<int32_t, std::shared_ptr<Player>> _players_by_id;
	int32_t _next_player_id = 1;

	// Phase 5D: Clubs.csv registry, codes & lock state
	std::unordered_map<std::string, int32_t> _club_id_by_name_lower;
	std::unordered_map<int32_t, int32_t> _club_id_by_csv_id;
	std::unordered_set<std::string> _redeemed_codes;
	bool _tuesday_league_unlocked = false;
	void _load_unlock_persistence();
	void _save_unlock_persistence() const;
	bool _is_club_or_league_locked(const std::string &club_name, const std::string &league_name) const;
	static std::string _normalize_club_key(const String &p_club_name);

	double _stamina_recovery_rate_per_day = DEFAULT_STAMINA_RECOVERY_PER_DAY;

	void _process_daily_recovery_and_injuries();
	bool _ensure_club_valid_matchday_xi(int32_t p_club_id);

	// Internal Helpers
	std::string _format_date_iso() const;
	std::string _format_season_label() const;
	int32_t _compute_season_week() const;
	void _generate_season_fixtures(int32_t start_year);
	void _advance_single_day();
	void _recalculate_standings();
	void _seed_default_objectives();
	void _simulate_ai_fixtures_for_current_date();
	void _populate_fixture_goal_events(FixtureRecord &fix);
	std::string _classify_match_importance(int32_t home_id, int32_t away_id, int32_t matchday, const std::string &competition) const;
	Dictionary _fixture_to_dict(const FixtureRecord &fix, bool include_events = false) const;
	void _process_weekly_progression();
	void _process_end_of_season_may();
	void _process_annual_rollover();
	void _evaluate_hidden_youth_prospects();
	Dictionary _player_to_dict(const std::shared_ptr<Player> &p) const;

protected:
	static void _bind_methods();

public:
	CareerState();
	~CareerState() override;

	// --- Career Bootstrap & Time Progression ---
	bool start_new_career(int32_t p_club_id, const String &p_manager_name, int32_t p_start_year = 2006);
	TypedArray<Dictionary> get_leagues_and_clubs() const;
	bool is_promo_code_redeemed(const String &p_code) const;
	bool is_tuesday_league_unlocked() const { return _tuesday_league_unlocked; }
	Dictionary redeem_promo_code(const String &p_code);
	bool is_career_active() const { return _career_active; }
	void advance_day(int32_t p_days = 1);
	int32_t advance_to_next_matchday();

	// --- Phase 5F: Stamina Persistence, Daily Recovery & Injury System ---

	void set_stamina_recovery_rate_per_day(double p_rate);
	double get_stamina_recovery_rate_per_day() const { return _stamina_recovery_rate_per_day; }
	int32_t project_player_stamina_on_date(int32_t p_player_id, const String &p_target_date_iso) const;

	Dictionary apply_player_injury(int32_t p_player_id, int32_t p_severity_tier, int32_t p_days = -1);
	bool clear_player_injury(int32_t p_player_id);
	TypedArray<Dictionary> get_injured_players(int32_t p_club_id = -1) const;
	bool ensure_club_valid_matchday_xi(int32_t p_club_id);

	// --- Top Header & Manager Career Queries ---
	int32_t get_managed_club_id() const { return _managed_club_id; }
	String get_current_date_iso() const;
	String get_season_label() const;
	Dictionary get_season_phase_info() const;
	int32_t get_managed_club_league_rank() const;
	Dictionary get_header_summary() const;
	Dictionary get_manager_profile() const;
	Dictionary get_finances() const;
	TypedArray<Dictionary> get_board_objectives() const;
	TypedArray<Dictionary> get_league_standings() const;
	Dictionary get_next_fixture() const;
	Dictionary get_club_info(int32_t p_club_id) const;

	// --- Phase 5A / 5B: Schedule & Fixture Queries ---
	TypedArray<Dictionary> get_fixtures(const String &p_competition = "", int32_t p_matchday = -1, int32_t p_club_id = -1) const;
	Dictionary get_fixture_details(int32_t p_fixture_id) const;
	int32_t get_total_matchdays(const String &p_competition = "") const;
	int32_t get_current_matchday() const;

	// --- Subsystem Read/Write Bridges (Zero Duplicate State) ---
	TypedArray<Dictionary> get_club_squad(int32_t p_club_id) const;
	Dictionary get_player_profile(int32_t p_player_id) const;
	TypedArray<Dictionary> get_youth_prospects() const;
	bool promote_youth_player(int32_t p_prospect_id);
	PackedInt32Array get_club_player_ids(int32_t p_club_id) const;
	int32_t get_player_count() const;

	// --- Phase 4: Squad Management, Permanent Tactics & Youth Academy ---
	bool set_starting_xi(int32_t p_club_id, const PackedInt32Array &p_xi_ids);
	bool swap_squad_players(int32_t p_club_id, int32_t p_player_a_id, int32_t p_player_b_id);
	bool set_club_captain(int32_t p_club_id, int32_t p_player_id);
	int32_t get_club_captain(int32_t p_club_id) const;

	Dictionary get_club_tactics(int32_t p_club_id) const;
	bool set_club_tactics(int32_t p_club_id, const Dictionary &p_tactics);

	Dictionary get_youth_academy_info() const;
	bool upgrade_youth_facility();
	bool upgrade_youth_academy();
	bool hire_youth_coach(const String &p_name, int32_t p_skill, double p_salary, const String &p_specialty);
	bool fire_youth_coach(int32_t p_coach_index);
	Dictionary scout_youth_prospect();

	// Mutation / State testing helpers
	void set_player_condition(int32_t p_player_id, int32_t p_condition);
	int32_t get_player_condition(int32_t p_player_id) const;
	void set_total_balance(int64_t p_balance);
	int64_t get_total_balance() const;
	void set_transfer_budget(int64_t p_budget);
	int64_t get_transfer_budget() const;
	void set_wage_budget(int64_t p_budget);
	int64_t get_wage_budget() const;

	// --- MatchEngine Handoff & Post-Match Commit ---
	bool prepare_next_match_payload(MatchEngine *p_engine) const;
	bool commit_match_result(MatchEngine *p_engine);

	// --- Event-Driven News / Inbox ---
	int32_t push_inbox_message(const String &p_category, const String &p_title, const String &p_body,
			const String &p_action_route = "", int32_t p_target_entity_id = -1);
	TypedArray<Dictionary> get_inbox_messages(const String &p_category_filter = "") const;
	bool mark_inbox_read(int32_t p_message_id);
	void mark_all_inbox_read();
	int32_t get_unread_inbox_count() const;

	// --- Versioned Save / Load Serialization ---
	bool save_career(const String &p_slot_name);
	bool load_career(const String &p_slot_name);
	TypedArray<Dictionary> list_save_slots() const;
	bool delete_save_slot(const String &p_slot_name);
};

} // namespace godot
