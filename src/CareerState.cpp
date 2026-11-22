#include "CareerState.h"
#include "match_engine.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cctype>
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

static void add_days_to_ymd(int &year, int &month, int &day, int delta_days) {
	while (delta_days > 0) {
		int dim = days_in_month(year, month);
		int rem = dim - day;
		if (delta_days <= rem) {
			day += delta_days;
			return;
		}
		delta_days -= (rem + 1);
		day = 1;
		month++;
		if (month > 12) {
			month = 1;
			year++;
		}
	}
}

static std::string format_ymd_iso(int year, int month, int day) {
	std::ostringstream ss;
	ss << year << "-"
	   << std::setw(2) << std::setfill('0') << month << "-"
	   << std::setw(2) << std::setfill('0') << day;
	return ss.str();
}

static int64_t civil_to_days(int y, int m, int d) {
	y -= m <= 2;
	const int era = (y >= 0 ? y : y - 399) / 400;
	const unsigned yoe = static_cast<unsigned>(y - era * 400);
	const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

static const char *DEFAULT_CLUB_NAMES[16] = {
	"Arsenal", "Chelsea", "Manchester United", "Liverpool",
	"Barcelona", "Real Madrid", "AC Milan", "Juventus",
	"Inter Milan", "Bayern Munich", "Lyon", "AS Roma",
	"Valencia", "Porto", "Ajax", "Benfica"
};

static const char *DEFAULT_STADIUM_NAMES[16] = {
	"Emirates Stadium", "Stamford Bridge", "Old Trafford", "Anfield",
	"Camp Nou", "Santiago Bernabeu", "San Siro", "Allianz Stadium",
	"Giuseppe Meazza", "Allianz Arena", "Groupama Stadium", "Stadio Olimpico",
	"Mestalla", "Estadio do Dragao", "Johan Cruyff Arena", "Estadio da Luz"
};

static bool is_classic_derby_pair(const std::string &a, const std::string &b) {
	auto match_pair = [&](const char *x, const char *y) {
		return (a == x && b == y) || (a == y && b == x);
	};
	return match_pair("Arsenal", "Chelsea") ||
	       match_pair("Manchester United", "Liverpool") ||
	       match_pair("Barcelona", "Real Madrid") ||
	       match_pair("AC Milan", "Inter Milan") ||
	       match_pair("Juventus", "Inter Milan") ||
	       match_pair("Porto", "Benfica");
}

static std::string to_lower_str(const std::string &s) {
	std::string out = s;
	std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return out;
}

static std::string normalize_code(const String &p_code) {
	std::string s = p_code.strip_edges().to_upper().utf8().get_data();
	std::string cleaned;
	for (char c : s) {
		if (!std::isspace(static_cast<unsigned char>(c)) && c != '-') {
			cleaned.push_back(c);
		}
	}
	return cleaned;
}

static std::string compute_secondary_kit_color(const std::string &primary_hex) {
	std::string u = primary_hex;
	std::transform(u.begin(), u.end(), u.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
	if (u == "#FFFFFF" || u == "#FFF" || u == "#FDE100" || u == "#FFE667" || u == "#FFE400" || u == "#FFED00" || u == "#87D8F7" || u == "#F2A9B8") {
		return "#111827";
	}
	return "#F8FAFC";
}

} // namespace

std::string CareerState::_normalize_club_key(const String &p_club_name) {
	const std::string raw = p_club_name.strip_edges().to_lower().utf8().get_data();
	std::string normalized; normalized.reserve(raw.size());
	bool previous_was_space = false;
	for (unsigned char ch : raw) {
		if (std::isspace(ch)) { if (!previous_was_space) { normalized.push_back(' '); previous_was_space = true; } }
		else { normalized.push_back(static_cast<char>(ch)); previous_was_space = false; }
	}
	return normalized;
}
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

// Formation-aware XI picker (supports 4-2-3-1, 4-3-3, 4-4-2, 3-5-2, 5-3-2, 4-1-4-1)
static std::vector<std::string> get_formation_slot_roles(const std::string &formation) {
	if (formation == "4-2-3-1") {
		return { "GK", "LB", "CB", "CB", "RB", "CDM", "CDM", "LW", "CAM", "RW", "ST" };
	}
	if (formation == "4-4-2") {
		return { "GK", "LB", "CB", "CB", "RB", "LW", "CM", "CM", "RW", "ST", "ST" };
	}
	if (formation == "3-5-2") {
		return { "GK", "CB", "CB", "CB", "LB", "CDM", "CAM", "CDM", "RB", "ST", "ST" };
	}
	if (formation == "5-3-2") {
		return { "GK", "LB", "CB", "CB", "CB", "RB", "CM", "CDM", "CM", "ST", "ST" };
	}
	if (formation == "4-1-4-1") {
		return { "GK", "LB", "CB", "CB", "RB", "CDM", "LW", "CM", "CM", "RW", "ST" };
	}
	return { "GK", "LB", "CB", "CB", "RB", "CDM", "CM", "CM", "LW", "RW", "ST" };
}

static int evaluate_slot_fit(const std::string &player_pos, const std::string &slot_pos) {
	if (slot_pos == "GK") return (player_pos == "GK") ? 100 : 0;
	if (player_pos == "GK") return 0;
	if (player_pos == slot_pos) return 100;

	if (slot_pos == "LB" && (player_pos == "LWB" || player_pos == "LM" || player_pos == "RB")) return 95;
	if (slot_pos == "RB" && (player_pos == "RWB" || player_pos == "RM" || player_pos == "LB")) return 95;
	if (slot_pos == "CB" && (player_pos == "CDM" || player_pos == "LB" || player_pos == "RB")) return 95;
	if (slot_pos == "CDM" && (player_pos == "CM" || player_pos == "CB" || player_pos == "CAM")) return 95;
	if (slot_pos == "CM" && (player_pos == "CDM" || player_pos == "CAM" || player_pos == "LM" || player_pos == "RM")) return 95;
	if (slot_pos == "CAM" && (player_pos == "CM" || player_pos == "LW" || player_pos == "RW" || player_pos == "ST" || player_pos == "CF")) return 95;
	if (slot_pos == "LW" && (player_pos == "LM" || player_pos == "CAM" || player_pos == "CF" || player_pos == "ST" || player_pos == "RW")) return 95;
	if (slot_pos == "RW" && (player_pos == "RM" || player_pos == "CAM" || player_pos == "CF" || player_pos == "ST" || player_pos == "LW")) return 95;
	if (slot_pos == "ST" && (player_pos == "CF" || player_pos == "CAM" || player_pos == "LW" || player_pos == "RW")) return 95;

	return 80;
}

static void auto_pick_best_xi_for_formation(Team &team, const std::string &formation) {
	const std::vector<std::string> slot_roles = get_formation_slot_roles(formation);
	auto &roster = team.GetRoster();
	if (roster.empty()) return;

	std::vector<std::shared_ptr<Player>> xi(11, nullptr);
	std::unordered_set<int32_t> used_ids;

	auto pick_for_slot = [&](int slot_idx, int min_fit) {
		if (xi[slot_idx] != nullptr) return;
		const std::string target_pos = slot_roles[slot_idx];
		std::shared_ptr<Player> best_player = nullptr;
		int best_score = -1;

		for (const auto &p : roster) {
			if (!p || used_ids.count(p->GetId())) continue;
			std::string p_pos = position_to_string(p->GetPosition());
			int fit = evaluate_slot_fit(p_pos, target_pos);
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

	for (int s = 0; s < 11; s++) pick_for_slot(s, 100);
	for (int s = 0; s < 11; s++) pick_for_slot(s, 95);
	for (int s = 0; s < 11; s++) pick_for_slot(s, 80);

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

	for (int s = 0; s < 11; s++) pick_for_slot(s, 100);
	for (int s = 0; s < 11; s++) pick_for_slot(s, 95);
	for (int s = 0; s < 11; s++) pick_for_slot(s, 80);

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
	ClassDB::bind_method(D_METHOD("start_new_career", "club_id", "manager_name", "start_year"), &CareerState::start_new_career, DEFVAL(2006));
	ClassDB::bind_method(D_METHOD("get_leagues_and_clubs"), &CareerState::get_leagues_and_clubs);
	ClassDB::bind_method(D_METHOD("is_promo_code_redeemed", "code"), &CareerState::is_promo_code_redeemed);
	ClassDB::bind_method(D_METHOD("is_tuesday_league_unlocked"), &CareerState::is_tuesday_league_unlocked);
	ClassDB::bind_method(D_METHOD("redeem_promo_code", "code"), &CareerState::redeem_promo_code);
	ClassDB::bind_method(D_METHOD("is_career_active"), &CareerState::is_career_active);
	ClassDB::bind_method(D_METHOD("advance_day", "days"), &CareerState::advance_day, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("advance_to_next_matchday"), &CareerState::advance_to_next_matchday);

	// Top Header & Manager Career Queries
	ClassDB::bind_method(D_METHOD("get_managed_club_id"), &CareerState::get_managed_club_id);
	ClassDB::bind_method(D_METHOD("get_current_date_iso"), &CareerState::get_current_date_iso);
	ClassDB::bind_method(D_METHOD("get_season_label"), &CareerState::get_season_label);
	ClassDB::bind_method(D_METHOD("get_season_phase_info"), &CareerState::get_season_phase_info);
	ClassDB::bind_method(D_METHOD("get_managed_club_league_rank"), &CareerState::get_managed_club_league_rank);
	ClassDB::bind_method(D_METHOD("get_header_summary"), &CareerState::get_header_summary);
	ClassDB::bind_method(D_METHOD("get_manager_profile"), &CareerState::get_manager_profile);
	ClassDB::bind_method(D_METHOD("get_finances"), &CareerState::get_finances);
	ClassDB::bind_method(D_METHOD("get_board_objectives"), &CareerState::get_board_objectives);
	ClassDB::bind_method(D_METHOD("get_league_standings"), &CareerState::get_league_standings);
	ClassDB::bind_method(D_METHOD("get_next_fixture"), &CareerState::get_next_fixture);
	ClassDB::bind_method(D_METHOD("get_club_info", "club_id"), &CareerState::get_club_info);

	// Phase 5A / 5B: Schedule & Fixture Queries
	ClassDB::bind_method(D_METHOD("get_fixtures", "competition", "matchday", "club_id"),
			&CareerState::get_fixtures, DEFVAL(""), DEFVAL(-1), DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_fixture_details", "fixture_id"), &CareerState::get_fixture_details);
	ClassDB::bind_method(D_METHOD("get_total_matchdays", "competition"), &CareerState::get_total_matchdays, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_current_matchday"), &CareerState::get_current_matchday);

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
	ClassDB::bind_method(D_METHOD("prepare_match_context", "fixture", "db"), &CareerState::prepare_match_context, DEFVAL(Ref<PlayerDatabase>()));

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
	_load_unlock_persistence();
}

Dictionary CareerState::prepare_match_context(const Dictionary &p_fixture, const Ref<PlayerDatabase> &/*p_db*/) const {
	Dictionary fix = p_fixture.is_empty() ? get_next_fixture() : p_fixture;
	int home_id = static_cast<int>(fix.get("home_club_id", _managed_club_id));
	int away_id = static_cast<int>(fix.get("away_club_id", 0));

	Dictionary home_info = get_club_info(home_id);
	Dictionary away_info = get_club_info(away_id);

	String home_name = String(fix.get("home_club_name", fix.get("home_club", home_info.get("club_name", "Home"))));
	String away_name = String(fix.get("away_club_name", fix.get("away_club", away_info.get("club_name", "Away"))));

	String home_fmt = String(home_info.get("formation", "4-3-3"));
	String away_fmt = String(away_info.get("formation", "4-3-3"));

	Array home_squad = get_club_squad(home_id);
	Array away_squad = get_club_squad(away_id);

	Array home_xi;
	for (int i = 0; i < std::min<int>(11, home_squad.size()); ++i) {
		home_xi.append(home_squad[i]);
	}
	Array away_xi;
	for (int i = 0; i < std::min<int>(11, away_squad.size()); ++i) {
		away_xi.append(away_squad[i]);
	}
	Array home_bench_arr;
	for (int i = 11; i < std::min<int>(23, home_squad.size()); ++i) {
		home_bench_arr.append(home_squad[i]);
	}
	Array away_bench_arr;
	for (int i = 11; i < std::min<int>(23, away_squad.size()); ++i) {
		away_bench_arr.append(away_squad[i]);
	}

	Color home_col = Color::html(String(home_info.get("primary_color", "#1E3A8A")));
	Color away_col = Color::html(String(away_info.get("primary_color", "#DC2626")));
	Color away_alt_col = Color::html(String(away_info.get("secondary_color", "#F8FAFC")));

	bool is_user_home = (home_id == _managed_club_id);
	Dictionary active_tactics = get_club_tactics(_managed_club_id);

	Dictionary imp;
	imp["label"] = String(fix.get("match_importance", "STANDARD"));
	imp["text_color"] = Color(0.11, 0.82, 0.63);
	imp["description"] = home_name + " vs " + away_name + " • " + String(fix.get("stadium", "Stadium"));

	Dictionary kits;
	kits["home_primary"] = home_col;
	kits["away_primary"] = away_alt_col;
	kits["away_default_primary"] = away_col;
	kits["active_away_primary"] = away_col;
	kits["clash_detected"] = (home_col.is_equal_approx(away_col));

	Dictionary ctx;
	ctx["fixture"] = fix;
	ctx["home_club_id"] = home_id;
	ctx["away_club_id"] = away_id;
	ctx["home_club"] = home_name;
	ctx["away_club"] = away_name;
	ctx["home_ovr"] = static_cast<int>(home_info.get("team_rating", 75));
	ctx["away_ovr"] = static_cast<int>(away_info.get("team_rating", 75));
	ctx["home_formation"] = home_fmt;
	ctx["away_formation"] = away_fmt;
	ctx["home_lineup"] = home_xi;
	ctx["away_lineup"] = away_xi;
	ctx["home_bench"] = home_bench_arr;
	ctx["away_bench"] = away_bench_arr;
	ctx["user_is_home"] = is_user_home;
	String stad_name = String(fix.get("stadium", ""));
	if (stad_name.is_empty()) {
		stad_name = String(home_info.get("stadium", home_name + " Stadium"));
	}
	ctx["stadium"] = stad_name;
	ctx["competition"] = String(fix.get("competition", "League Match"));
	ctx["tactics"] = active_tactics;
	ctx["importance"] = imp;
	ctx["kits"] = kits;
	return ctx;
}

void CareerState::_load_unlock_persistence() {
	_redeemed_codes.clear();
	_tuesday_league_unlocked = false;
	const String path = "user://cfm1_unlocks.json";
	if (!FileAccess::file_exists(path)) return;
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
	if (f.is_null()) return;
	Variant parsed = JSON::parse_string(f->get_as_text());
	f->close();
	if (parsed.get_type() != Variant::DICTIONARY) return;
	Dictionary d = parsed;
	_tuesday_league_unlocked = bool(d.get("tuesday_league_unlocked", false));
	Array codes = d.get("redeemed_codes", Array());
	for (int i = 0; i < codes.size(); ++i) {
		std::string code = String(codes[i]).strip_edges().to_upper().utf8().get_data();
		if (!code.empty()) {
			_redeemed_codes.insert(code);
			if (code == "NHHA72528") {
				_tuesday_league_unlocked = true;
			}
		}
	}
}

void CareerState::_save_unlock_persistence() const {
	Ref<DirAccess> dir = DirAccess::open("user://");
	if (dir.is_valid()) {
		dir->make_dir_recursive("user://");
	}
	Dictionary d;
	d["tuesday_league_unlocked"] = _tuesday_league_unlocked;
	Array codes;
	for (const auto &c : _redeemed_codes) {
		codes.append(String(c.c_str()));
	}
	d["redeemed_codes"] = codes;
	Ref<FileAccess> f = FileAccess::open("user://cfm1_unlocks.json", FileAccess::WRITE);
	if (f.is_valid()) {
		f->store_string(JSON::stringify(d, "	"));
		f->close();
	}
}

bool CareerState::_is_club_or_league_locked(const std::string &club_name, const std::string &league_name) const {
	if (_tuesday_league_unlocked) return false;
	std::string c_low = to_lower_str(club_name);
	std::string l_low = to_lower_str(league_name);
	return (l_low == "tuesday league" || c_low == "11a7");
}

bool CareerState::is_promo_code_redeemed(const String &p_code) const {
	std::string norm = normalize_code(p_code);
	return !norm.empty() && _redeemed_codes.count(norm) > 0;
}

Dictionary CareerState::redeem_promo_code(const String &p_code) {
	Dictionary res;
	std::string norm = normalize_code(p_code);
	if (norm.empty()) {
		res["ok"] = false;
		res["message"] = String("Please enter a valid code.");
		return res;
	}

	if (norm != "NHHA72528") {
		res["ok"] = false;
		res["message"] = String("Invalid or unrecognized code: ") + String(norm.c_str());
		return res;
	}

	if (_redeemed_codes.count(norm) > 0) {
		_tuesday_league_unlocked = true;
		_save_unlock_persistence();
		res["ok"] = true;
		res["already_claimed"] = true;
		res["code"] = String("NHHA72528");
		res["unlocked_league"] = String("Tuesday League");
		res["unlocked_club"] = String("11A7");
		res["message"] = String("Code NHHA72528 is already active. Tuesday League & 11A7 are unlocked.");
		return res;
	}

	_redeemed_codes.insert(norm);
	_tuesday_league_unlocked = true;
	_save_unlock_persistence();

	int64_t bonus_balance = 50000000;
	int64_t bonus_transfer = 35000000;
	int64_t bonus_wage = 250000;
	int32_t bonus_reputation = 10;

	if (_career_active) {
		_finances.total_balance += bonus_balance;
		_finances.transfer_budget += bonus_transfer;
		_finances.wage_budget += bonus_wage;
		_manager.reputation = std::min(100, _manager.reputation + bonus_reputation);

		if (_clubs.find(_managed_club_id) != _clubs.end() && _clubs[_managed_club_id]) {
			_clubs[_managed_club_id]->AddBudget(static_cast<double>(bonus_transfer));
			_clubs[_managed_club_id]->AddTrainingPoints(5);
		}

		push_inbox_message("BOARD", "Special Code Redeemed: NHHA72528",
			"Code NHHA72528 activated! Unlocked Tuesday League & 11A7, credited +$50.0M Balance, +$35.0M Transfer Budget, +$250K/wk Wage Budget, +5 Skip/Training Tokens, and +10 Reputation.",
			"transfers", -1);

		emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
		emit_signal("squad_updated", _managed_club_id);
	}

	res["ok"] = true;
	res["already_claimed"] = false;
	res["code"] = String("NHHA72528");
	res["unlocked_league"] = String("Tuesday League");
	res["unlocked_club"] = String("11A7");
	res["bonus_balance"] = bonus_balance;
	res["bonus_transfer_budget"] = bonus_transfer;
	res["bonus_wage_budget"] = bonus_wage;
	res["bonus_skip_tokens"] = 5;
	res["bonus_reputation"] = bonus_reputation;
	res["message"] = String("Redeemed NHHA72528! Unlocked Tuesday League & 11A7 plus +$50M Balance & +$35M Transfer Budget.");
	return res;
}

TypedArray<Dictionary> CareerState::get_leagues_and_clubs() const {
	TypedArray<Dictionary> out;
	if (_clubs.empty()) {
		Ref<PlayerDatabase> temp_db;
		temp_db.instantiate();
		const char *club_paths[] = {
			"res://data/clubs.csv",
			"res://clubs.csv",
			"clubs.csv",
			"demo/data/clubs.csv"
		};
		for (const char *p : club_paths) {
			if (temp_db->load_clubs_from_csv(p)) break;
		}
		Array all_c = temp_db->get_all_clubs();
		for (int i = 0; i < all_c.size(); ++i) {
			Dictionary c = all_c[i];
			int32_t cid = static_cast<int32_t>(c.get("club_id", 1001 + i));
			std::string cname = String(c.get("club", c.get("name", ""))).utf8().get_data();
			std::string cleague = String(c.get("league", "Independent League")).utf8().get_data();
			std::string p_kit = String(c.get("primary_color", "#1E3A8A")).utf8().get_data();
			std::string s_kit = String(c.get("secondary_color", "")).utf8().get_data();
			if (s_kit.empty()) {
				s_kit = compute_secondary_kit_color(p_kit);
			}

			Dictionary entry;
			entry["club_id"] = cid;
			entry["club_name"] = String(cname.c_str());
			entry["club"] = String(cname.c_str());
			entry["league"] = String(cleague.c_str());
			entry["short_code"] = c.get("short_code", String(cname.c_str()).substr(0, 3).to_upper());
			entry["country"] = c.get("country", "Unknown");
			entry["reputation"] = c.get("reputation", 50);
			entry["total_balance"] = c.get("total_balance", 5000000);
			entry["transfer_budget"] = c.get("transfer_budget", 1500000);
			entry["wage_budget"] = c.get("wage_budget", 50000);
			entry["budget"] = c.get("total_balance", 5000000);
			entry["default_formation"] = c.get("default_formation", "4-4-2");
			entry["primary_color"] = String(p_kit.c_str());
			entry["secondary_color"] = String(s_kit.c_str());
			entry["stadium"] = c.get("stadium", String((cname + " Stadium").c_str()));
			entry["is_locked"] = _is_club_or_league_locked(cname, cleague);
			out.append(entry);
		}
		return out;
	}

	std::vector<int32_t> sorted_ids;
	sorted_ids.reserve(_clubs.size());
	for (const auto &kv : _clubs) {
		if (kv.second) sorted_ids.push_back(kv.first);
	}
	std::sort(sorted_ids.begin(), sorted_ids.end());

	for (int32_t cid : sorted_ids) {
		const auto &team = _clubs.at(cid);
		Dictionary entry;
		entry["club_id"] = team->GetId();
		entry["club_name"] = String(team->GetName().c_str());
		entry["club"] = String(team->GetName().c_str());
		entry["league"] = String(team->League.c_str());
		entry["short_code"] = String(team->ShortCode.c_str());
		entry["country"] = String(team->Country.c_str());
		entry["reputation"] = team->Reputation;
		entry["total_balance"] = team->TotalBalance;
		entry["transfer_budget"] = static_cast<int64_t>(team->GetBudget());
		entry["wage_budget"] = team->WageBudget;
		entry["budget"] = team->GetBudget();
		entry["team_rating"] = team->GetTeamRating();
		entry["default_formation"] = String(team->Formation.c_str());
		entry["primary_color"] = String(team->KitColor.c_str());
		entry["secondary_color"] = String(team->SecondaryColor.c_str());
		entry["stadium"] = String(team->Stadium.c_str());
		entry["is_locked"] = _is_club_or_league_locked(team->GetName(), team->League);
		out.append(entry);
	}
	return out;
}

CareerState::~CareerState() = default;

std::string CareerState::_format_date_iso() const {
	return format_ymd_iso(_year, _month, _day);
}

std::string CareerState::_format_season_label() const {
	std::ostringstream ss;
	int next_year = (_season_start_year % 100) + 1;
	ss << _season_start_year << "/" << std::setw(2) << std::setfill('0') << next_year;
	return ss.str();
}

int32_t CareerState::_compute_season_week() const {
	int64_t start_days = civil_to_days(_season_start_year, 7, 1);
	int64_t now_days = civil_to_days(_year, _month, _day);
	int64_t diff = std::max<int64_t>(0, now_days - start_days);
	return static_cast<int32_t>(diff / 7) + 1;
}

String CareerState::get_current_date_iso() const {
	return String(_format_date_iso().c_str());
}

String CareerState::get_season_label() const {
	return String(_format_season_label().c_str());
}

Dictionary CareerState::get_season_phase_info() const {
	Dictionary d;
	int32_t week = _compute_season_week();
	d["week"] = week;
	d["season_label"] = get_season_label();
	d["current_date"] = get_current_date_iso();
	String managed_league_str = "Premier Division";
	auto club_it = _clubs.find(_managed_club_id);
	if (club_it != _clubs.end() && club_it->second && !club_it->second->League.empty()) {
		managed_league_str = String(club_it->second->League.c_str());
	}
	d["current_matchday"] = get_current_matchday();
	d["total_matchdays"] = get_total_matchdays(managed_league_str);
	d["friendlies_month"] = String("July");
	d["league_start_date"] = String(format_ymd_iso(_season_start_year, 8, 15).c_str());
	d["season_end_date"] = String(format_ymd_iso(_season_start_year + 1, 5, 31).c_str());

	// Phase classification:
	// Weeks 1-2 (July 1 - Aug 14): Pre-season window (friendlies in July, Matchday 1 starts Week 3 / Mid-August)
	// Mid-August -> May 31: Competitive league season
	// June 1 -> June 30: Off-season / summer break
	if (_month == 6) {
		d["phase_code"] = String("OFF_SEASON");
		d["phase_label"] = String("End of Season • Summer Break (June)");
	} else if (week <= 2 || (_year == _season_start_year && (_month == 7 || (_month == 8 && _day < 15)))) {
		d["phase_code"] = String("PRE_SEASON");
		d["phase_label"] = String("Pre-Season Window • Weeks 1–2 (July Friendlies)");
	} else {
		d["phase_code"] = String("LEAGUE_SEASON");
		d["phase_label"] = String("Competitive Season • Mid-Aug to May");
	}
	return d;
}

void CareerState::_generate_season_fixtures(int32_t start_year) {
	_fixtures.clear();

	std::string league_comp = "Premier League";
	auto managed_it = _clubs.find(_managed_club_id);
	if (managed_it != _clubs.end() && managed_it->second && !managed_it->second->League.empty()) {
		league_comp = managed_it->second->League;
	}

	std::vector<int> team_ids;
	for (const auto &kv : _clubs) {
		if (!kv.second) continue;
		if (to_lower_str(kv.second->League) == to_lower_str(league_comp)) {
			team_ids.push_back(kv.first);
		}
	}
	std::sort(team_ids.begin(), team_ids.end());

	// Strict league isolation: single-club leagues do not borrow clubs from other leagues
	const int teams_count = static_cast<int>(team_ids.size());
	if (teams_count < 2) {
		return;
	}

	auto resolve_stadium = [&](int home_id) -> std::string {
		auto cit = _clubs.find(home_id);
		if (cit != _clubs.end() && cit->second) {
			if (!cit->second->Stadium.empty()) {
				return cit->second->Stadium;
			}
			if (home_id >= 1 && home_id <= 16 && cit->second->GetName() == DEFAULT_CLUB_NAMES[home_id - 1]) {
				return DEFAULT_STADIUM_NAMES[home_id - 1];
			}
			return cit->second->GetName() + " Stadium";
		}
		return "Main Stadium";
	};

	int fix_id = 1;

	// -------------------------------------------------------------------------
	// 1. PRE-SEASON WINDOW (Weeks 1–2 in July: Friendly 1 on Jul 18, Friendly 2 on Jul 25)
	//    In odd-sized leagues, the trailing club receives a single bye (no double-booking).
	// -------------------------------------------------------------------------
	for (int pre_wk = 1; pre_wk <= 2; pre_wk++) {
		int f_day = (pre_wk == 1) ? 18 : 25;
		std::string f_date = format_ymd_iso(start_year, 7, f_day);

		std::vector<int> f_rot = team_ids;
		int shift = (pre_wk - 1) % teams_count;
		std::rotate(f_rot.begin(), f_rot.begin() + shift, f_rot.end());

		for (int m = 0; m + 1 < teams_count; m += 2) {
			int home = f_rot[m];
			int away = f_rot[m + 1];
			if (pre_wk == 2) {
				std::swap(home, away);
			}

			FixtureRecord fix;
			fix.fixture_id = fix_id++;
			fix.week = pre_wk;
			fix.matchday = 0;
			fix.date_iso = f_date;
			fix.competition = "Pre-Season Friendly";
			fix.home_club_id = home;
			fix.away_club_id = away;
			fix.home_goals = -1;
			fix.away_goals = -1;
			fix.played = false;
			fix.is_friendly = true;
			fix.stadium = resolve_stadium(home);
			fix.match_importance = "FRIENDLY";
			_fixtures.push_back(fix);
		}
	}

	// -------------------------------------------------------------------------
	// 2. COMPETITIVE LEAGUE SEASON (Week 3+ • Mid-August to May)
	//    Full Double Round-Robin (handles both even and odd team counts via bye)
	// -------------------------------------------------------------------------
	std::vector<std::vector<std::pair<int, int>>> first_half_pairs;
	std::vector<int> rot = team_ids;
	if (rot.size() % 2 == 1) {
		rot.push_back(-1); // Bye sentinel for odd-numbered leagues
	}
	const int n_even = static_cast<int>(rot.size());
	const int rounds_per_half = n_even - 1;

	for (int round = 0; round < rounds_per_half; round++) {
		std::vector<std::pair<int, int>> round_matches;
		for (int m = 0; m < n_even / 2; m++) {
			int home = rot[m];
			int away = rot[n_even - 1 - m];
			if (home == -1 || away == -1) {
				continue;
			}
			if (m == 0 && (round % 2 == 1)) {
				std::swap(home, away);
			}
			round_matches.emplace_back(home, away);
		}
		first_half_pairs.push_back(std::move(round_matches));

		int last = rot.back();
		rot.pop_back();
		rot.insert(rot.begin() + 1, last);
	}

	const int total_matchdays = rounds_per_half * 2;
	const int64_t md1_days = civil_to_days(start_year, 8, 15);
	const int64_t md_end_days = civil_to_days(start_year + 1, 5, 23);
	const int span_days = static_cast<int>(md_end_days - md1_days);

	for (int md_idx = 0; md_idx < total_matchdays; md_idx++) {
		int matchday = md_idx + 1;
		int week = 3 + md_idx;
		int day_offset = (total_matchdays > 1)
			? static_cast<int>(std::llround((static_cast<double>(md_idx) * span_days) / (total_matchdays - 1)))
			: 0;

		int y = start_year;
		int m = 8;
		int d = 15;
		add_days_to_ymd(y, m, d, day_offset);
		std::string md_date = format_ymd_iso(y, m, d);

		bool is_second_half = (md_idx >= rounds_per_half);
		int base_round = md_idx % rounds_per_half;

		for (const auto &pair : first_half_pairs[base_round]) {
			int home = is_second_half ? pair.second : pair.first;
			int away = is_second_half ? pair.first : pair.second;

			FixtureRecord fix;
			fix.fixture_id = fix_id++;
			fix.week = week;
			fix.matchday = matchday;
			fix.date_iso = md_date;
			fix.competition = league_comp;
			fix.home_club_id = home;
			fix.away_club_id = away;
			fix.home_goals = -1;
			fix.away_goals = -1;
			fix.played = false;
			fix.is_friendly = false;
			fix.stadium = resolve_stadium(home);
			fix.match_importance = _classify_match_importance(home, away, matchday, fix.competition);
			_fixtures.push_back(fix);
		}
	}
}

void CareerState::_seed_default_objectives() {
	_board_objectives.clear();

	BoardObjective obj1;
	obj1.id = 1;
	obj1.category = "LEAGUE";
	obj1.title = "Top 4 Finish";
	obj1.description = "Secure qualification for continental competition by finishing in the top 4 by May.";
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
		// Pre-season friendlies do not award competitive league points
		if (!fix.played || fix.is_friendly) continue;
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

std::string CareerState::_classify_match_importance(int32_t home_id, int32_t away_id, int32_t matchday, const std::string &competition) const {
	if (competition.find("Friendly") != std::string::npos || matchday <= 0) {
		return "FRIENDLY";
	}
	if (competition.find("Final") != std::string::npos || competition.find("Cup") != std::string::npos) {
		return "CUP_FINAL";
	}
	auto h_it = _clubs.find(home_id);
	auto a_it = _clubs.find(away_id);
	std::string h_name = (h_it != _clubs.end() && h_it->second) ? h_it->second->GetName() : "";
	std::string a_name = (a_it != _clubs.end() && a_it->second) ? a_it->second->GetName() : "";

	if (is_classic_derby_pair(h_name, a_name)) {
		return "RIVALRY_DERBY";
	}

	int h_rank = 16;
	int a_rank = 16;
	for (size_t i = 0; i < _standings.size(); i++) {
		if (_standings[i].club_id == home_id) h_rank = static_cast<int>(i + 1);
		if (_standings[i].club_id == away_id) a_rank = static_cast<int>(i + 1);
	}

	if (matchday >= 26 && (h_rank <= 2 || a_rank <= 2)) {
		return "TITLE_DECIDER";
	}
	if (h_rank <= 4 && a_rank <= 4) {
		return "TOP_CLASH";
	}
	return "STANDARD";
}

void CareerState::_populate_fixture_goal_events(FixtureRecord &fix) {
	fix.events.clear();
	if (!fix.played) return;

	auto assign_team_goals = [&](int32_t club_id, int32_t goals_count, int32_t seed_offset) {
		if (goals_count <= 0) return;
		auto c_it = _clubs.find(club_id);
		if (c_it == _clubs.end() || !c_it->second) return;

		std::vector<std::shared_ptr<Player>> outfield;
		for (const auto &p : c_it->second->GetStartingXI()) {
			if (p && p->GetPosition() != Position::GK) {
				outfield.push_back(p);
			}
		}
		if (outfield.empty()) return;

		std::sort(outfield.begin(), outfield.end(), [](const std::shared_ptr<Player> &a, const std::shared_ptr<Player> &b) {
			int sa = a->GetAttributes().Shooting * 2 + a->GetAttributes().Dribbling;
			int sb = b->GetAttributes().Shooting * 2 + b->GetAttributes().Dribbling;
			return sa > sb;
		});

		size_t pool_size = std::min<size_t>(outfield.size(), 6);
		for (int g = 0; g < goals_count; g++) {
			int hash = (fix.fixture_id * 13) + (g * 29) + seed_offset;
			const auto &scorer = outfield[hash % pool_size];
			scorer->SetGoals(scorer->GetGoals() + 1);

			MatchEventRecord ev;
			ev.minute = std::clamp(6 + ((hash * 7) % 84), 2, 90);
			ev.club_id = club_id;
			ev.player_id = scorer->GetId();
			ev.player_name = scorer->GetName();
			ev.type = "GOAL";

			if (outfield.size() >= 2 && (hash % 4) != 0) {
				size_t assister_idx = (hash + 1) % std::min<size_t>(outfield.size(), 8);
				if (outfield[assister_idx]->GetId() == scorer->GetId()) {
					assister_idx = (assister_idx + 1) % outfield.size();
				}
				const auto &assister = outfield[assister_idx];
				assister->SetAssists(assister->GetAssists() + 1);
				ev.assist_player_id = assister->GetId();
				ev.assist_player_name = assister->GetName();
			}

			fix.events.push_back(ev);
		}
	};

	assign_team_goals(fix.home_club_id, fix.home_goals, 3);
	assign_team_goals(fix.away_club_id, fix.away_goals, 11);

	std::sort(fix.events.begin(), fix.events.end(), [](const MatchEventRecord &a, const MatchEventRecord &b) {
		return a.minute < b.minute;
	});

	if (fix.attendance <= 0) {
		int base_att = fix.is_friendly ? 26000 : ((fix.match_importance == "RIVALRY_DERBY" || fix.match_importance == "TITLE_DECIDER") ? 56000 : 42000);
		fix.attendance = base_att + ((fix.fixture_id * 733) % 14000);
	}
}

Dictionary CareerState::_fixture_to_dict(const FixtureRecord &fix, bool include_events) const {
	Dictionary d;
	d["fixture_id"] = fix.fixture_id;
	d["week"] = fix.week;
	d["matchday"] = fix.matchday;
	d["date_iso"] = String(fix.date_iso.c_str());
	d["competition"] = String(fix.competition.c_str());
	d["is_friendly"] = fix.is_friendly;
	d["season_phase"] = fix.is_friendly ? String("PRE_SEASON") : String("LEAGUE");
	d["home_club_id"] = fix.home_club_id;
	d["away_club_id"] = fix.away_club_id;

	auto h_it = _clubs.find(fix.home_club_id);
	auto a_it = _clubs.find(fix.away_club_id);
	d["home_club_name"] = (h_it != _clubs.end() && h_it->second) ? String(h_it->second->GetName().c_str()) : String("Home");
	d["away_club_name"] = (a_it != _clubs.end() && a_it->second) ? String(a_it->second->GetName().c_str()) : String("Away");
	d["home_ovr"] = (h_it != _clubs.end() && h_it->second) ? h_it->second->GetTeamRating() : 75;
	d["away_ovr"] = (a_it != _clubs.end() && a_it->second) ? a_it->second->GetTeamRating() : 75;
	d["home_formation"] = (h_it != _clubs.end() && h_it->second) ? String(h_it->second->Formation.c_str()) : String("4-3-3");
	d["away_formation"] = (a_it != _clubs.end() && a_it->second) ? String(a_it->second->Formation.c_str()) : String("4-3-3");

	d["home_goals"] = fix.home_goals;
	d["away_goals"] = fix.away_goals;
	d["played"] = fix.played;

	bool is_user = (fix.home_club_id == _managed_club_id || fix.away_club_id == _managed_club_id);
	bool is_home = (fix.home_club_id == _managed_club_id);
	d["is_user_match"] = is_user;
	d["is_home"] = is_home;

	String user_res = "";
	if (fix.played && is_user) {
		int ug = is_home ? fix.home_goals : fix.away_goals;
		int og = is_home ? fix.away_goals : fix.home_goals;
		if (ug > og) user_res = "W";
		else if (ug < og) user_res = "L";
		else user_res = "D";
	}
	d["user_result"] = user_res;

	std::string stad = fix.stadium;
	if (stad.empty() && h_it != _clubs.end() && h_it->second && !h_it->second->Stadium.empty()) {
		stad = h_it->second->Stadium;
	}
	if (stad.empty() && fix.home_club_id >= 1 && fix.home_club_id <= 16) {
		stad = DEFAULT_STADIUM_NAMES[fix.home_club_id - 1];
	}
	d["stadium"] = String(stad.c_str());
	d["attendance"] = fix.attendance;

	std::string imp = fix.match_importance.empty()
		? _classify_match_importance(fix.home_club_id, fix.away_club_id, fix.matchday, fix.competition)
		: fix.match_importance;
	d["match_importance"] = String(imp.c_str());

	if (include_events) {
		Array ev_arr;
		for (const auto &ev : fix.events) {
			Dictionary ed;
			ed["minute"] = ev.minute;
			ed["club_id"] = ev.club_id;
			auto ec_it = _clubs.find(ev.club_id);
			ed["club_name"] = (ec_it != _clubs.end() && ec_it->second) ? String(ec_it->second->GetName().c_str()) : String("Club");
			ed["player_id"] = ev.player_id;
			ed["player_name"] = String(ev.player_name.c_str());
			ed["assist_player_id"] = ev.assist_player_id;
			ed["assist_player_name"] = String(ev.assist_player_name.c_str());
			ed["type"] = String(ev.type.c_str());
			ev_arr.append(ed);
		}
		d["events"] = ev_arr;
	}

	return d;
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
	d["retirement_age"] = p->GetRetirementAge();
	d["club_name"] = String(p->GetClubName().c_str());
	d["csv_club_name"] = String(p->GetClubName().c_str());
	d["league"] = String(p->GetLeagueName().c_str());
	d["nationality"] = String(p->GetNationality().c_str());
	Array playstyles;
	for (const auto& style : p->GetPlaystyles()) playstyles.append(String(style.c_str()));
	d["playstyles"] = playstyles;

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
	d["stamina"] = p->GetStamina();
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
	_load_unlock_persistence();

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

	// Phase 5D: Anchor new Manager Career Mode saves to 15 July 2006 (2006-07-15, Season 2006/07)
	int32_t start_yr = (p_start_year > 0 && p_start_year != 2026) ? p_start_year : 2006;
	_season_start_year = start_yr;
	_year = start_yr;
	_month = 7;
	_day = 15;
	_current_matchday = 1;

	_clubs.clear();
	_club_id_by_name_lower.clear();
	_captain_by_club.clear();
	_players_by_id.clear();
	_fixtures.clear();
	_standings.clear();
	_inbox.clear();
	_board_objectives.clear();
	_next_player_id = 1;
	_next_inbox_id = 1;

	_hidden_youth_prospects.clear();
	if (!_youth_academy) {
		_youth_academy = std::make_unique<YouthAcademy>();
	} else {
		_youth_academy->Clear();
	_hidden_youth_prospects.clear();
	}

	_player_db.instantiate();
	const char *club_candidates[] = {
		"res://data/clubs.csv",
		"res://clubs.csv",
		"clubs.csv",
		"demo/data/clubs.csv"
	};
	bool clubs_loaded = false;
	for (const char *path : club_candidates) {
		if (_player_db->load_clubs_from_csv(path)) {
			clubs_loaded = true;
			break;
		}
	}

	const char *csv_candidates[] = {
		"res://data/players_2006_with_price.csv",
		"res://players_2006_with_price.csv",
		"players_2006_with_price.csv",
		"res://../Players_Database/players_2006_with_price.csv",
		"Players_Database/players_2006_with_price.csv"
	};
	bool db_loaded = false;
	for (const char *path : csv_candidates) {
		if (_player_db->load_from_csv(path)) {
			db_loaded = true;
			break;
		}
	}

	if (clubs_loaded) {
		Array raw_clubs = _player_db->get_all_clubs();
		for (int i = 0; i < raw_clubs.size(); ++i) {
			Dictionary cdict = raw_clubs[i];
			int32_t cid = static_cast<int32_t>(cdict.get("club_id", 1001 + i));
			std::string cname = String(cdict.get("club", cdict.get("name", ""))).utf8().get_data();
			if (cname.empty()) continue;

			std::string cleague = String(cdict.get("league", "Independent League")).utf8().get_data();
			std::string cshort = String(cdict.get("short_code", String(cname.c_str()).substr(0, 3).to_upper())).utf8().get_data();
			std::string ccountry = String(cdict.get("country", "Unknown")).utf8().get_data();
			int crep = static_cast<int>(cdict.get("reputation", 50));
			int64_t total_bal = static_cast<int64_t>(cdict.get("total_balance", 5000000));
			int64_t trans_bud = static_cast<int64_t>(cdict.get("transfer_budget", 1500000));
			int64_t wage_bud = static_cast<int64_t>(cdict.get("wage_budget", 50000));
			std::string def_form = String(cdict.get("default_formation", "4-3-3")).utf8().get_data();
			std::string p_kit = String(cdict.get("primary_color", "#1E3A8A")).utf8().get_data();
			std::string s_kit = String(cdict.get("secondary_color", "")).utf8().get_data();
			if (s_kit.empty()) {
				s_kit = compute_secondary_kit_color(p_kit);
			}

			auto team = std::make_unique<Team>(cname, static_cast<double>(trans_bud));
			team->SetId(cid);
			team->League = cleague;
			team->ShortCode = cshort;
			team->Country = ccountry;
			team->Reputation = crep;
			team->TotalBalance = total_bal;
			// Transfer budget is initialized through Team's public constructor/API.
			team->WageBudget = wage_bud;
			std::string cstadium = String(cdict.get("stadium", "")).strip_edges().utf8().get_data();
			if (cstadium.empty()) {
				cstadium = cname + " Stadium";
			}

			team->KitColor = p_kit;
			team->SecondaryColor = s_kit;
			team->Stadium = cstadium;
			team->Formation = def_form.empty() ? "4-3-3" : def_form;

			_club_id_by_name_lower[to_lower_str(cname)] = cid;
			_clubs[cid] = std::move(team);
		}
	}

	if (_clubs.empty()) {
		for (int i = 1; i <= 16; i++) {
			std::string club_name = DEFAULT_CLUB_NAMES[i - 1];
			double budget = 25000000.0 + (17 - i) * 3000000.0;
			auto team = std::make_unique<Team>(club_name, budget);
			team->SetId(i);
			team->League = "Premier League";
			team->Stadium = DEFAULT_STADIUM_NAMES[i - 1];
			team->Formation = "4-3-3";
			_club_id_by_name_lower[to_lower_str(club_name)] = i;
			_clubs[i] = std::move(team);
		}
	}

	// Route CSV players into their matching Club by Club name (unmatched -> Free Agent club_id = 0)
	if (db_loaded) {
		for (const auto &src : _player_db->get_raw_players()) {
			int pid = _next_player_id++;
			std::string p_name = src.name.utf8().get_data();
			std::string p_pos_str = src.position.strip_edges().to_upper().utf8().get_data();
			Position p_pos = string_to_position(p_pos_str);

			PlayerAttributes attrs;
			if (p_pos == Position::GK) {
				attrs.Pace = std::clamp(src.diving, 1, 99);
				attrs.Shooting = std::clamp(src.kicking, 1, 99);
				attrs.Passing = std::clamp(src.positioning, 1, 99);
				attrs.Dribbling = std::clamp(src.reflex, 1, 99);
				attrs.Defending = std::clamp(src.reaction, 1, 99);
				attrs.Physical = std::clamp(src.physical, 1, 99);
			} else {
				attrs.Pace = std::clamp(src.pace, 1, 99);
				attrs.Shooting = std::clamp(src.shooting, 1, 99);
				attrs.Passing = std::clamp(src.passing, 1, 99);
				attrs.Dribbling = std::clamp(src.dribbling, 1, 99);
				attrs.Defending = std::clamp(src.defending, 1, 99);
				attrs.Physical = std::clamp(src.physical, 1, 99);
			}

			int raw_age = src.age;
			int debut_age = std::clamp(src.debut_age, 15, 25);
			int p_pot = std::clamp(src.potential, src.overall, 99);
			bool is_below_debut = (raw_age < debut_age);
			bool meets_youth_req = (raw_age > 14 && raw_age < 18);

			// Youth Academy players have maximum 70 OVR (~ -20 from their OVR in players_2006_with_price.csv)
			PlayerAttributes effective_attrs = attrs;
			if (is_below_debut && meets_youth_req) {
				int target_ovr = std::clamp(src.overall - 20, 45, 70);
				int reduction = std::max(0, src.overall - target_ovr);
				effective_attrs.Pace = std::clamp(attrs.Pace - reduction, 40, 70);
				effective_attrs.Shooting = std::clamp(attrs.Shooting - reduction, 40, 70);
				effective_attrs.Passing = std::clamp(attrs.Passing - reduction, 40, 70);
				effective_attrs.Dribbling = std::clamp(attrs.Dribbling - reduction, 40, 70);
				effective_attrs.Defending = std::clamp(attrs.Defending - reduction, 40, 70);
				effective_attrs.Physical = std::clamp(attrs.Physical - reduction, 40, 70);
			}

			auto p_ptr = std::make_shared<Player>(
				p_name, p_pos, effective_attrs, raw_age, p_pot, debut_age, 37
			);
			p_ptr->SetId(pid);
			p_ptr->SetStamina(std::clamp(src.stamina, 1, 99));

			std::string p_club = src.club.strip_edges().utf8().get_data();
			std::string p_league = src.league.strip_edges().utf8().get_data();
			std::string p_nat = src.nationality.strip_edges().utf8().get_data();

			p_ptr->SetClubName(p_club.empty() ? "Free Agent" : p_club);
			p_ptr->SetLeagueName(p_league.empty() ? "Free Agents" : p_league);
			p_ptr->SetNationality(p_nat.empty() ? "Unknown" : p_nat);

			std::vector<std::string> styles;
			for (const auto &ps : src.playstyles) {
				std::string s_val = ps.strip_edges().utf8().get_data();
				if (!s_val.empty() && s_val != "Balanced") {
					styles.push_back(s_val);
				}
			}
			p_ptr->SetPlaystyles(styles);

			if (src.price > 0) {
				p_ptr->SetMarketValue(static_cast<double>(src.price));
			}
			p_ptr->SetCondition(100);
			p_ptr->SetMorale(85);
			p_ptr->SetWage(src.wage > 0 ? src.wage : std::max<int64_t>(1500, static_cast<int64_t>(p_ptr->GetOVR()) * 850));
			p_ptr->SetContractYears(3);

			std::string c_key = to_lower_str(p_club);
			auto c_it = _club_id_by_name_lower.find(c_key);
			int32_t parent_cid = (c_it != _club_id_by_name_lower.end() && _clubs.count(c_it->second) && _clubs[c_it->second])
				? c_it->second
				: 0;

			if (is_below_debut) {
				if (meets_youth_req) {
					// Eligible for Youth Academy (14 < age < 18, capped at 70 OVR)
					p_ptr->SetClubId(-1);
					p_ptr->SetWage(500);
					_players_by_id[pid] = p_ptr;
					_youth_academy->AddYouthPlayer(p_ptr);
				} else {
					// Below debut age & outside 14 < age < 18: hide from first team and youth academy until eligible
					p_ptr->SetClubId(-2);
					_players_by_id[pid] = p_ptr;
					_hidden_youth_prospects.push_back(p_ptr);
				}
			} else {
				if (parent_cid > 0) {
					p_ptr->SetClubId(parent_cid);
					_clubs[parent_cid]->AddPlayer(p_ptr);
				} else {
					p_ptr->SetClubId(0);
				}
				_players_by_id[pid] = p_ptr;
			}
		}
	}

	// Ensure every club has at least 11 players and auto-picks a valid Best XI with GK in slot 0
	for (auto &kv : _clubs) {
		auto &team = kv.second;
		if (!team) continue;
		while (team->GetRoster().size() < 11) {
			int pid = _next_player_id++;
			Position pos = team->GetRoster().empty() ? Position::GK : Position::CM;
			PlayerAttributes attrs{ 68, 65, 70, 68, 65, 70, 75 };
			auto filler = std::make_shared<Player>("Academy Reserve " + std::to_string(pid), pos, attrs, 20, 76, 16, 37);
			filler->SetId(pid);
			filler->SetStamina(75);
			filler->SetClubId(team->GetId());
			filler->SetClubName(team->GetName());
			filler->SetLeagueName(team->League);
			filler->SetCondition(100);
			filler->SetMorale(80);
			filler->SetWage(2000);
			_players_by_id[pid] = filler;
			team->AddPlayer(filler);
		}
		auto_pick_best_xi_for_formation(*team, team->Formation);

		const auto &xi = team->GetStartingXI();
		int default_captain = (!xi.empty() && xi[0]) ? xi[0]->GetId() : -1;
		for (const auto &starter : xi) {
			if (starter && starter->GetPosition() != Position::GK) {
				default_captain = starter->GetId();
				break;
			}
		}
		_captain_by_club[team->GetId()] = default_captain;
	}

	// Validate requested club_id safely without assuming ID 1 exists
	int32_t chosen_id = p_club_id;
	auto chosen_it = _clubs.find(chosen_id);
	if (chosen_it == _clubs.end() || !chosen_it->second) {
		chosen_id = -1;
		std::vector<int32_t> sorted_cids;
		for (const auto &kv : _clubs) {
			if (kv.second) sorted_cids.push_back(kv.first);
		}
		std::sort(sorted_cids.begin(), sorted_cids.end());
		for (int32_t cid : sorted_cids) {
			if (!_is_club_or_league_locked(_clubs[cid]->GetName(), _clubs[cid]->League)) {
				chosen_id = cid;
				break;
			}
		}
		if (chosen_id < 0 && !sorted_cids.empty()) {
			chosen_id = sorted_cids.front();
		}
	}

	if (chosen_id < 0 || _clubs.find(chosen_id) == _clubs.end() || !_clubs[chosen_id]) {
		UtilityFunctions::printerr("CareerState: No valid club available to start career.");
		return false;
	}

	if (_is_club_or_league_locked(_clubs[chosen_id]->GetName(), _clubs[chosen_id]->League)) {
		UtilityFunctions::printerr("CareerState: Attempted to start career with locked club/league without redeeming NHHA72528.");
		return false;
	}
	_managed_club_id = chosen_id;
	_manager.preferred_formation = _clubs[_managed_club_id]->Formation;

	// Build strictly league-isolated standings for the selected club's league
	std::string managed_league = _clubs[_managed_club_id]->League;
	for (const auto &kv : _clubs) {
		if (!kv.second) continue;
		if (to_lower_str(kv.second->League) != to_lower_str(managed_league)) {
			continue;
		}
		LeagueStandingRow row;
		row.club_id = kv.second->GetId();
		row.club_name = kv.second->GetName();
		_standings.push_back(row);
	}
	std::sort(_standings.begin(), _standings.end(), [](const LeagueStandingRow &a, const LeagueStandingRow &b) {
		return a.club_id < b.club_id;
	});

	Team *managed_team = _clubs[_managed_club_id].get();
	_finances.total_balance = managed_team->TotalBalance > 0
		? managed_team->TotalBalance
		: static_cast<int64_t>(std::max(15000000.0, managed_team->GetBudget()));
	_finances.transfer_budget = managed_team->GetBudget() > 0
		? static_cast<int64_t>(managed_team->GetBudget())
		: static_cast<int64_t>(std::max(10000000.0, _finances.total_balance * 0.35));
	_finances.wage_budget = managed_team->WageBudget > 0 ? managed_team->WageBudget : 650000;
	_finances.weekly_wage_bill = 0;
	for (const auto &p : managed_team->GetRoster()) {
		if (p) _finances.weekly_wage_bill += p->GetWage();
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
		PlayerAttributes attrs{ ys.ovr_seed, ys.ovr_seed, ys.ovr_seed, ys.ovr_seed, ys.ovr_seed, ys.ovr_seed, 75 };
		auto prospect = std::make_shared<Player>(ys.name, ys.pos, attrs, 15, ys.pot, 16, 37);
		prospect->SetId(y_id);
		prospect->SetStamina(75);
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

	_generate_season_fixtures(start_yr);
	_seed_default_objectives();
	_recalculate_standings();

	push_inbox_message("BOARD", "Welcome to " + String(managed_team->GetName().c_str()) + " (15 July 2006)",
		"Manager " + String(_manager.manager_name.c_str()) +
		", your 2006/07 career begins on 15 July 2006 in " + String(managed_team->League.c_str()) + ".",
		"schedule", _managed_club_id);

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

	// Official End of Competitive Season on May 31
	if (_month == 5 && _day == 31) {
		_process_end_of_season_may();
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

		if (home_it != _clubs.end() && home_it->second) {
			for (auto &p : home_it->second->GetStartingXI()) {
				if (p) p->RecordAppearance();
			}
		}
		if (away_it != _clubs.end() && away_it->second) {
			for (auto &p : away_it->second->GetStartingXI()) {
				if (p) p->RecordAppearance();
			}
		}

		_populate_fixture_goal_events(fix);
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

	if (_youth_academy) {
		_youth_academy->ProcessWeeklyTraining();
		emit_signal("youth_updated");
	}

	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
}

void CareerState::_process_end_of_season_may() {
	_recalculate_standings();
	int32_t final_rank = get_managed_club_league_rank();
	std::string champion_name = _standings.empty() ? "Unknown" : _standings[0].club_name;

	if (!_standings.empty() && _standings[0].club_id == _managed_club_id) {
		_manager.trophies_won++;
		_manager.reputation = std::min(100, _manager.reputation + 8);
	}

	for (auto &obj : _board_objectives) {
		if (obj.category == "LEAGUE" && !obj.is_completed) {
			obj.current_value = final_rank;
			if (final_rank <= obj.target_value) {
				obj.is_completed = true;
				_finances.total_balance += obj.reward_amount;
			}
		}
	}

	push_inbox_message("BOARD", "May End-of-Season Review (" + get_season_label() + ")",
		"The " + get_season_label() + " competitive campaign has concluded in May. Champions: " +
		String(champion_name.c_str()) + ". Your club finished Rank #" + String::num_int64(final_rank) +
		". Pre-season friendlies begin in July.",
		"schedule", _managed_club_id);

	emit_signal("balance_changed", _finances.total_balance, _finances.transfer_budget, _finances.wage_budget);
}

void CareerState::_process_annual_rollover() {
	_manager.seasons_managed++;
	_season_start_year = _year;
	_current_matchday = 1;

	for (auto &pair : _clubs) {
		if (pair.second) {
			pair.second->ProcessEndOfWeekOrSeason();
		}
	}
	if (_youth_academy) {
		_youth_academy->ProcessSeasonDevelopment();
		emit_signal("youth_updated");
	}
	_evaluate_hidden_youth_prospects();

	// Regenerate new season schedule (July Friendlies + Mid-August to May League Fixtures)
	_generate_season_fixtures(_season_start_year);
	_recalculate_standings();
	_seed_default_objectives();

	push_inbox_message("YOUTH", "New Season Rollover & Pre-Season Schedule (" + get_season_label() + ")",
		"Season " + get_season_label() + " has begun! Players aged +1 year, and the July Pre-Season Friendlies + Mid-August to May league calendar are now live.",
		"schedule", -1);
}

void CareerState::_evaluate_hidden_youth_prospects() {
	if (_hidden_youth_prospects.empty()) return;

	std::vector<std::shared_ptr<Player>> still_hidden;
	for (auto &p : _hidden_youth_prospects) {
		if (!p) continue;
		p->AgeOneYear();

		int age = p->GetAge();
		int debut_age = p->GetDebutAge();

		if (age >= debut_age) {
			// Reached debut age: assign to parent club first team (or Free Agents if unattached)
			std::string c_key = to_lower_str(p->GetClubName());
			auto c_it = _club_id_by_name_lower.find(c_key);
			if (c_it != _club_id_by_name_lower.end() && _clubs.count(c_it->second) && _clubs[c_it->second]) {
				int32_t target_cid = c_it->second;
				p->SetClubId(target_cid);
				_clubs[target_cid]->AddPlayer(p);
			} else {
				p->SetClubId(0);
			}
		} else if (age > 14 && age < 18 && _youth_academy) {
			// Meets Youth Academy minimum age requirement (14 < age < 18), capped at 70 OVR (~ -20 from CSV OVR)
			int target_ovr = std::clamp(p->GetOVR() - 20, 45, 70);
			int reduction = std::max(0, p->GetOVR() - target_ovr);
			PlayerAttributes a = p->GetAttributes();
			a.Pace = std::clamp(a.Pace - reduction, 40, 70);
			a.Shooting = std::clamp(a.Shooting - reduction, 40, 70);
			a.Passing = std::clamp(a.Passing - reduction, 40, 70);
			a.Dribbling = std::clamp(a.Dribbling - reduction, 40, 70);
			a.Defending = std::clamp(a.Defending - reduction, 40, 70);
			a.Physical = std::clamp(a.Physical - reduction, 40, 70);
			p->SetAttributes(a);
			p->SetClubId(-1);
			p->SetWage(500);
			_youth_academy->AddYouthPlayer(p);
		} else {
			still_hidden.push_back(p);
		}
	}

	_hidden_youth_prospects = std::move(still_hidden);
	emit_signal("youth_updated");
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

int32_t CareerState::advance_to_next_matchday() {
	if (!_career_active) return 0;

	std::string target_date;
	for (const auto &fix : _fixtures) {
		if (!fix.played && (fix.home_club_id == _managed_club_id || fix.away_club_id == _managed_club_id)) {
			target_date = fix.date_iso;
			break;
		}
	}
	if (target_date.empty()) return 0;

	int32_t days_advanced = 0;
	const int32_t safety_cap = 365;
	while (_format_date_iso() < target_date && days_advanced < safety_cap) {
		_advance_single_day();
		_simulate_ai_fixtures_for_current_date();
		if (_day % 7 == 0) {
			_process_weekly_progression();
		}
		days_advanced++;
	}

	if (days_advanced > 0) {
		emit_signal("date_changed", get_current_date_iso());
	}
	return days_advanced;
}

Dictionary CareerState::get_header_summary() const {
	Dictionary d;
	d["managed_club_id"] = _managed_club_id;

	auto it = _clubs.find(_managed_club_id);
	d["managed_club_name"] = (it != _clubs.end() && it->second) ? String(it->second->GetName().c_str()) : String("My Club");
	d["manager_name"] = String(_manager.manager_name.c_str());
	d["season_label"] = get_season_label();
	d["current_date"] = get_current_date_iso();
	d["season_week"] = _compute_season_week();
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
			return _fixture_to_dict(fix, false);
		}
	}
	return Dictionary();
}

TypedArray<Dictionary> CareerState::get_fixtures(const String &p_competition, int32_t p_matchday, int32_t p_club_id) const {
	TypedArray<Dictionary> list;
	std::string comp_filter = p_competition.strip_edges().utf8().get_data();

	for (const auto &fix : _fixtures) {
		if (!comp_filter.empty() && comp_filter != "ALL" && fix.competition != comp_filter) {
			continue;
		}
		if (p_matchday >= 0 && fix.matchday != p_matchday) {
			continue;
		}
		if (p_club_id > 0 && fix.home_club_id != p_club_id && fix.away_club_id != p_club_id) {
			continue;
		}
		list.append(_fixture_to_dict(fix, false));
	}
	return list;
}

Dictionary CareerState::get_fixture_details(int32_t p_fixture_id) const {
	for (const auto &fix : _fixtures) {
		if (fix.fixture_id == p_fixture_id) {
			return _fixture_to_dict(fix, true);
		}
	}
	return Dictionary();
}

int32_t CareerState::get_total_matchdays(const String &p_competition) const {
	std::string comp_filter = p_competition.strip_edges().utf8().get_data();
	int32_t max_md = 1;
	for (const auto &fix : _fixtures) {
		if (!comp_filter.empty() && comp_filter != "ALL" && fix.competition != comp_filter) {
			continue;
		}
		max_md = std::max(max_md, fix.matchday);
	}
	return max_md;
}

int32_t CareerState::get_current_matchday() const {
	for (const auto &fix : _fixtures) {
		if (!fix.played && !fix.is_friendly && (fix.home_club_id == _managed_club_id || fix.away_club_id == _managed_club_id)) {
			return fix.matchday;
		}
	}
	String managed_league_str = "Premier Division";
	auto club_it = _clubs.find(_managed_club_id);
	if (club_it != _clubs.end() && club_it->second && !club_it->second->League.empty()) {
		managed_league_str = String(club_it->second->League.c_str());
	}
	return get_total_matchdays(managed_league_str);
}

Dictionary CareerState::get_club_info(int32_t p_club_id) const {
	Dictionary d;
	auto it = _clubs.find(p_club_id);
	if (it == _clubs.end() || !it->second) return d;
	const Team &team = *it->second;
	d["id"] = team.GetId();
	d["club_id"] = team.GetId();
	d["club"] = String(team.GetName().c_str());
	d["name"] = String(team.GetName().c_str());
	d["club_name"] = String(team.GetName().c_str());
	d["league"] = String(team.League.c_str());
	d["short_code"] = String(team.ShortCode.c_str());
	d["country"] = String(team.Country.c_str());
	d["reputation"] = team.Reputation;
	d["total_balance"] = (p_club_id == _managed_club_id) ? _finances.total_balance : team.TotalBalance;
	d["transfer_budget"] = (p_club_id == _managed_club_id) ? _finances.transfer_budget : static_cast<int64_t>(team.GetBudget());
	d["wage_budget"] = (p_club_id == _managed_club_id) ? _finances.wage_budget : team.WageBudget;
	d["budget"] = d["transfer_budget"];
	d["default_formation"] = String(team.Formation.c_str());
	d["formation"] = String(team.Formation.c_str());
	d["primary_color"] = String(team.KitColor.c_str());
	d["secondary_color"] = String(team.SecondaryColor.c_str());
	d["stadium"] = String(team.Stadium.c_str());
	d["is_locked"] = _is_club_or_league_locked(team.GetName(), team.League);
	d["team_rating"] = team.GetTeamRating();
	d["squad_size"] = static_cast<int32_t>(team.GetRoster().size());
	d["captain_id"] = get_club_captain(p_club_id);
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
		if (seen.count(pid)) return false;
		seen.insert(pid);

		auto pit = _players_by_id.find(pid);
		if (pit == _players_by_id.end() || !pit->second || pit->second->GetClubId() != p_club_id) {
			return false;
		}
		new_xi.push_back(pit->second);
	}

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

	int base_ovr = std::clamp(56 + (_youth_academy->FacilityLevel * 2) + (seed % 5), 52, 70);
	int pot = std::clamp(78 + (_youth_academy->AcademyLevel * 3) + (seed % 6), base_ovr + 10, 95);

	int y_id = _next_player_id++;
	PlayerAttributes attrs{ base_ovr, base_ovr, base_ovr, base_ovr, base_ovr, base_ovr };
	auto prospect = std::make_shared<Player>(full_name, pos, attrs, 15, pot, 16, 37);
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

	auto build_lineup = [this](Team *team) -> Array {
		Array lineup;
		const auto &xi = team->GetStartingXI();
		if (xi.size() >= 11) {
			for (size_t i = 0; i < 11; i++) {
				if (xi[i]) lineup.append(_player_to_dict(xi[i]));
			}
		} else {
			const auto &roster = team->GetRoster();
			size_t limit = std::min<size_t>(11, roster.size());
			for (size_t i = 0; i < limit; i++) {
				if (roster[i]) lineup.append(_player_to_dict(roster[i]));
			}
		}
		return lineup;
	};

	auto build_bench = [this](Team *team) -> Array {
		Array bench;
		std::unordered_set<int32_t> xi_ids;
		for (const auto &sp : team->GetStartingXI()) {
			if (sp) xi_ids.insert(sp->GetId());
		}
		for (const auto &rp : team->GetRoster()) {
			if (!rp) continue;
			if (xi_ids.find(rp->GetId()) != xi_ids.end()) continue;
			bench.append(_player_to_dict(rp));
			if (bench.size() >= 12) break;
		}
		return bench;
	};

	p_engine->set_home_lineup(build_lineup(home_team));
	p_engine->set_away_lineup(build_lineup(away_team));
	p_engine->set_home_bench(build_bench(home_team));
	p_engine->set_away_bench(build_bench(away_team));

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
			_populate_fixture_goal_events(fix);

			bool is_user_home = (fix.home_club_id == _managed_club_id);
			int user_goals = is_user_home ? home_score : away_score;
			int opp_goals = is_user_home ? away_score : home_score;

			if (!fix.is_friendly) {
				if (user_goals > opp_goals) {
					_manager.career_wins++;
					_manager.board_confidence = std::min(100, _manager.board_confidence + 3);
				} else if (user_goals == opp_goals) {
					_manager.career_draws++;
				} else {
					_manager.career_losses++;
					_manager.board_confidence = std::max(0, _manager.board_confidence - 4);
				}
			}

			int64_t gate_receipts = fix.is_friendly ? (is_user_home ? 650000 : 250000) : (is_user_home ? 1800000 : 450000);
			_finances.total_balance += gate_receipts;

			auto user_club_it = _clubs.find(_managed_club_id);
			if (user_club_it != _clubs.end() && user_club_it->second) {
				Dictionary m_stats = p_engine->get_match_stats();
				Array end_stamina = is_user_home
					? Array(m_stats.get("home_player_stamina", Array()))
					: Array(m_stats.get("away_player_stamina", Array()));
				Array sub_off_records = is_user_home
					? Array(m_stats.get("home_subbed_off_records", Array()))
					: Array(m_stats.get("away_subbed_off_records", Array()));
				Array sub_on_ids = is_user_home
					? Array(m_stats.get("home_subbed_on_ids", Array()))
					: Array(m_stats.get("away_subbed_on_ids", Array()));

				std::unordered_set<int32_t> processed_ids;
				const auto &xi = user_club_it->second->GetStartingXI();

				// 1. Sync final on-pitch 11 (includes starters who finished the match + subbed-on bench players)
				for (size_t i = 0; i < xi.size(); i++) {
					if (!xi[i]) continue;
					int32_t pid = xi[i]->GetId();
					processed_ids.insert(pid);
					xi[i]->RecordAppearance();
					if (static_cast<int>(i) < end_stamina.size()) {
						int final_cond = std::clamp(static_cast<int>(end_stamina[static_cast<int>(i)]), 25, 100);
						xi[i]->SetCondition(final_cond);
					} else {
						xi[i]->SetCondition(std::max(40, xi[i]->GetCondition() - (fix.is_friendly ? 8 : 14)));
					}
				}

				// 2. Sync subbed-on bench players by ID (appearance credit + ending match stamina)
				for (int s = 0; s < sub_on_ids.size(); s++) {
					int32_t sub_pid = String(sub_on_ids[s]).to_int();
					if (sub_pid <= 0 || processed_ids.find(sub_pid) != processed_ids.end()) continue;
					auto p_it = _players_by_id.find(sub_pid);
					if (p_it != _players_by_id.end() && p_it->second) {
						processed_ids.insert(sub_pid);
						p_it->second->RecordAppearance();
						int final_cond = std::max(45, p_it->second->GetCondition() - (fix.is_friendly ? 5 : 9));
						// Check matching slot stamina if present in active lineup
						PackedStringArray live_names = p_engine->get_player_names();
						int offset = is_user_home ? 0 : 11;
						String target_name = String(p_it->second->GetName().c_str());
						for (int slot = 0; slot < 11 && (offset + slot) < live_names.size(); slot++) {
							if (live_names[offset + slot] == target_name && slot < end_stamina.size()) {
								final_cond = std::clamp(static_cast<int>(end_stamina[slot]), 25, 100);
								break;
							}
						}
						p_it->second->SetCondition(final_cond);
					}
				}

				// 3. Sync subbed-off starters (appearance credit + exact stamina at time of substitution)
				for (int o = 0; o < sub_off_records.size(); o++) {
					if (sub_off_records[o].get_type() != Variant::DICTIONARY) continue;
					Dictionary rec = sub_off_records[o];
					int32_t off_pid = String(rec.get("id", "0")).to_int();
					if (off_pid <= 0 || processed_ids.find(off_pid) != processed_ids.end()) continue;
					auto p_it = _players_by_id.find(off_pid);
					if (p_it != _players_by_id.end() && p_it->second) {
						processed_ids.insert(off_pid);
						p_it->second->RecordAppearance();
						int off_stam = std::clamp(static_cast<int>(rec.get("stamina", 65)), 25, 100);
						p_it->second->SetCondition(off_stam);
					}
				}
			}

			std::string result_str = is_user_home ?
				(std::to_string(home_score) + " - " + std::to_string(away_score)) :
				(std::to_string(away_score) + " - " + std::to_string(home_score));

			push_inbox_message("MATCH", fix.is_friendly ? "Pre-Season Friendly Report" : "Full-Time Match Report",
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
	root["tuesday_league_unlocked"] = _tuesday_league_unlocked;
	Array saved_codes;
	for (const auto &c : _redeemed_codes) saved_codes.append(String(c.c_str()));
	root["redeemed_codes"] = saved_codes;
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
		d["week"] = fix.week;
		d["matchday"] = fix.matchday;
		d["date_iso"] = String(fix.date_iso.c_str());
		d["competition"] = String(fix.competition.c_str());
		d["home_club_id"] = fix.home_club_id;
		d["away_club_id"] = fix.away_club_id;
		d["home_goals"] = fix.home_goals;
		d["away_goals"] = fix.away_goals;
		d["played"] = fix.played;
		d["is_friendly"] = fix.is_friendly;
		d["stadium"] = String(fix.stadium.c_str());
		d["attendance"] = fix.attendance;
		d["match_importance"] = String(fix.match_importance.c_str());

		Array ev_arr;
		for (const auto &ev : fix.events) {
			Dictionary ed;
			ed["minute"] = ev.minute;
			ed["club_id"] = ev.club_id;
			ed["player_id"] = ev.player_id;
			ed["player_name"] = String(ev.player_name.c_str());
			ed["assist_player_id"] = ev.assist_player_id;
			ed["assist_player_name"] = String(ev.assist_player_name.c_str());
			ed["type"] = String(ev.type.c_str());
			ev_arr.append(ed);
		}
		d["events"] = ev_arr;
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
		cd["club"] = String(team->GetName().c_str());
		cd["captain_id"] = get_club_captain(team->GetId());
		cd["name"] = String(team->GetName().c_str());
		cd["budget"] = team->GetBudget();
		cd["default_formation"] = String(team->Formation.c_str());
		cd["short_code"] = String(team->ShortCode.c_str());
		cd["league"] = String(team->League.c_str());
		cd["country"] = String(team->Country.c_str());
		cd["reputation"] = team->Reputation;
		cd["total_balance"] = team->TotalBalance;
		cd["transfer_budget"] = static_cast<int64_t>(team->GetBudget());
		cd["wage_budget"] = team->WageBudget;
		cd["primary_color"] = String(team->KitColor.c_str());
		cd["secondary_color"] = String(team->SecondaryColor.c_str());
		cd["stadium"] = String(team->Stadium.c_str());
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
	PackedInt32Array hidden_ids;
	for (const auto &p : _hidden_youth_prospects) {
		if (p) hidden_ids.append(p->GetId());
	}
	yd["prospect_ids"] = y_ids;
	yd["hidden_prospect_ids"] = hidden_ids;
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
		pd["club_name"] = String(p->GetClubName().c_str());
		pd["league"] = String(p->GetLeagueName().c_str());
		pd["nationality"] = String(p->GetNationality().c_str());
		Array saved_playstyles;
		for (const auto& style : p->GetPlaystyles()) saved_playstyles.append(String(style.c_str()));
		pd["playstyles"] = saved_playstyles;

		PlayerAttributes attrs = p->GetAttributes();
		pd["pace"] = attrs.Pace;
		pd["shooting"] = attrs.Shooting;
		pd["passing"] = attrs.Passing;
		pd["dribbling"] = attrs.Dribbling;
		pd["defending"] = attrs.Defending;
		pd["physical"] = attrs.Physical;

		pd["stamina"] = p->GetStamina();
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
	_club_id_by_name_lower.clear();
	_club_id_by_csv_id.clear();
	_captain_by_club.clear();
	_players_by_id.clear();
	_fixtures.clear();
	_standings.clear();
	_inbox.clear();
	_board_objectives.clear();
	if (!_youth_academy) _youth_academy = std::make_unique<YouthAcademy>();
	_youth_academy->Clear();
	_hidden_youth_prospects.clear();

	if (root.has("clock")) {
		Dictionary clock = root["clock"];
		_season_start_year = clock.get("season_start_year", 2006);
		_year = clock.get("year", 2006);
		_month = clock.get("month", 7);
		_day = clock.get("day", 15);
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

	_load_unlock_persistence();
	if (root.has("tuesday_league_unlocked")) {
		if (bool(root["tuesday_league_unlocked"])) {
			_tuesday_league_unlocked = true;
			_redeemed_codes.insert("NHHA72528");
		}
	}
	if (root.has("redeemed_codes")) {
		Array r_codes = root["redeemed_codes"];
		for (int i = 0; i < r_codes.size(); ++i) {
			std::string rc = String(r_codes[i]).strip_edges().to_upper().utf8().get_data();
			if (!rc.empty()) {
				_redeemed_codes.insert(rc);
				if (rc == "NHHA72528") _tuesday_league_unlocked = true;
			}
		}
	}
	_save_unlock_persistence();

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
			int ret_age = pd.get("retirement_age", 37);

			auto player_ptr = std::make_shared<Player>(name, pos, attrs, age, potential, debut_age, ret_age);
			player_ptr->SetClubName(String(pd.get("club_name", "Free Agent")).utf8().get_data());
			player_ptr->SetLeagueName(String(pd.get("league", "Free Agent")).utf8().get_data());
			player_ptr->SetNationality(String(pd.get("nationality", "Unknown")).utf8().get_data());
			std::vector<std::string> loaded_playstyles;
			Array playstyle_array = pd.get("playstyles", Array());
			for (int style_index = 0; style_index < playstyle_array.size(); ++style_index) {
				loaded_playstyles.push_back(String(playstyle_array[style_index]).utf8().get_data());
			}
			player_ptr->SetPlaystyles(loaded_playstyles);
			player_ptr->SetId(pid);
			player_ptr->SetStamina(static_cast<int>(pd.get("stamina", 75)));
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
			player_ptr->SetRetired(pd.get("is_retired", false) || age >= ret_age);

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

			std::string cname = String(cd.get("club", cd.get("name", "Club"))).utf8().get_data();
			double budget = cd.get("budget", cd.get("transfer_budget", 20000000.0));
			auto team = std::make_unique<Team>(cname, budget);
			team->SetId(cid);
			team->ShortCode = String(cd.get("short_code", "")).utf8().get_data();
			team->League = String(cd.get("league", "Independent League")).utf8().get_data();
			team->Country = String(cd.get("country", "Unknown")).utf8().get_data();
			team->Reputation = static_cast<int>(cd.get("reputation", 50));
			team->TotalBalance = static_cast<int64_t>(cd.get("total_balance", budget));
			team->WageBudget = static_cast<int64_t>(cd.get("wage_budget", 0));
			team->KitColor = String(cd.get("primary_color", "#FFFFFF")).utf8().get_data();
			team->SecondaryColor = String(cd.get("secondary_color", "#111111")).utf8().get_data();
			team->Stadium = String(cd.get("stadium", (cname + " Stadium").c_str())).utf8().get_data();
			team->Formation = String(cd.get("formation", cd.get("default_formation", "4-3-3"))).utf8().get_data();
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

			_club_id_by_csv_id[cid] = cid;
			_club_id_by_name_lower[_normalize_club_key(String(cname.c_str()))] = cid;
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
		PackedInt32Array hidden_ids = yd.get("hidden_prospect_ids", PackedInt32Array());
		for (int k = 0; k < hidden_ids.size(); k++) {
			int pid = hidden_ids[k];
			if (_players_by_id.find(pid) != _players_by_id.end()) {
				_hidden_youth_prospects.push_back(_players_by_id[pid]);
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
			fix.week = fd.get("week", fix.matchday <= 0 ? 1 : (fix.matchday + 2));
			fix.date_iso = String(fd.get("date_iso", "")).utf8().get_data();
			fix.competition = String(fd.get("competition", "")).utf8().get_data();
			fix.home_club_id = fd.get("home_club_id", -1);
			fix.away_club_id = fd.get("away_club_id", -1);
			fix.home_goals = fd.get("home_goals", -1);
			fix.away_goals = fd.get("away_goals", -1);
			fix.played = fd.get("played", false);
			fix.is_friendly = fd.get("is_friendly", fix.matchday <= 0 || fix.competition.find("Friendly") != std::string::npos);
			fix.stadium = String(fd.get("stadium", "")).utf8().get_data();
			fix.attendance = fd.get("attendance", 0);
			fix.match_importance = String(fd.get("match_importance", "STANDARD")).utf8().get_data();

			if (fd.has("events")) {
				Array ev_arr = fd["events"];
				for (int k = 0; k < ev_arr.size(); k++) {
					Dictionary ed = ev_arr[k];
					MatchEventRecord ev;
					ev.minute = ed.get("minute", 1);
					ev.club_id = ed.get("club_id", -1);
					ev.player_id = ed.get("player_id", -1);
					ev.player_name = String(ed.get("player_name", "")).utf8().get_data();
					ev.assist_player_id = ed.get("assist_player_id", -1);
					ev.assist_player_name = String(ed.get("assist_player_name", "")).utf8().get_data();
					ev.type = String(ed.get("type", "GOAL")).utf8().get_data();
					fix.events.push_back(ev);
				}
			}

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
