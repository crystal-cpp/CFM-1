#include "CareerState.h"
#include "match_engine.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <unordered_set>

using namespace godot;

namespace {

static int days_in_month(int year, int month) {
	if (month == 2) {
		bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
		return is_leap ? 29 : 28;
	}
	if (month == 4 || month == 6 || month == 9 || month == 11) {
		return 30;
	}
	return 31;
}

static const char *DEFAULT_CLUB_NAMES[16] = {
	"Arsenal", "Chelsea", "Manchester United", "Liverpool",
	"Barcelona", "Real Madrid", "AC Milan", "Juventus",
	"Inter Milan", "Bayern Munich", "Lyon", "AS Roma",
	"Valencia", "Porto", "Ajax", "Benfica"
};

} // namespace

static int evaluate_slot_fit_433(const std::string &player_pos, const std::string &slot_pos) {
	if (slot_pos == "GK") {
		return (player_pos == "GK") ? 100 : 0;
	}
	if (player_pos == "GK") {
		return 0; // Never put a GK in an outfield slot
	}
	if (player_pos == slot_pos) {
		return 100; // Exact natural position
	}

	// Compatible secondary positions
	if (slot_pos == "LB" && (player_pos == "LWB" || player_pos == "LM" || player_pos == "RB")) return 95;
	if (slot_pos == "RB" && (player_pos == "RWB" || player_pos == "RM" || player_pos == "LB")) return 95;
	if (slot_pos == "CB" && (player_pos == "CDM" || player_pos == "LB" || player_pos == "RB")) return 95;
	if (slot_pos == "CDM" && (player_pos == "CM" || player_pos == "CB")) return 95;
	if (slot_pos == "CM" && (player_pos == "CDM" || player_pos == "CAM" || player_pos == "LM" || player_pos == "RM")) return 95;
	if (slot_pos == "LW" && (player_pos == "LM" || player_pos == "CAM" || player_pos == "CF" || player_pos == "ST" || player_pos == "RW")) return 95;
	if (slot_pos == "RW" && (player_pos == "RM" || player_pos == "CAM" || player_pos == "CF" || player_pos == "ST" || player_pos == "LW")) return 95;
	if (slot_pos == "ST" && (player_pos == "CF" || player_pos == "CAM" || player_pos == "LW" || player_pos == "RW")) return 95;

	return 80; // Out-of-position outfield fallback
}

static void auto_pick_best_xi_433(Team &team) {
	static const char *SLOT_POSITIONS[11] = {
		"GK", "LB", "CB", "CB", "RB", "CDM", "CM", "CM", "LW", "RW", "ST"
	};

	auto &roster = team.GetRoster();
	if (roster.empty()) return;

	std::vector<std::shared_ptr<Player>> xi(11, nullptr);
	std::unordered_set<int32_t> used_ids;

	auto pick_for_slot = [&](int slot_idx, int min_fit) {
		if (xi[slot_idx] != nullptr) return;
		const std::string target_pos = SLOT_POSITIONS[slot_idx];
		std::shared_ptr<Player> best_player = nullptr;
		int best_score = -1;

		for (const auto &p : roster) {
			if (!p || used_ids.count(p->GetId())) continue;
			std::string p_pos = position_to_string(p->GetPosition());
			int fit = evaluate_slot_fit_433(p_pos, target_pos);
			if (fit < min_fit) continue;

			int weighted_ovr = p->GetOVR() * fit;
			if (weighted_ovr > best_score) {
				best_score = weighted_ovr;
				best_player = p;
			}
		}

		if (best_player) {
			xi[slot_idx] = best_player;
			used_ids.insert(best_player->GetId());
		}
	};

	// Pass 1: Exact position matches (GK -> GK, LB -> LB, CB -> CB, ST -> ST, etc.)
	for (int s = 0; s < 11; s++) pick_for_slot(s, 100);
	// Pass 2: Compatible secondary positions (CAM -> CM/LW/RW, CF -> ST, etc.)
	for (int s = 0; s < 11; s++) pick_for_slot(s, 95);
	// Pass 3: Best remaining outfielder (still blocking outfielders from GK slot 0)
	for (int s = 0; s < 11; s++) pick_for_slot(s, 80);

	// Rebuild roster so indices 0..10 are Starting XI in exact slot order, followed by Bench/Reserves
	std::vector<std::shared_ptr<Player>> ordered_roster;
	std::vector<std::shared_ptr<Player>> final_xi;
	ordered_roster.reserve(roster.size());
	final_xi.reserve(11);

	for (int s = 0; s < 11; s++) {
		if (xi[s]) {
			xi[s]->SetIsStartingXI(true);
			ordered_roster.push_back(xi[s]);
			final_xi.push_back(xi[s]);
		}
	}
	for (const auto &p : roster) {
		if (p && !used_ids.count(p->GetId())) {
			p->SetIsStartingXI(false);
			ordered_roster.push_back(p);
		}
	}

	roster = std::move(ordered_roster);
	team.SetStartingXI(final_xi);
}

void CareerState::_bind_methods() {
	// Career Bootstrap & Time Progression
	ClassDB::bind_method(D_METHOD("start_new_career", "club_id", "manager_name", "start_year"), &CareerState::start_new_career, DEFVAL(2026));
	ClassDB::bind_method(D_METHOD("is_career_active"), &CareerState::is_career_active);
	ClassDB::bind_method(D_METHOD("advance_day", "days"), &CareerState::advance_day, DEFVAL(1));

	// Top Header & Manager Career Queries
	ClassDB::bind_method(D_METHOD("get_managed_club_id"), &CareerState::get_managed_club_id);
	ClassDB::bind_method(D_METHOD("get_current_date_iso"), &CareerState::get_current_date_iso);
	ClassDB::bind_method(D_METHOD("get_season_label"), &CareerState::get_season_label);
	ClassDB::bind_method(D_METHOD("get_managed_club_league_rank"), &CareerState::get_managed_club_league_rank);
	ClassDB::bind_method(D_METHOD("get_header_summary"), &CareerState::get_header_summary);
	ClassDB::bind_method(D_METHOD("get_manager_profile"), &CareerState::get_manager_profile);
	ClassDB::bind_method(D_METHOD("get_finances"), &CareerState::get_finances);
	ClassDB::bind_method(D_METHOD("get_board_objectives"), &CareerState::get_board_objectives);
	ClassDB::bind_method(D_METHOD("get_league_standings"), &CareerState::get_league_standings);
	ClassDB::bind_method(D_METHOD("get_next_fixture"), &CareerState::get_next_fixture);
	ClassDB::bind_method(D_METHOD("get_club_info", "club_id"), &CareerState::get_club_info);

	// Subsystem Bridges
	ClassDB::bind_method(D_METHOD("get_club_squad", "club_id"), &CareerState::get_club_squad);
	ClassDB::bind_method(D_METHOD("get_player_profile", "player_id"), &CareerState::get_player_profile);
	ClassDB::bind_method(D_METHOD("get_youth_prospects"), &CareerState::get_youth_prospects);
	ClassDB::bind_method(D_METHOD("promote_youth_player", "prospect_id"), &CareerState::promote_youth_player);
	ClassDB::bind_method(D_METHOD("get_club_player_ids", "club_id"), &CareerState::get_club_player_ids);
	ClassDB::bind_method(D_METHOD("get_player_count"), &CareerState::get_player_count);

	// Phase 4: Squad Management, Permanent Tactics & Youth Academy
	ClassDB::bind_method(D_METHOD("set_starting_xi", "club_id", "xi_ids"), &CareerState::set_starting_xi);
	ClassDB::bind_method(D_METHOD("swap_squad_players", "club_id", "player_a_id", "player_b_id"), &CareerState::swap_squad_players);
	ClassDB::bind_method(D_METHOD("set_club_captain", "club_id", "player_id"), &CareerState::set_club_captain);
	ClassDB::bind_method(D_METHOD("get_club_captain", "club_id"), &CareerState::get_club_captain);

	ClassDB::bind_method(D_METHOD("get_club_tactics", "club_id"), &CareerState::get_club_tactics);
	ClassDB::bind_method(D_METHOD("set_club_tactics", "club_id", "tactics"), &CareerState::set_club_tactics);

	ClassDB::bind_method(D_METHOD("get_youth_academy_info"), &CareerState::get_youth_academy_info);
	ClassDB::bind_method(D_METHOD("upgrade_youth_facility"), &CareerState::upgrade_youth_facility);
	ClassDB::bind_method(D_METHOD("upgrade_youth_academy"), &CareerState::upgrade_youth_academy);
	ClassDB::bind_method(D_METHOD("hire_youth_coach", "name", "skill", "salary", "specialty"), &CareerState::hire_youth_coach);
	ClassDB::bind_method(D_METHOD("fire_youth_coach", "coach_index"), &CareerState::fire_youth_coach);
	ClassDB::bind_method(D_METHOD("scout_youth_prospect"), &CareerState::scout_youth_prospect);

	// Mutation & Testing Helpers
	ClassDB::bind_method(D_METHOD("set_player_condition", "player_id", "condition"), &CareerState::set_player_condition);
	ClassDB::bind_method(D_METHOD("get_player_condition", "player_id"), &CareerState::get_player_condition);
	ClassDB::bind_method(D_METHOD("set_total_balance", "balance"), &CareerState::set_total_balance);
	ClassDB::bind_method(D_METHOD("get_total_balance"), &CareerState::get_total_balance);
	ClassDB::bind_method(D_METHOD("set_transfer_budget", "budget"), &CareerState::set_transfer_budget);
	ClassDB::bind_method(D_METHOD("get_transfer_budget"), &CareerState::get_transfer_budget);
	ClassDB::bind_method(D_METHOD("set_wage_budget", "budget"), &CareerState::set_wage_budget);
	ClassDB::bind_method(D_METHOD("get_wage_budget"), &CareerState::get_wage_budget);

	// MatchEngine Handoff & Post-Match Commit
	ClassDB::bind_method(D_METHOD("prepare_next_match_payload", "engine"), &CareerState::prepare_next_match_payload);
	ClassDB::bind_method(D_METHOD("commit_match_result", "engine"), &CareerState::commit_match_result);

	// Event-Driven News / Inbox
	ClassDB::bind_method(D_METHOD("push_inbox_message", "category", "title", "body", "action_route", "target_entity_id"),
			&CareerState::push_inbox_message, DEFVAL(""), DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_inbox_messages", "category_filter"), &CareerState::get_inbox_messages, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("mark_inbox_read", "message_id"), &CareerState::mark_inbox_read);
	ClassDB::bind_method(D_METHOD("mark_all_inbox_read"), &CareerState::mark_all_inbox_read);
	ClassDB::bind_method(D_METHOD("get_unread_inbox_count"), &CareerState::get_unread_inbox_count);

	// Versioned Save / Load Serialization
	ClassDB::bind_method(D_METHOD("save_career", "slot_name"), &CareerState::save_career);
	ClassDB::bind_method(D_METHOD("load_career", "slot_name"), &CareerState::load_career);
	ClassDB::bind_method(D_METHOD("list_save_slots"), &CareerState::list_save_slots);
	ClassDB::bind_method(D_METHOD("delete_save_slot", "slot_name"), &CareerState::delete_save_slot);

	// Signals
	ADD_SIGNAL(MethodInfo("date_changed", PropertyInfo(Variant::STRING, "new_date_iso")));
	ADD_SIGNAL(MethodInfo("balance_changed", PropertyInfo(Variant::INT, "total_balance"), PropertyInfo(Variant::INT, "transfer_budget"), PropertyInfo(Variant::INT, "wage_budget")));
	ADD_SIGNAL(MethodInfo("inbox_updated", PropertyInfo(Variant::INT, "unread_count")));
	ADD_SIGNAL(MethodInfo("standings_updated"));
	ADD_SIGNAL(MethodInfo("squad_updated", PropertyInfo(Variant::INT, "club_id")));
	ADD_SIGNAL(MethodInfo("tactics_updated", PropertyInfo(Variant::INT, "club_id")));
	ADD_SIGNAL(MethodInfo("youth_updated"));
	ADD_SIGNAL(MethodInfo("career_saved", PropertyInfo(Variant::STRING, "slot_name")));
	ADD_SIGNAL(MethodInfo("career_loaded", PropertyInfo(Variant::STRING, "slot_name")));
}

CareerState::CareerState() {
	_youth_academy = std::make_unique<YouthAcademy>();
}

CareerState::~CareerState() = default;

std::string CareerState::_format_date_iso() const {
	std::ostringstream ss;
	ss << _year << "-"
	   << std::setw(2) << std::setfill('0') << _month << "-"
	   << std::setw(2) << std::setfill('0') << _day;
	return ss.str();
}

std::string CareerState::_format_season_label() const {
	std::ostringstream ss;
	int next_year = (_year % 100) + 1;
	ss << _year << "/" << std::setw(2) << std::setfill('0') << next_year;
	return ss.str();
}

String CareerState::get_current_date_iso() const {
	return String(_format_date_iso().c_str());
}

String CareerState::get_season_label() const {
	return String(_format_season_label().c_str());
}

void CareerState::_seed_default_objectives() {
	_board_objectives.clear();

	BoardObjective obj1;
	obj1.id = 1;
	obj1.category = "LEAGUE";
	obj1.title = "Top 4 Finish";
	obj1.description = "Secure qualification for continental competition by finishing in the top 4.";
	obj1.target_value = 4;
	obj1.current_value = get_managed_club_league_rank();
	obj1.reward_amount = 5000000;
	obj1.is_mandatory = true;
	obj1.is_completed = false;
	_board_objectives.push_back(obj1);

	BoardObjective obj2;
	obj2.id = 2;
	obj2.category = "FINANCE";
	obj2.title = "Wage Cap Discipline";
	obj2.description = "Maintain weekly wage outlay strictly within the allocated wage budget.";
	obj2.target_value = 0;
	obj2.current_value = (_finances.weekly_wage_bill <= _finances.wage_budget) ? 1 : 0;
	obj2.reward_amount = 2000000;
	obj2.is_mandatory = true;
	obj2.is_completed = (_finances.weekly_wage_bill <= _finances.wage_budget);
	_board_objectives.push_back(obj2);

	BoardObjective obj3;
	obj3.id = 3;
	obj3.category = "YOUTH";
	obj3.title = "Promote Academy Graduate";
	obj3.description = "Promote at least one standout youth academy prospect into the senior squad.";
	obj3.target_value = 1;
	obj3.current_value = 0;
	obj3.reward_amount = 1500000;
	obj3.is_mandatory = false;
	obj3.is_completed = false;
	_board_objectives.push_back(obj3);
}

void CareerState::_recalculate_standings() {
	for (auto &row : _standings) {
		row.played = 0;
		row.won = 0;
		row.drawn = 0;
		row.lost = 0;
		row.goals_for = 0;
		row.goals_against = 0;
		row.points = 0;
	}

	std::unordered_map<int32_t, size_t> club_to_idx;
	for (size_t i = 0; i < _standings.size(); i++) {
		club_to_idx[_standings[i].club_id] = i;
	}

	for (const auto &fix : _fixtures) {
		if (!fix.played) continue;
		if (club_to_idx.find(fix.home_club_id) == club_to_idx.end() ||
			club_to_idx.find(fix.away_club_id) == club_to_idx.end()) {
			continue;
		}

		auto &h = _standings[club_to_idx[fix.home_club_id]];
		auto &a = _standings[club_to_idx[fix.away_club_id]];

		h.played++;
		a.played++;
		h.goals_for += fix.home_goals;
		h.goals_against += fix.away_goals;
		a.goals_for += fix.away_goals;
		a.goals_against += fix.home_goals;

		if (fix.home_goals > fix.away_goals) {
			h.won++;
			h.points += 3;
			a.lost++;
		} else if (fix.home_goals < fix.away_goals) {
			a.won++;
			a.points += 3;
			h.lost++;
		} else {
			h.drawn++;
			h.points += 1;
			a.drawn++;
			a.points += 1;
		}
	}

	std::sort(_standings.begin(), _standings.end(), [](const LeagueStandingRow &a, const LeagueStandingRow &b) {
		if (a.points != b.points) return a.points > b.points;
		int gd_a = a.goals_for - a.goals_against;
		int gd_b = b.goals_for - b.goals_against;
		if (gd_a != gd_b) return gd_a > gd_b;
		if (a.goals_for != b.goals_for) return a.goals_for > b.goals_for;
		return a.club_name < b.club_name;
	});

	emit_signal("standings_updated");
}

int32_t CareerState::get_managed_club_league_rank() const {
	for (size_t i = 0; i < _standings.size(); i++) {
		if (_standings[i].club_id == _managed_club_id) {
			return static_cast<int32_t>(i + 1);
		}
	}
	return 1;
}

Dictionary CareerState::_player_to_dict(const std::shared_ptr<Player> &p) const {
	Dictionary d;
	if (!p) return d;

	int pid = p->GetId();
	int cid = p->GetClubId();
	d["id"] = pid;
	d["club_id"] = cid;
	d["name"] = String(p->GetName().c_str());
	d["position"] = String(position_to_string(p->GetPosition()).c_str());
	d["age"] = p->GetAge();
	d["debut_age"] = p->GetDebutAge();
	d["potential"] = p->GetPotential();
	d["ovr"] = p->GetOVR();

	PlayerAttributes attrs = p->GetAttributes();
	d["pace"] = attrs.Pace;
	d["shooting"] = attrs.Shooting;
	d["passing"] = attrs.Passing;
	d["dribbling"] = attrs.Dribbling;
	d["defending"] = attrs.Defending;
	d["physical"] = attrs.Physical;

	d["condition"] = p->GetCondition();
	d["morale"] = p->GetMorale();
	d["wage"] = p->GetWage();
	d["contract_years"] = p->GetContractYears();
	d["market_value"] = p->GetMarketValue();
	d["appearances"] = p->GetAppearances();
	d["goals"] = p->GetGoals();
	d["assists"] = p->GetAssists();
	d["is_starting_xi"] = p->GetIsStartingXI();
	d["is_retired"] = p->IsRetired();
	d["is_available_transfer"] = p->IsAvailableForTransfer();
	d["status"] = static_cast<int>(p->GetStatus());

	auto cap_it = _captain_by_club.find(cid);
	d["is_captain"] = (cap_it != _captain_by_club.end() && cap_it->second == pid);

	auto club_it = _clubs.find(cid);
	if (club_it != _clubs.end() && club_it->second) {
		d["club_name"] = String(club_it->second->GetName().c_str());
	} else if (cid == -1) {
		d["club_name"] = String("Youth Academy");
	} else {
		d["club_name"] = String("Free Agent");
	}

	return d;
}

bool CareerState::start_new_career(int32_t p_club_id, const String &p_manager_name, int32_t p_start_year) {
	_managed_club_id = (p_club_id >= 1 && p_club_id <= 16) ? p_club_id : 1;
	_manager.manager_name = p_manager_name.strip_edges().is_empty() ? "Manager" : p_manager_name.strip_edges().utf8().get_data();
	_manager.nationality = "Vietnam";
	_manager.preferred_formation = "4-3-3";
	_manager.reputation = 50;
	_manager.board_confidence = 75;
	_manager.seasons_managed = 0;
	_manager.trophies_won = 0;
	_manager.career_wins = 0;
	_manager.career_draws = 0;
	_manager.career_losses = 0;

	_season_start_year = p_start_year;
	_year = p_start_year;
	_month = 7;
	_day = 1;
	_current_matchday = 1;

	_clubs.clear();
	_captain_by_club.clear();
	_players_by_id.clear();
	_fixtures.clear();
	_standings.clear();
	_inbox.clear();
	_board_objectives.clear();
	_next_player_id = 1;
	_next_inbox_id = 1;

	if (!_youth_academy) {
		_youth_academy = std::make_unique<YouthAcademy>();
	} else {
		_youth_academy->Clear();
	}

	_player_db.instantiate();
	const char *csv_candidates[] = {
		"res://data/players_2006_with_price.csv",
		"res://../Players_Database/players_2006_with_price.csv",
		"res://data/players.csv",
		"res://../Players_Database/players.csv",
		"Players_Database/players_2006_with_price.csv",
		"Players_Database/players.csv"
	};

	bool db_loaded = false;
	for (const char *path : csv_candidates) {
		if (_player_db->load_from_csv(path)) {
			db_loaded = true;
			break;
		}
	}

	for (int i = 1; i <= 16; i++) {
		std::string club_name = DEFAULT_CLUB_NAMES[i - 1];
		double budget = 25000000.0 + (17 - i) * 3000000.0;
		auto team = std::make_unique<Team>(club_name, budget);
		team->SetId(i);
		team->Formation = "4-3-3";
		_clubs[i] = std::move(team);

		LeagueStandingRow row;
		row.club_id = i;
		row.club_name = club_name;
		_standings.push_back(row);
	}

// Bucket loaded CSV players by positional group so every club gets a balanced squad (GK, DEF, MID, FWD)
	std::vector<Player> gk_pool, def_pool, mid_pool, fwd_pool;
	if (db_loaded) {
		for (const auto &src : _player_db->get_raw_players()) {
			std::string pos_str = position_to_string(src.GetPosition());
			if (pos_str == "GK") {
				gk_pool.push_back(src);
			} else if (pos_str == "CB" || pos_str == "LB" || pos_str == "RB" || pos_str == "LWB" || pos_str == "RWB") {
				def_pool.push_back(src);
			} else if (pos_str == "CDM" || pos_str == "CM" || pos_str == "CAM" || pos_str == "LM" || pos_str == "RM") {
				mid_pool.push_back(src);
			} else {
				fwd_pool.push_back(src);
			}
		}
	}

	size_t gk_idx = 0, def_idx = 0, mid_idx = 0, fwd_idx = 0;

	for (int i = 1; i <= 16; i++) {
		auto &team = _clubs[i];

		// Deal 2 GK + 6 DEF + 5 MID + 5 FWD = 18 players per club
		for (int p = 0; p < 18; p++) {
			int pid = _next_player_id++;
			std::shared_ptr<Player> player_ptr;

			auto take_from_pool = [&](std::vector<Player> &pool, size_t &idx) -> bool {
				if (idx < pool.size()) {
					const Player &src = pool[idx++];
					player_ptr = std::make_shared<Player>(
						src.GetName(), src.GetPosition(), src.GetAttributes(),
						src.GetAge(), src.GetPotential(), src.GetDebutAge(), src.GetRetirementAge()
					);
					if (src.GetMarketValue() > 0.0) {
						player_ptr->SetMarketValue(src.GetMarketValue());
					}
					return true;
				}
				return false;
			};

			bool picked = false;
			Position fallback_pos = Position::CM;
			if (p < 2) {
				fallback_pos = Position::GK;
				picked = take_from_pool(gk_pool, gk_idx);
			} else if (p < 8) {
				fallback_pos = (p == 2) ? Position::LB : ((p == 3) ? Position::RB : Position::CB);
				picked = take_from_pool(def_pool, def_idx);
			} else if (p < 13) {
				fallback_pos = (p == 8) ? Position::CDM : Position::CM;
				picked = take_from_pool(mid_pool, mid_idx);
			} else {
				fallback_pos = (p == 13) ? Position::LW : ((p == 14) ? Position::RW : Position::ST);
				picked = take_from_pool(fwd_pool, fwd_idx);
			}

			if (!picked) {
				PlayerAttributes attrs{ 68, 65, 70, 68, 65, 70 };
				player_ptr = std::make_shared<Player>("Player " + std::to_string(pid), fallback_pos, attrs, 24, 78, 16, 36);
			}

			player_ptr->SetId(pid);
			player_ptr->SetClubId(i);
			player_ptr->SetCondition(100);
			player_ptr->SetMorale(85);
			player_ptr->SetWage(std::max<int64_t>(1500, static_cast<int64_t>(player_ptr->GetOVR()) * 850));
			player_ptr->SetContractYears(3);

			_players_by_id[pid] = player_ptr;
			team->AddPlayer(player_ptr);
		}

		// Sort Starting XI into true 4-3-3 positions (GK at slot 0, defenders at 1..4, midfielders at 5..7, forwards at 8..10)
		auto_pick_best_xi_433(*team);

		const auto &xi = team->GetStartingXI();
		int default_captain = -1;
		for (const auto &starter : xi) {
			if (starter && starter->GetPosition() != Position::GK) {
				default_captain = starter->GetId();
				break;
			}
		}
		if (default_captain == -1 && !xi.empty() && xi[0]) {
			default_captain = xi[0]->GetId();
		}
		_captain_by_club[i] = default_captain;
	}

	_finances.total_balance = 50000000;
	_finances.transfer_budget = 35000000;
	_finances.wage_budget = 650000;
	_finances.weekly_wage_bill = 0;

	if (_clubs.find(_managed_club_id) != _clubs.end()) {
		for (const auto &p : _clubs[_managed_club_id]->GetRoster()) {
			if (p) _finances.weekly_wage_bill += p->GetWage();
		}
	}

	struct YouthSeed {
		const char *name;
		Position pos;
		int ovr_seed;
		int pot;
	};
	YouthSeed youth_seeds[6] = {
		{ "Nguyen Van Son", Position::GK, 62, 86 },
		{ "Doan Van Tuan", Position::CB, 64, 88 },
		{ "Tran Hoang Long", Position::LB, 63, 85 },
		{ "Pham Duc Huy", Position::CM, 66, 90 },
		{ "Le Minh Quang", Position::CAM, 67, 92 },
		{ "Nguyen Tien Phat", Position::ST, 65, 89 }
	};

	for (const auto &ys : youth_seeds) {
		int y_id = _next_player_id++;
		PlayerAttributes attrs{ ys.ovr_seed, ys.ovr_seed, ys.ovr_seed, ys.ovr_seed, ys.ovr_seed, ys.ovr_seed };
		auto prospect = std::make_shared<Player>(ys.name, ys.pos, attrs, 15, ys.pot, 16, 36);
		prospect->SetId(y_id);
		prospect->SetClubId(-1);
		prospect->SetCondition(100);
		prospect->SetMorale(90);
		prospect->SetWage(500);
		prospect->SetContractYears(2);

		_players_by_id[y_id] = prospect;
		_youth_academy->AddYouthPlayer(prospect);
	}

	_youth_academy->HireCoach(Coach{ "Tran Van Phuc", 75, 4500.0, "Attacking" });
	_youth_academy->HireCoach(Coach{ "Nguyen Sy Hien", 72, 3800.0, "Defending" });

	int teams_count = 16;
	std::vector<int> team_ids;
	for (int i = 1; i <= teams_count; i++) team_ids.push_back(i);

	int fix_id = 1;
	for (int round = 0; round < teams_count - 1; round++) {
		int matchday = round + 1;
		int match_month = 8 + (round / 4);
		int match_day = 1 + ((round % 4) * 7);
		if (match_month > 12) {
			match_month -= 12;
		}

		std::ostringstream ds;
		ds << _year << "-"
		   << std::setw(2) << std::setfill('0') << match_month << "-"
		   << std::setw(2) << std::setfill('0') << match_day;

		for (int m = 0; m < teams_count / 2; m++) {
			int home = team_ids[m];
			int away = team_ids[teams_count - 1 - m];

			if (round % 2 == 1) {
				std::swap(home, away);
			}

			FixtureRecord fix;
			fix.fixture_id = fix_id++;
			fix.matchday = matchday;
			fix.date_iso = ds.str();
			fix.competition = "Premier Division";
			fix.home_club_id = home;
			fix.away_club_id = away;
			fix.home_goals = -1;
			fix.away_goals = -1;
			fix.played = false;
			_fixtures.push_back(fix);
		}

		int last = team_ids.back();
		team_ids.pop_back();
		team_ids.insert(team_ids.begin() + 1, last);
	}

	_seed_default_objectives();
	_recalculate_standings();

	push_inbox_message("BOARD", "Welcome to the Club!",
		"The Board of Directors welcomes manager " + String(_manager.manager_name.c_str()) +
		". We expect full dedication towards our season targets.", "dashboard", _managed_club_id);

	_career_active = true;

	emit_signal("date_changed", get_current_date_iso());
	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
	emit_signal("inbox_updated", get_unread_inbox_count());
	emit_signal("standings_updated");

	return true;
}

void CareerState::_advance_single_day() {
	_day++;
	int dim = days_in_month(_year, _month);
	if (_day > dim) {
		_day = 1;
		_month++;
		if (_month > 12) {
			_month = 1;
			_year++;
		}
		// Annual season rollover on July 1
		if (_month == 7 && _day == 1) {
			_process_annual_rollover();
		}
	}
}

void CareerState::_simulate_ai_fixtures_for_current_date() {
	std::string current_date = _format_date_iso();

	for (auto &fix : _fixtures) {
		if (fix.played || fix.date_iso != current_date) continue;
		if (fix.home_club_id == _managed_club_id || fix.away_club_id == _managed_club_id) {
			continue;
		}

		auto home_it = _clubs.find(fix.home_club_id);
		auto away_it = _clubs.find(fix.away_club_id);
		int home_ovr = (home_it != _clubs.end() && home_it->second) ? home_it->second->GetTeamRating() : 75;
		int away_ovr = (away_it != _clubs.end() && away_it->second) ? away_it->second->GetTeamRating() : 75;

		int diff = (home_ovr + 3) - away_ovr;
		int h_goals = std::clamp(1 + (diff / 8) + (fix.fixture_id % 3), 0, 5);
		int a_goals = std::clamp(1 - (diff / 10) + ((fix.fixture_id * 2) % 3), 0, 4);

		fix.home_goals = h_goals;
		fix.away_goals = a_goals;
		fix.played = true;
	}

	_recalculate_standings();
}

void CareerState::_process_weekly_progression() {
	int64_t coach_salaries = _youth_academy ? static_cast<int64_t>(_youth_academy->GetTotalWeeklySalaries()) : 0;
	_finances.total_balance -= (_finances.weekly_wage_bill + coach_salaries);
	if (_finances.total_balance < 0) {
		_finances.transfer_budget = std::max<int64_t>(0, _finances.transfer_budget + _finances.total_balance);
	}

	for (auto &pair : _players_by_id) {
		if (!pair.second) continue;
		int cond = pair.second->GetCondition();
		if (cond < 100) {
			pair.second->SetCondition(std::min(100, cond + 12));
		}
	}

	// Weekly youth training XP (without aging)
	if (_youth_academy) {
		_youth_academy->ProcessWeeklyTraining();
		emit_signal("youth_updated");
	}

	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
}

void CareerState::_process_annual_rollover() {
	_manager.seasons_managed++;
	for (auto &pair : _clubs) {
		if (pair.second) {
			pair.second->ProcessEndOfWeekOrSeason();
		}
	}
	if (_youth_academy) {
		_youth_academy->ProcessSeasonDevelopment();
		emit_signal("youth_updated");
	}
	push_inbox_message("YOUTH", "Annual Academy Intake & Aging Report",
		"Season rollover complete: senior players and youth prospects have aged +1 year and completed annual potential evaluations.",
		"youth", -1);
}

void CareerState::advance_day(int32_t p_days) {
	if (!_career_active) return;
	int days_to_advance = std::max(1, p_days);

	for (int d = 0; d < days_to_advance; d++) {
		_advance_single_day();
		_simulate_ai_fixtures_for_current_date();

		if (_day % 7 == 0) {
			_process_weekly_progression();
		}
	}

	emit_signal("date_changed", get_current_date_iso());
}

Dictionary CareerState::get_header_summary() const {
	Dictionary d;
	d["managed_club_id"] = _managed_club_id;

	auto it = _clubs.find(_managed_club_id);
	d["managed_club_name"] = (it != _clubs.end() && it->second) ? String(it->second->GetName().c_str()) : String("My Club");
	d["manager_name"] = String(_manager.manager_name.c_str());
	d["season_label"] = get_season_label();
	d["current_date"] = get_current_date_iso();
	d["league_rank"] = get_managed_club_league_rank();
	d["total_balance"] = _finances.total_balance;
	d["transfer_budget"] = _finances.transfer_budget;
	d["wage_budget"] = _finances.wage_budget;
	d["weekly_wage_bill"] = _finances.weekly_wage_bill;
	d["unread_inbox_count"] = get_unread_inbox_count();
	return d;
}

Dictionary CareerState::get_manager_profile() const {
	Dictionary d;
	d["manager_name"] = String(_manager.manager_name.c_str());
	d["nationality"] = String(_manager.nationality.c_str());
	d["preferred_formation"] = String(_manager.preferred_formation.c_str());
	d["reputation"] = _manager.reputation;
	d["board_confidence"] = _manager.board_confidence;
	d["seasons_managed"] = _manager.seasons_managed;
	d["trophies_won"] = _manager.trophies_won;
	d["career_wins"] = _manager.career_wins;
	d["career_draws"] = _manager.career_draws;
	d["career_losses"] = _manager.career_losses;
	return d;
}

Dictionary CareerState::get_finances() const {
	Dictionary d;
	d["total_balance"] = _finances.total_balance;
	d["transfer_budget"] = _finances.transfer_budget;
	d["wage_budget"] = _finances.wage_budget;
	d["weekly_wage_bill"] = _finances.weekly_wage_bill;
	return d;
}

TypedArray<Dictionary> CareerState::get_board_objectives() const {
	TypedArray<Dictionary> list;
	for (const auto &obj : _board_objectives) {
		Dictionary d;
		d["id"] = obj.id;
		d["category"] = String(obj.category.c_str());
		d["title"] = String(obj.title.c_str());
		d["description"] = String(obj.description.c_str());
		d["target_value"] = obj.target_value;
		d["current_value"] = obj.current_value;
		d["reward_amount"] = obj.reward_amount;
		d["is_mandatory"] = obj.is_mandatory;
		d["is_completed"] = obj.is_completed;
		list.append(d);
	}
	return list;
}

TypedArray<Dictionary> CareerState::get_league_standings() const {
	TypedArray<Dictionary> list;
	for (size_t i = 0; i < _standings.size(); i++) {
		const auto &row = _standings[i];
		Dictionary d;
		d["rank"] = static_cast<int32_t>(i + 1);
		d["club_id"] = row.club_id;
		d["club_name"] = String(row.club_name.c_str());
		d["played"] = row.played;
		d["won"] = row.won;
		d["drawn"] = row.drawn;
		d["lost"] = row.lost;
		d["goals_for"] = row.goals_for;
		d["goals_against"] = row.goals_against;
		d["goal_diff"] = row.goals_for - row.goals_against;
		d["points"] = row.points;
		list.append(d);
	}
	return list;
}

Dictionary CareerState::get_next_fixture() const {
	for (const auto &fix : _fixtures) {
		if (fix.played) continue;
		if (fix.home_club_id == _managed_club_id || fix.away_club_id == _managed_club_id) {
			Dictionary d;
			d["fixture_id"] = fix.fixture_id;
			d["matchday"] = fix.matchday;
			d["date_iso"] = String(fix.date_iso.c_str());
			d["competition"] = String(fix.competition.c_str());
			d["home_club_id"] = fix.home_club_id;
			d["away_club_id"] = fix.away_club_id;

			auto h_it = _clubs.find(fix.home_club_id);
			auto a_it = _clubs.find(fix.away_club_id);
			d["home_club_name"] = (h_it != _clubs.end() && h_it->second) ? String(h_it->second->GetName().c_str()) : String("Home");
			d["away_club_name"] = (a_it != _clubs.end() && a_it->second) ? String(a_it->second->GetName().c_str()) : String("Away");
			d["is_home"] = (fix.home_club_id == _managed_club_id);
			return d;
		}
	}
	return Dictionary();
}

Dictionary CareerState::get_club_info(int32_t p_club_id) const {
	Dictionary d;
	auto it = _clubs.find(p_club_id);
	if (it != _clubs.end() && it->second) {
		d["id"] = it->second->GetId();
		d["name"] = String(it->second->GetName().c_str());
		d["budget"] = it->second->GetBudget();
		d["formation"] = String(it->second->Formation.c_str());
		d["team_rating"] = it->second->GetTeamRating();
		d["squad_size"] = static_cast<int32_t>(it->second->GetRoster().size());
		d["captain_id"] = get_club_captain(p_club_id);
	}
	return d;
}

TypedArray<Dictionary> CareerState::get_club_squad(int32_t p_club_id) const {
	TypedArray<Dictionary> list;
	auto it = _clubs.find(p_club_id);
	if (it == _clubs.end() || !it->second) {
		return list;
	}

	for (const auto &player : it->second->GetRoster()) {
		if (player) {
			list.append(_player_to_dict(player));
		}
	}
	return list;
}

Dictionary CareerState::get_player_profile(int32_t p_player_id) const {
	auto it = _players_by_id.find(p_player_id);
	if (it != _players_by_id.end() && it->second) {
		return _player_to_dict(it->second);
	}
	return Dictionary();
}

TypedArray<Dictionary> CareerState::get_youth_prospects() const {
	TypedArray<Dictionary> list;
	if (!_youth_academy) return list;

	for (const auto &player : _youth_academy->GetYouthRoster()) {
		if (player) {
			list.append(_player_to_dict(player));
		}
	}
	return list;
}

bool CareerState::promote_youth_player(int32_t p_prospect_id) {
	if (!_youth_academy) return false;
	auto it = _players_by_id.find(p_prospect_id);
	if (it == _players_by_id.end() || !it->second) return false;

	std::shared_ptr<Player> p = it->second;
	if (p->GetClubId() != -1) {
		return false;
	}

	if (!_youth_academy->RemoveYouthPlayer(p_prospect_id)) {
		return false;
	}

	auto club_it = _clubs.find(_managed_club_id);
	if (club_it == _clubs.end() || !club_it->second) {
		return false;
	}

	p->SetClubId(_managed_club_id);
	p->SetWage(2500);
	p->SetContractYears(3);
	p->SetIsStartingXI(club_it->second->GetStartingXI().size() < 11);
	club_it->second->AddPlayer(p);

	_finances.weekly_wage_bill += p->GetWage();

	for (auto &obj : _board_objectives) {
		if (obj.category == "YOUTH" && !obj.is_completed) {
			obj.current_value++;
			if (obj.current_value >= obj.target_value) {
				obj.is_completed = true;
				_finances.total_balance += obj.reward_amount;
				push_inbox_message("BOARD", "Board Objective Completed!",
					"You have completed: " + String(obj.title.c_str()) + ". $" + String::num_int64(obj.reward_amount) + " added to balance.",
					"club", obj.id);
			}
		}
	}

	push_inbox_message("YOUTH", "Prospect Promoted to Senior Squad",
		String(p->GetName().c_str()) + " has successfully graduated from the Youth Academy.", "squad", p_prospect_id);

	emit_signal("youth_updated");
	emit_signal("squad_updated", _managed_club_id);
	emit_signal("inbox_updated", get_unread_inbox_count());
	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
	return true;
}

PackedInt32Array CareerState::get_club_player_ids(int32_t p_club_id) const {
	PackedInt32Array ids;
	auto it = _clubs.find(p_club_id);
	if (it != _clubs.end() && it->second) {
		for (const auto &p : it->second->GetRoster()) {
			if (p) ids.append(p->GetId());
		}
	}
	return ids;
}

int32_t CareerState::get_player_count() const {
	return static_cast<int32_t>(_players_by_id.size());
}

// -----------------------------------------------------------------------------
// Phase 4: Squad Management, Permanent Tactics & Youth Academy
// -----------------------------------------------------------------------------
bool CareerState::set_starting_xi(int32_t p_club_id, const PackedInt32Array &p_xi_ids) {
	auto club_it = _clubs.find(p_club_id);
	if (club_it == _clubs.end() || !club_it->second) return false;
	if (p_xi_ids.size() != 11) return false;

	Team *team = club_it->second.get();
	auto &roster = team->GetRoster();

	std::unordered_set<int32_t> seen;
	std::vector<std::shared_ptr<Player>> new_xi;
	new_xi.reserve(11);

	for (int i = 0; i < 11; i++) {
		int32_t pid = p_xi_ids[i];
		if (seen.count(pid)) return false; // Prevent duplicates
		seen.insert(pid);

		auto pit = _players_by_id.find(pid);
		if (pit == _players_by_id.end() || !pit->second || pit->second->GetClubId() != p_club_id) {
			return false;
		}
		new_xi.push_back(pit->second);
	}

	// Rebuild roster with Starting XI first (in exact slot order), followed by bench/reserves
	std::vector<std::shared_ptr<Player>> reordered;
	reordered.reserve(roster.size());
	for (auto &starter : new_xi) {
		starter->SetIsStartingXI(true);
		reordered.push_back(starter);
	}
	for (auto &p : roster) {
		if (p && !seen.count(p->GetId())) {
			p->SetIsStartingXI(false);
			reordered.push_back(p);
		}
	}

	roster = std::move(reordered);
	team->SetStartingXI(new_xi);

	emit_signal("squad_updated", p_club_id);
	return true;
}

bool CareerState::swap_squad_players(int32_t p_club_id, int32_t p_player_a_id, int32_t p_player_b_id) {
	if (p_player_a_id == p_player_b_id) return false;
	auto club_it = _clubs.find(p_club_id);
	if (club_it == _clubs.end() || !club_it->second) return false;

	Team *team = club_it->second.get();
	auto &roster = team->GetRoster();

	int idx_a = -1;
	int idx_b = -1;
	for (int i = 0; i < static_cast<int>(roster.size()); i++) {
		if (!roster[i]) continue;
		if (roster[i]->GetId() == p_player_a_id) idx_a = i;
		if (roster[i]->GetId() == p_player_b_id) idx_b = i;
	}

	if (idx_a < 0 || idx_b < 0) return false;

	std::swap(roster[idx_a], roster[idx_b]);

	std::vector<std::shared_ptr<Player>> new_xi;
	for (size_t i = 0; i < roster.size(); i++) {
		if (!roster[i]) continue;
		bool is_xi = (i < 11);
		roster[i]->SetIsStartingXI(is_xi);
		if (is_xi) {
			new_xi.push_back(roster[i]);
		}
	}
	team->SetStartingXI(new_xi);

	emit_signal("squad_updated", p_club_id);
	return true;
}

bool CareerState::set_club_captain(int32_t p_club_id, int32_t p_player_id) {
	auto club_it = _clubs.find(p_club_id);
	if (club_it == _clubs.end() || !club_it->second) return false;
	if (!club_it->second->HasPlayer(p_player_id)) return false;

	_captain_by_club[p_club_id] = p_player_id;
	emit_signal("squad_updated", p_club_id);
	return true;
}

int32_t CareerState::get_club_captain(int32_t p_club_id) const {
	auto it = _captain_by_club.find(p_club_id);
	if (it != _captain_by_club.end()) {
		return it->second;
	}
	return -1;
}

Dictionary CareerState::get_club_tactics(int32_t p_club_id) const {
	Dictionary d;
	auto club_it = _clubs.find(p_club_id);
	if (club_it == _clubs.end() || !club_it->second) return d;

	const Team *team = club_it->second.get();
	d["club_id"] = p_club_id;
	d["formation"] = String(team->Formation.c_str());
	d["mentality"] = team->Mentality;
	d["pressing"] = team->Pressing;
	d["defensive_line"] = team->DefensiveLine;
	d["passing_style"] = team->PassingStyle;
	d["width"] = team->Width;
	d["tempo"] = team->Tempo;
	d["defensive_approach"] = team->DefensiveApproach;
	d["captain_id"] = get_club_captain(p_club_id);
	return d;
}

bool CareerState::set_club_tactics(int32_t p_club_id, const Dictionary &p_tactics) {
	auto club_it = _clubs.find(p_club_id);
	if (club_it == _clubs.end() || !club_it->second) return false;

	Team *team = club_it->second.get();
	if (p_tactics.has("formation")) {
		String f = String(p_tactics["formation"]).strip_edges();
		if (!f.is_empty()) {
			team->Formation = f.utf8().get_data();
			if (p_club_id == _managed_club_id) {
				_manager.preferred_formation = team->Formation;
			}
		}
	}
	if (p_tactics.has("mentality")) team->Mentality = std::clamp<int>(p_tactics["mentality"], 0, 4);
	if (p_tactics.has("pressing")) team->Pressing = std::clamp<int>(p_tactics["pressing"], 0, 2);
	if (p_tactics.has("defensive_line")) team->DefensiveLine = std::clamp<int>(p_tactics["defensive_line"], 0, 2);
	if (p_tactics.has("passing_style")) team->PassingStyle = std::clamp<int>(p_tactics["passing_style"], 0, 2);
	if (p_tactics.has("width")) team->Width = std::clamp<int>(p_tactics["width"], 0, 2);
	if (p_tactics.has("tempo")) team->Tempo = std::clamp<int>(p_tactics["tempo"], 0, 2);
	if (p_tactics.has("defensive_approach")) team->DefensiveApproach = std::clamp<int>(p_tactics["defensive_approach"], 0, 2);

	emit_signal("tactics_updated", p_club_id);
	return true;
}

Dictionary CareerState::get_youth_academy_info() const {
	Dictionary d;
	if (!_youth_academy) return d;

	int fac_lvl = _youth_academy->FacilityLevel;
	int acad_lvl = _youth_academy->AcademyLevel;

	d["facility_level"] = fac_lvl;
	d["academy_level"] = acad_lvl;
	d["max_level"] = 5;
	d["average_coach_skill"] = _youth_academy->GetAverageCoachSkill();
	d["total_weekly_salaries"] = static_cast<int64_t>(_youth_academy->GetTotalWeeklySalaries());
	d["prospect_count"] = static_cast<int32_t>(_youth_academy->GetYouthRoster().size());
	d["max_prospects"] = 12;
	d["facility_upgrade_cost"] = (fac_lvl < 5) ? static_cast<int64_t>(fac_lvl) * 3500000 : 0;
	d["academy_upgrade_cost"] = (acad_lvl < 5) ? static_cast<int64_t>(acad_lvl) * 3000000 : 0;

	Array coaches_arr;
	const auto &coaches = _youth_academy->GetCoaches();
	for (size_t i = 0; i < coaches.size(); i++) {
		Dictionary cd;
		cd["index"] = static_cast<int32_t>(i);
		cd["name"] = String(coaches[i].name.c_str());
		cd["coaching_skill"] = coaches[i].coachingSkill;
		cd["weekly_salary"] = static_cast<int64_t>(coaches[i].weeklySalary);
		cd["specialty"] = String(coaches[i].specialty.c_str());
		coaches_arr.append(cd);
	}
	d["coaches"] = coaches_arr;
	return d;
}

bool CareerState::upgrade_youth_facility() {
	if (!_youth_academy || _youth_academy->FacilityLevel >= 5) return false;
	int64_t cost = static_cast<int64_t>(_youth_academy->FacilityLevel) * 3500000;
	if (_finances.total_balance < cost) return false;

	_finances.total_balance -= cost;
	_youth_academy->FacilityLevel++;

	push_inbox_message("YOUTH", "Youth Training Facilities Upgraded",
		"Youth training facilities have reached Level " + String::num_int64(_youth_academy->FacilityLevel) +
		". Weekly prospect attribute growth is now accelerated.", "youth", -1);

	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
	emit_signal("youth_updated");
	return true;
}

bool CareerState::upgrade_youth_academy() {
	if (!_youth_academy || _youth_academy->AcademyLevel >= 5) return false;
	int64_t cost = static_cast<int64_t>(_youth_academy->AcademyLevel) * 3000000;
	if (_finances.total_balance < cost) return false;

	_finances.total_balance -= cost;
	_youth_academy->AcademyLevel++;

	push_inbox_message("YOUTH", "Youth Scouting Network Expanded",
		"Youth scouting reach has reached Level " + String::num_int64(_youth_academy->AcademyLevel) +
		", unlocking higher-potential academy recruits.", "youth", -1);

	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
	emit_signal("youth_updated");
	return true;
}

bool CareerState::hire_youth_coach(const String &p_name, int32_t p_skill, double p_salary, const String &p_specialty) {
	if (!_youth_academy) return false;
	if (_youth_academy->GetCoaches().size() >= 4) return false;

	Coach c;
	c.name = p_name.strip_edges().is_empty() ? "Youth Coach" : p_name.strip_edges().utf8().get_data();
	c.coachingSkill = std::clamp<int>(p_skill, 40, 99);
	c.weeklySalary = std::max(1000.0, p_salary);
	c.specialty = p_specialty.strip_edges().is_empty() ? "General" : p_specialty.strip_edges().utf8().get_data();

	_youth_academy->HireCoach(c);
	emit_signal("youth_updated");
	return true;
}

bool CareerState::fire_youth_coach(int32_t p_coach_index) {
	if (!_youth_academy) return false;
	if (!_youth_academy->RemoveCoach(p_coach_index)) return false;
	emit_signal("youth_updated");
	return true;
}

Dictionary CareerState::scout_youth_prospect() {
	Dictionary empty;
	if (!_youth_academy || _youth_academy->GetYouthRoster().size() >= 12) return empty;

	static const char *FIRST_NAMES[] = { "Bui", "Hoang", "Vo", "Dang", "Ngo", "Duong", "Phan", "Vu" };
	static const char *LAST_NAMES[] = { "Gia Bao", "Minh Tri", "Anh Khoa", "Quoc Huy", "Thanh Tung", "Hoai Nam", "Tuan Kiet", "Nhat Minh" };
	static const Position POSITIONS[] = { Position::GK, Position::CB, Position::LB, Position::CM, Position::CAM, Position::ST };

	int seed = _next_player_id + (_day * 17) + (_month * 31);
	std::string full_name = std::string(FIRST_NAMES[seed % 8]) + " " + LAST_NAMES[(seed / 3) % 8];
	Position pos = POSITIONS[seed % 6];

	int base_ovr = std::clamp(58 + (_youth_academy->FacilityLevel * 2) + (seed % 5), 55, 72);
	int pot = std::clamp(78 + (_youth_academy->AcademyLevel * 3) + (seed % 6), base_ovr + 10, 95);

	int y_id = _next_player_id++;
	PlayerAttributes attrs{ base_ovr, base_ovr, base_ovr, base_ovr, base_ovr, base_ovr };
	auto prospect = std::make_shared<Player>(full_name, pos, attrs, 15, pot, 16, 36);
	prospect->SetId(y_id);
	prospect->SetClubId(-1);
	prospect->SetCondition(100);
	prospect->SetMorale(90);
	prospect->SetWage(500);
	prospect->SetContractYears(2);

	_players_by_id[y_id] = prospect;
	_youth_academy->AddYouthPlayer(prospect);

	emit_signal("youth_updated");
	return _player_to_dict(prospect);
}

void CareerState::set_player_condition(int32_t p_player_id, int32_t p_condition) {
	auto it = _players_by_id.find(p_player_id);
	if (it != _players_by_id.end() && it->second) {
		it->second->SetCondition(p_condition);
	}
}

int32_t CareerState::get_player_condition(int32_t p_player_id) const {
	auto it = _players_by_id.find(p_player_id);
	if (it != _players_by_id.end() && it->second) {
		return it->second->GetCondition();
	}
	return 0;
}

void CareerState::set_total_balance(int64_t p_balance) {
	_finances.total_balance = p_balance;
	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
}

int64_t CareerState::get_total_balance() const {
	return _finances.total_balance;
}

void CareerState::set_transfer_budget(int64_t p_budget) {
	_finances.transfer_budget = p_budget;
	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
}

int64_t CareerState::get_transfer_budget() const {
	return _finances.transfer_budget;
}

void CareerState::set_wage_budget(int64_t p_budget) {
	_finances.wage_budget = p_budget;
	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
}

int64_t CareerState::get_wage_budget() const {
	return _finances.wage_budget;
}

bool CareerState::prepare_next_match_payload(MatchEngine *p_engine) const {
	if (!p_engine) return false;

	Dictionary fix_dict = get_next_fixture();
	if (fix_dict.is_empty()) return false;

	int home_id = fix_dict["home_club_id"];
	int away_id = fix_dict["away_club_id"];

	auto h_it = _clubs.find(home_id);
	auto a_it = _clubs.find(away_id);
	if (h_it == _clubs.end() || a_it == _clubs.end() || !h_it->second || !a_it->second) {
		return false;
	}

	Team *home_team = h_it->second.get();
	Team *away_team = a_it->second.get();

	p_engine->set_home_formation(String(home_team->Formation.c_str()));
	p_engine->set_away_formation(String(away_team->Formation.c_str()));

	Array h_lineup;
	for (const auto &p : home_team->GetStartingXI()) {
		if (p) h_lineup.append(_player_to_dict(p));
	}
	p_engine->set_home_lineup(h_lineup);

	Array a_lineup;
	for (const auto &p : away_team->GetStartingXI()) {
		if (p) a_lineup.append(_player_to_dict(p));
	}
	p_engine->set_away_lineup(a_lineup);

	p_engine->set_home_team_ovr(home_team->GetTeamRating());
	p_engine->set_away_team_ovr(away_team->GetTeamRating());

	p_engine->set_home_mentality(home_team->Mentality);
	p_engine->set_away_mentality(away_team->Mentality);
	p_engine->set_home_pressing(home_team->Pressing);
	p_engine->set_away_pressing(away_team->Pressing);
	p_engine->set_home_defensive_line(home_team->DefensiveLine);
	p_engine->set_away_defensive_line(away_team->DefensiveLine);
	p_engine->set_home_passing_style(home_team->PassingStyle);
	p_engine->set_away_passing_style(away_team->PassingStyle);
	p_engine->set_home_attacking_width(home_team->Width);
	p_engine->set_away_attacking_width(away_team->Width);
	p_engine->set_home_tempo(home_team->Tempo);
	p_engine->set_away_tempo(away_team->Tempo);
	p_engine->set_home_defensive_approach(home_team->DefensiveApproach);
	p_engine->set_away_defensive_approach(away_team->DefensiveApproach);

	return true;
}

bool CareerState::commit_match_result(MatchEngine *p_engine) {
	if (!p_engine) return false;

	Dictionary fix_dict = get_next_fixture();
	if (fix_dict.is_empty()) return false;

	int fix_id = fix_dict["fixture_id"];
	int home_score = p_engine->get_home_score();
	int away_score = p_engine->get_away_score();

	for (auto &fix : _fixtures) {
		if (fix.fixture_id == fix_id) {
			fix.home_goals = home_score;
			fix.away_goals = away_score;
			fix.played = true;

			bool is_user_home = (fix.home_club_id == _managed_club_id);
			int user_goals = is_user_home ? home_score : away_score;
			int opp_goals = is_user_home ? away_score : home_score;

			if (user_goals > opp_goals) {
				_manager.career_wins++;
				_manager.board_confidence = std::min(100, _manager.board_confidence + 3);
			} else if (user_goals == opp_goals) {
				_manager.career_draws++;
			} else {
				_manager.career_losses++;
				_manager.board_confidence = std::max(0, _manager.board_confidence - 4);
			}

			int64_t gate_receipts = is_user_home ? 1800000 : 450000;
			_finances.total_balance += gate_receipts;

			auto user_club_it = _clubs.find(_managed_club_id);
			if (user_club_it != _clubs.end() && user_club_it->second) {
				for (auto &p : user_club_it->second->GetStartingXI()) {
					if (p) {
						p->RecordAppearance();
						p->SetCondition(std::max(40, p->GetCondition() - 14));
					}
				}
			}

			std::string result_str = is_user_home ?
				(std::to_string(home_score) + " - " + std::to_string(away_score)) :
				(std::to_string(away_score) + " - " + std::to_string(home_score));

			push_inbox_message("MATCH", "Full-Time Match Report",
				"Final Score: " + String(result_str.c_str()) + ". Gate receipts of $" +
				String::num_int64(gate_receipts) + " credited.", "schedule", fix_id);
			break;
		}
	}

	_recalculate_standings();
	emit_signal("standings_updated");
	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
	return true;
}

int32_t CareerState::push_inbox_message(const String &p_category, const String &p_title, const String &p_body,
		const String &p_action_route, int32_t p_target_entity_id) {
	InboxMessage msg;
	msg.id = _next_inbox_id++;
	msg.date_iso = _format_date_iso();
	msg.category = p_category.strip_edges().to_upper().utf8().get_data();
	msg.title = p_title.utf8().get_data();
	msg.body = p_body.utf8().get_data();
	msg.is_read = false;
	msg.action_route = p_action_route.utf8().get_data();
	msg.target_entity_id = p_target_entity_id;

	_inbox.insert(_inbox.begin(), msg);
	emit_signal("inbox_updated", get_unread_inbox_count());
	return msg.id;
}

TypedArray<Dictionary> CareerState::get_inbox_messages(const String &p_category_filter) const {
	TypedArray<Dictionary> list;
	std::string filter = p_category_filter.strip_edges().to_upper().utf8().get_data();

	for (const auto &msg : _inbox) {
		if (!filter.empty() && filter != "ALL" && msg.category != filter) {
			continue;
		}
		Dictionary d;
		d["id"] = msg.id;
		d["date_iso"] = String(msg.date_iso.c_str());
		d["category"] = String(msg.category.c_str());
		d["title"] = String(msg.title.c_str());
		d["body"] = String(msg.body.c_str());
		d["is_read"] = msg.is_read;
		d["action_route"] = String(msg.action_route.c_str());
		d["target_entity_id"] = msg.target_entity_id;
		list.append(d);
	}
	return list;
}

bool CareerState::mark_inbox_read(int32_t p_message_id) {
	for (auto &msg : _inbox) {
		if (msg.id == p_message_id) {
			if (!msg.is_read) {
				msg.is_read = true;
				emit_signal("inbox_updated", get_unread_inbox_count());
			}
			return true;
		}
	}
	return false;
}

void CareerState::mark_all_inbox_read() {
	bool changed = false;
	for (auto &msg : _inbox) {
		if (!msg.is_read) {
			msg.is_read = true;
			changed = true;
		}
	}
	if (changed) {
		emit_signal("inbox_updated", 0);
	}
}

int32_t CareerState::get_unread_inbox_count() const {
	int32_t count = 0;
	for (const auto &msg : _inbox) {
		if (!msg.is_read) count++;
	}
	return count;
}

bool CareerState::save_career(const String &p_slot_name) {
	if (!_career_active) {
		UtilityFunctions::printerr("CareerState: Cannot save inactive career!");
		return false;
	}

	String slot_clean = p_slot_name.strip_edges().replace("/", "_").replace("\\", "_");
	if (slot_clean.is_empty()) slot_clean = "career_autosave";

	Ref<DirAccess> dir = DirAccess::open("user://");
	if (dir.is_null()) {
		UtilityFunctions::printerr("CareerState: Failed to access user:// directory!");
		return false;
	}
	dir->make_dir_recursive("user://saves");

	String base_path = "user://saves/" + slot_clean;
	String tmp_path = base_path + ".tmp.json";
	String save_path = base_path + ".json";
	String meta_path = base_path + ".meta.json";

	Dictionary root;
	root["save_version"] = SAVE_SCHEMA_VERSION;
	root["timestamp_unix"] = static_cast<int64_t>(Time::get_singleton()->get_unix_time_from_system());

	Dictionary meta;
	meta["slot_name"] = slot_clean;
	meta["managed_club_id"] = _managed_club_id;
	auto club_it = _clubs.find(_managed_club_id);
	meta["managed_club_name"] = (club_it != _clubs.end() && club_it->second) ? String(club_it->second->GetName().c_str()) : String("My Club");
	meta["manager_name"] = String(_manager.manager_name.c_str());
	meta["season_label"] = get_season_label();
	meta["date_iso"] = get_current_date_iso();
	meta["total_balance"] = _finances.total_balance;
	meta["league_rank"] = get_managed_club_league_rank();
	meta["timestamp_unix"] = root["timestamp_unix"];
	root["metadata"] = meta;

	Dictionary clock;
	clock["season_start_year"] = _season_start_year;
	clock["year"] = _year;
	clock["month"] = _month;
	clock["day"] = _day;
	clock["current_matchday"] = _current_matchday;
	root["clock"] = clock;

	Dictionary mgr;
	mgr["manager_name"] = String(_manager.manager_name.c_str());
	mgr["nationality"] = String(_manager.nationality.c_str());
	mgr["preferred_formation"] = String(_manager.preferred_formation.c_str());
	mgr["reputation"] = _manager.reputation;
	mgr["board_confidence"] = _manager.board_confidence;
	mgr["seasons_managed"] = _manager.seasons_managed;
	mgr["trophies_won"] = _manager.trophies_won;
	mgr["career_wins"] = _manager.career_wins;
	mgr["career_draws"] = _manager.career_draws;
	mgr["career_losses"] = _manager.career_losses;
	root["manager"] = mgr;

	Dictionary fin;
	fin["total_balance"] = _finances.total_balance;
	fin["transfer_budget"] = _finances.transfer_budget;
	fin["wage_budget"] = _finances.wage_budget;
	fin["weekly_wage_bill"] = _finances.weekly_wage_bill;
	root["finances"] = fin;

	Array objs_arr;
	for (const auto &obj : _board_objectives) {
		Dictionary d;
		d["id"] = obj.id;
		d["category"] = String(obj.category.c_str());
		d["title"] = String(obj.title.c_str());
		d["description"] = String(obj.description.c_str());
		d["target_value"] = obj.target_value;
		d["current_value"] = obj.current_value;
		d["reward_amount"] = obj.reward_amount;
		d["is_mandatory"] = obj.is_mandatory;
		d["is_completed"] = obj.is_completed;
		objs_arr.append(d);
	}
	root["board_objectives"] = objs_arr;

	Array inbox_arr;
	for (const auto &msg : _inbox) {
		Dictionary d;
		d["id"] = msg.id;
		d["date_iso"] = String(msg.date_iso.c_str());
		d["category"] = String(msg.category.c_str());
		d["title"] = String(msg.title.c_str());
		d["body"] = String(msg.body.c_str());
		d["is_read"] = msg.is_read;
		d["action_route"] = String(msg.action_route.c_str());
		d["target_entity_id"] = msg.target_entity_id;
		inbox_arr.append(d);
	}
	root["inbox"] = inbox_arr;

	Array fixtures_arr;
	for (const auto &fix : _fixtures) {
		Dictionary d;
		d["fixture_id"] = fix.fixture_id;
		d["matchday"] = fix.matchday;
		d["date_iso"] = String(fix.date_iso.c_str());
		d["competition"] = String(fix.competition.c_str());
		d["home_club_id"] = fix.home_club_id;
		d["away_club_id"] = fix.away_club_id;
		d["home_goals"] = fix.home_goals;
		d["away_goals"] = fix.away_goals;
		d["played"] = fix.played;
		fixtures_arr.append(d);
	}
	root["fixtures"] = fixtures_arr;

	Array standings_arr;
	for (const auto &row : _standings) {
		Dictionary d;
		d["club_id"] = row.club_id;
		d["club_name"] = String(row.club_name.c_str());
		d["played"] = row.played;
		d["won"] = row.won;
		d["drawn"] = row.drawn;
		d["lost"] = row.lost;
		d["goals_for"] = row.goals_for;
		d["goals_against"] = row.goals_against;
		d["points"] = row.points;
		standings_arr.append(d);
	}
	root["standings"] = standings_arr;

	Array clubs_arr;
	for (const auto &pair : _clubs) {
		const auto &team = pair.second;
		if (!team) continue;
		Dictionary cd;
		cd["club_id"] = team->GetId();
		cd["captain_id"] = get_club_captain(team->GetId());
		cd["name"] = String(team->GetName().c_str());
		cd["budget"] = team->GetBudget();
		cd["training_points"] = team->GetTrainingPoints();
		cd["formation"] = String(team->Formation.c_str());
		cd["mentality"] = team->Mentality;
		cd["pressing"] = team->Pressing;
		cd["defensive_line"] = team->DefensiveLine;
		cd["passing_style"] = team->PassingStyle;
		cd["width"] = team->Width;
		cd["tempo"] = team->Tempo;
		cd["defensive_approach"] = team->DefensiveApproach;

		PackedInt32Array roster_ids;
		for (const auto &p : team->GetRoster()) {
			if (p) roster_ids.append(p->GetId());
		}
		cd["roster_ids"] = roster_ids;

		PackedInt32Array xi_ids;
		for (const auto &p : team->GetStartingXI()) {
			if (p) xi_ids.append(p->GetId());
		}
		cd["starting_xi_ids"] = xi_ids;

		clubs_arr.append(cd);
	}
	root["clubs"] = clubs_arr;

	Dictionary yd;
	yd["facility_level"] = _youth_academy ? _youth_academy->FacilityLevel : 1;
	yd["academy_level"] = _youth_academy ? _youth_academy->AcademyLevel : 1;
	PackedInt32Array y_ids;
	Array coaches_arr;
	if (_youth_academy) {
		for (const auto &p : _youth_academy->GetYouthRoster()) {
			if (p) y_ids.append(p->GetId());
		}
		for (const auto &c : _youth_academy->GetCoaches()) {
			Dictionary cd;
			cd["name"] = String(c.name.c_str());
			cd["coaching_skill"] = c.coachingSkill;
			cd["weekly_salary"] = c.weeklySalary;
			cd["specialty"] = String(c.specialty.c_str());
			coaches_arr.append(cd);
		}
	}
	yd["prospect_ids"] = y_ids;
	yd["coaches"] = coaches_arr;
	root["youth_academy"] = yd;

	Array players_arr;
	for (const auto &pair : _players_by_id) {
		const auto &p = pair.second;
		if (!p) continue;
		Dictionary pd;
		pd["id"] = p->GetId();
		pd["club_id"] = p->GetClubId();
		pd["name"] = String(p->GetName().c_str());
		pd["position"] = String(position_to_string(p->GetPosition()).c_str());
		pd["age"] = p->GetAge();
		pd["debut_age"] = p->GetDebutAge();
		pd["retirement_age"] = p->GetRetirementAge();
		pd["potential"] = p->GetPotential();

		PlayerAttributes attrs = p->GetAttributes();
		pd["pace"] = attrs.Pace;
		pd["shooting"] = attrs.Shooting;
		pd["passing"] = attrs.Passing;
		pd["dribbling"] = attrs.Dribbling;
		pd["defending"] = attrs.Defending;
		pd["physical"] = attrs.Physical;

		pd["condition"] = p->GetCondition();
		pd["morale"] = p->GetMorale();
		pd["wage"] = p->GetWage();
		pd["contract_years"] = p->GetContractYears();
		pd["appearances"] = p->GetAppearances();
		pd["goals"] = p->GetGoals();
		pd["assists"] = p->GetAssists();
		pd["market_value"] = p->GetMarketValue();
		pd["is_starting_xi"] = p->GetIsStartingXI();
		pd["is_retired"] = p->IsRetired();

		players_arr.append(pd);
	}
	root["players"] = players_arr;

	String json_str = JSON::stringify(root, "\t");
	Ref<FileAccess> tmp_file = FileAccess::open(tmp_path, FileAccess::WRITE);
	if (tmp_file.is_null()) {
		UtilityFunctions::printerr("CareerState: Failed to open tmp file for write: ", tmp_path);
		return false;
	}
	tmp_file->store_string(json_str);
	tmp_file->close();

	if (dir->file_exists(save_path)) {
		dir->remove(save_path);
	}
	Error err = dir->rename(tmp_path, save_path);
	if (err != OK) {
		UtilityFunctions::printerr("CareerState: Failed to atomically rename tmp to save file!");
		return false;
	}

	String meta_str = JSON::stringify(meta, "\t");
	Ref<FileAccess> meta_file = FileAccess::open(meta_path, FileAccess::WRITE);
	if (meta_file.is_valid()) {
		meta_file->store_string(meta_str);
		meta_file->close();
	}

	emit_signal("career_saved", slot_clean);
	return true;
}

bool CareerState::load_career(const String &p_slot_name) {
	String slot_clean = p_slot_name.strip_edges().replace("/", "_").replace("\\", "_");
	String save_path = "user://saves/" + slot_clean + ".json";

	if (!FileAccess::file_exists(save_path)) {
		save_path = "res://saves/" + slot_clean + ".json";
		if (!FileAccess::file_exists(save_path)) {
			UtilityFunctions::printerr("CareerState: Save slot file not found: ", save_path);
			return false;
		}
	}

	Ref<FileAccess> file = FileAccess::open(save_path, FileAccess::READ);
	if (file.is_null()) {
		UtilityFunctions::printerr("CareerState: Failed to open save file: ", save_path);
		return false;
	}

	String json_text = file->get_as_text();
	file->close();

	Variant parsed = JSON::parse_string(json_text);
	if (parsed.get_type() != Variant::DICTIONARY) {
		UtilityFunctions::printerr("CareerState: Corrupted save file, root is not Dictionary!");
		return false;
	}

	Dictionary root = parsed;
	uint32_t ver = static_cast<uint32_t>(root.get("save_version", 0));
	if (ver > SAVE_SCHEMA_VERSION || ver == 0) {
		UtilityFunctions::printerr("CareerState: Unsupported save schema version: ", ver);
		return false;
	}

	_clubs.clear();
	_captain_by_club.clear();
	_players_by_id.clear();
	_fixtures.clear();
	_standings.clear();
	_inbox.clear();
	_board_objectives.clear();
	if (!_youth_academy) _youth_academy = std::make_unique<YouthAcademy>();
	_youth_academy->Clear();

	if (root.has("clock")) {
		Dictionary clock = root["clock"];
		_season_start_year = clock.get("season_start_year", 2026);
		_year = clock.get("year", 2026);
		_month = clock.get("month", 7);
		_day = clock.get("day", 1);
		_current_matchday = clock.get("current_matchday", 1);
	}

	if (root.has("manager")) {
		Dictionary mgr = root["manager"];
		_manager.manager_name = String(mgr.get("manager_name", "Manager")).utf8().get_data();
		_manager.nationality = String(mgr.get("nationality", "Vietnam")).utf8().get_data();
		_manager.preferred_formation = String(mgr.get("preferred_formation", "4-3-3")).utf8().get_data();
		_manager.reputation = mgr.get("reputation", 50);
		_manager.board_confidence = mgr.get("board_confidence", 75);
		_manager.seasons_managed = mgr.get("seasons_managed", 0);
		_manager.trophies_won = mgr.get("trophies_won", 0);
		_manager.career_wins = mgr.get("career_wins", 0);
		_manager.career_draws = mgr.get("career_draws", 0);
		_manager.career_losses = mgr.get("career_losses", 0);
	}

	if (root.has("finances")) {
		Dictionary fin = root["finances"];
		_finances.total_balance = fin.get("total_balance", 0);
		_finances.transfer_budget = fin.get("transfer_budget", 0);
		_finances.wage_budget = fin.get("wage_budget", 0);
		_finances.weekly_wage_bill = fin.get("weekly_wage_bill", 0);
	}

	if (root.has("metadata")) {
		Dictionary meta = root["metadata"];
		_managed_club_id = meta.get("managed_club_id", 1);
	}

	int max_pid = 0;
	if (root.has("players")) {
		Array players_arr = root["players"];
		for (int i = 0; i < players_arr.size(); i++) {
			Dictionary pd = players_arr[i];
			int pid = pd.get("id", 0);
			if (pid <= 0) continue;
			max_pid = std::max(max_pid, pid);

			std::string name = String(pd.get("name", "Player")).utf8().get_data();
			Position pos = string_to_position(String(pd.get("position", "CM")).utf8().get_data());

			PlayerAttributes attrs;
			attrs.Pace = pd.get("pace", 60);
			attrs.Shooting = pd.get("shooting", 60);
			attrs.Passing = pd.get("passing", 60);
			attrs.Dribbling = pd.get("dribbling", 60);
			attrs.Defending = pd.get("defending", 60);
			attrs.Physical = pd.get("physical", 60);

			int age = pd.get("age", 24);
			int potential = pd.get("potential", 75);
			int debut_age = pd.get("debut_age", 16);
			int ret_age = pd.get("retirement_age", 36);

			auto player_ptr = std::make_shared<Player>(name, pos, attrs, age, potential, debut_age, ret_age);
			player_ptr->SetId(pid);
			player_ptr->SetClubId(pd.get("club_id", -1));
			player_ptr->SetCondition(pd.get("condition", 100));
			player_ptr->SetMorale(pd.get("morale", 80));
			player_ptr->SetWage(pd.get("wage", 3000));
			player_ptr->SetContractYears(pd.get("contract_years", 3));
			player_ptr->SetAppearances(pd.get("appearances", 0));
			player_ptr->SetGoals(pd.get("goals", 0));
			player_ptr->SetAssists(pd.get("assists", 0));
			player_ptr->SetIsStartingXI(pd.get("is_starting_xi", false));
			player_ptr->SetMarketValue(pd.get("market_value", 0.0));
			player_ptr->SetRetired(pd.get("is_retired", false));

			_players_by_id[pid] = player_ptr;
		}
	}
	_next_player_id = max_pid + 1;

	if (root.has("clubs")) {
		Array clubs_arr = root["clubs"];
		for (int i = 0; i < clubs_arr.size(); i++) {
			Dictionary cd = clubs_arr[i];
			int cid = cd.get("club_id", 0);
			if (cid <= 0) continue;

			std::string cname = String(cd.get("name", "Club")).utf8().get_data();
			double budget = cd.get("budget", 20000000.0);
			auto team = std::make_unique<Team>(cname, budget);
			team->SetId(cid);
			team->Formation = String(cd.get("formation", "4-3-3")).utf8().get_data();
			team->Mentality = cd.get("mentality", 2);
			team->Pressing = cd.get("pressing", 1);
			team->DefensiveLine = cd.get("defensive_line", 1);
			team->PassingStyle = cd.get("passing_style", 1);
			team->Width = cd.get("width", 1);
			team->Tempo = cd.get("tempo", 1);
			team->DefensiveApproach = cd.get("defensive_approach", 1);

			if (cd.has("captain_id")) {
				_captain_by_club[cid] = cd["captain_id"];
			}

			PackedInt32Array roster_ids = cd.get("roster_ids", PackedInt32Array());
			for (int k = 0; k < roster_ids.size(); k++) {
				int pid = roster_ids[k];
				if (_players_by_id.find(pid) != _players_by_id.end()) {
					team->AddPlayer(_players_by_id[pid]);
				}
			}

			PackedInt32Array xi_ids = cd.get("starting_xi_ids", PackedInt32Array());
			std::vector<std::shared_ptr<Player>> xi;
			for (int k = 0; k < xi_ids.size(); k++) {
				int pid = xi_ids[k];
				if (_players_by_id.find(pid) != _players_by_id.end()) {
					xi.push_back(_players_by_id[pid]);
				}
			}
			team->SetStartingXI(xi);

			_clubs[cid] = std::move(team);
		}
	}

	if (root.has("youth_academy")) {
		Dictionary yd = root["youth_academy"];
		_youth_academy->FacilityLevel = yd.get("facility_level", 1);
		_youth_academy->AcademyLevel = yd.get("academy_level", 1);
		PackedInt32Array y_ids = yd.get("prospect_ids", PackedInt32Array());
		for (int k = 0; k < y_ids.size(); k++) {
			int pid = y_ids[k];
			if (_players_by_id.find(pid) != _players_by_id.end()) {
				_youth_academy->AddYouthPlayer(_players_by_id[pid]);
			}
		}
		if (yd.has("coaches")) {
			Array coaches_arr = yd["coaches"];
			for (int k = 0; k < coaches_arr.size(); k++) {
				Dictionary cd = coaches_arr[k];
				Coach c;
				c.name = String(cd.get("name", "Coach")).utf8().get_data();
				c.coachingSkill = cd.get("coaching_skill", 70);
				c.weeklySalary = cd.get("weekly_salary", 3000.0);
				c.specialty = String(cd.get("specialty", "General")).utf8().get_data();
				_youth_academy->HireCoach(c);
			}
		}
	}

	if (root.has("board_objectives")) {
		Array objs_arr = root["board_objectives"];
		for (int i = 0; i < objs_arr.size(); i++) {
			Dictionary od = objs_arr[i];
			BoardObjective obj;
			obj.id = od.get("id", 0);
			obj.category = String(od.get("category", "")).utf8().get_data();
			obj.title = String(od.get("title", "")).utf8().get_data();
			obj.description = String(od.get("description", "")).utf8().get_data();
			obj.target_value = od.get("target_value", 0);
			obj.current_value = od.get("current_value", 0);
			obj.reward_amount = od.get("reward_amount", 0);
			obj.is_mandatory = od.get("is_mandatory", true);
			obj.is_completed = od.get("is_completed", false);
			_board_objectives.push_back(obj);
		}
	}

	int max_inbox_id = 0;
	if (root.has("inbox")) {
		Array inbox_arr = root["inbox"];
		for (int i = 0; i < inbox_arr.size(); i++) {
			Dictionary md = inbox_arr[i];
			InboxMessage msg;
			msg.id = md.get("id", 0);
			max_inbox_id = std::max(max_inbox_id, msg.id);
			msg.date_iso = String(md.get("date_iso", "")).utf8().get_data();
			msg.category = String(md.get("category", "")).utf8().get_data();
			msg.title = String(md.get("title", "")).utf8().get_data();
			msg.body = String(md.get("body", "")).utf8().get_data();
			msg.is_read = md.get("is_read", false);
			msg.action_route = String(md.get("action_route", "")).utf8().get_data();
			msg.target_entity_id = md.get("target_entity_id", -1);
			_inbox.push_back(msg);
		}
	}
	_next_inbox_id = max_inbox_id + 1;

	if (root.has("fixtures")) {
		Array fixtures_arr = root["fixtures"];
		for (int i = 0; i < fixtures_arr.size(); i++) {
			Dictionary fd = fixtures_arr[i];
			FixtureRecord fix;
			fix.fixture_id = fd.get("fixture_id", 0);
			fix.matchday = fd.get("matchday", 1);
			fix.date_iso = String(fd.get("date_iso", "")).utf8().get_data();
			fix.competition = String(fd.get("competition", "")).utf8().get_data();
			fix.home_club_id = fd.get("home_club_id", -1);
			fix.away_club_id = fd.get("away_club_id", -1);
			fix.home_goals = fd.get("home_goals", -1);
			fix.away_goals = fd.get("away_goals", -1);
			fix.played = fd.get("played", false);
			_fixtures.push_back(fix);
		}
	}

	if (root.has("standings")) {
		Array standings_arr = root["standings"];
		for (int i = 0; i < standings_arr.size(); i++) {
			Dictionary sd = standings_arr[i];
			LeagueStandingRow row;
			row.club_id = sd.get("club_id", -1);
			row.club_name = String(sd.get("club_name", "")).utf8().get_data();
			row.played = sd.get("played", 0);
			row.won = sd.get("won", 0);
			row.drawn = sd.get("drawn", 0);
			row.lost = sd.get("lost", 0);
			row.goals_for = sd.get("goals_for", 0);
			row.goals_against = sd.get("goals_against", 0);
			row.points = sd.get("points", 0);
			_standings.push_back(row);
		}
	}

	_career_active = true;

	emit_signal("career_loaded", slot_clean);
	emit_signal("date_changed", get_current_date_iso());
	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
	emit_signal("inbox_updated", get_unread_inbox_count());
	emit_signal("standings_updated");
	emit_signal("squad_updated", _managed_club_id);
	emit_signal("tactics_updated", _managed_club_id);
	emit_signal("youth_updated");

	return true;
}

TypedArray<Dictionary> CareerState::list_save_slots() const {
	TypedArray<Dictionary> slots;
	Ref<DirAccess> dir = DirAccess::open("user://saves");
	if (dir.is_null()) {
		return slots;
	}

	dir->list_dir_begin();
	String file_name = dir->get_next();
	while (!file_name.is_empty()) {
		if (!dir->current_is_dir() && file_name.ends_with(".json") &&
			!file_name.ends_with(".tmp.json") && !file_name.ends_with(".meta.json")) {

			String slot_name = file_name.get_basename();
			String meta_path = "user://saves/" + slot_name + ".meta.json";
			String full_path = "user://saves/" + file_name;

			Dictionary slot_dict;
			bool meta_read = false;

			if (FileAccess::file_exists(meta_path)) {
				Ref<FileAccess> mf = FileAccess::open(meta_path, FileAccess::READ);
				if (mf.is_valid()) {
					Variant p = JSON::parse_string(mf->get_as_text());
					if (p.get_type() == Variant::DICTIONARY) {
						slot_dict = p;
						meta_read = true;
					}
					mf->close();
				}
			}

			if (!meta_read && FileAccess::file_exists(full_path)) {
				Ref<FileAccess> f = FileAccess::open(full_path, FileAccess::READ);
				if (f.is_valid()) {
					Variant p = JSON::parse_string(f->get_as_text());
					if (p.get_type() == Variant::DICTIONARY) {
						Dictionary root = p;
						if (root.has("metadata")) {
							slot_dict = root["metadata"];
						}
					}
					f->close();
				}
			}

			if (!slot_dict.has("slot_name")) {
				slot_dict["slot_name"] = slot_name;
			}
			slots.append(slot_dict);
		}
		file_name = dir->get_next();
	}
	dir->list_dir_end();

	return slots;
}



bool CareerState::delete_save_slot(const String &p_slot_name) {
	String slot_clean = p_slot_name.strip_edges().replace("/", "_").replace("\\", "_");
	Ref<DirAccess> dir = DirAccess::open("user://saves");
	if (dir.is_null()) return false;

	String save_file = slot_clean + ".json";
	String meta_file = slot_clean + ".meta.json";
	String tmp_file = slot_clean + ".tmp.json";

	bool deleted = false;
	if (dir->file_exists(save_file)) {
		dir->remove(save_file);
		deleted = true;
	}
	if (dir->file_exists(meta_file)) {
		dir->remove(meta_file);
	}
	if (dir->file_exists(tmp_file)) {
		dir->remove(tmp_file);
	}
	return deleted;
}