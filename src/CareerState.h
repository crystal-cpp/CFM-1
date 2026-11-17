#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
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

struct FixtureRecord {
	int32_t fixture_id = 0;
	int32_t matchday = 1;
	std::string date_iso;
	std::string competition;       // e.g., "Premier Division", "National Cup"
	int32_t home_club_id = -1;
	int32_t away_club_id = -1;
	int32_t home_goals = -1;
	int32_t away_goals = -1;
	bool played = false;
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
	static constexpr uint32_t SAVE_SCHEMA_VERSION = 1;

private:
	bool _career_active = false;
	int32_t _managed_club_id = -1;

	// Calendar & Season Clock
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
	std::unordered_map<int32_t, std::unique_ptr<Team>> _clubs;
	std::unordered_map<int32_t, int32_t> _captain_by_club;
	std::unordered_map<int32_t, std::shared_ptr<Player>> _players_by_id;
	int32_t _next_player_id = 1;

	// Internal Helpers
	std::string _format_date_iso() const;
	std::string _format_season_label() const;
	void _advance_single_day();
	void _recalculate_standings();
	void _seed_default_objectives();
	void _simulate_ai_fixtures_for_current_date();
	void _process_weekly_progression();
	void _process_annual_rollover();
	Dictionary _player_to_dict(const std::shared_ptr<Player> &p) const;

protected:
	static void _bind_methods();

public:
	CareerState();
	~CareerState() override;

	// --- Career Bootstrap & Time Progression ---
	bool start_new_career(int32_t p_club_id, const String &p_manager_name, int32_t p_start_year = 2026);
	bool is_career_active() const { return _career_active; }
	void advance_day(int32_t p_days = 1);

	// --- Top Header & Manager Career Queries ---
	int32_t get_managed_club_id() const { return _managed_club_id; }
	String get_current_date_iso() const;
	String get_season_label() const;
	int32_t get_managed_club_league_rank() const;
	Dictionary get_header_summary() const;
	Dictionary get_manager_profile() const;
	Dictionary get_finances() const;
	TypedArray<Dictionary> get_board_objectives() const;
	TypedArray<Dictionary> get_league_standings() const;
	Dictionary get_next_fixture() const;
	Dictionary get_club_info(int32_t p_club_id) const;

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