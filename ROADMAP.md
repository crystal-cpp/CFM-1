# CFM-1 Project Roadmap & Complete Career Overhaul

**Repository / Organization**: `crystal-cpp` · **Engine**: Godot 4.7 (`api_version=4.7`) + C++23 GDExtension

**Target Platforms**: Windows (MinGW/GCC 16.1.0 UCRT64) & Android (Touchscreen / Adaptive Aspect Ratios)

**Visual Identity**: Dark-Navy (`#06090E` / `#0B101B`) & Pitch-Green (`#10AC84` / `#1DD1A1`) · Zero-3D UI Architecture

**Current Milestone**: **Phase 4A & 4B Implemented -> Entering Phase 4C (Build, Automated Regression & Manual Verification)**

## 1. Master Phase Overview

| Phase | Milestone | Status | Primary C++ / GDScript Deliverables |
| :--- | :--- | :---: | :--- |
| **Phase 1** | C++23 `MatchEngine` Base Core | **Complete** | 22-player kinematic simulation, role attribute profiles, normalized `formations.csv` parser/validator, ball-carrier AI, `get_match_stats()`, skip-token economy. |
| **Phase 2A** | Chance-Gated Scoring, Direct Control & Physics | **Complete** | Chance-only scoring (`CHANCE_NONE` / `HOME_ATTACK` / `AWAY_ATTACK`), stamina & tactics physics coupling, direct control APIs (move, pass, through-ball, shot, switch, tackle). |
| **Phase 2B** | Dark-Navy Match UI, Gestures & In-Match Tactics | **Complete** | Unified desktop + Android gestures, Match HUD (scoreboard, 2D pitch, commentary strip), temporary full-screen in-match `TACTIC` screen with pause & XI preservation. |
| **Phase 2C** | Build & Match Regression Suite | **Complete** | SCons Windows UCRT64 compilation (`libfootball.windows.template_debug.x86_64.dll`) and headless/live match simulation regression suite. |
| **Phase 3** | Unified `CareerState`, Save/Load & Global Shell | **Complete** | Central C++ source of truth (`CareerState`), atomic JSON save/load, `CareerShell` 10-route router, `DashboardView`, `InboxView`, `PlayerProfileModal`, and `CareerRegressionTest` (Suites 1–8). |
| **Phase 4** | Team Management, Permanent Tactics & Youth UI | **In Progress (4C)** | **4A** C++ squad/tactics/youth bindings, GK 6-stat export & strict GK-only Slot `0` guard (`Complete`), **4B** procedural `SquadView` (drag-and-drop + click-swap GK lock), `TacticsView`, `YouthView`, `PlayerProfileModal` GK stats (`Complete`), **4C** SCons build, regression Suites 9–12 & manual QA (`Active`). |
| **Phase 5** | Schedule, Competitions, Pre-Match, `clubs.csv` & Codes | *Next Up* | **5A–5C** Calendar, read-only match modals, League Table, Pre-Match Center, Post-Match report pipeline; **5D** `res://data/clubs.csv` registry, New Career `League -> Club` setup, runtime CSV metadata loading for (`Club`, `League`, `Nationality`, `Playstyle 1..3`) — loader implemented, build verification pending — with `RetirementAge = 37`, `Tuesday League` / `11A7` exclusive lock, and Main Managing Menu **Codes Mechanism (`NHHA72528`)**. |
| **Phase 6** | Club Management, Finances & Board Rewards | *Scheduled* | Separated financial ledgers (Balance, Transfer Budget, Wage Budget), Board confidence & objectives tracker, stadium/facility upgrades, and competition prize payouts. |
| **Phase 7** | Dynamic Transfer Market, Search, Reputation & Playstyles | *Scheduled* | **7A–7D** Valuation engine, two-stage fee + wage negotiations, transfer windows, 6-field Transfer Search (`Name`, `Club`, `Age`, `Nationality`, `League`, `Position`) with `Tuesday League` / `11A7` lock gating, AI market transfers, loyalty & reputation; **7E** Exclusive $> 96$ Base OVR (non-academy) 22-Playstyle system; **7F** Mandatory Age-37 retirement & squad turnover pipeline. |
| **Phase 8** | Full Career Acceptance & Multi-Platform Release | *Scheduled* | Cross-platform desktop + Android verification, save/load integrity stress tests, performance profiling, and zero-3D release packaging. |

## 2. Completed Foundation (Phases 1–3)

### Phase 1 — C++23 `MatchEngine` Base Core `[COMPLETE]`

* **Kinematic 2D Simulation (`MatchEngine.h` / `MatchEngine.cpp`)**: 22 autonomous player entities with velocity, acceleration, facing, ball possession state, and role-anchored tactical zones.

* **Normalized Formation Loader (`formations.csv`)**: Parses and validates 11-slot formation coordinates (`[0.0, 1.0]` pitch space) and position tags (`GK`, `CB`, `LB`, `RB`, `CDM`, `CM`, `CAM`, `LW`, `RW`, `ST`).

* **Telemetry & Economy**: Exposes `get_match_stats()` (possession %, shots, shots on target, passes, tackles, fouls, xG/chances) and skip-token fast-resolution logic.

### Phase 2A — Chance-Gated Scoring & Direct Control `[COMPLETE]`

* **Strict Chance-Only Scoring**: Goals can only occur during active attacking transitions (`HOME_ATTACK` / `AWAY_ATTACK`), preventing off-screen/non-chance goal anomalies.

* **Tactics & Stamina Coupling**: `Mentality`, `Pressing`, `Defensive Line`, `Tempo`, `Passing Style`, and `Width` directly modulate sprint drain, interception radius, and pass risk in C++.

* **Direct Player Input API**: Exposed movement vector, ground pass, through-ball, shot power/aim, nearest-defender switch, and standing/sliding tackle bindings to GDScript.

### Phase 2B & 2C — Match UI, Gestures & Regression Verification `[COMPLETE]`

* **Unified Input Layer**: Seamless mouse/keyboard + Android touch virtual stick and swipe gestures.

* **Match HUD & In-Match Tactics**: Live scoreboard, 2D radar pitch, commentary ticker, and full-screen temporary `TACTIC` overlay that pauses simulation and preserves lineup state upon resume.

### Phase 3A — C++23 `CareerState` Core & Atomic Save/Load `[COMPLETE]`

* **Single Source of Truth (`CareerState.h` / `CareerState.cpp`)**: Owns and synchronizes `PlayerDatabase`, user and AI `Team` instances, `YouthAcademy`, season calendar, league standings, board objectives, and inbox feed.

* **Deterministic Bootstrap (`start_new_career`)**: Initializes club rosters, contracts, valuations, season fixtures, board objectives, and welcome inbox messages from a deterministic seed.

* **Atomic Schema-Versioned Persistence**:

  * `save_career(slot_name)` writes to `<slot>.tmp.json` and atomically renames to `<slot>.json` alongside `<slot>.meta.json` (club name, manager name, date, season, schema version).

  * `load_career(slot_name)`, `list_save_slots()`, and `delete_save_slot(slot_name)` validate schema compatibility and restore all C++ subsystems without orphaned pointers.

* **Stable Integer IDs**: All UI queries resolve via `player_id` and `club_id` dictionaries (`get_player_profile`, `get_club_squad`, `get_club_info`) rather than duplicating mutable objects in GDScript.

### Phase 3B — Global Navigation Shell & Persistent Header `[COMPLETE]`

* **Outer Frame (`demo/CareerShell.gd` / `demo/CareerShell.tscn`)**:

  * Persistent Top Header Bar: Club 3-letter badge, club name, season pill (`2026/27`), ISO date pill, live league rank pill, formatted club balance pill (`$50.0M`), `+1 Day` progression button, `Save` button, unread Inbox badge counter, and `Manager` profile modal trigger.

  * Persistent 10-Route Router: Caches view nodes inside `_router_container` and toggles `.visible` (`dashboard`, `squad`, `tactics`, `youth`, `schedule`, `competitions`, `transfers`, `club`, `finances`, `inbox`) with `"board"` normalized to `"club"`.

  * Adaptive Layout & Shortcuts: Auto-collapses sidebar drawer below `780px` viewport width toggle, supports `KEY_C` overlay toggle and `KEY_ESCAPE` modal dismissal.

### Phase 3C — Dashboard, Event-Driven Inbox & Universal Player Modal `[COMPLETE]`

* **Career Dashboard (`demo/DashboardView.gd` / `demo/DashboardView.tscn`)**: Auto-refreshes on `_ready()` and `visibility_changed`, featuring the Next Match Hero Card (with opponent rank & W-D-L record), Club Overview header strip + Top-5 standings table, Squad Condition summary, active Board Objectives, latest 3 Inbox alerts, and 6 Quick Action navigation buttons.

* **Event-Driven News / Inbox (`demo/InboxView.gd` / `demo/InboxView.tscn`)**: Category filter chips (`ALL`, `MATCH`, `TRANSFER`, `BOARD`, `YOUTH`, `INJURY`) with active pitch-green highlight, filter reset, live read/unread state sync (`mark_inbox_read` / `mark_all_inbox_read`), and route/entity deep-links (`squad`, `youth`, `transfers`, `club`).

* **Universal Context-Aware Player Profile Modal (`demo/PlayerProfileModal.gd` / `demo/PlayerProfileModal.tscn`)**: Centered `580x500` overlay (`PRESET_CENTER` with symmetric offsets) supporting 4 operational modes (`SQUAD`, `YOUTH`, `TRANSFER`, `MATCH`), displaying outfield attributes (`PAC`, `SHO`, `PAS`, `DRI`, `DEF`, `PHY`) or Goalkeeper attributes (`Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, `Physical`), styled `Condition`/`Morale` bars, context-adapted contract/youth rows, season stats, and mode-specific footer actions.

### Phase 3D — Build & Career Persistence Regression Suite `[COMPLETE]`

* Built `libfootball.windows.template_debug.x86_64.dll` via SCons and validated `CareerRegressionTest.gd` Suites 1–8 covering bootstrap determinism, atomic Save/Load round-trips, route normalization, modal centering, and inbox filtering.

## 3. Detailed Engineering Specification (Phases 4–8)

### Phase 4 — Team Management, Permanent Tactics & Youth Academy UI `[IN PROGRESS — PHASE 4C]`

#### Phase 4A — C++23 `CareerState` Squad, Tactics & Youth Academy Bindings `[COMPLETE]`

* **Squad Lineup & Strict GK-Only Slot `0` Validation Engine (`CareerState.h` / `CareerState.cpp`)**:

  * Manages `starting_xi` (11 slots mapped to active formation indices `0–10`), `bench` (7 slots), and `reserves` (`player_id` vectors).

  * `swap_squad_players(club_id, slot_a, slot_b)` and `set_starting_xi(club_id, xi_ids)` atomically validate all roster moves (`XI <-> XI`, `XI <-> Bench`, `XI <-> Reserves`, `Bench <-> Reserves`) while enforcing **strict Goalkeeper-only rules**:
    * Starting XI Slot `0` (`GK`) **must** be occupied by a player whose `Position == Position::GK`; any swap or drag attempt placing a non-`GK` outfielder (`CB`, `LB`, `RB`, `CDM`, `CM`, `CAM`, `LW`, `RW`, `ST`) into Slot `0` is immediately rejected (`return false`).
    * Outfield Starting XI Slots (`1–10`) **cannot** hold a `GK`; any attempt to place a `GK` into Slots `1–10` is immediately rejected, while `GK <-> GK` swaps between Slot `0` and Bench/Reserves are allowed.
    * Rejects duplicate `player_id` assignments across Starting XI, Bench, and Reserves.

  * Evaluates positional familiarity for every Starting XI slot (`NATURAL` = $100\%$ effective OVR, `COMPATIBLE` = $-5\%$ OVR modifier, `OUT_OF_POSITION` = $-15\%$ OVR modifier) and exposes effective slot OVR to GDScript.

  * Exposes club role & kit number setters (`set_club_roles(club_id, roles_dict)` for Captain, Vice-Captain, Penalties, Free Kicks, Corners; `set_player_jersey_number(club_id, player_id, number)` enforcing unique `1–99` numbers across the club).

* **Goalkeeper 6-Stat Profile Mapping (`src/mainplayer.h` & `src/CareerState.cpp`)**:

  * `PlayerAttributes` exposes semantic Goalkeeper getters mapped onto the 6 core attribute slots (`Pace -> Diving`, `Shooting -> Reflex`, `Passing -> Positioning`, `Dribbling -> Reaction`, `Defending -> Kicking`, `Physical -> Physical`) so `players_2006_with_price.csv` stays 100% compatible without extra columns.

  * `CareerState::_player_to_dict()` exports `"is_gk": true` alongside `"diving"`, `"reflex"`, `"positioning"`, `"reaction"`, `"kicking"`, and `"physical"` whenever `player->GetPosition() == Position::GK`.

* **Permanent Club Tactics Persistence (`CareerState` & `Team`)**:

  * Stores persistent tactical configuration (`formation_id`, `mentality`, `pressing`, `defensive_line`, `passing_style`, `width`, `tempo`) on the club `Team` entity and serializes it inside `<slot>.json`.

  * Enforces strict isolation between permanent club tactics (`TacticsView` <-> `CareerState`) and temporary in-match `TACTIC` overrides during live `MatchEngine` execution.

  * Direct `MatchEngine` handoff API packages exact Starting XI, Bench order, jersey numbers, roles, and permanent tactics when launching a match.

* **Youth Academy Infrastructure, Coaching & Dual-Cycle Growth (`YouthAcademy` / `CareerState`)**:

  * Exposes `get_youth_academy_info(club_id)` and `upgrade_youth_facility(club_id, upgrade_type)` for Academy Facility Level (`1–5`) and Scouting Network Level (`1–5`), deducting upfront cost from `club_balance` and updating weekly upkeep.

  * Exposes `assign_youth_coach(club_id, specialty, coach_id)` across four specialties (`Technical`, `Physical`, `Tactical`, `Goalkeeping`) that scale weekly attribute XP gains.

  * Separates **weekly training progression** (incremental attribute XP ticks and `dev_progress` updates executed every 7 days in `advance_day()`) from **annual season rollover** (age `+1`, debut age qualification checks, and potential range narrowing at season boundary).

  * Exposes `promote_youth_player(player_id)` to remove the prospect from `YouthAcademy`, append them to the senior `Team` reserves, initialize a senior contract/wage, and dispatch a `"BOARD"` (`"club"`) Inbox confirmation.

#### Phase 4B — Procedural GDScript UI Views & `CareerShell` Integration `[COMPLETE]`

* **1. Squad Management View (`demo/SquadView.gd` / `demo/SquadView.tscn`)**:

  * **Interactive 2D Formation Pitch**: Renders the 11 Starting XI player nodes at normalized `formations.csv` coordinates with jersey number badge, surname, position tag, natural vs. effective OVR, and condition bar.

  * **Bench (7 Slots) & Reserves List**: Scrollable side/bottom roster panels with position filters and fatigue/suspension indicators.

  * **Drag-and-Drop & Two-Click/Tap Swap with Strict GK-Only Slot `0` Lock**:
    * Attaches native Godot drag-and-drop (`set_drag_forwarding`) and click-to-swap validation (`_can_swap_slots(slot_a, pos_a, slot_b, pos_b)`) across all pitch nodes, bench slots, and reserve rows.
    * **Prevents Non-GK Dragging/Swapping into GK**: Any attempt to drag or click-swap a non-`GK` player onto Starting XI Slot `0` (`GK`)—or drag a `GK` onto outfield pitch slots (`1–10`)—is blocked at `_can_drop_data` and `_on_player_clicked`, displaying a Coral (`#FF6B6B`) warning banner (`"Only Goalkeepers (GK) can play in the GK slot!"`) and dimming incompatible slots.
    * Valid swap targets highlight with color-coded fit indicators (Pitch-Green `#1DD1A1` natural, Amber `#FECA57` compatible, Coral `#FF6B6B` out-of-position) and commit swaps directly to `CareerState`.

  * **Squad Roles, Kit Numbers & Inspection**: Inline selectors for Captain and Set-Piece takers, jersey number editor (`1–99`), and inspect button opening `PlayerProfileModal` in `"SQUAD"` mode.

* **2. Goalkeeper 6-Stat Profile Display (`demo/PlayerProfileModal.gd`)**:

  * Automatically detects `is_gk` / `position == "GK"` and switches the 6 attribute bars and labels from outfield stats (`Pace`, `Shooting`, `Passing`, `Dribbling`, `Defending`, `Physical`) to Goalkeeper stats:
    1. **`Diving`** (`diving`)
    2. **`Reflex`** (`reflex`)
    3. **`Positioning`** (`positioning`)
    4. **`Reaction`** (`reaction`)
    5. **`Kicking`** (`kicking`)
    6. **`Physical`** (`physical`)

* **3. Permanent Club Tactics View (`demo/TacticsView.gd` / `demo/TacticsView.tscn`)**:

  * **Formation Selector & Live 2D Shape Preview**: Dropdown/chip selector (`4-3-3`, `4-2-3-1`, `4-4-2`, `3-5-2`, `5-3-2`, etc.) dynamically repositioning 11 tactical markers on a 2D pitch preview and recalculating Starting XI positional fit.

  * **Tactical Sliders & Impact Readouts**: Controls for `Mentality` (`Ultra Defensive` -> `Ultra Attacking`), `Pressing Intensity` (`Low Block` -> `Gegenpress`), `Defensive Line` (`Deep` -> `High`), `Passing Style` (`Short` -> `Direct`), `Width` (`Narrow` -> `Wide`), and `Tempo` (`Slow` -> `Fast`) with live stamina-drain, defensive vulnerability, and build-up tempo tooltips.

* **4. Youth Academy View (`demo/YouthView.gd` / `demo/YouthView.tscn`)**:

  * **Academy Infrastructure Panel**: Displays Facility Level (`1–5`), Scouting Level (`1–5`), weekly upkeep cost, and balance-gated upgrade buttons (auto-disabled with shortfall tooltip when `club_balance < upgrade_cost`).

  * **Coaching Staff Panel**: Displays active `Technical`, `Physical`, `Tactical`, and `Goalkeeping` youth coaches and weekly development multipliers.

  * **Prospect Roster & Promotion Flow**: Lists academy prospects with `OVR`, `Potential Range`, `Weekly Dev Progress %`, `Condition`, and `Age`; supports inspecting via `PlayerProfileModal` (`"YOUTH"` mode) or direct promotion via `CareerState.promote_youth_player(player_id)`.

* **`CareerShell.gd` Router Wiring**: Replaced the `"squad"`, `"tactics"`, and `"youth"` placeholder cards with live instances of `SquadView`, `TacticsView`, and `YouthView`, synchronized with header balance/rank pills and `visibility_changed` hooks.

#### Phase 4C — Build, Automated Regression & Manual Verification `[CURRENT — ACTIVE]`

* **SCons GDExtension Compilation**: Recompile `libfootball.windows.template_debug.x86_64.dll` under MinGW/GCC 16.1.0 UCRT64 (`scons platform=windows target=template_debug api_version=4.7 -j6`) to expose all Phase 4A C++ methods to Godot 4.7.

* **Expanded Regression Suite (`demo/CareerRegressionTest.gd` Suites 9–12)**:

  * **Suite 9 (Squad Swaps, GK-Only Slot Lock & GK 6-Stat Profile)**: Verify `XI <-> Bench <-> Reserves` swaps, strict rejection of non-`GK` drag/swap into Slot `0` (`GK`) and `GK` into Slots `1–10`, `GK <-> GK` swap allowance, `is_gk` 6-stat dictionary keys (`diving`, `reflex`, `positioning`, `reaction`, `kicking`, `physical`), positional OVR penalty math, captaincy/set-piece/jersey number persistence, and `MatchEngine` squad handoff.

  * **Suite 10 (Permanent Tactics & Scope Isolation)**: Verify formation and slider mutations in `TacticsView` persist across route switches and Save/Load without leaking temporary in-match `TACTIC` changes.

  * **Suite 11 (Youth Academy Lifecycle)**: Verify facility/scouting upgrades (balance deduction + insufficient-funds guard), coach assignment bonuses, weekly training XP ticks vs. annual rollover aging (`+1` age), and `promote_youth_player()` roster migration + Inbox dispatch.

  * **Suite 12 (Full Phase 4 Save/Load Round-Trip)**: Mutate Starting XI, jersey numbers, club tactics, youth upgrades, and promote a prospect -> `save_career()` -> `load_career()` -> assert bit-exact restoration.

* **Manual Cross-Viewport & Interactive QA**: Verify `SquadView` (including drag-and-drop GK lock warning), `TacticsView`, `YouthView`, and `PlayerProfileModal` GK 6-stat display, `"normal"` `Label` pill overrides, modal centering, and responsive stacking on desktop (`1280x720`) and narrow/mobile (`800x480` / portrait) viewports, and delete legacy `*_2` duplicate files.

### Phase 5 — Schedule, Competitions, Pre-Match Center, Deterministic Club Database & Codes System `[NEXT UP]`

* **Target Files**: `demo/ScheduleView.gd` / `.tscn`, `demo/CompetitionsView.gd` / `.tscn`, `demo/PreMatchView.gd` / `.tscn`, `demo/PostMatchModal.gd` / `.tscn`, `demo/NewCareerView.gd` / `.tscn`, `demo/RedeemCodeModal.gd` / `.tscn`, `demo/CareerShell.gd`, `demo/data/clubs.csv`, `Players_Database/players_2006_with_price.csv`, `src/mainplayer.h` / `.cpp`, `src/team.h` / `.cpp`, `Players_Database/player_database.h` / `.cpp`, `src/CareerState.h` / `.cpp`.

* **5A. Schedule & Fixtures (`ScheduleView`)**:

  * **Dual View Modes**: Monthly calendar grid (highlighting matchdays, transfer window dates, and training blocks) and chronological fixture list with filters (`ALL`, `UPCOMING`, `COMPLETED`, `HOME`, `AWAY`).

  * **Read-Only Match Detail Modal**: Clicking any completed fixture opens full scoreline, goalscorers/assists timeline, yellow/red cards, and `MatchEngine` possession/shot statistics; clicking upcoming fixtures previews venue and kickoff metadata.

* **5B. Competitions Center (`CompetitionsView`)**:

  * **Live League Table**: Full standings (`Pos`, `Club`, `P`, `W`, `D`, `L`, `GF`, `GA`, `GD`, `Pts`, `Last 5 Form`) grouped by league from `res://data/clubs.csv` with qualification/relegation zone color bands.

  * **Player Leaderboards & History**: Sortable Top Scorers, Top Assists, Clean Sheets, and Average Match Rating tables plus multi-season archive selector (`2026/27`, `2027/28`, ...).

* **5C. Pre-Match Center & Post-Match Pipeline (`PreMatchView` & `PostMatchModal`)**:

  * **Pre-Match Briefing**: Side-by-side club comparison (OVR, tactical formation/mentality tendencies, Last-5 form, head-to-head record) and Starting XI fitness/suspension readiness check (auto-warning if any starter has $< 65\%$ condition).

  * **Match Launch & Commit**: `START MATCH` packages user + AI `Team` squads and permanent tactics from Phase 4 into `MatchEngine`; upon full-time whistle, `CareerState.commit_match_result()` updates league standings, player stats (`appearances`, `goals`, `assists`), post-match stamina/condition drain, gate receipts, and displays `PostMatchModal` before returning to `CareerShell`.

* **5D. Deterministic `res://data/clubs.csv` Registry, New Career `League -> Club` Selector, Exclusive `Tuesday League` / `11A7` Gate & Main Managing Menu Codes Mechanism `[MANDATORY DATA PIPELINE]`**:

  * **1. Canonical Club & League Registry (`demo/data/clubs.csv` -> `res://data/clubs.csv`)**:

    * Eliminates random club imports and hardcoded `DEFAULT_CLUB_NAMES[16]` arrays. Every league and club in the simulation must be defined explicitly in `res://data/clubs.csv` (which pairs each `League` and `Club` column) with the following header schema:

      ```csv
      ClubID,Club,League,ShortCode,Country,Reputation,TotalBalance,TransferBudget,WageBudget,DefaultFormation,PrimaryColor,SecondaryColor
      ```

    * `CareerState` parses `res://data/clubs.csv` during pre-career catalog inspection and `start_new_career()`, validating unique positive `ClubID` integers, non-empty `Club` and `League` keys, valid `DefaultFormation` IDs from `formations.csv`, and exact league groupings. Any club or league not present in `res://data/clubs.csv` is **never** randomly generated.

    * **New Career Setup Flow (`New Career -> Select League -> Select Club -> Manager Name` in `demo/NewCareerView.gd`)**:

      * When creating a new career in the main career flow, the **Select League** step dynamically lists the distinct `League` entries defined in `res://data/clubs.csv` (alongside country and top-club star rating).

      * Selecting a `League` dynamically filters the **Select Club** step strictly to the clubs belonging to that `League` in `res://data/clubs.csv`, displaying each club's squad OVR, star rating, and budgets before invoking `career_state.start_new_career(selected_club_id, manager_name, 2026)`.

  * **2. Merging `Club`, `League`, `Nationality`, and `Playstyle 1..3` Columns in `players_2006_with_price.csv` (and `players.csv`)**:

    * Standardizes the player CSV header schema to include `Club`, `League`, `Nationality`, and `Playstyle 1..3` (for Phase 7E):

      ```csv
      Name,Position,Age,Potential,DebutAge,RetirementAge,Pace,Shooting,Passing,Dribbling,Defending,Physical,Price,Club,League,Nationality,Playstyle 1,Playstyle 2,Playstyle 3
      ```

    * **`src/mainplayer.h` / `src/mainplayer.cpp`**:

      * Add `std::string ClubName = "Free Agent"`, `std::string LeagueName = "Free Agent"`, `std::string Nationality = "Unknown"`, `int BaseOVR = 0`, `bool IsAcademyProduct = false`, `bool IsRetired = false`, `int RetirementAge = 37`, and `std::vector<std::string> Playstyles` with getters and setters (`GetClubName()`/`SetClubName()`, `GetLeagueName()`/`SetLeagueName()`, `GetNationality()`/`SetNationality()`, `GetBaseOVR()`/`SetBaseOVR()`, `GetIsAcademyProduct()`/`SetIsAcademyProduct()`, `GetIsRetired()`/`SetIsRetired()`, `GetRetirementAge()`/`SetRetirementAge()`, `GetPlaystyles()`/`SetPlaystyles()`).

    * **`Players_Database/player_database.h` / `Players_Database/player_database.cpp`**:

      * Update `load_from_csv()` to resolve the `"club"`, `"league"`, `"nationality"`, `"playstyle 1"`, `"playstyle 2"`, and `"playstyle 3"` (case-insensitive) header column indices dynamically, strip trailing `\r` and leading/trailing whitespace, default blank `Club`/`League` cells to `"Free Agent"` and blank `Nationality` cells to `"Unknown"`, and record `player.SetBaseOVR(player.GetOVR())`.
      * Normalize `RetirementAge` to **`37`** for all imported rows (overriding any legacy `36` defaults or CSV `RetirementAge` values so every player retires uniformly at age `37` in Phase 7F).

      * Add `load_clubs_from_csv(const std::string& path)` to parse `res://data/clubs.csv` into structured `ClubDefinition` records (`ClubID`, `Club`, `League`, `ShortCode`, `Country`, budgets, colors, and `DefaultFormation`).

    * **`src/CareerState.h` / `src/CareerState.cpp`**:

      * Expose `get_leagues_and_clubs()` returning all leagues and clubs loaded from `res://data/clubs.csv` (including `is_locked` status) so `NewCareerView.gd` renders the exact `League -> Club` hierarchy.

      * In `start_new_career()`, instantiate `Team` instances strictly from `res://data/clubs.csv`, then route every player from `players_2006_with_price.csv` into their exact matching `Team` by normalized `Club` name (supporting clean alias matching such as `FC Barcelona` -> `Barcelona`), syncing the club's canonical `League` while preserving each player's `Nationality` and `League` attributes.

      * Players whose `Club` value is empty, `"Free Agent"`, or does not match an entry in `res://data/clubs.csv` are placed into the **Free Agent** pool (`club_id = 0`, `LeagueName = "Free Agent"`) rather than spawning unconfigured clubs.

      * Run positional Best-XI sorting (`auto_pick_best_xi`) on every club roster after CSV import so `GK` always occupies Slot `0` and outfielders fill natural roles.

      * Expose `"club_name"`, `"league"`, `"nationality"`, `"retirement_age"`, and `"is_retired"` in `_player_to_dict()` and persist `"csv_club_name"`, `"league"`, `"nationality"`, `"base_ovr"`, `"is_academy_product"`, `"retirement_age"`, `"is_retired"`, `"country"`, `"short_code"`, `"playstyles"`, and `"redeemed_codes"` inside `save_career()` / `load_career()` JSON snapshots.

  * **3. Exclusive Access Lock for `Tuesday League` & `11A7` Club**:

    * **Strict Access Rule**: All players tagged with `League = "Tuesday League"` and `Club = "11A7"` in `players_2006_with_price.csv`, as well as the **`Tuesday League -> 11A7`** club entry in `res://data/clubs.csv`, are **exclusively locked by default** and **only accessible by redeeming the secret code `NHHA72528`**.
    * **New Career Lock (`NewCareerView.gd` & `CareerState::get_leagues_and_clubs()`)**:
      * While `NHHA72528` has not been redeemed, `Tuesday League -> 11A7` cannot be selected when starting a new career (displayed with a `LOCKED — Redeem Code in Managing Menu` lock badge or hidden from unlocked league selection until unlocked).
      * Attempting to invoke `start_new_career()` with `11A7`'s `club_id` while locked is rejected by `CareerState` with an explicit access-denied error.
      * Once `NHHA72528` is redeemed, the unlock state persists globally (`user://cfm1_unlocks.json`), immediately unlocking **`Tuesday League -> 11A7`** in New Career setup so the player can manage `11A7` directly.
    * **In-Career Player & Transfer Lock (`TransfersView.gd` & `CareerState::search_transfer_players()`)**:
      * While locked, players in `Tuesday League` / `11A7` are excluded from Transfer Market search results, scouting lists, and AI transfer bids—they cannot be viewed, scouted, or signed until `NHHA72528` is redeemed.
      * Once `NHHA72528` is redeemed, all `Tuesday League` / `11A7` players become immediately accessible across the career database and Transfer Market.

  * **4. Main Managing Menu Codes Mechanism (`CareerShell.gd` + `RedeemCodeModal.gd` / `.tscn` + C++ `CareerState`)**:

    * **UI Entry Point in Main Managing Menu (`CareerShell.gd`)**:
      * Adds a dedicated **`Codes`** button directly in the Main Managing Menu (`CareerShell` persistent top header bar next to `Save` / `Manager`, and mirrored inside the `Manager` profile modal quick-actions) so players can open the **Redeem Code Modal (`RedeemCodeModal.tscn`)** at any point during career management.
      * `RedeemCodeModal` follows the mandatory Phase 3B/3C centered modal layout (`PRESET_CENTER`, `460x320` symmetric offsets, `GROW_DIRECTION_BOTH`, `"normal"` `Label` badge overrides) with a dark-navy input field, `REDEEM CODE` primary pitch-green button, live status banner, and a **Redeemed Codes History** list.
    * **C++ Redemption Engine (`CareerState::redeem_promo_code(const std::string& raw_code)`)**:
      * Normalizes input (trims whitespace, converts ASCII letters to uppercase) and validates against the C++ `PromoCodeRegistry`.
      * **Active Secret Code — `NHHA72528`**:
        * **Code String**: `NHHA72528`
        * **Unlock Reward**: Permanently sets `tuesday_league_11a7_unlocked = true` in both the active `CareerState` save (`<slot>.json`) and persistent global profile unlock storage (`user://cfm1_unlocks.json`), unlocking **`Tuesday League`** and **`11A7` Club** (plus all `Tuesday League` / `11A7` players) in both New Career (`Tuesday League -> 11A7`) and Transfer Market searches.
        * **Career Treasury & Club Rewards** (credited immediately upon redemption in an active career):
          1. **`+$50,000,000` (`$50.0M`)** added to `Total Club Balance`.
          2. **`+$35,000,000` (`$35.0M`)** added to `Transfer Budget`.
          3. **`+$250,000 / week`** added to `Wage Budget`.
          4. **`+5` Match Skip Tokens** added to the club's skip-token inventory.
          5. **`+10` Club Reputation** (clamped to max `100`).
        * **Inbox & HUD Feedback**: Dispatches a `"BOARD"` (`"club"`) Inbox message (`"Special Code Redeemed: NHHA72528 — Tuesday League & 11A7 Club Unlocked + $50.0M Reward Package"`), triggers `CareerShell.refresh_header()` so the top-bar balance pill and unread Inbox counter update immediately, and fires a pitch-green confirmation toast.
      * **Duplicate & Invalid Code Validation**:
        * Prevents double-claiming currency/token rewards within the same career save (`redeemed_codes` set in `<slot>.json`) while still ensuring global `Tuesday League -> 11A7` unlock persistence.
        * Displays clear feedback banners for `SUCCESS` (Pitch-Green `#1DD1A1`), `ALREADY_REDEEMED` (Amber `#FECA57`), and `INVALID_CODE` (Coral `#FF6B6B`).

  * **5. Custom Player & Classmate Roster Addition Workflow**:

    * Supports adding custom real-world legends and user-created players (such as the user's classmates in `11A7`) to the runtime dataset `demo/data/players_2006_with_price.csv`; the `Players_Database/` copy is maintained separately as a preventive/source copy:

      * Populate `Club = "11A7"`, `League = "Tuesday League"`, and `Nationality` (matching the `11A7` / `Tuesday League` entry in `res://data/clubs.csv`) so they remain exclusively protected behind code `NHHA72528` and load deterministically onto `11A7`'s roster once unlocked.

      * Custom players authored with base attributes yielding $\text{BaseOVR} > 96$ (`97`, `98`, or `99` OVR) and populated `Playstyle 1..3` columns automatically unlock **Phase 7E Playstyles**.

* **Phase 5 Acceptance Criteria**:

  * Selecting a `League` and `Club` in New Career setup reflects the exact `League` and `Club` entries from `res://data/clubs.csv`, with zero random clubs generated.

  * Before redeeming `NHHA72528`, `Tuesday League` / `11A7` is locked in New Career (`Tuesday League -> 11A7`) and inaccessible in player searches; entering `NHHA72528` in the Main Managing Menu `Codes` modal grants the full reward package, unlocks `Tuesday League -> 11A7` and all `11A7` players, and persists across Save/Load.

  * Playing or simulating a match updates the League Table, player season stats, club gate-receipt balance, and advances the Next Match Hero Card to the subsequent matchday.

### Phase 6 — Club Management, Finances & Board Rewards

* **Target Files**: `demo/ClubView.gd` / `.tscn`, `demo/FinancesView.gd` / `.tscn`, plus C++ `CareerState` ledger & objective evaluators.

* **1. Separated Financial Ledgers (`FinancesView`)**:

  * **Three Distinct Buckets**:

    1. `Total Club Balance`: Overall cash reserves.

    2. `Transfer Budget`: Dedicated capital for incoming transfer fees.

    3. `Wage Budget`: Weekly payroll cap vs. current committed weekly wage bill (`Available Wage Cap = Wage Budget - Current Weekly Payroll`).

  * **Budget Allocation Slider**: Allows shifting funds between Transfer Budget and Wage Budget within Board-approved limits.

  * **Income & Expense Ledger**: Itemized breakdown of matchday gate receipts, TV/broadcast rights, sponsor payouts, competition prize money, player wages, staff/academy upkeep, and transfer amortization.

* **2. Club Management & Board Objectives (`ClubView`)**:

  * **Board Confidence Meter**: $0\text{--}100\%$ job security gauge driven by league rank vs. expectation, financial health, and objective completion.

  * **Season Objectives Tracker**: Domestic league finish target, cup progression target, youth promotion quota, and wage-control target—each displaying live progress, deadline, and reward payout.

  * **Stadium & Facilities Upgrades**: Tiered upgrades (`Stadium Capacity`, `Training Complex`, `Medical Center`, `Youth Academy`) with construction duration in days, upfront cost, and ongoing weekly maintenance.

* **Phase 6 Acceptance Criteria**:

  * Weekly wage deductions execute automatically every 7 days on `advance_day()`; completing a board objective credits `Total Club Balance` and dispatches a `BOARD` Inbox notification.

### Phase 7 — Dynamic Transfer Market, Multi-Criteria Search, Contracts, Reputation, Playstyles & Retirement

* **Target Files**: `demo/TransfersView.gd` / `.tscn`, `demo/NegotiationModal.gd` / `.tscn`, `demo/PlayerProfileModal.gd`, `demo/SquadView.gd`, `src/mainplayer.h` / `.cpp`, `src/MatchEngine.h` / `.cpp`, `src/CareerState.h` / `.cpp`.

* **7A. Scalable Player Valuation Engine**:

  * Computes dynamic market value from:

    $$
    \text{Value} = \text{BaseOVR}(\text{OVR}) \times \text{AgeCurve}(\text{Age}, \text{Potential}) \times \text{PosMult}(\text{Pos}) \times \text{ContractMult}(\text{YearsLeft}) \times \text{RoleMult}(\text{SquadRole})
    $$

  * Accounts for expiring contracts ($\le 1\text{ year}$ discount), high-potential U21 wonderkid premiums, and selling club financial pressure.

* **7B. Two-Stage Transfer & Contract Negotiations (`NegotiationModal`)**:

  * **Stage 1 — Club-to-Club Fee Negotiation**: Submit initial transfer fee (plus optional sell-on clause); AI club evaluates against player valuation and squad depth, responding with `Accept`, `Counteroffer` (up to 3 rounds), or `Reject`.

  * **Stage 2 — Player Personal Terms**: Upon fee agreement, negotiate `Weekly Wage`, `Contract Length (1–5 yrs)`, and `Squad Role` (`Crucial`, `Important`, `Rotation`, `Prospect`), clamped by the mandatory Age-37 retirement ceiling ($\min(5, \max(1, 37 - \text{Age}))$).

* **7C. Multi-Criteria Transfer Search Engine, Access Gating, Transfer Windows & AI Market Activity (`TransfersView`)**:

  * **6-Criteria Player Search & Filter Engine**: Backed by C++ `CareerState::search_transfer_players(filters_dict)` querying the merged `players_2006_with_price.csv` + `clubs.csv` database across all 6 search parameters (excluding retired players):

    1. **`Name`**: Case-insensitive substring query matching player full name or surname.

    2. **`Club`**: Dropdown/autocomplete filter sourced from `res://data/clubs.csv` plus `"Free Agent"` (hides `11A7` until `NHHA72528` is redeemed).

    3. **`Age`**: Minimum and maximum age bounds (`Min Age` to `Max Age`, bounded below `37`).

    4. **`Nationality`**: Country filter sourced directly from the merged `Nationality` column in `players_2006_with_price.csv`.

    5. **`League`**: Competition filter sourced directly from the merged `League` column in `players_2006_with_price.csv` and `res://data/clubs.csv` (hides `Tuesday League` until `NHHA72528` is redeemed).

    6. **`Position`**: Exact position (`GK`, `CB`, `LB`, `RB`, `CDM`, `CM`, `CAM`, `LW`, `RW`, `ST`) or positional group (`DEF`, `MID`, `FWD`) filter.

  * **`Tuesday League` / `11A7` Visibility Gate**: Enforces that players with `League == "Tuesday League"` and `Club == "11A7"` remain completely hidden and un-signable in `TransfersView` until `NHHA72528` has been redeemed via the Main Managing Menu `Codes` modal.

  * **Transfer Windows & AI Offers**:

    * Enforces Summer (`July 1 – August 31`) and Winter (`January 1 – January 31`) transfer windows (pre-contract agreements allowed outside windows for players with $< 6\text{ months}$ remaining).

    * AI clubs evaluate positional weak spots and submit incoming transfer offers for user players via `TRANSFER` Inbox messages.

* **7D. Player Loyalty & Evolving Club Reputation**:

  * **Player Loyalty ($0\text{--}100$)**: Increases with consecutive seasons at the club, consistent starts, and trophies; decreases when benched below promised `Squad Role` or when transfer requests are blocked.

  * **Club Reputation ($1\text{--}100$)**: Scales dynamically with league finish, continental qualification, and trophy wins—governing which tier of players will enter contract talks.

* **7E. Exclusive $> 96$ Base OVR Playstyles System (`PlaystylesDescription.txt` Spec)**:

  * **1. Strict Eligibility Rule (Generational / Custom Icon Gate)**:

    * Playstyles are **exclusively** unlocked and active for senior players whose **initial base OVR at database import/creation satisfies** $\text{BaseOVR} > 96$ (i.e., $\text{BaseOVR} \in \{97, 98, 99\}$) **AND** `IsAcademyProduct == false`.

    * Players who start with $\text{BaseOVR} \le 96$ and later grow above $96$ OVR through Youth Academy development, weekly training XP, or seasonal progression **never** unlock Playstyles ($\text{BaseOVR} \le 96 \lor \text{IsAcademyProduct} = \text{true} \implies \text{Playstyles} = \emptyset$).

    * Designed specifically so custom high-OVR players added to the database (including the user's classmates in `11A7` and custom icons) stand apart with specialized on-pitch behaviors.

  * **2. Master 22-Playstyle Registry (Imported from `PlaystylesDescription.txt`)**:

| Category | Playstyle Name | Exact Behavioral Description (`PlaystylesDescription.txt`) | `MatchEngine` C++ Simulation Effect |
| :--- | :--- | :--- | :--- |
| **Finishing & Set Pieces** | **Finesse Expert** | Chooses curled shots more intelligently | Increases lateral Magnus curve spin and far-post corner aim bias when shooting from inside/edge of penalty area at angled approaches. |
| **Finishing & Set Pieces** | **Clinical Finisher** | Better shot selection and execution | Reduces shot dispersion variance and increases low-corner placement accuracy inside the box. |
| **Finishing & Set Pieces** | **Power Shot** | More likely to attempt powerful shots | Boosts long-range shot selection utility weight and increases maximum ball exit velocity on driven strikes. |
| **Finishing & Set Pieces** | **Chip Shot** | Recognises advancing goalkeeper | Triggers lofted chip trajectory selection when opponent `GK` advances beyond $6\text{ m}$ from the goal line during 1v1s. |
| **Finishing & Set Pieces** | **Penalty Expert** | Better penalty execution | Tightens penalty kick target variance and reduces composure penalty under high-pressure match states. |
| **Finishing & Set Pieces** | **Dead Ball** | Specialised free-kick execution | Increases direct free-kick curl, dip, and wall-clearance trajectory accuracy. |
| **Passing & Vision** | **Tiki Taka** | Prefers short combinations | Elevates first-time short triangle pass selection weight and reduces pass release latency. |
| **Passing & Vision** | **Bullet Pass** | Better driven pass execution | Increases ground pass velocity and narrows defender interception corridor width. |
| **Passing & Vision** | **Whipped Cross** | Uses fast, dangerous crosses | Applies high-pace early curl trajectory on crosses delivered from wide flank zones. |
| **Passing & Vision** | **Incisive Through Pass** | Detects passing lanes behind defence | Expands forward vision cone for ground through-balls splitting center-backs and full-backs. |
| **Passing & Vision** | **Driven Lob Through** | Recognises opportunities for lofted passes | Increases lofted over-the-top through-ball selection when opponent defensive line pushes high. |
| **Defending** | **Anticipate** | Better defensive positioning and interception decisions | Widens pass-lane read radius and accelerates interception step-in reaction time. |
| **Defending** | **Guardian** | More effective physical challenges | Increases clean standing tackle win probability and immediate ball-control retention. |
| **Defending** | **Slide Tackle** | More likely to use sliding challenges | Increases clean sliding tackle trigger utility when recovering laterally/from behind with reduced foul risk. |
| **Dribbling & Physical** | **Rapid** | Better sprint dribbling | Reduces ball-touch separation distance and speed loss while sprinting with possession. |
| **Dribbling & Physical** | **Trickster** | More frequent and effective skill moves | Increases 1v1 dribble beat probability and lateral evasion agility against pressing defenders. |
| **Dribbling & Physical** | **Bruiser** | Better shielding and physical duels | Increases shoulder-to-shoulder physical duel win rate and possession shielding radius. |
| **Dribbling & Physical** | **Accelerator** | More explosive off-ball runs | Boosts initial burst acceleration during forward off-ball runs into space. |
| **Dribbling & Physical** | **Relentless** | Maintains work rate and movement intensity | Reduces stamina-driven pressing and sprint work-rate decay across late match minutes. |
| **Aerial** | **Precision Header** | Better aerial execution | Improves jump timing and downward target accuracy on contested headers. |
| **Goalkeeping** | **Deflector** | Better save direction | Biases parry/deflection angles away from central rebound zones toward wide touchlines. |
| **Goalkeeping** | **Rush Out** | Better decisions when leaving the goal | Optimizes sweeper-keeper charge trigger distance and angle closing in 1v1 breakaways. |

  * **3. CSV, C++23 & UI Data Flow for Phase 7E**:

    * **CSV Assignment (`Playstyle 1`, `Playstyle 2`, `Playstyle 3` columns in `players_2006_with_price.csv`)**: Reads up to 3 Playstyle columns per row. During CSV import, if `BaseOVR > 96` and all 3 Playstyle columns are empty, `CareerState` assigns 2–3 signature Playstyles matching the player's `Position` and highest attributes; if `BaseOVR <= 96`, any Playstyle tags are stripped.

    * **C++ Bitmask & Serialization**: `Player` stores `uint32_t PlaystyleMask` + `std::vector<std::string> Playstyles`, serialized as `"playstyles"` in `<slot>.json` and passed inside `prepare_next_match_payload()` to `MatchEngine`.

    * **UI Display (`PlayerProfileModal.gd` & `SquadView.gd`)**: Renders a dedicated **PLAYSTYLES** badge section with exact descriptions from `PlaystylesDescription.txt` on `PlayerProfileModal` for eligible `BaseOVR > 96` players.

* **7F. Mandatory Age-37 Retirement & Squad Turnover Pipeline**:
  * **1. Universal Age-37 Retirement Rule (`src/mainplayer.h`, `Players_Database/player_database.cpp`, `src/CareerState.cpp`)**:
    * Every player in the database (CSV imports from `players_2006_with_price.csv`, custom classmates/icons, Youth Academy graduates, and fallback/generated players) has a fixed mandatory retirement age of **`37`** (`retirement_age = 37`), overriding any legacy `36` defaults or per-row CSV `RetirementAge` variance.
    * Players at `Age == 36` display a `"Retiring at End of Season"` status badge in `PlayerProfileModal` and `SquadView` and are blocked from incoming transfer bids during their final season.
  * **2. Contract Length Cap (`NegotiationModal` & `CareerState`)**:
    * Maximum contract duration during transfer negotiations and contract renewals is clamped to $\min(5, \max(1, 37 - \text{Age}))$ years so every player's contract terminates at or before age `37`.
  * **3. Annual Rollover Retirement Execution (`CareerState::_process_annual_rollover()` on July 1)**:
    * During the July 1 season boundary (`_month == 7 && _day == 1`), after incrementing every player's age (`Age + 1`), any player reaching **`Age >= 37`** immediately triggers retirement (`player->SetRetired(true)`):
      * **Roster Removal & Best-XI Rebalance**: Automatically removes all retired players from every club's `Starting XI`, `Bench`, and `Reserves` (`Team::GetRoster()`), reassigns captaincy (`_captain_by_club`) if the retiring player was captain, and runs positional Best-XI sorting (`auto_pick_best_xi`) to fill vacated starting spots.
      * **Wage Bill Relief**: Deducts retired players' weekly wages from `_finances.weekly_wage_bill`.
      * **Market & Match Exclusion**: Filters `IsRetired() == true` players out of `TransfersView` search queries, AI transfer activity, and `MatchEngine` matchday payloads while keeping their historical profile records in `_players_by_id` for career stats lookup.
      * **Inbox Retirement Report**: Dispatches a `"BOARD"` Inbox summary listing every retiring player at the managed club alongside their final OVR and career appearances/goals/assists.
  * **4. Save/Load Normalization (`save_career` / `load_career`)**:
    * Serializes `"retirement_age": 37` and `"is_retired"` inside `<slot>.json` and normalizes legacy saves to `retirement_age = 37` on load.

* **Phase 7 Acceptance Criteria**:

  * Filtering `TransfersView` by any combination of `Name`, `Club`, `Age`, `Nationality`, `League`, and `Position` returns exact matching active players from the merged database, respecting the `NHHA72528` unlock gate for `Tuesday League` and `11A7` and excluding retired players.

  * Completing a two-stage transfer deducts `Transfer Budget` + commits `Wage Budget`, transfers the `player_id` to the user `Team`, generates a `TRANSFER` Inbox receipt, and persists cleanly across Save/Load.

  * Players with `BaseOVR > 96` (non-academy) display their assigned Playstyles in `PlayerProfileModal`, execute Playstyle-weighted behaviors in `MatchEngine`, and academy graduates remain strictly excluded from Playstyles even if trained past $96$ OVR.

  * Advancing across the July 1 season rollover automatically retires every player reaching `Age >= 37`, removes them from active squads/market/match payloads, rebalances Starting XIs (preserving a `GK` in Slot `0`), and posts a `"BOARD"` Inbox retirement summary.

### Phase 8 — Full Career Acceptance Suite & Multi-Platform Release

* **End-to-End Automated Test Harness**: Expand `CareerRegressionTest.gd` to cover full-season simulation, `clubs.csv` `League -> Club` selection, `Club`/`League`/`Nationality` column loading, `NHHA72528` code redemption + anti-duplicate guard + `Tuesday League -> 11A7` unlock verification, GK 6-stat profile export + strict GK-only Slot `0` drag/swap lock, 6-criteria Transfer Search, transfer window round-trips, financial ledger invariants, $> 96$ Base OVR Playstyles gating, and mandatory Age-37 retirement during multi-season rollover.

* **Android & Aspect-Ratio Verification**: Validate touch targets ($\ge 44\text{dp}$), collapsible drawer ergonomics, modal bounds, and font legibility across `16:9`, `19.5:9`, and `20:9` viewports.

* **Release Packaging**: Strip debug overlays/test harnesses and build release GDExtension binaries (`target=template_release`) for Windows and Android.