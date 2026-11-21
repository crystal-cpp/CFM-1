# CFM-1 Project Roadmap & Complete Career Overhaul

**Repository / Organization**: `crystal-cpp` · **Engine**: Godot 4.7 (`api_version=4.7`) + C++23 GDExtension  
**Target Platforms**: Windows (MinGW/GCC 16.1.0 UCRT64) & Android (Touchscreen / Adaptive Aspect Ratios)  
**Visual Identity**: Dark-Navy (`#06090E` / `#0B101B`) & Pitch-Green (`#10AC84` / `#1DD1A1`) · Zero-3D UI Architecture · Crisp Football-Management Typography  
**Current Milestone**: **Phase 5E is approximately 50% complete by source-implementation scope (an engineering progress estimate, not a QA pass). Latest reviewed source commit: `52729abc3b335a757a85a72d35f5460d1d911806`. Implemented source slices include stamina-attribute drain and end-match telemetry/report handoff, attribute/OVR-weighted attacking chances, individual attribute influence in tackles/duels/interceptions/shot blocks, OVR-scaled defensive AI, smoother steering, goalkeeper goal-corridor handling, fitness-aware Starting XI selection, and HUD refresh hooks for squad/substitution events. Commit `52729a` also removes the stray C++ token from `_compute_tactical_target`. Important current blocker: `src/CareerState.h` now declares `MatchEngine` rather than `CareerState`, while `CareerState.cpp` and `register_types.cpp` still include/use `CareerState`; restore the correct CareerState declaration before treating the current source as buildable. The tactical screen still initializes a placeholder bench and has no `sync_squad_from_engine()` implementation, so live bench/substitution data synchronization is not complete. The primary visible Skip button remains; only the duplicate hidden control was removed. `KEY_R` is the post-match Next Match shortcut, not Advanced Dribbling. Comparative simulation regressions, a clean current-source SCons build, and Godot runtime QA remain outstanding. Phase 5D source implementation is complete, but its build/regression/runtime acceptance and 15 July 2006 historical-data audit remain outstanding.**

---

## 1. Master Phase Overview

| Phase | Milestone | Status | Primary C++ / GDScript Deliverables |
| :--- | :--- | :---: | :--- |
| **Phase 1** | C++23 `MatchEngine` Base Core | **Complete** | 22-player kinematic simulation, role attribute profiles, normalized `formations.csv` parser/validator, ball-carrier AI, `get_match_stats()`, skip-token economy. |
| **Phase 2A** | Chance-Gated Scoring, Direct Control & Physics | **Complete** | Chance-only scoring (`CHANCE_NONE` / `HOME_ATTACK` / `AWAY_ATTACK`), stamina & tactics physics coupling, direct control APIs (move, pass, through-ball, shot, switch, tackle). |
| **Phase 2B** | Dark-Navy Match UI, Gestures & In-Match Tactics | **Complete** | Unified desktop + Android gestures, Match HUD (scoreboard, 2D pitch, commentary strip), temporary full-screen in-match `TACTIC` screen with pause & XI preservation. |
| **Phase 2C** | Build & Match Regression Suite | **Complete** | SCons Windows UCRT64 compilation (`libfootball.windows.template_debug.x86_64.dll`) and headless/live match simulation regression suite. |
| **Phase 3** | Unified `CareerState`, Save/Load & Global Shell | **Complete** | Central C++ source of truth (`CareerState`), atomic JSON save/load, `CareerShell` route router, `DashboardView`, `InboxView`, `PlayerProfileModal`, and `CareerRegressionTest` (Suites 1–8). |
| **Phase 4** | Team Management, Permanent Tactics & Youth UI | **Implemented; acceptance verification outstanding** | **4A** C++ squad/tactics/youth bindings, GK 6-stat export & strict GK-only Slot `0` guard (`Complete`), **4B** procedural `SquadView` (drag-and-drop + click-swap GK lock), `TacticsView`, `YouthView`, `PlayerProfileModal` GK stats (`Complete`), **4C** SCons build, regression Suites 9–12 & manual QA (`Active`). |
| **Phase 5** | Schedule, Competitions, Historical 2006 Club DB, Stamina/Injuries, Match Realism & Audio | **In Progress (5D source implementation complete; 5E ~50% implemented)** | **5A–5C** Calendar, competitions integration, Pre-Match Center, Post-Match pipeline and startup flow (runtime QA pending); **5D** club/player data, canonical ClubRecord and CSV ClubID bootstrap, league-isolated fixtures/standings, bye-aware scheduling, career regression coverage, stadium metadata, formation-aware XI selection, hidden youth-prospect handling and match club/scorer-name integration (source implemented; build/regression/runtime and historical-data acceptance pending); `Tuesday League` / `11A7` lock and `NHHA72528`; **5E — APPROXIMATELY 50% SOURCE IMPLEMENTED** stamina drain/telemetry/report handoff, attribute/OVR-weighted attacking chances, partial defender attribute coupling, OVR-scaled defensive AI, steering, goalkeeper corridor handling, fitness-aware XI selection and HUD refresh hooks; remaining work includes dedicated clearances and broader defender-role coupling, real match bench/substitution synchronization, resolving the current `CareerState.h` declaration regression, defensive balance, comparative simulation, clean build and Godot QA; **5F** post-match stamina persistence, configurable daily recovery and injury system; **5G** menu music, stadium atmosphere, contextual commentary, goal reactions/celebrations and full-time audio. |
| **Phase 6** | Club Management, Finances, Senior Training Tab & Design System | *Scheduled* | **6A** Separated financial ledgers (Balance, Transfer Budget, Wage Budget), Board confidence & objectives tracker, stadium/facility upgrades; **6B** Dedicated Senior **Training** tab (`TrainingView`) with 4 positional group programs (Attackers, Midfielders, Defenders, Goalkeepers) and potential/age-aware OVR growth; **6C** Global UI & Typography Design System overhaul (custom fonts, high-clarity player cards, non-AI-slop disciplined aesthetic). |
| **Phase 7** | Dynamic Transfer Market, Search, Reputation & Playstyles | *Scheduled* | **7A–7D** Valuation engine, two-stage fee + wage negotiations, transfer windows, 6-field Transfer Search (`Name`, `Club`, `Age`, `Nationality`, `League`, `Position`) with `Tuesday League` / `11A7` lock gating, AI market transfers, loyalty & reputation; **7E** Exclusive $> 96$ Base OVR (non-academy) 22-Playstyle system; **7F** Mandatory Age-37 retirement & squad turnover pipeline. |
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
  * Persistent Top Header Bar: Club 3-letter badge, club name, season pill (starting `2006/07` from **15 July 2006** from the CSV-backed 15 July 2006 career bootstrap (historical data audit pending)), ISO date pill, live league rank pill, formatted club balance pill, `+1 Day` progression button, `Save` button, unread Inbox badge counter, and `Manager` profile modal trigger.
  * Persistent Route Router: Caches view nodes inside `_router_container` and toggles `.visible` (`dashboard`, `squad`, `tactics`, `training` [Phase 6B], `youth`, `schedule`, `competitions`, `transfers`, `club`, `finances`, `inbox`) with `"board"` normalized to `"club"`.
  * Adaptive Layout & Shortcuts: Auto-collapses sidebar drawer below `780px` viewport width toggle, supports `KEY_C` overlay toggle and `KEY_ESCAPE` modal dismissal.

### Phase 3C — Dashboard, Event-Driven Inbox & Universal Player Modal `[COMPLETE]`
* **Career Dashboard (`demo/DashboardView.gd` / `demo/DashboardView.tscn`)**: Auto-refreshes on `_ready()` and `visibility_changed`, featuring the Next Match Hero Card (with opponent rank & W-D-L record), Club Overview header strip + Top-5 standings table, Squad Condition summary, active Board Objectives, latest 3 Inbox alerts, and Quick Action navigation buttons.
* **Event-Driven News / Inbox (`demo/InboxView.gd` / `demo/InboxView.tscn`)**: Category filter chips (`ALL`, `MATCH`, `TRANSFER`, `BOARD`, `YOUTH`, `INJURY`) with active pitch-green highlight, filter reset, live read/unread state sync (`mark_inbox_read` / `mark_all_inbox_read`), and route/entity deep-links (`squad`, `youth`, `transfers`, `club`).
* **Universal Context-Aware Player Profile Modal (`demo/PlayerProfileModal.gd` / `demo/PlayerProfileModal.tscn`)**: Centered `580x500` overlay (`PRESET_CENTER` with symmetric offsets) supporting 4 operational modes (`SQUAD`, `YOUTH`, `TRANSFER`, `MATCH`), displaying outfield attributes (`PAC`, `SHO`, `PAS`, `DRI`, `DEF`, `PHY`) or Goalkeeper attributes (`Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, `Physical`), styled `Condition`/`Morale` bars, context-adapted contract/youth rows, season stats, and mode-specific footer actions.

### Phase 3D — Build & Career Persistence Regression Suite `[COMPLETE]`
* Built `libfootball.windows.template_debug.x86_64.dll` via SCons and validated `CareerRegressionTest.gd` Suites 1–8 covering bootstrap determinism, atomic Save/Load round-trips, route normalization, modal centering, and inbox filtering.

---

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
  * `PlayerAttributes` exposes semantic Goalkeeper getters mapped onto the 6 core attribute slots (`Pace -> Diving`, `Shooting -> Reflex`, `Passing -> Positioning`, `Dribbling -> Reaction`, `Defending -> Kicking`, `Physical -> Physical`) so `res://data/players_2006_with_price.csv` stays compatible without extra stat columns.
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
  * Automatically detects `is_gk` / `position == "GK"` and switches the 6 attribute bars and labels from outfield stats (`Pace`, `Shooting`, `Passing`, `Dribbling`, `Defending`, `Physical`) to Goalkeeper stats: `Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, and `Physical`.
* **3. Permanent Club Tactics View (`demo/TacticsView.gd` / `demo/TacticsView.tscn`)**:
  * **Formation Selector & Live 2D Shape Preview**: Dropdown/chip selector (`4-3-3`, `4-2-3-1`, `4-4-2`, `3-5-2`, `5-3-2`, etc.) dynamically repositioning 11 tactical markers on a 2D pitch preview and recalculating Starting XI positional fit.
  * **Tactical Sliders & Impact Readouts**: Controls for `Mentality`, `Pressing Intensity`, `Defensive Line`, `Passing Style`, `Width`, and `Tempo` with live stamina-drain, defensive vulnerability, and build-up tempo tooltips.
* **4. Youth Academy View (`demo/YouthView.gd` / `demo/YouthView.tscn`)**:
  * **Academy Infrastructure Panel**: Displays Facility Level (`1–5`), Scouting Level (`1–5`), weekly upkeep cost, and balance-gated upgrade buttons (auto-disabled with shortfall tooltip when `club_balance < upgrade_cost`).
  * **Coaching Staff Panel**: Displays active `Technical`, `Physical`, `Tactical`, and `Goalkeeping` youth coaches and weekly development multipliers.
  * **Prospect Roster & Promotion Flow**: Lists academy prospects with `OVR`, `Potential Range`, `Weekly Dev Progress %`, `Condition`, and `Age`; supports inspecting via `PlayerProfileModal` (`"YOUTH"` mode) or direct promotion via `CareerState.promote_youth_player(player_id)`.
* **`CareerShell.gd` Router Wiring**: Replaced the `"squad"`, `"tactics"`, and `"youth"` placeholder cards with live instances of `SquadView`, `TacticsView`, and `YouthView`, synchronized with header balance/rank pills and `visibility_changed` hooks.

#### Phase 4C — Build, Automated Regression & Manual Verification `[CURRENT — ACTIVE]`
* **SCons GDExtension Compilation**: Recompile `libfootball.windows.template_debug.x86_64.dll` under MinGW/GCC 16.1.0 UCRT64 (`scons platform=windows target=template_debug api_version=4.7 -j6`) to expose all Phase 4A C++ methods and the `4aa3a2a4b227006758f2421c1042657c921b3fe6` metadata loader to Godot 4.7.
* **Expanded Regression Suite (`demo/CareerRegressionTest.gd` Suites 9–12)**:
  * **Suite 9 (Squad Swaps, GK-Only Slot Lock & GK 6-Stat Profile)**: Verify `XI <-> Bench <-> Reserves` swaps, strict rejection of non-`GK` drag/swap into Slot `0` (`GK`) and `GK` into Slots `1–10`, `GK <-> GK` swap allowance, `is_gk` 6-stat dictionary keys (`diving`, `reflex`, `positioning`, `reaction`, `kicking`, `physical`), positional OVR penalty math, captaincy/set-piece/jersey number persistence, and `MatchEngine` squad handoff.
  * **Suite 10 (Permanent Tactics & Scope Isolation)**: Verify formation and slider mutations in `TacticsView` persist across route switches and Save/Load without leaking temporary in-match `TACTIC` changes.
  * **Suite 11 (Youth Academy Lifecycle)**: Verify facility/scouting upgrades (balance deduction + insufficient-funds guard), coach assignment bonuses, weekly training XP ticks vs. annual rollover aging (`+1` age), and `promote_youth_player()` roster migration + Inbox dispatch.
  * **Suite 12 (Full Phase 4 Save/Load Round-Trip)**: Mutate Starting XI, jersey numbers, club tactics, youth upgrades, and promote a prospect -> `save_career()` -> `load_career()` -> assert bit-exact restoration.
* **Manual Cross-Viewport & Interactive QA**: Verify `SquadView` (including drag-and-drop GK lock warning), `TacticsView`, `YouthView`, and `PlayerProfileModal` GK 6-stat display, `"normal"` `Label` pill overrides, modal centering, and responsive stacking on desktop (`1280x720`) and narrow/mobile (`800x480` / portrait) viewports, and delete legacy `*_2` duplicate files.

---

### Phase 5 — Schedule, Competitions, Historical 15 July 2006 Club DB, Stamina/Injuries, Match Realism & Audio `[IN PROGRESS — 5E PARTIALLY IMPLEMENTED; QA PENDING]`
* **Target Files**: `demo/ScheduleView.gd` / `.tscn`, `demo/CompetitionView.gd`, `demo/PreMatchView.gd`, `demo/PostMatchModel.gd` (class `PostMatchModal`), `demo/NewCareerView.gd` / `.tscn`, `demo/RedeemCodeModal.gd` / `.tscn`, `demo/CareerShell.gd`, `demo/MatchHud.gd` (or active match presentation view), `demo/AudioManager.gd`, `demo/assets/audio/`, `demo/data/clubs.csv`, `demo/data/players_2006_with_price.csv`, `src/mainplayer.h` / `.cpp`, `src/team.h` / `.cpp`, `Players_Database/player_database.h` / `.cpp`, `src/MatchEngine.h` / `.cpp`, `src/CareerState.h` / `.cpp`.

* **5A. Schedule & Fixtures (`ScheduleView`)**:
  * **Dual View Modes**: Monthly calendar grid (highlighting matchdays, transfer window dates, recovery windows, and training blocks) and chronological fixture list with filters (`ALL`, `UPCOMING`, `COMPLETED`, `HOME`, `AWAY`).
  * **Read-Only Match Detail Modal**: Clicking any completed fixture opens full scoreline, goalscorers/assists timeline, yellow/red cards, injuries sustained, and `MatchEngine` possession/shot statistics; clicking upcoming fixtures previews venue and kickoff metadata.
  * **Annual Season Calendar Lifecycle (`July 1 – May 31`)**:
    * Every career season starts on **July 1**, opening with a **2-week Pre-Season Window (Weeks 1–2 in July)** featuring scheduled pre-season friendlies (`is_friendly = true`, non-league points).
    * Competitive domestic league fixtures kick off on **Week 3 (Mid-August, `Aug 15`)** and run through **May (`May 23` final matchday / `May 31` end-of-season review & trophy ceremony)**, followed by a June summer break before the July 1 rollover.
  * **Tabbed Fixture Browser**: `My Club Fixtures | League Matchdays | Results Archive` with live round/outcome filters and read-only post-match reports.

* **5B. Competitions Center (`CompetitionsView`)**:
  * **5B-1 — Domestic Competition Foundation**: Domestic league competition and standings are the initial scope.
  * **5B-2 — Competitions UI / Live Refresh Integration**: `demo/CompetitionView.gd` (`class_name CompetitionsView`) provides the competitions center UI (domestic league table, UCL presentation tab, player statistics and prize information). `CareerShell.gd` refresh wiring is implemented in all three required paths: opening the `competitions` route in `navigate_to()`, global refresh in `refresh_all_views()`, and standings-change handling in `_on_standings_updated()`. The wiring was committed in `d43ba52e4a7d2295790b0c3f1bb2f8827a974385`. Follow-up `e2f9351` makes `CompetitionView.gd` read the active `CareerState` club registry, hardens normalized CSV fallback parsing, and adapts the no-argument C++ standings API to the view's expected row fields. Commit `626f2d5` fixes the initial league selection: after CareerState is injected, the table now selects the managed club's actual league, uses its live standings, preserves deliberate dropdown changes, and resets correctly when switching to a career in another league. Runtime confirmation is still pending. ScheduleView responsive layout work is committed in `018eef5e4d73a340c7714c164ca69104f5ce95b6`. Source inspection confirms the hooks exist; runtime behavior and desktop/narrow viewport presentation still require Godot QA.
  * **Deferred — UEFA Champions League Fixtures**: The UCL tab/presentation shell is present, but authoritative UCL fixture generation, qualification/participant selection, knockout progression, result integration and prize payout logic remain deferred to a later 5B milestone. Do not treat the visual tab or placeholder data as completed UCL fixtures.

* **5C. Pre-Match Center & Post-Match Pipeline** (`PreMatchView` & `PostMatchModal`) — **INTEGRATION COMMITTED; RUNTIME QA PENDING**:
   * **Pre-Match Routing**: Dashboard / Schedule launch requests now open `PreMatchView` through `MatchScene.open_pre_match_center()`, using the next unplayed fixture from `CareerState`. Back navigation returns to the CareerShell.
   * **Match Launch**: `MatchScene.launch_career_match(career_state, context)` applies the prepared context through `CareerState.apply_context_to_match_engine()` immediately before `match_engine.start_match()`. Selected home/away primary kit colors are applied to the 2D outfield player markers; goalkeeper markers retain role-specific colors.
   * **Final Whistle & Report**: `MatchScene` retains the existing chance-state final-whistle deferral, then commits the career result once through `CareerState.commit_match_result(match_engine)` and calls `PostMatchModal.show_report(summary)`. Continuing from the report returns to the CareerShell and refreshes career views. Verify the complete flow, data mapping, no-double-commit behavior, kit colors, report content, and viewport behavior in Godot; source commits are not runtime QA.
* **5C-2. Main Menu, Application Startup & Career Entry Flow `[IMPLEMENTED IN SOURCE; RUNTIME QA PENDING]**:
  * **Project Entry Point**: Pressing **F5 / Run Project** opens a dedicated `MainMenu` screen (not the match pitch and not a bare `CareerShell`). The menu is the normal first screen for a fresh launch.
  * **Main Menu Actions**: Provide **New Career**, **Continue Career**, **Load Career**, **Settings**, and **Exit**. Continue is enabled only when a valid continue target exists; otherwise show a clear empty-state explanation. Load Career lists available save slots with useful metadata and handles corrupt/incompatible saves without crashing.
  * **New Career Routing**: New Career opens the dedicated setup wizard. The menu/startup milestone defines the navigation and lifecycle; Phase 5D supplies the authoritative historical 15 July 2006 league/club data and completes the data-backed `League -> Club -> Manager Name` setup.
  * **Automatic Career Entry**: After a new career is successfully created or an existing save is loaded, transition directly to the `CareerShell` Dashboard. Users must not need to press `C` to reveal the primary career interface. The existing `KEY_C` shortcut may remain only as a secondary overlay/debug/navigation convenience.
  * **Persistent App / Match Coordination**: Introduce a stable application-level root or scene coordinator that owns startup/menu/career/match transitions. Keep the match-launch integration reachable from CareerShell so Dashboard/Schedule -> Pre-Match -> Match -> Post-Match -> CareerShell works regardless of which screen is visible. Do not rely on `CareerShell` being launched as a standalone scene, a fragile direct-parent assumption, or an unverified global group lookup to find the active `MatchScene`; use an explicit coordinator/reference or a clearly owned persistent scene lifecycle.
  * **Return & Failure Paths**: Returning from a match report restores the active CareerShell and refreshed Dashboard; returning to the main menu must not silently discard unsaved progress. Missing saves, failed new-career initialization, and failed scene transitions must show actionable feedback.
This milestone's startup shell, routing, save-slot entry points and scene ownership are implemented in source; runtime QA remains pending. Phase 5D continues the club data schema/audit and multi-league bootstrap work; music and audio remain Phase 5G.
  * **Acceptance / Runtime QA**: Test F5 startup with no saves, Continue with/without a valid save, Load Career, New Career, automatic Dashboard entry, match launch from Dashboard and Schedule, PostMatchModal return, and viewport resizing. Verify the main scene in `project.godot` points to the new application entry scene. Do not mark complete based on source inspection alone.
* **5D. Historical 15 July 2006 Club Registry, Structured Player Data, New Career Bootstrap & League Fixtures**:
  * **5D-1 — Club Registry & Player Metadata**:
    * Current source-level CSV audit: demo/data/clubs.csv contains **156 data rows**, **13 columns** (including Stadium), **28 league labels**, **156 unique ClubIDs** and no duplicate club names. There are **12 single-club leagues**; these remain isolated rather than borrowing teams from other competitions.
    * demo/data/players_2006_with_price.csv contains **1,716 player rows**. Every player Club value maps to a registry club, and all **156 clubs have at least one imported player row**. This confirms dataset coverage, not historical accuracy.
    * The club schema preserves ClubID, Club, League, ShortCode, Country, Reputation, TotalBalance, TransferBudget, WageBudget, DefaultFormation, PrimaryColor, SecondaryColor and Stadium. Player metadata includes IDs, age, nationality, club/league, outfield and goalkeeper attributes, price, wage, Stamina and Playstyle fields. Do not duplicate the existing Stamina column.
    * NHHA72528 persistence, Tuesday League / 11A7 gating and RedeemCodeModal are present in source; reward mapping still requires QA.
  * **5D-2 — Structured Bootstrap & League-Scoped Fixtures [SOURCE IMPLEMENTATION COMPLETE; ACCEPTANCE QA PENDING]**:
    * **Canonical club data integration:** CareerState consumes the canonical ClubRecord dictionary contract (club, reputation, total_balance, transfer_budget, wage_budget, default_formation, primary_color, secondary_color) and maintains a normalized CSV ClubID mapping. New Career UI accepts the canonical club-name fields without fabricating sequential IDs.
    * **Team metadata and finance access:** complete club metadata is propagated into Team/career save data. CareerState reads the public Team::GetBudget() accessor rather than accessing private TransferBudget directly.
    * **League-scoped competition generation:** fixtures and standings are derived from the selected club's league; single-club leagues do not borrow teams from another league. Odd-sized league scheduling uses bye-aware round-robin handling, and pre-season pairings avoid scheduling a club twice in the same round.
    * **CareerState source cleanup:** removed the orphaned duplicate career-start block and duplicate get_header_summary() / get_club_info() definitions that caused C++ global-scope and redefinition errors.
    * **New Career league-registry fix:** normalized CSV header matching tolerates spaces, underscores and hyphens; an empty club registry no longer reports a successful load. NewCareerView logs playable league names/count at startup.
    * **Latest match/bootstrap integration (`0ea259c`):** ClubRecord now carries stadium metadata into the canonical club dictionary; CareerState adds formation-aware Starting XI selection and hidden youth-prospect evaluation/age progression support; MatchEngine lineup setters initialize safely and consume non-empty player names; MatchHUD and MatchScene display the selected home/away club names and scorer attribution. PreMatchView now loads available formations and synchronizes its match context with CareerState. These are source-level changes and still require runtime validation.
  * **Regression suite additions:** Phase 5D checks cover the current 156-club / 28-league registry, CSV ClubID 1098 for 11A7, 21-team La Liga scheduling, no double-booking in the tested odd-sized league, and single-club Allsvenskan isolation. A duplicate GDScript declaration in CareerRegressionTest.gd was removed in commit c5cd7b6c44eda90581ad91b8c44363c6dd62fab6; Godot must still reload and execute the suite.
    * **Relevant source-fix commits:** 0c25cc8, 97bd023, 40fb353, 760aeaf, f363c28, 5504b33, 2204a80, and c5cd7b6. These are source changes, not evidence of a successful local build.
  * **Remaining acceptance work**:
    1. Run the configured Windows MinGW/UCRT64 SCons build.
    2. Open the Godot 4.7 project, confirm all scripts parse, run CareerRegressionTest.gd (F10), and resolve any runtime/test failures.
    3. Manually verify New Career League -> Club selection, displayed club metadata, player-to-club assignment, selected-league standings/fixtures, odd-team byes, pre-season pairings, and Save/Load persistence.
    4. Audit all 156 clubs, 28 league labels, finances, colours, formations, stadiums and player memberships for historical accuracy on **15 July 2006**. Dataset coverage is complete at the club level, but historical correctness is not yet certified.
  * **Tuesday League / Custom Players:** keep Club = 11A7, League = Tuesday League and the NHHA72528 access gate consistent in New Career and future Transfer Market search.

* **5E. `MatchEngine` Simulation Realism, Attribute-Driven Defense, Stamina, Movement & Match Integration**:
  * **Implementation snapshot — approximately 50% source complete (reviewed through `52729a`):** `30e281a` implements stamina-attribute drain, home/away/combined end-match stamina telemetry, attribute/OVR-weighted chance triggers and xG, OVR-scaled defensive tracking/lane coverage, and smoother acceleration/deceleration/turn response. `0822a95` passes the user's end-match stamina array into `stamina_report` and tightens goalkeeper near-ball handling to the goal corridor. `52729a` removes the stray standalone C++ token in `_compute_tactical_target`, updates PreMatchView to use the configured Starting XI and `condition`/fitness, and adds MatchHUD refresh hooks when squad updates or substitutions occur. These are source-level findings; do not treat them as a successful current-source build or gameplay acceptance.
  * **HUD and skip state:** `9a1e9a5` removes obsolete `_continue_play_btn` code. `0822a95` removes only a duplicate hidden Skip control; the primary visible Skip button remains in `MatchHUD.gd` and is still connected to `skip_requested`. Skip-token and backend match-skip logic remain preserved. If the intended design is still to remove the visible button, that UI-only task remains open.
  * **Input audit:** the current `KEY_R` handler in `MatchScene.gd` calls `_request_next_match()` after full time; it is not Advanced Dribbling. No Advanced Dribbling `KEY_R` binding was found in the match backend. Preserve the Next Match shortcut and do not remove internal dribble timers as part of a nonexistent R-binding cleanup.
  * **Defensive attribute snapshot:** standing/AI tackle and 1v1 duel calculations use `Defending`, `Physical` and fatigue; pass interceptions use `Defending`, fatigue and pressing; shot-block probability uses `Defending` and fatigue. OVR also scales defensive tracking/coverage and related movement behavior. A dedicated clearance-resolution path was not found, and broader role/position/awareness coupling remains incomplete.
  * **Lineup/substitution integration snapshot:** `PreMatchView` now extracts the configured Starting XI from CareerState and displays `condition` as fitness. `MatchHUD` listens for squad/substitution updates and refreshes the HUD/tactical view when supported. However, `TacticalScreen.gd` still creates a placeholder `Home #12`–`Home #18` bench and does not implement `sync_squad_from_engine()`, so the actual CareerState bench/player identities and substitution payloads are not yet fully synchronized into live match substitutions.
  * **Current source blocker found in `52729a`:** `src/CareerState.h` currently contains a `MatchEngine` declaration rather than the `CareerState` class, while `src/CareerState.cpp` includes that header and defines `CareerState` methods and `src/register_types.cpp` registers `CareerState`. Restore the correct `CareerState.h` declaration (and keep `MatchEngine` declarations in `src/match_engine.h`) before treating the current source as buildable. The earlier stray `2` in `_compute_tactical_target` was removed in `52729a`; it is no longer the current blocker.
  * **Still outstanding for 5E:** restore `CareerState.h`; complete defender attribute/positioning coupling and dedicated clearance resolution; synchronize the real bench and substitution player data; tune defensive quality and movement; add/execute comparative simulation regressions; run a clean SCons build and Godot runtime QA.
  * **1. Match Stamina Drain Coupled to CSV `Stamina` Attribute (`src/MatchEngine.h` / `.cpp`)**:
    * Replaces flat/generic stamina decay with a per-player endurance curve driven directly by each player's imported CSV **`Stamina`** attribute ($1\text{--}99$) combined with active tactical `Pressing` / `Tempo` and sprint distance:
      $$\Delta \text{Stamina}(t) = \text{BaseDrain}(t) \times \text{TacticMult} \times \left(1.35 - 0.65 \times \frac{\text{Clamp}(\text{PlayerStamina}, 1, 99)}{99.0}\right)$$
    * High-`Stamina` players ($85\text{--}99$) sustain high-tempo pressing across 90 minutes with gradual fatigue, while low-`Stamina` players ($40\text{--}60$) fatigue noticeably faster in the second half, degrading sprint speed and reaction sharpness.
    * Exports each player's exact end-of-match remaining `current_stamina` (`0.0–100.0%`) in `get_match_stats()` / `commit_match_result()`.
  * **2. Attack Chance — Individual Attributes & OVR Coupling**:
    * Attacking chance execution (`shot accuracy`, `shot power`, `dribble beat rate`, `through-ball weight`, `off-ball run timing`) evaluates individual player attributes (`Shooting`, `Pace`, `Dribbling`, `Passing`, and position-specific role weights for `ST/CF/LW/RW/CAM`) alongside player `OVR`.
    * Different player archetypes produce distinct attacking outcomes (e.g., high-`Pace` wingers stretch wide channels; high-`Passing` `CAM`s unlock central through-balls; high-`Shooting` `ST`s convert tight-angle box chances) without making `OVR` the sole determinant.
  * **3. Defensive Chance — Individual Attributes, Positioning & OVR Coupling**:
    * Target behavior: defensive duels, standing/sliding tackle success, passing-lane interceptions, shot blocks and clearances should scale with defender `Defending`, `Physical`, `Pace`, role/position and positional/tactical awareness (`Marking`, `Interceptions`, `Positioning`), modulated by team `Pressing`, `Defensive Line` and defender `OVR`. Current source only partially covers these paths; dedicated clearance resolution and the broader role/awareness model remain outstanding.
  * **4. Opponent Defensive AI Scaling**:
    * Opponent defensive AI decision-making scales smoothly with opponent team `OVR` and individual defender quality:
      * Higher-OVR opponents maintain compact defensive lines, shift horizontally as a unit, close down passing lanes proactively, and double-mark dangerous ball-carriers—while still remaining vulnerable to quick combinations, tactical overloads, and individual brilliance (never artificially omniscient or unbeatable).
      * Lower-OVR opponents maintain sensible basic defensive shape but exhibit wider marking gaps, slower recovery transitions, and slightly higher mistimed-challenge rates.
  * **5. Balanced Attack & Defensive Chance Probability**:
    * Chance generation frequency and conversion rates reflect relative team `OVR` (`user_team_ovr` vs. `opponent_team_ovr`), individual positional matchups, and tactical risk-reward trade-offs (`Mentality`, `Defensive Line`, `Pressing`, `Tempo`), while preserving stochastic variance so lower-OVR underdogs retain realistic counter-attacking and set-piece upset chances.
  * **6. Real-Time Believable Player Movement (`MatchEngine` Steering Physics)**:
    * Eliminates instant point-to-point snapping or unrealistic instant full-speed dashes toward clicked/tapped destinations.
    * Implements continuous 2D steering kinematics: top sprint speed scales with player `Pace` and remaining `CurrentStamina`, paired with realistic acceleration ramp-up, braking deceleration, and angular turn-rate inertia so players take believable time to change direction and reach targets across both mouse-click and Android touchscreen inputs.
  * **7. Match Input Mapping Audit (`demo/MatchScene.gd` / `demo/MatchHUD.gd` / `src/match_engine.cpp`)**:
    * Current source maps `KEY_R` to `_request_next_match()` after full time; it is not an Advanced Dribbling action. Preserve this Next Match shortcut and verify it does not interfere with live-match movement or standard controls. Do not remove internal dribble timers/boost logic merely because the old roadmap called them an `R` ability; they are backend movement mechanics and require a separate design decision before changing.

* **5F. Post-Match Stamina Persistence, Calendar Recovery & World Soccer Champs-Inspired Injury System**:
  * **1. Post-Match Stamina Persistence & Configurable Daily Recovery (`CareerState.h` / `.cpp`)**:
    * **No Automatic Full Reset**: Remaining stamina at the end of a match persists directly on each player in `CareerState` (`player->SetCurrentStamina(end_stamina)`).
    * **Calendar-Driven Recovery**: Every in-game day advanced via `CareerState::advance_day()` (never real-world wall-clock time) recovers a configurable fraction of maximum stamina (`stamina_recovery_rate_per_day = 0.30f`, tunable within `0.25f–0.30f`):
      $$\text{Stamina}_{\text{next}} = \min\left(100.0,\; \text{Stamina}_{\text{curr}} + 100.0 \times r_{\text{recovery}}\right)$$
    * Surface recovery projections in `ScheduleView` and `PreMatchView` so congested fixture weeks require realistic squad rotation.
  * **2. World Soccer Champs-Inspired Player Injury System (`CareerState` & `MatchEngine`)**:
    * **Match Injury Generation**: During `MatchEngine` execution, physical challenges and high-fatigue sprints evaluate injury risk (higher risk when `CurrentStamina < 35%` or after heavy tackles).
    * **Simple, Readable Severity Tiers & Recovery Timers**:
      * `Minor Knock`: `2–5` in-game days out.
      * `Muscle Strain`: `7–14` in-game days out.
      * `Ligament Sprain`: `21–35` in-game days out.
      * `Severe Injury`: `45–90` in-game days out.
    * **Calendar Recovery & Availability Enforcement**: `CareerState::advance_day()` decrements `injury_days_remaining` by `1` per day until `0` (automatically clearing `is_injured = false` and posting a clearance note). While `is_injured == true`, the player cannot be selected in Starting XI/Bench for matches (`SquadView` & `PreMatchView` enforce replacement) and does not gain active training XP.
    * **UI & Save/Load Integration**: Displays injury cross icon + remaining days badge in `SquadView`, `DashboardView`, `PreMatchView`, and `PlayerProfileModal`; dispatches `"INJURY"` Inbox alerts; persists all injury fields in `<slot>.json`.

* **5G. Audio System, Stadium Atmosphere, Contextual Commentary & Goal Celebrations (`demo/AudioManager.gd`, `MatchEngine`, `MatchHud`)**:
  * **1. Centralized `AudioManager` Autoload & Audio Buses (`demo/AudioManager.gd` & `demo/assets/audio/`)**:
    * Establishes dedicated Godot `AudioServer` buses (`Master`, `Music`, `Atmosphere`, `SFX`, `Commentary`) with clean crossfading, mute/volume controls accessible from the main menu and `CareerShell` header/Manager modal, and safe graceful fallback if any optional audio stream is absent on desktop or Android.
  * **2. Menu & Career Interface Background Music (`Menu Music`)**:
    * Plays curated, non-intrusive background music tracks across the main menu, `NewCareerView`, and `CareerShell` career management views, smoothly fading out on match kickoff and fading back in upon returning from `PostMatchModal`.
  * **3. Match Atmosphere & Importance-Scaled Stadium Ambience (`Match Atmosphere` & `Stadium Atmosphere`)**:
    * **Continuous Match Ambience**: Loops a realistic stadium crowd bed throughout active match play, dynamically modulating volume and excitement swells when possession enters the final third, during shots on target, and in stoppage/closing minutes.
    * **Match Importance Scaling**: `CareerState` passes a fixture `match_importance` tier (`STANDARD`, `RIVALRY_DERBY`, `TOP_CLASH`, `TITLE_DECIDER`, `CUP_FINAL`) into `MatchEngine` / `AudioManager`, scaling baseline crowd density, chant frequency, and peak eruption intensity so a high-stakes title clash or derby feels noticeably louder and more intense than a routine mid-table fixture.
  * **4. Contextual Commentary (`Commentary`)**:
    * `MatchEngine` emits structured match-event hooks (`GOAL`, `SAVE`, `FOUL`, `CARD`, `SUBSTITUTION`) carrying player name, club side, minute, and event context.
    * Triggers contextual commentary callouts (paired audio commentary cues/stingers + synchronized `MatchHud` broadcast ticker lines) tailored to key match moments: clinical goals, dramatic goalkeeper saves, tactical fouls/cards, and bench substitutions.
  * **5. Distinct Home & Away Goal Announcements (`Goal Announcements`)**:
    * **Home Goal Reaction**: Triggers stadium PA goal horn/anthem sting, massive home-crowd eruption, and full-width home scoreboard banner pulse.
    * **Away Goal Reaction**: Triggers distinct away-end cheer + subdued home-crowd groan/murmur and away-team scoreboard reaction so home vs. away scoring feels immediately distinct.
  * **6. Goal Celebrations with 3–5s Touchline Run (`Goal Celebrations`)**:
    * **C++23 `MatchEngine` Celebration State (`GOAL_CELEBRATION`)**: When a valid goal is scored, `MatchEngine` transitions into a `GOAL_CELEBRATION` state lasting **`3.0–5.0` real-time seconds** (`celebration_timer` configurable within `3.0s–5.0s`) before resetting players to kickoff positions.
    * **Touchline Run & Visual/Audio Reactions**: During the `3.0–5.0s` celebration window, the goalscorer sprints toward the nearest touchline/corner flag while nearby teammates converge, paired with crowd eruption audio, celebration SFX, and 2D pitch visual reactions (scorer highlight ring, celebration callout banner, and subtle pitch pulse).
  * **7. Full-Time Audio & Match-End Atmosphere (`Full-Time Audio`)**:
    * At the final whistle (`FULL_TIME`), triggers the referee triple-whistle SFX followed by result-aware post-match crowd reactions—euphoric celebration roar for a home victory or underdog upset, warm applause for a draw, or disappointed groans/murmurs for a home defeat—persisting through the `PostMatchModal` summary until transitioning back to career menu music.

* **Phase 5 Acceptance Criteria**:
  * F5 opens the dedicated Main Menu; New Career / Continue / Load route safely into a valid career, and successful creation/loading automatically opens the CareerShell Dashboard without requiring `KEY_C`. MatchScene remains correctly coordinated for the complete Pre-Match -> Match -> Post-Match -> CareerShell cycle.
  * Starting a new career initializes on **15 July 2006** from the 13-column `res://data/clubs.csv`; selected club name, CSV ClubID, player roster mapping, club metadata and kit colours must survive the New Career -> CareerState bootstrap. Fixtures and standings must contain only clubs from the selected league; even/odd league sizes must pass round-robin and no-double-booking tests.
  * Match stamina drain scales with the CSV `Stamina` column, post-match stamina persists into `CareerState`, advancing calendar days recovers $\approx 30\%$ stamina/day capped at $100\%$, and match injuries sideline players with day-by-day calendar recovery.
  * `KEY_R` remains the post-match Next Match shortcut (not an Advanced Dribbling action), and player movement exhibits realistic acceleration, deceleration, and turning inertia without breaking standard match controls.
  * Menu/career background music loops cleanly; matches play importance-scaled stadium ambience, contextual commentary for goals/saves/fouls/substitutions, distinct home vs. away goal reactions, a `3.0–5.0s` touchline goal celebration run before kickoff reset, and full-time whistle + result-aware crowd audio.

---

### Phase 6 — Club Management, Finances, Senior Squad Training Tab & Global UI Design System `[SCHEDULED]`
* **Target Files**: `demo/ClubView.gd` / `.tscn`, `demo/FinancesView.gd` / `.tscn`, `demo/TrainingView.gd` / `.tscn`, `demo/CareerShell.gd`, `demo/assets/fonts/`, `demo/theme/cfm_theme.tres`, plus C++ `CareerState` ledger, objective, and training progression engines.

* **6A. Separated Financial Ledgers & Board Objectives (`FinancesView` & `ClubView`)**:
  * **Three Distinct Buckets**: `Total Club Balance`, `Transfer Budget`, and `Wage Budget` (`Available Wage Cap = Wage Budget - Current Weekly Payroll`), with Board-capped budget allocation slider and itemized income/expense ledger.
  * **Board Confidence & Objectives (`ClubView`)**: $0\text{--}100\%$ confidence gauge, live season objectives tracker with prize payouts, and tiered facility upgrades (`Stadium Capacity`, `Training Complex`, `Medical Center`, `Youth Academy`).

* **6B. Senior Squad Training Mechanism & Dedicated `Training` Tab (`demo/TrainingView.gd` / `.tscn`)**:
  * **UI-Driven Positional Group Training (No Training Mini-Games)**:
    * Adds a dedicated **`Training`** route tab in `CareerShell` (`"training"`) where the manager selects weekly focus programs for **four positional groups**:
      1. **Attackers (`ST`, `CF`, `LW`, `RW`)**: `Finishing & Shooting`, `Dribbling & 1v1`, `Attacking Movement & Off-Ball Runs`, or `Balanced Attack`.
      2. **Midfielders (`CDM`, `CM`, `CAM`, `LM`, `RM`)**: `Passing & Vision`, `Ball Control & Press Resistance`, `Box-to-Box Defending`, or `Balanced Midfield`.
      3. **Defenders (`CB`, `LB`, `RB`)**: `Tackling & Duels`, `Interceptions & Positioning`, `Aerial & Heading`, or `Balanced Defense`.
      4. **Goalkeepers (`GK`)**: `Reflexes & Shot Stopping`, `1v1 & Reaction`, `Positioning & Distribution`, or `Balanced Goalkeeping`.
  * **Gradual, Non-Uniform Attribute & OVR Development (`CareerState::advance_day()`)**:
    * Players accumulate fractional attribute XP over in-game calendar days based on their positional group's assigned focus, scaling `Player::RecalculateOVR()` as individual attributes improve.
    * **Individualized Growth Modifiers**: Growth rate is never identical across players—it scales dynamically with remaining headroom to `Potential` ($\text{Potential} - \text{CurrentOVR}$), `Age` development curve (faster for U23 players, steady for peak 24–29 players, tapering for 30+ veterans), active `Training Complex` facility tier (Phase 6A), and availability (injured players pause pitch training).
  * **Extensible & Persistent**: Session definitions are data-driven so additional drills and individual focus slots can be added later; active group assignments and fractional attribute XP persist inside `<slot>.json`.

* **6C. Global UI & Typography Design System Overhaul (Authentic Football-Management Aesthetic)**:
  * **Typography & Font Hierarchy (`demo/assets/fonts/` & `cfm_theme.tres`)**:
    * Replaces default fallback UI fonts with a clean, high-legibility geometric/condensed sports-management font stack (`Display/Headers`, `UI Body`, and `Tabular Monospace/Numerals` for OVR ratings, match clocks, wages, and league tables).
    * Establishes strict typographic scale (`22px` view title, `15px` section header, `13px` primary body/table row, `11px` metadata badge) with consistent line height and letter spacing.
  * **Redesigned Player Cards & Roster Rows**:
    * Upgrades player cards across `SquadView`, `PlayerProfileModal`, `TransfersView`, `TrainingView`, and `YouthView` for instant readability of `OVR`, `Position` tag, `Stamina` bar, `Injury` status, `Age`, and 6 core/GK attributes.
  * **Anti-AI-Slop Visual Restraint**:
    * Explicitly eliminates generic AI-generated UI tropes: **zero multi-stop neon gradients, zero glowing drop-shadows, zero overly rounded bubbly cards, and zero cluttered decorative widgets**.
    * Enforces crisp `4px–6px` border radii, flat dark-navy (`#06090E` / `#0B101B`) surfaces with hairline `1px` borders (`#1C2638`), disciplined grid alignment, purposeful negative space, and restrained pitch-green (`#10AC84` / `#1DD1A1`) semantic highlights.

* **Phase 6 Acceptance Criteria**:
  * Weekly wage deductions execute every 7 days; completing board objectives credits `Total Club Balance`.
  * Assigning training programs across the 4 positional groups in the `Training` tab produces gradual, potential/age-differentiated attribute and OVR gains over calendar days and persists across Save/Load.
  * Typography hierarchy, player cards, and dark-navy surfaces render consistently across all views without decorative gradient/glow clutter.

---

### Phase 7 — Dynamic Transfer Market, Multi-Criteria Search, Contracts, Reputation, Playstyles & Retirement `[SCHEDULED]`
* **Target Files**: `demo/TransfersView.gd` / `.tscn`, `demo/NegotiationModal.gd` / `.tscn`, `demo/PlayerProfileModal.gd`, `demo/SquadView.gd`, `src/mainplayer.h` / `.cpp`, `src/MatchEngine.h` / `.cpp`, `src/CareerState.h` / `.cpp`.

* **7A. Scalable Player Valuation Engine**:
  * Computes dynamic market value from:
    $$\text{Value} = \text{BaseOVR}(\text{OVR}) \times \text{AgeCurve}(\text{Age}, \text{Potential}) \times \text{PosMult}(\text{Pos}) \times \text{ContractMult}(\text{YearsLeft}) \times \text{RoleMult}(\text{SquadRole})$$
  * Accounts for expiring contracts ($\le 1\text{ year}$ discount), high-potential U21 wonderkid premiums, and selling club financial pressure.

* **7B. Two-Stage Transfer & Contract Negotiations (`NegotiationModal`)**:
  * **Stage 1 — Club-to-Club Fee Negotiation**: Submit initial transfer fee (plus optional sell-on clause); AI club evaluates against player valuation and squad depth, responding with `Accept`, `Counteroffer` (up to 3 rounds), or `Reject`.
  * **Stage 2 — Player Personal Terms**: Upon fee agreement, negotiate `Weekly Wage`, `Contract Length (1–5 yrs)`, and `Squad Role` (`Crucial`, `Important`, `Rotation`, `Prospect`), clamped by the mandatory Age-37 retirement ceiling ($\min(5, \max(1, 37 - \text{Age}))$).

* **7C. Multi-Criteria Transfer Search Engine, Access Gating, Transfer Windows & AI Market Activity (`TransfersView`)**:
  * **6-Criteria Player Search & Filter Engine**: Backed by C++ `CareerState::search_transfer_players(filters_dict)` querying the merged `res://data/players_2006_with_price.csv` + `clubs.csv` database across all 6 search parameters (excluding retired players):
    1. **`Name`**: Case-insensitive substring query matching player full name or surname.
    2. **`Club`**: Dropdown/autocomplete filter sourced from `res://data/clubs.csv` plus `"Free Agent"` (hides `11A7` until `NHHA72528` is redeemed).
    3. **`Age`**: Minimum and maximum age bounds (`Min Age` to `Max Age`, bounded below `37`).
    4. **`Nationality`**: Country filter sourced directly from the merged `Nationality` column in `res://data/players_2006_with_price.csv`.
    5. **`League`**: Competition filter sourced directly from the merged `League` column in `res://data/players_2006_with_price.csv` and `res://data/clubs.csv` (hides `Tuesday League` until `NHHA72528` is redeemed).
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
    * Players who start with $\text{BaseOVR} \le 96$ and later grow above $96$ OVR through Youth Academy development, senior squad training, or seasonal progression **never** unlock Playstyles ($\text{BaseOVR} \le 96 \lor \text{IsAcademyProduct} = \text{true} \implies \text{Playstyles} = \emptyset$).
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
    * Reads `Playstyle 1`, `Playstyle 2`, `Playstyle 3` from `res://data/players_2006_with_price.csv`, stores `uint32_t PlaystyleMask` + `std::vector<std::string> Playstyles` on `Player`, serializes `"playstyles"` in `<slot>.json`, and renders Playstyle badges in `PlayerProfileModal.gd`.

* **7F. Mandatory Age-37 Retirement & Squad Turnover Pipeline**:
  * **1. Universal Age-37 Retirement Rule (`src/mainplayer.h`, `Players_Database/player_database.cpp`, `src/CareerState.cpp`)**:
    * Every player in the database (CSV imports from `res://data/players_2006_with_price.csv`, custom classmates/icons, Youth Academy graduates, and fallback/generated players) has a fixed mandatory retirement age of **`37`** (`retirement_age = 37`), overriding any legacy `36` defaults or per-row CSV `RetirementAge` variance *(data/save normalization committed in `4aa3a2a4b227006758f2421c1042657c921b3fe6`)*.
    * Players at `Age == 36` display a `"Retiring at End of Season"` status badge in `PlayerProfileModal` and `SquadView` and are blocked from incoming transfer bids during their final season.
  * **2. Contract Length Cap (`NegotiationModal` & `CareerState`)**:
    * Maximum contract duration during transfer negotiations and contract renewals is clamped to $\min(5, \max(1, 37 - \text{Age}))$ years so every player's contract terminates at or before age `37`.
  * **3. Annual Rollover Retirement Execution (`CareerState::_process_annual_rollover()` on July 1)**:
    * During the July 1 season boundary (`_month == 7 && _day == 1`), after incrementing every player's age (`Age + 1`), any player reaching **`Age >= 37`** immediately triggers retirement (`player->SetRetired(true)`):
      * **Roster Removal & Best-XI Rebalance**: Automatically removes all retired players from every club's `Starting XI`, `Bench`, and `Reserves` (`Team::GetRoster()`), reassigns captaincy (`_captain_by_club`) if the retiring player was captain, and runs positional Best-XI sorting (`auto_pick_best_xi`) to fill vacated starting spots (preserving a `GK` in Slot `0`).
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

---

### Phase 8 — Full Career Acceptance Suite & Multi-Platform Release
* **End-to-End Automated Test Harness**: Expand `CareerRegressionTest.gd` to cover full-season simulation, **15 July 2006** `clubs.csv` `League -> Club` selection + kit colour application, `Club`/`League`/`Nationality`/`Stamina` column loading, post-match stamina persistence + $\approx 30\%$/day recovery, World Soccer Champs-inspired injury lifecycle, `MatchEngine` attribute-coupled attack/defense chances + real-time movement + `KEY_R` post-match Next Match shortcut verification, `AudioManager` bus/menu music transitions + importance-scaled stadium atmosphere + contextual commentary (`GOAL`/`SAVE`/`FOUL`/`SUBSTITUTION`) + distinct home/away goal announcements + `3.0–5.0s` touchline goal celebration state + full-time whistle audio, Senior `Training` tab progression, `NHHA72528` code redemption + `Tuesday League -> 11A7` unlock, GK 6-stat profile export + strict GK-only Slot `0` drag/swap lock, 6-criteria Transfer Search, transfer window round-trips, financial ledger invariants, $> 96$ Base OVR Playstyles gating, and mandatory Age-37 retirement during multi-season rollover.
* **Android & Aspect-Ratio Verification**: Validate touch targets ($\ge 44\text{dp}$), collapsible drawer ergonomics, modal bounds, and font legibility across `16:9`, `19.5:9`, and `20:9` viewports.
* **Release Packaging**: Strip debug overlays/test harnesses and build release GDExtension binaries (`target=template_release`) for Windows and Android.