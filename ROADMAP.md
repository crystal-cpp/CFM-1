# CFM-1 Project Roadmap & Complete Career Overhaul

**Repository / Organization**: `crystal-cpp` · **Engine**: Godot 4.7 (`api_version=4.7`) + C++23 GDExtension  
**Target Platforms**: Windows (MinGW/GCC 16.1.0 UCRT64) & Android (Touchscreen / Adaptive Aspect Ratios)  
**Visual Identity**: Dark-Navy (`#06090E` / `#0B101B`) & Pitch-Green (`#10AC84` / `#1DD1A1`) · Zero-3D UI Architecture  
**Current Milestone**: **Phase 4A & 4B Implemented -> Entering Phase 4C (Build, Automated Regression & Manual Verification)**

---

## 1. Master Phase Overview

| Phase | Milestone | Status | Primary C++ / GDScript Deliverables |
| :--- | :--- | :---: | :--- |
| **Phase 1** | C++23 `MatchEngine` Base Core | **Complete** | 22-player kinematic simulation, role attribute profiles, normalized `formations.csv` parser/validator, ball-carrier AI, `get_match_stats()`, skip-token economy. |
| **Phase 2A** | Chance-Gated Scoring, Direct Control & Physics | **Complete** | Chance-only scoring (`CHANCE_NONE` / `HOME_ATTACK` / `AWAY_ATTACK`), stamina & tactics physics coupling, direct control APIs (move, pass, through-ball, shot, switch, tackle). |
| **Phase 2B** | Dark-Navy Match UI, Gestures & In-Match Tactics | **Complete** | Unified desktop + Android gestures, Match HUD (scoreboard, 2D pitch, commentary strip), temporary full-screen in-match `TACTIC` screen with pause & XI preservation. |
| **Phase 2C** | Build & Match Regression Suite | **Complete** | SCons Windows UCRT64 compilation (`libfootball.windows.template_debug.x86_64.dll`) and headless/live match simulation regression suite. |
| **Phase 3** | Unified `CareerState`, Save/Load & Global Shell | **Complete** | Central C++ source of truth (`CareerState`), atomic JSON save/load, `CareerShell` 10-route router, `DashboardView`, `InboxView`, `PlayerProfileModal`, and `CareerRegressionTest` (Suites 1–8). |
| **Phase 4** | Team Management, Permanent Tactics & Youth UI | **In Progress (4C)** | **4A** C++ squad/tactics/youth bindings (`Complete`), **4B** procedural `SquadView`, `TacticsView`, `YouthView` (`Complete`), **4C** SCons build, regression Suites 9–12 & manual QA (`Active`). |
| **Phase 5** | Schedule, Competitions, Pre-Match & `club.csv` Merge | *Next Up* | **5A–5E** Calendar, read-only match modals, League Table, Pre-Match Center, Post-Match report pipeline; **5F** `res://data/club.csv` registry + (`Club` + `Nationality`) column merge in `players_2006_with_price.csv` + custom player roster integration. |
| **Phase 6** | Club Management, Finances & Board Rewards | *Scheduled* | Separated financial ledgers (Balance, Transfer Budget, Wage Budget), Board confidence & objectives tracker, stadium/facility upgrades, and competition prize payouts. |
| **Phase 7** | Dynamic Transfer Market, Contracts, Reputation & Playstyles | *Scheduled* | **7A–7E** Valuation engine, two-stage fee + wage negotiations, transfer windows, AI market transfers, loyalty & reputation; **7F** Exclusive $> 96$ Base OVR (non-academy) 22-Playstyle system. |
| **Phase 8** | Full Career Acceptance & Multi-Platform Release | *Scheduled* | Cross-platform desktop + Android verification, save/load integrity stress tests, performance profiling, and zero-3D release packaging. |

---

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
* **Universal Context-Aware Player Profile Modal (`demo/PlayerProfileModal.gd` / `demo/PlayerProfileModal.tscn`)**: Centered `580x500` overlay (`PRESET_CENTER` with symmetric offsets) supporting 4 operational modes (`SQUAD`, `YOUTH`, `TRANSFER`, `MATCH`), displaying attributes (`PAC`, `SHO`, `PAS`, `DRI`, `DEF`, `PHY`), styled `Condition`/`Morale` bars, context-adapted contract/youth rows, season stats, and mode-specific footer actions.

### Phase 3D — Build & Career Persistence Regression Suite `[COMPLETE]`
* Built `libfootball.windows.template_debug.x86_64.dll` via SCons and validated `CareerRegressionTest.gd` Suites 1–8 covering bootstrap determinism, atomic Save/Load round-trips, route normalization, modal centering, and inbox filtering.

---

## 3. Detailed Engineering Specification (Phases 4–8)

### Phase 4 — Team Management, Permanent Tactics & Youth Academy UI `[IN PROGRESS — PHASE 4C]`

#### Phase 4A — C++23 `CareerState` Squad, Tactics & Youth Academy Bindings `[COMPLETE]`
* **Squad Lineup & Validation Engine (`CareerState.h` / `CareerState.cpp`)**:
  * Manages `starting_xi` (11 slots mapped to active formation indices `0–10`), `bench` (7 slots), and `reserves` (`player_id` vectors).
  * `swap_squad_players(club_id, slot_a, slot_b)` atomically swaps any two roster slots (`XI <-> XI`, `XI <-> Bench`, `XI <-> Reserves`, `Bench <-> Reserves`) while enforcing single-GK rules (`slot 0` must remain a goalkeeper) and rejecting duplicate `player_id` assignments.
  * Evaluates positional familiarity for every Starting XI slot (`NATURAL` = $100\%$ effective OVR, `COMPATIBLE` = $-5\%$ OVR modifier, `OUT_OF_POSITION` = $-15\%$ OVR modifier) and exposes effective slot OVR to GDScript.
  * Exposes club role & kit number setters (`set_club_roles(club_id, roles_dict)` for Captain, Vice-Captain, Penalties, Free Kicks, Corners; `set_player_jersey_number(club_id, player_id, number)` enforcing unique `1–99` numbers across the club).
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
  * **Two-Click/Tap Swap & Fit Highlighting**: Selecting a player highlights valid swap targets with color-coded fit indicators (Pitch-Green `#1DD1A1` natural, Amber `#FECA57` compatible, Coral `#FF6B6B` out-of-position) and commits swaps directly to `CareerState`.
  * **Squad Roles, Kit Numbers & Inspection**: Inline selectors for Captain and Set-Piece takers, jersey number editor (`1–99`), and inspect button opening `PlayerProfileModal` in `"SQUAD"` mode.
* **2. Permanent Club Tactics View (`demo/TacticsView.gd` / `demo/TacticsView.tscn`)**:
  * **Formation Selector & Live 2D Shape Preview**: Dropdown/chip selector (`4-3-3`, `4-2-3-1`, `4-4-2`, `3-5-2`, `5-3-2`, etc.) dynamically repositioning 11 tactical markers on a 2D pitch preview and recalculating Starting XI positional fit.
  * **Tactical Sliders & Impact Readouts**: Controls for `Mentality` (`Ultra Defensive` -> `Ultra Attacking`), `Pressing Intensity` (`Low Block` -> `Gegenpress`), `Defensive Line` (`Deep` -> `High`), `Passing Style` (`Short` -> `Direct`), `Width` (`Narrow` -> `Wide`), and `Tempo` (`Slow` -> `Fast`) with live stamina-drain, defensive vulnerability, and build-up tempo tooltips.
* **3. Youth Academy View (`demo/YouthView.gd` / `demo/YouthView.tscn`)**:
  * **Academy Infrastructure Panel**: Displays Facility Level (`1–5`), Scouting Level (`1–5`), weekly upkeep cost, and balance-gated upgrade buttons (auto-disabled with shortfall tooltip when `club_balance < upgrade_cost`).
  * **Coaching Staff Panel**: Displays active `Technical`, `Physical`, `Tactical`, and `Goalkeeping` youth coaches and weekly development multipliers.
  * **Prospect Roster & Promotion Flow**: Lists academy prospects with `OVR`, `Potential Range`, `Weekly Dev Progress %`, `Condition`, and `Age`; supports inspecting via `PlayerProfileModal` (`"YOUTH"` mode) or direct promotion via `CareerState.promote_youth_player(player_id)`.
* **`CareerShell.gd` Router Wiring**: Replaced the `"squad"`, `"tactics"`, and `"youth"` placeholder cards with live instances of `SquadView`, `TacticsView`, and `YouthView`, synchronized with header balance/rank pills and `visibility_changed` hooks.

#### Phase 4C — Build, Automated Regression & Manual Verification `[CURRENT — ACTIVE]`
* **SCons GDExtension Compilation**: Recompile `libfootball.windows.template_debug.x86_64.dll` under MinGW/GCC 16.1.0 UCRT64 (`scons platform=windows target=template_debug api_version=4.7 -j6`) to expose all Phase 4A C++ methods to Godot 4.7.
* **Expanded Regression Suite (`demo/CareerRegressionTest.gd` Suites 9–12)**:
  * **Suite 9 (Squad Swaps & Lineup Rules)**: Verify `XI <-> Bench <-> Reserves` swaps, single-GK guard, duplicate-ID guard, positional OVR penalty math, captaincy/set-piece/jersey number persistence, and `MatchEngine` squad handoff.
  * **Suite 10 (Permanent Tactics & Scope Isolation)**: Verify formation and slider mutations in `TacticsView` persist across route switches and Save/Load without leaking temporary in-match `TACTIC` changes.
  * **Suite 11 (Youth Academy Lifecycle)**: Verify facility/scouting upgrades (balance deduction + insufficient-funds guard), coach assignment bonuses, weekly training XP ticks vs. annual rollover aging (`+1` age), and `promote_youth_player()` roster migration + Inbox dispatch.
  * **Suite 12 (Full Phase 4 Save/Load Round-Trip)**: Mutate Starting XI, jersey numbers, club tactics, youth upgrades, and promote a prospect -> `save_career()` -> `load_career()` -> assert bit-exact restoration.
* **Manual Cross-Viewport & Interactive QA**: Verify `SquadView`, `TacticsView`, and `YouthView` rendering, `"normal"` `Label` pill overrides, modal centering, and responsive stacking on desktop (`1280x720`) and narrow/mobile (`800x480` / portrait) viewports, and delete legacy `*_2` duplicate files.

---

### Phase 5 — Schedule, Competitions, Pre-Match Center & Deterministic Club Database `[NEXT UP]`
* **Target Files**: `demo/ScheduleView.gd` / `.tscn`, `demo/CompetitionsView.gd` / `.tscn`, `demo/PreMatchView.gd` / `.tscn`, `demo/PostMatchModal.gd` / `.tscn`, `demo/data/club.csv`, `Players_Database/players_2006_with_price.csv`, `src/mainplayer.h`, `src/team.h`, `Players_Database/player_database.h` / `.cpp`, `src/CareerState.h` / `.cpp`.
* **5A. Schedule & Fixtures (`ScheduleView`)**:
  * **Dual View Modes**: Monthly calendar grid (highlighting matchdays, transfer window dates, and training blocks) and chronological fixture list with filters (`ALL`, `UPCOMING`, `COMPLETED`, `HOME`, `AWAY`).
  * **Read-Only Match Detail Modal**: Clicking any completed fixture opens full scoreline, goalscorers/assists timeline, yellow/red cards, and `MatchEngine` possession/shot statistics; clicking upcoming fixtures previews venue and kickoff metadata.
* **5B. Competitions Center (`CompetitionsView`)**:
  * **Live League Table**: Full standings (`Pos`, `Club`, `P`, `W`, `D`, `L`, `GF`, `GA`, `GD`, `Pts`, `Last 5 Form`) grouped by league from `res://data/club.csv` with qualification/relegation zone color bands.
  * **Player Leaderboards & History**: Sortable Top Scorers, Top Assists, Clean Sheets, and Average Match Rating tables plus multi-season archive selector (`2026/27`, `2027/28`, ...).
* **5C. Pre-Match Center & Post-Match Pipeline (`PreMatchView` & `PostMatchModal`)**:
  * **Pre-Match Briefing**: Side-by-side club comparison (OVR, tactical formation/mentality tendencies, Last-5 form, head-to-head record) and Starting XI fitness/suspension readiness check (auto-warning if any starter has $< 65\%$ condition).
  * **Match Launch & Commit**: `START MATCH` packages user + AI `Team` squads and permanent tactics from Phase 4 into `MatchEngine`; upon full-time whistle, `CareerState.commit_match_result()` updates league standings, player stats (`appearances`, `goals`, `assists`), post-match stamina/condition drain, gate receipts, and displays `PostMatchModal` before returning to `CareerShell`.
* **5F. Deterministic `res://data/club.csv` Registry & `players_2006_with_price.csv` `Club` Column Merge `[MANDATORY DATA PIPELINE]`**:
  * **1. Canonical Club Registry (`demo/data/club.csv` -> `res://data/club.csv`)**:
    * Eliminates random club imports and hardcoded `DEFAULT_CLUB_NAMES[16]` arrays. Every club in the simulation must be defined explicitly in `res://data/club.csv` with the following header schema:
      ```csv
      ClubID,ClubName,ShortCode,League,Country,Reputation,TotalBalance,TransferBudget,WageBudget,DefaultFormation,PrimaryColor,SecondaryColor
      ```
    * `CareerState` parses `res://data/club.csv` during `start_new_career()`, validating unique positive `ClubID` integers, non-empty `ClubName` keys, valid `DefaultFormation` IDs from `formations.csv`, and exact league groupings. Any club not present in `res://data/club.csv` is **never** randomly generated.
  * **2. Merging the `Club` Column in `players_2006_with_price.csv` (and `players.csv`)**:
    * Standardizes the player CSV header schema to include `Club` (plus optional `Playstyles` for Phase 7F):
      ```csv
      Name,Position,Age,Potential,DebutAge,RetirementAge,Pace,Shooting,Passing,Dribbling,Defending,Physical,Price,Club,League,Nationality,Playstyle 1,Playstyle 2,Playstyle 3
      ```
    * **`src/mainplayer.h` / `src/mainplayer.cpp`**: Add `std::string ClubName = "Free Agent"`, `int BaseOVR = 0`, and `bool IsAcademyProduct = false` with getters/setters (`GetClubName()`, `SetClubName()`, `GetBaseOVR()`, `SetBaseOVR()`, `GetIsAcademyProduct()`, `SetIsAcademyProduct()`).
    * **`Players_Database/player_database.h` / `Players_Database/player_database.cpp`**:
      * Update `load_from_csv()` to resolve the `"club"` (case-insensitive) header column index dynamically, strip Windows `\r` and leading/trailing whitespace, and store `player.SetClubName(...)` and `player.SetBaseOVR(player.GetOVR())`.
      * Add `load_clubs_from_csv(const std::string& path)` to parse `res://data/club.csv` into structured `ClubDefinition` records.
    * **`src/CareerState.h` / `src/CareerState.cpp`**:
      * In `start_new_career()`, instantiate `Team` instances strictly from `res://data/club.csv`, then route every player from `players_2006_with_price.csv` into their exact matching `Team` by normalized `ClubName` (supporting clean alias matching such as `FC Barcelona` -> `Barcelona`).
      * Players whose `Club` value is empty, `"Free Agent"`, or does not match an entry in `res://data/club.csv` are placed into the **Free Agent** pool (`club_id = 0`) rather than spawning unconfigured clubs.
      * Run positional Best-XI sorting on every club roster after CSV import so `GK` always occupies Slot `0` and outfielders fill natural roles.
      * Persist `"csv_club_name"`, `"base_ovr"`, `"is_academy_product"`, `"league"`, `"country"`, and `"short_code"` inside `save_career()` / `load_career()` JSON snapshots.
      * Do the same for `Nationality` and `League` columns.
    * **GDScript UI Synchronization**:
      * `CareerShell.gd` reads `ShortCode` and `PrimaryColor` from `get_club_info(club_id)` for the top-bar club crest badge.
      * `DashboardView.gd`, `SquadView.gd`, `CompetitionsView.gd`, `TransfersView.gd`, and `PlayerProfileModal.gd` display exact `club.csv` club names, leagues, and Free Agent tags.
  * **3. Custom Player & Classmate Roster Addition Workflow**:
    * Supports adding custom real-world legends and user-created players (such as the user's classmates) directly into `Players_Database/players_2006_with_price.csv` (and synced to `demo/data/players_2006_with_price.csv`):
      * Assign each custom player's `Club` column to an exact `ClubName` from `res://data/club.csv` so they load deterministically onto that club's roster on career start.
      * Custom players authored with base attributes yielding $\text{BaseOVR} > 96$ (`97`, `98`, or `99` OVR) automatically qualify for **Phase 7F Playstyles** (see Section 3, Phase 7F below).
* **Phase 5 Acceptance Criteria**:
  * Playing or simulating a match updates the League Table, player season stats, club gate-receipt balance, and advances the Next Match Hero Card to the subsequent matchday.
  * Zero random clubs are created at career start; 100% of clubs originate from `res://data/club.csv` and 100% of `Club`-tagged players in `players_2006_with_price.csv` land in their assigned clubs.

---

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

---

### Phase 7 — Dynamic Transfer Market, Contracts, Loyalty, Reputation & Exclusive Playstyles
* **Target Files**: `demo/TransfersView.gd` / `.tscn`, `demo/NegotiationModal.gd` / `.tscn`, `demo/PlayerProfileModal.gd`, `src/mainplayer.h` / `.cpp`, `src/MatchEngine.h` / `.cpp`, `src/CareerState.h` / `.cpp`.
* **7A. Scalable Player Valuation Engine**:
  * Computes dynamic market value from:
    $$\text{Value} = \text{BaseOVR}(\text{OVR}) \times \text{AgeCurve}(\text{Age}, \text{Potential}) \times \text{PosMult}(\text{Pos}) \times \text{ContractMult}(\text{YearsLeft}) \times \text{RoleMult}(\text{SquadRole})$$
  * Accounts for expiring contracts ($\le 1\text{ year}$ discount), high-potential U21 wonderkid premiums, and selling club financial pressure.
* **7B. Two-Stage Transfer & Contract Negotiations (`NegotiationModal`)**:
  * **Stage 1 — Club-to-Club Fee Negotiation**: Submit initial transfer fee (plus optional sell-on clause); AI club evaluates against player valuation and squad depth, responding with `Accept`, `Counteroffer` (up to 3 rounds), or `Reject`.
  * **Stage 2 — Player Personal Terms**: Upon fee agreement, negotiate `Weekly Wage`, `Contract Length (1–5 yrs)`, and `Squad Role` (`Crucial`, `Important`, `Rotation`, `Prospect`).
* **7C. Transfer Windows & AI Market Activity**:
  * Enforces Summer (`July 1 – August 31`) and Winter (`January 1 – January 31`) transfer windows (pre-contract agreements allowed outside windows for players with $< 6\text{ months}$ remaining).
  * AI clubs evaluate positional weak spots and submit incoming transfer offers for user players via `TRANSFER` Inbox messages.
* **7D. Player Loyalty & Evolving Club Reputation**:
  * **Player Loyalty ($0\text{--}100$)**: Increases with consecutive seasons at the club, consistent starts, and trophies; decreases when benched below promised `Squad Role` or when transfer requests are blocked.
  * **Club Reputation ($1\text{--}100$)**: Scales dynamically with league finish, continental qualification, and trophy wins—governing which tier of players will enter contract talks.
* **7F. Exclusive $> 96$ Base OVR Playstyles System (`PlaystylesDescription.txt` Spec)**:
  * **1. Strict Eligibility Rule (Generational / Custom Icon Gate)**:
    * Playstyles are **exclusively** unlocked and active for senior players whose **initial base OVR at database import/creation satisfies $\text{BaseOVR} > 96$** (i.e., $\text{BaseOVR} \in \{97, 98, 99\}$) **AND** `IsAcademyProduct == false`.
    * Players who start with $\text{BaseOVR} \le 96$ and later grow above $96$ OVR through Youth Academy development, weekly training XP, or seasonal progression **never** unlock Playstyles ($\text{BaseOVR} \le 96 \lor \text{IsAcademyProduct} = \text{true} \implies \text{Playstyles} = \emptyset$).
    * Designed specifically so custom high-OVR players added to the database (including the user's classmates and custom icons) stand apart with specialized on-pitch behaviors.
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

  * **3. CSV, C++23 & UI Data Flow for Phase 7F**:
    * **CSV Assignment (`Playstyles` column in `players_2006_with_price.csv`)**: Supports pipe-separated Playstyle names (e.g., `Finesse Expert|Tiki Taka|Trickster`). During CSV import, if `BaseOVR > 96` and `Playstyles` is empty, `CareerState` assigns 2–3 signature Playstyles matching the player's `Position` and highest attributes; if `BaseOVR <= 96`, any Playstyle tags are stripped.
    * **C++ Bitmask & Serialization**: `Player` stores `uint32_t PlaystyleMask` + `std::vector<std::string> Playstyles`, serialized as `"playstyles"` in `<slot>.json` and passed inside `prepare_next_match_payload()` to `MatchEngine`.
    * **UI Display (`PlayerProfileModal.gd` & `SquadView.gd`)**: Renders a dedicated **PLAYSTYLES** badge section with exact descriptions from `PlaystylesDescription.txt` on `PlayerProfileModal` for eligible `BaseOVR > 96` players.
* **Phase 7 Acceptance Criteria**:
  * Completing a two-stage transfer deducts `Transfer Budget` + commits `Wage Budget`, transfers the `player_id` to the user `Team`, generates a `TRANSFER` Inbox receipt, and persists cleanly across Save/Load.
  * Players with `BaseOVR > 96` (non-academy) display their assigned Playstyles in `PlayerProfileModal`, execute Playstyle-weighted behaviors in `MatchEngine`, and academy graduates remain strictly excluded from Playstyles even if trained past $96$ OVR.

---

### Phase 8 — Full Career Acceptance Suite & Multi-Platform Release
* **End-to-End Automated Test Harness**: Expand `CareerRegressionTest.gd` to cover full-season simulation, `club.csv` + `Club` column loading, transfer window round-trips, financial ledger invariants, $> 96$ Base OVR Playstyles gating, and multi-season rollover.
* **Android & Aspect-Ratio Verification**: Validate touch targets ($\ge 44\text{dp}$), collapsible drawer ergonomics, modal bounds, and font legibility across `16:9`, `19.5:9`, and `20:9` viewports.
* **Release Packaging**: Strip debug overlays/test harnesses and build release GDExtension binaries (`target=template_release`) for Windows and Android.