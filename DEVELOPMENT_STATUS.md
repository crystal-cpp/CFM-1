# CFM-1 Development Status & Engineering Handoff

**Last Updated**: 2026-11-20

**Reviewed Repository Baseline**: `7b2e2e53e20ca03d82ed4e22680ee638ee1d2a61` *(latest main-branch merge; includes the 12-column club registry, structured player/club CSV parsing and league-derived fixture generation from `a32b521fde9cd013edaa8f9c01ab93945666a099`)*

**Current Phase**: Phase 5

**Current Milestone**: **Phase 5D-2 — Structured Player/Club Data & League-Scoped Fixture Generation; source implementation committed, integration corrections and build/runtime verification pending. Phase 5C-2 startup flow is implemented in source; runtime QA pending.**

## 1. Milestone & Subsystem Status Matrix

| Subsystem / Milestone | Layer | Implementation Status | Verification Status | Notes | 
| ----- | ----- | ----- | ----- | ----- | 
| **Phase 1: `MatchEngine` Core** | C++23 | Complete | Verified | 22-player simulation, `formations.csv`, telemetry, skip tokens. | 
| **Phase 2A: Chance-Gated Scoring & Physics** | C++23 | Complete | Verified | Chance-only goals, tactics/stamina coupling, direct input APIs. | 
| **Phase 2B: Match HUD & In-Match Tactics** | GDScript | Complete | Verified | Desktop + Android gestures, live pause `TACTIC` overlay. | 
| **Phase 2C: Match Build & Regression** | SCons / GD | Complete | Verified | Windows UCRT64 DLL & match regression suite passing. | 
| **Phase 3A: `CareerState` & Atomic Save/Load** | C++23 | Complete | Verified | DLL built; `CareerRegressionTest.gd` Suites 1–6 passed. | 
| **Phase 3B: `CareerShell` Global Frame** | GDScript | Complete | Verified | Fixed `Label` `"normal"` StyleBoxes, centered Manager modal, `KEY_C`/`ESC`. | 
| **Phase 3C: `DashboardView`, `InboxView`, `PlayerProfileModal`** | GDScript | Complete | Verified | Suites 1–8 passed; `"board"` -> `"club"` routing and `YOUTH` mode verified. | 
| **Phase 3D: Phase 3 Acceptance Suite** | Both | Complete | Verified | DLL built and Suites 1–8 passing on desktop & narrow viewports. | 
| **Phase 4A: C++ Squad, Tactics, GK Stats & Youth Bindings** | C++23 | **Complete** | **Pending Phase 4C** | Swaps, strict GK-only Slot `0` guard, GK 6-stat export (`Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, `Physical`), fit OVR, roles/numbers, tactics, youth training/aging/promo. | 
| **Phase 4B: `SquadView`, `TacticsView`, `YouthView` & GK UI** | GDScript | **Complete** | **Pending Phase 4C** | Procedural views wired into `CareerShell.gd`, `SquadView` drag-and-drop/click-swap GK-only Slot `0` lock, and `PlayerProfileModal.gd` 6-stat GK display. | 
| **Phase 4C: Phase 4 Build & Acceptance Suite** | Both | **Ready to Run** | **Pending / not current focus** | Recompile DLL + run Suites 1–12 + manual cross-viewport QA + `*_2` cleanup; Phase 5D is the active milestone. | 
| **Phase 5A: Season Calendar & ScheduleView Foundation** | Both | **Implemented in repository** | **Runtime QA pending** | July–May calendar, July pre-season friendlies, domestic league fixture generation, and tabbed ScheduleView (`My Club Fixtures | League Matchdays | Results Archive`). Latest commit `018eef5` adds responsive flow layouts and compact labels/tooltips. |
| **Phase 5B-1: Domestic Competition Foundation** | Both | **UI implemented in repository** | **Runtime / data QA pending** | `CompetitionView.gd` builds the domestic league table and reads live `CareerState.get_league_standings()` data when available, with CSV fallback. This does not imply UCL fixtures are complete. |
| **Phase 5B-2: CompetitionsView Integration & Responsive UI** | GDScript | **Implemented in repository** | **Source inspection complete; runtime QA pending** | `CareerShell.gd` calls `_competitions_view.refresh_competitions()` when navigating to Competitions, in `refresh_all_views()`, and in `_on_standings_updated()` (commit `d43ba52`). `ScheduleView` responsive layout refinements are in `018eef5`. Verify live updates and desktop/narrow viewport behavior in Godot. UCL fixture generation, qualification and authoritative results remain deferred. |
| **Phase 5C: Pre-Match Center & Post-Match Pipeline** | GDScript / CareerState | **Integration committed** | **Source inspection complete; runtime QA pending** | Dashboard / Schedule requests open `PreMatchView`; `MatchScene.launch_career_match()` applies the context before kickoff, passes primary kit colors to 2D outfield markers, commits the result after the final-whistle chance deferral, and displays `PostMatchModal.show_report(summary)`. Continue returns to CareerShell and refreshes views. Validate the full flow and result mapping in Godot. |
| **Phase 5C-2: Main Menu & Application Startup / Career Entry Flow** | GDScript / Scene Architecture | **Implemented in repository** | **Source inspection complete; runtime QA pending** | `demo/project.godot` starts `res://app_root.tscn`; `AppRoot.gd` owns `CareerState`, persistent `MatchScene`, `CareerShell`, `MainMenuView` and `NewCareerView`, with New / Continue / Load / Settings / Exit actions, automatic Dashboard entry, load-error handling and an unsaved-progress confirmation. Settings currently exposes fullscreen only; audio settings remain Phase 5G. Validate all transitions in Godot. |
| **Phase 5D-1: Club Registry Schema & Player CSV Metadata** | C++23 / CSV | **Implemented in source** | **Data/historical audit pending** | `demo/data/clubs.csv` now has 106 data rows and the 12-column schema across 28 league labels; 802 player rows have Club values matching registry names, but 23 clubs have no player rows. Historical 15 July 2006 accuracy and roster completeness remain unverified. `NHHA72528` code and unlock UI are present. |
| **Phase 5D-2: Structured Bootstrap & League-Scoped Fixtures** | C++23 / CareerState | **Source implementation committed; integration defects found** | **Build + Godot regression/runtime QA pending** | Commit `a32b521` adds structured PlayerRecord/ClubRecord parsing, league-derived fixture participants, double round-robin generation with odd-team bye support, dynamic competition labels and retirement-age save persistence. Blockers: `club_to_dict()` keys (`club`, `reputation`, `total_balance`) do not match CareerState consumers (`name`, `rating`, `budget`); CSV ClubID and several metadata fields are dropped; leagues with fewer than two clubs borrow teams from other leagues; odd-sized pre-season friendlies double-book the first club. Regression script remains Phase 3-only with stale 2026/16-club expectations. No SCons build or Godot runtime QA was performed in this review. |
| **Phase 5E: `MatchEngine` Realism, Stamina Drain & Controls Cleanup** | Both | **Spec Ready** | **Scheduled (Phase 5)** | CSV `Stamina`-driven match drain, attribute-coupled attack/defense chances, scaled opponent defensive AI, balanced chance probability, real-time steering movement, and removal of Advanced Dribbling (`R`). | 
| **Phase 5F: Stamina Persistence,** $\approx 30\%$**/Day Recovery & Injury System** | Both | **Spec Ready** | **Scheduled (Phase 5)** | Post-match stamina preservation, configurable $\approx 30\%$/day calendar recovery, and World Soccer Champs-inspired injury generation & calendar recovery. | 
| **Phase 5G: Audio System, Stadium Atmosphere, Commentary & Goal Celebrations** | Both | **Spec Ready** | **Scheduled (Phase 5)** | Menu/career background music, match crowd ambience, match-importance stadium intensity, contextual commentary (`GOAL`/`SAVE`/`FOUL`/`SUB`), distinct home/away goal reactions, `3.0–5.0s` touchline goal celebration run, and full-time whistle/crowd audio. | 
| **Phase 6A: Club Management, Finances & Board** | Both | Placeholder Views | Scheduled | Separated financial ledgers, board confidence, stadium/facility upgrades. | 
| **Phase 6B: Senior Squad Training Mechanism & `Training` Tab** | Both | **Spec Ready** | **Scheduled (Phase 6)** | Dedicated `Training` tab with 4 positional group focus programs (Attackers, Midfielders, Defenders, Goalkeepers) and potential/age-aware OVR growth. | 
| **Phase 6C: Global UI & Typography Design System** | GDScript | **Spec Ready** | **Scheduled (Phase 6)** | Custom typography hierarchy, redesigned high-clarity player cards, and disciplined non-AI-slop dark-navy aesthetic. | 
| **Phase 7A–7D: Transfers, Search & Reputation** | Both | Placeholder Views | Scheduled | Valuation, fee/wage negotiations, transfer windows, six-field player search, AI transfers, loyalty, and reputation. | 
| **Phase 7E: Exclusive** $> 96$ **Base OVR Playstyles** | Both | **Spec Ready** | **Scheduled (Phase 7)** | 22 Playstyles (`PlaystylesDescription.txt`) gated strictly to `BaseOVR > 96` non-academy players. | 
| **Phase 7F: Mandatory Age-37 Retirement Pipeline** | Both | **Partially Implemented (Data/Save)** | **Scheduled (Phase 7)** | `RetirementAge = 37` normalization committed in CSV loader & saves; contract length clamping, `Age == 36` badge, July 1 rollover removal + Best-XI rebalance, and Board retirement report remain scheduled. | 

## 2. Phase 4 Implementation Summary & Phase 3 Patch Baseline

### A. Phase 4A C++23 Backend Additions (`src/mainplayer.h`, `src/CareerState.h` / `src/CareerState.cpp`)

1. **Squad Roster, Strict GK-Only Slot `0` Lock & Positional Fit Engine**:

   * Added `swap_squad_players(club_id, slot_a, slot_b)` and `set_starting_xi(club_id, xi_ids)` supporting `Starting XI (0–10)`, `Bench (11–17)`, and `Reserves (18+)` swaps while enforcing **strict Goalkeeper-only rules**:

     * Starting XI Slot `0` (`GK`) **must** hold a `Position::GK` player; any swap or drag attempt moving an outfield player into Slot `0` is rejected (`return false`).

     * Outfield Starting XI Slots (`1–10`) **cannot** hold a `GK`; `GK <-> GK` swaps between Slot `0` and Bench/Reserves are permitted.

     * Duplicate `player_id` assignments across XI, Bench, and Reserves are rejected.

   * Added positional compatibility evaluator (`NATURAL` = $1.00\times$, `COMPATIBLE` = $0.95\times$, `OUT_OF_POSITION` = $0.85\times$ effective OVR) synced to the club's active `formations.csv` slot tags.

   * Added `set_club_roles(club_id, roles_dict)` (Captain, Vice-Captain, Penalties, Free Kicks, Corners) and `set_player_jersey_number(club_id, player_id, number)` enforcing unique `1–99` kit numbers.

2. **Goalkeeper 6-Stat Profile Mapping (`src/mainplayer.h` & `src/CareerState.cpp`)**:

   * Mapped `PlayerAttributes` getters for Goalkeepers (`GetDiving() -> Pace`, `GetReflex() -> Shooting`, `GetPositioning() -> Passing`, `GetReaction() -> Dribbling`, `GetKicking() -> Defending`, `GetGKPhysical() -> Physical`) so existing 6-attribute CSV columns remain compatible without schema breakage.

   * Updated `CareerState::_player_to_dict()` to export `"is_gk": true` along with `"diving"`, `"reflex"`, `"positioning"`, `"reaction"`, `"kicking"`, and `"physical"` when `Position == Position::GK`.

3. **Permanent Club Tactics & `MatchEngine` Handoff**:

   * Added `get_club_tactics(club_id)` and `set_club_tactics(club_id, tactics_dict)` persisting `formation_id`, `mentality`, `pressing`, `defensive_line`, `passing_style`, `width`, and `tempo` on `Team` and inside atomic JSON saves—isolated from temporary in-match `TACTIC` overrides.

   * Wired `CareerState` lineup, jersey numbers, and permanent tactics directly into `MatchEngine` team initialization.

4. **Youth Academy Infrastructure, Coaching & Dual-Cycle Progression**:

   * Added `get_youth_academy_info(club_id)` and `upgrade_youth_facility(club_id, upgrade_type)` for Academy Facility Level (`1–5`) and Scouting Network Level (`1–5`), enforcing `club_balance` checks and updating weekly upkeep.

   * Added `assign_youth_coach(club_id, specialty, coach_id)` across four specialties (`Technical`, `Physical`, `Tactical`, `Goalkeeping`) that scale weekly attribute XP gains.

   * Separated **weekly training XP ticks** (executed every 7 days in `advance_day()`) from **annual season rollover** (age `+1`, debut age eligibility update, and potential range narrowing).

   * Verified `promote_youth_player(player_id)` atomically removes the prospect from `YouthAcademy`, appends to senior `Team`, assigns a senior wage/contract, and posts a `"BOARD"` (`"club"`) Inbox confirmation.

### B. Phase 4B GDScript Views & Shell Integration (`demo/`)

1. **`demo/SquadView.gd` / `demo/SquadView.tscn`**:

   * Procedural 2D pitch rendering Starting XI nodes at normalized `formations.csv` coordinates alongside a 7-slot Bench and Reserves table.

   * **Drag-and-Drop (`set_drag_forwarding`) & Two-Click/Tap Swap with Strict GK-Only Slot `0` Lock**: `_can_swap_slots(slot_a, pos_a, slot_b, pos_b)` prevents dragging or clicking any non-`GK` outfielder into Starting XI Slot `0` (`GK`) or dragging a `GK` into outfield pitch slots (`1–10`), displaying a Coral warning banner (`"Only Goalkeepers (GK) can play in the GK slot!"`) on invalid attempts while highlighting valid swap targets with color-coded positional fit indicators (Pitch-Green natural, Amber compatible, Coral out-of-position).

   * Inline Captain/Set-Piece and Jersey Number (`1–99`) controls, plus `PlayerProfileModal` (`"SQUAD"` mode) inspection.

2. **`demo/PlayerProfileModal.gd` / `demo/PlayerProfileModal.tscn` (GK 6-Stat Display)**:

   * Checks `is_gk` / `position == "GK"` and dynamically switches the 6 attribute bars and labels from outfield (`Pace`, `Shooting`, `Passing`, `Dribbling`, `Defending`, `Physical`) to Goalkeeper (`Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, `Physical`).

3. **`demo/TacticsView.gd` / `demo/TacticsView.tscn`**:

   * Formation selector (`4-3-3`, `4-2-3-1`, `4-4-2`, `3-5-2`, `5-3-2`, etc.) with live 2D shape preview and 6 tactical sliders/selectors (`Mentality`, `Pressing`, `Defensive Line`, `Passing Style`, `Width`, `Tempo`) displaying live stamina-drain and tactical risk tooltips.

4. **`demo/YouthView.gd` / `demo/YouthView.tscn`**:

   * Academy Infrastructure panel (Facility/Scouting tiers, weekly upkeep, balance-gated upgrade buttons), Coaching Staff assignment panel, Prospect Roster table (`OVR`, `Potential Range`, `Dev Progress %`, `Condition`, `Age`), `PlayerProfileModal` (`"YOUTH"` mode) trigger, and one-click promotion action.

5. **`demo/CareerShell.gd` Integration**:

   * Replaced placeholder cards for `"squad"`, `"tactics"`, and `"youth"` with live view instances refreshing on `_ready()` and `visibility_changed`.

### C. Retained Phase 3B/3C UI Conventions (Enforced Across All Views)

* **`Label` Theme Overrides**: Always use `"normal"` `StyleBoxFlat` overrides (never `"panel"`) for pills, badges, and status tags.

* **Modal Centering**: Always configure `anchor_left/top/right/bottom = 0.5` with symmetric negative/positive offsets and `GROW_DIRECTION_BOTH`.

* **Route Normalization**: `"board"` resolves to `"club"` in both `InboxView` and `CareerShell.navigate_to()`.

## 3. Upcoming Engineering Blueprints & Technical Dependencies (Phases 5–7)

> **Status Note**: Phase 5A–5C source integrations remain subject to Godot runtime QA; authoritative UCL fixture generation and progression are deferred. Phase 5C-2 is implemented in source, but runtime QA remains pending. Phase 5D-1's 12-column registry and structured CSV parsing are present; Phase 5D-2's league-derived double round-robin source is committed at reviewed baseline `7b2e2e53`, but integration defects remain in dictionary mapping, stable ClubID/metadata propagation, strict league isolation and odd-sized pre-season pairing. The 106-club dataset's historical accuracy and 23 missing club rosters remain unaudited. Do not infer build/runtime verification from source inspection.

### Phase 5A–5B — Season Calendar Cycle & UEFA Champions League Specification
1. **July–May Season Calendar (`src/CareerState.cpp` & `demo/ScheduleView.gd`)**:
   * **Weeks 1–2 (July)**: Pre-Season Window with 2 scheduled friendlies (`is_friendly = true`, excluded from league standings points).
   * **Week 3+ (Mid-August `Aug 15` – `May 23`)**: Double round-robin domestic league season ending in **May** (`May 31` end-of-season review & champion crowning), followed by a June summer break before the July 1 rollover.
2. **UEFA Champions League Fixtures (`CompetitionRegistry` & `ScheduleView` / `CompetitionsView`)**:
   * Midweek continental fixtures (`September – May Final`) populated from the **top clubs of each European league** in `res://data/clubs.csv` (`Premier League`, `La Liga`, `Serie A`, `Bundesliga`, `Ligue 1`, `Primeira Liga`, `Eredivisie`, `Scottish Premiership`, `Belgian Pro League`, `Ukrainian Premier League`, `Danish Superliga`, `Süper Lig`, `Croatian First Football League`, `Eliteserien`, `Allsvenskan`).

### Phase 5C-2 — Main Menu, Application Startup & Career Entry Flow `[IMPLEMENTED IN SOURCE; RUNTIME QA PENDING]`

- Make a dedicated application root / scene coordinator the `project.godot` main scene. F5 should open **New Career**, **Continue Career**, **Load Career**, **Settings**, and **Exit**.
- Continue is available only when a valid save exists; Load lists save slots and metadata and handles invalid/incompatible saves safely.
- New Career opens the setup wizard. Phase 5D supplies the historical 15 July 2006 club/league registry and completes `League -> Club -> Manager Name`.
- After successful career creation or save loading, automatically open the `CareerShell` Dashboard. `KEY_C` must not be required to access the primary career interface.
- The app root/coordinator owns Main Menu, CareerShell, and MatchScene transitions. It provides an explicit MatchScene reference or persistent scene lifecycle; do not rely on standalone-scene assumptions, fragile parent traversal, or a global group lookup alone.
- Preserve Dashboard / Schedule -> Pre-Match -> Match -> Post-Match -> refreshed CareerShell. Handle missing saves, failed transitions, and unsaved progress safely.
- Acceptance tests: F5 with no saves; New / Continue / Load; automatic Dashboard entry; match launch from Dashboard and Schedule; PostMatchModal return; desktop and narrow viewport transitions. Update `project.godot` only after the new coordinator and routes are implemented. Source inspection alone is not runtime QA.

### Phase 5D — Historical 15 July 2006 Club Registry, Structured Player Data, New Career Bootstrap & League Fixtures

**5D-1 — Club Registry & Player Metadata**
- `demo/data/clubs.csv` now contains **106 data rows** and the 12-column schema: `ClubID,Club,League,ShortCode,Country,Reputation,TotalBalance,TransferBudget,WageBudget,DefaultFormation,PrimaryColor,SecondaryColor`.
- Source-level CSV audit: **28 distinct league labels**, **11 single-club leagues**, no duplicate ClubIDs/names, and no malformed column counts. Historical accuracy for 15 July 2006 remains unverified.
- `demo/data/players_2006_with_price.csv` contains **802 player rows**. Every player Club value matches a registry club name, but **23 clubs have no player rows** and currently rely on generated Academy Reserve fillers to reach 11 players.
- `Players_Database/player_database.cpp` parses structured club records and player records: IDs, age, nationality, club/league, outfield/GK attributes, price, wage, existing Stamina and Playstyle metadata. Do not add a duplicate Stamina column.
- `NHHA72528` persistence, Tuesday League / 11A7 gating and `RedeemCodeModal` are present in source. Reward mapping still requires QA.

**5D-2 — Structured Bootstrap & League-Scoped Fixture Generation**
- **Source implementation committed:** `a32b521fde9cd013edaa8f9c01ab93945666a099`, merged into reviewed main baseline `7b2e2e53e20ca03d82ed4e22680ee638ee1d2a61`.
- Implemented in source: structured PlayerRecord/ClubRecord parsing and query APIs; league-derived participant selection; double round-robin fixture generation with a bye sentinel for odd team counts; dynamic competition labels and matchday counts; club-name-based stadium fallback; and preservation of player retirement age in career saves.
- **Critical dictionary contract mismatch:** `PlayerDatabase::club_to_dict()` returns `club`, `reputation`, `total_balance`, `transfer_budget` and `wage_budget`, while `CareerState::start_new_career()` and `get_leagues_and_clubs()` still read `name`, `rating` and `budget`. Club names can therefore become empty, breaking New Career entries, the club-name index and player-to-team assignment. Align the contracts or consume `ClubRecord` directly.
- **Club identity/metadata propagation incomplete:** CSV `ClubID` is parsed but CareerState assigns sequential IDs. The bootstrap also fails to consume separate finance fields, default formation and CSV secondary kit colour; several values are defaulted or derived.
- **League isolation defect:** if the managed league has fewer than two clubs, `_generate_season_fixtures()` adds clubs from other leagues (up to 16). The registry currently has 11 single-club leagues, so this creates cross-league fixtures/standings. Do not borrow clubs from another league; require two clubs or implement a separate explicit exhibition-opponent model.
- **Odd-sized pre-season defect:** the trailing club is paired with the first club even though the first club already appears in a normal pair in the same round. Replace this with bye-aware pairing so no club is scheduled twice in a pre-season round.
- **Roster coverage:** 23 clubs have no imported player rows and receive generated Academy Reserve players. Complete the historical rosters or explicitly document the fallback.
- **Regression gap:** `demo/CareerRegressionTest.gd` currently contains only the older Phase 3 baseline suites (1–8). It still expects a 2026-07-01 start and a fixed 16-club table; it has no Phase 5D-2 tests for 2006-07-15 startup, ClubID preservation, club dictionary mapping, league isolation, even/odd fixture invariants or roster coverage.

**Required 5D-2 exit criteria**
1. Align club dictionary keys and CareerState consumers; verify visible club names, correct League -> Club choices and correct player-to-club assignment.
2. Preserve CSV ClubID and load all Phase 5D club metadata intended for the bootstrap.
3. Keep fixtures and standings strictly inside the selected league. A league with fewer than two clubs must not borrow clubs from another league.
4. For even and odd league sizes, verify every club pair meets exactly twice (one home, one away), no club plays twice on one matchday, and each odd-sized round has exactly one bye.
5. Ensure pre-season friendlies also have no double-booked clubs.
6. Expand regression coverage for CSV parsing, 2006-07-15 bootstrap, club/player mapping, league isolation, fixture invariants, standings membership and Save/Load.
7. Run the MinGW/UCRT64 SCons build and Godot 4.7 regression/runtime checks. Source inspection is not verification.

**Historical/data audit still outstanding:** verify all 106 clubs and 28 league labels against 15 July 2006; resolve the single-club leagues deliberately; complete the 23 missing club rosters.

### Phase 5E — `MatchEngine` Simulation Realism, Attribute-Coupled Chances & Controls Cleanup

1. **Dependencies & Partial Support**:

   * Depends on Phase 5D loading the CSV `Stamina` attribute and club kit colours onto `Team` / `Player`.

   * Partially supported by Phase 2A's chance-gated state machine (`CHANCE_NONE` / `HOME_ATTACK` / `AWAY_ATTACK`), stamina/tactics coupling, and direct control bindings in `MatchEngine.cpp`.

2. **Planned Deliverables**:

   * **CSV `Stamina`-Coupled Match Drain (`MatchEngine.cpp`)**: Scales per-tick sprint/pressing stamina depletion inversely with each player's imported CSV `Stamina` value ($1\text{--}99$) and exports exact ending `current_stamina` at full-time.

   * **Attribute & OVR-Driven Attack/Defense Chances (`MatchEngine.cpp`)**: Evaluates `Shooting`, `Pace`, `Dribbling`, `Passing`, `Defending`, `Physical`, and positional sub-weights alongside `OVR` so individual attributes shape shot selection, dribbling, interceptions, and tackles without making `OVR` deterministic.

   * **Scaled Opponent Defensive AI & Balanced Chance Probability**: Scales defensive compactness, pressing triggers, and lane coverage with opponent `OVR` and tactics while keeping weaker teams competitive.

   * **Real-Time Believable Movement**: Replaces instant point-to-point movement with continuous 2D acceleration, deceleration, turn-rate inertia, and `Pace`/stamina-scaled top speed across mouse and touchscreen inputs.

   * **Remove Advanced Dribbling (`R`)**: Strips `KEY_R` Advanced Dribbling bindings and HUD hints while preserving standard dribbling and match gestures.

### Phase 5F — Post-Match Stamina Persistence, $\approx 30\%$/Day Recovery & Injury System

1. **Dependencies & Partial Support**:

   * Depends on Phase 5E exporting post-match `current_stamina` and match injury events, and integrates with `CareerState::advance_day()`, `SquadView`, `PreMatchView`, `InboxView` (which already has an `"INJURY"` filter chip), and `save_career()` / `load_career()`.

2. **Planned Deliverables**:

   * **Stamina Persistence & Configurable Calendar Recovery**: Preserves post-match stamina without automatic full resets and recovers `stamina_recovery_rate_per_day` (default `0.30` / $30\%$ of max stamina per in-game calendar day, tunable in `0.25–0.30` range, capped at $100\%$).

   * **World Soccer Champs-Inspired Injury System**: Match-sustained injuries with severity-based day durations (`Minor Knock`, `Muscle Strain`, `Ligament Sprain`, `Severe Injury`), daily calendar countdown in `advance_day()`, squad selection lock while injured, `"INJURY"` Inbox alerts, and JSON save persistence.

### Phase 5G — Audio System, Stadium Atmosphere, Contextual Commentary & Goal Celebrations

1. **Dependencies & Partial Support**:

   * Depends on Phase 5C (`PreMatchView` / `PostMatchModal` match-importance metadata) and Phase 5E (`MatchEngine` structured event hooks for goals, saves, fouls, and substitutions).

   * Partially supported by Phase 2B's `MatchHud` commentary strip, which will be synchronized with the new contextual commentary and audio event dispatcher.

2. **Planned Deliverables**:

   * **`demo/AudioManager.gd` & `demo/assets/audio/`**: Autoload audio controller managing `Music`, `Atmosphere`, `SFX`, and `Commentary` buses, volume/mute controls in the main menu and `CareerShell`, and clean crossfading.

   * **Menu Music**: Loops main menu and career management background music across `CareerShell` views, fading out cleanly when launching a match and resuming after `PostMatchModal`.

   * **Match & Stadium Atmosphere (Importance-Scaled)**: Continuous crowd ambience loop throughout matches modulated by final-third danger and fixture `match_importance` (`STANDARD`, `RIVALRY_DERBY`, `TOP_CLASH`, `TITLE_DECIDER`, `CUP_FINAL`).

   * **Contextual Commentary**: Broadcast/audio cues and synchronized `MatchHud` ticker callouts triggered on `GOAL`, `SAVE`, `FOUL`, `CARD`, and `SUBSTITUTION` events.

   * **Distinct Home & Away Goal Announcements**: Home goals trigger stadium PA goal horn/anthem + massive home roar + home scoreboard pulse; away goals trigger distinct away-end cheer + subdued home crowd reaction.

   * **Goal Celebrations (`3.0–5.0s` Touchline Run)**: `MatchEngine` enters a `GOAL_CELEBRATION` state for `3.0–5.0` real-time seconds before kickoff reset, during which the scorer sprints toward the touchline/corner flag as teammates converge alongside crowd eruption SFX and pitch visual reactions.

   * **Full-Time Audio**: Referee final whistle SFX + result-aware post-match crowd reaction (home win celebration, draw applause, or home defeat groan) persisting through `PostMatchModal`.

### Phase 6B & 6C — Senior Squad Training Tab & Global UI/Typography Overhaul

1. **Dependencies & Partial Support**:

   * **Training (`Phase 6B`)**: Builds on Phase 4A's weekly XP progression pattern and Phase 6A's `Training Complex` facility tier, extending `CareerShell` with a `"training"` route (`demo/TrainingView.gd` / `.tscn`) and saving assignments/XP in `<slot>.json`.

   * **UI & Typography (`Phase 6C`)**: Builds on Phase 3B/3C's `StyleBoxFlat` `"normal"` `Label` rules and centered modal standards across all views.

2. **Planned Deliverables**:

   * **Dedicated `Training` Tab (`TrainingView`)**: UI-only positional group session assignment for **Attackers (`ST, CF, LW, RW`)**, **Midfielders (`CDM, CM, CAM, LM, RM`)**, **Defenders (`CB, LB, RB`)**, and **Goalkeepers (`GK`)**, driving gradual, potential- and age-differentiated attribute and OVR growth over in-game calendar days.

   * **UI & Typography Design System**: Bundled clean geometric/condensed font stack + tabular numerals, redesigned high-clarity player cards, and strict avoidance of AI-slop aesthetics (no heavy gradients, glowing drop-shadows, over-rounded cards, or generic dashboard clutter).

### Phase 7E & 7F — Exclusive $> 96$ Base OVR Playstyles & Mandatory Age-37 Retirement

* **Phase 7E (`PlaystylesDescription.txt`)**: Unlocks the 22 Playstyles exclusively for senior players with `BaseOVR > 96 && !IsAcademyProduct`, wiring `Playstyle 1..3` metadata into `MatchEngine` behaviors and `PlayerProfileModal` / `SquadView` badges.

* **Phase 7F (Mandatory Age-37 Retirement)**: Builds on the committed `RetirementAge = 37` data/save normalization to add contract length clamping ($\min(5, \max(1, 37 - \text{Age}))$), `"Retiring at End of Season"` status badges at `Age == 36`, July 1 rollover removal of `Age >= 37` players from active squads/market/match payloads, automatic Best-XI rebalance (preserving `GK` in Slot `0`), and a `"BOARD"` Inbox retirement report.

## 4. Technical Risks & Required Regression Tests (Phases 5–7)

### Key Technical Risks to Monitor

1. **CSV `Stamina` vs. `Physical` Mapping**: `PlayerAttributes` currently stores 6 core attributes (`Pace`, `Shooting`, `Passing`, `Dribbling`, `Defending`, `Physical`). Binding the existing CSV `Stamina` column onto a dedicated `int Stamina` field on `Player` ensures match endurance scales accurately without distorting `Physical` duel calculations or GK 6-stat mappings.

2. **Match Stamina & Recovery Balance**: If `MatchEngine` drains $> 75\%$ stamina in a single 90-minute match while recovery is $30\%$/day, congested 3-day fixture turnarounds could leave starters below $60\%$ stamina unless `BaseDrain` and `stamina_recovery_rate_per_day` are tuned together.

3. **Real-Time Movement Responsiveness vs. Inertia**: Excessive turn-rate or deceleration damping in `MatchEngine` could make direct user movement feel sluggish; turning/acceleration constants must be profiled on both mouse-click and Android virtual stick inputs.

4. **`GOAL_CELEBRATION` State Timer & Match Clock Integrity**: During the `3.0–5.0s` touchline goal celebration run in `MatchEngine.cpp`, active ball physics, shot scoring, and match clock progression must remain cleanly paused so goals cannot double-trigger and celebration timers always expire deterministically even when skipping or pausing.

5. **Injury & Age-37 Retirement Squad Soft-Lock Edge Cases**: If multiple injuries or July 1 retirements thin out a squad (especially `GK`s), `CareerState` must ensure a club always retains at least 11 available players (including a `GK` in Slot `0`) via emergency youth/reserve call-up fallback so matches cannot soft-lock.

### Planned Regression Suites for Upcoming Phases

* **Phase 5 Regression Suites**:

  * Historical **15 July 2006** `clubs.csv` load (`NewCareerView` `League -> Club`) & kit colour propagation to `MatchEngine`.

  * CSV `Stamina` column differential drain test (90-rated `Stamina` player vs. 50-rated `Stamina` player over identical 90-minute simulation).

  * Post-match stamina persistence + 3-day `advance_day()` recovery ($+30\%$/day capped at $100\%$) + Save/Load round-trip.

  * Injury lifecycle test (sustained in match -> blocked in `SquadView`/`PreMatchView` -> daily countdown in `advance_day()` -> cleared on day `0` -> Save/Load persistence).

  * `KEY_R` Advanced Dribbling removal check (no action trigger, intact pass/shot/tackle/dribble controls) and real-time steering movement velocity ramp verification.

  * `GOAL_CELEBRATION` `3.0–5.0s` touchline run timer & kickoff reset verification, contextual event emission (`GOAL`, `SAVE`, `FOUL`, `SUBSTITUTION`), home vs. away goal announcement routing, and `AudioManager` menu/match bus crossfade checks.

* **Phase 6 & 7 Regression Suites**:

  * `TrainingView` 4-group assignment persistence, OVR recalculation after attribute XP thresholds, and age/potential growth differentiation.

  * 6-criteria Transfer Search (`Name`, `Club`, `Age`, `Nationality`, `League`, `Position`), `NHHA72528` unlock gate, $> 96$ Base OVR Playstyles gate, and July 1 Age-37 mandatory retirement.

## 5. File Inventory & Cleanup Checklist

### Active Project Files

* **C++23 GDExtension Core (`src/`)**:

  * `src/MatchEngine.h` / `src/MatchEngine.cpp` (Phase 1–2C simulation engine; planned Phase 5E stamina/attribute/AI/movement/R-removal; planned Phase 5G `GOAL_CELEBRATION` 3–5s state & event hooks; planned Phase 7E Playstyle hooks)

  * `src/CareerState.h` / `src/CareerState.cpp` (Phase 3A & Phase 4A central career state, GK-only Slot `0` guard, GK 6-stat export, player metadata & Age-37 save normalization; planned Phase 5D `clubs.csv` & Codes; planned Phase 5F/5G/6B/7E/7F systems)

  * `src/mainplayer.h` / `src/mainplayer.cpp` (GK 6-stat attribute accessors, player metadata, `RetirementAge = 37`; planned `Stamina`, `CurrentStamina`, `IsInjured`, `IsRetired`) & `src/team.h` / `src/team.cpp`

* **Data & CSV Registries (`demo/data/` & `Players_Database/`)**:

  * `demo/data/formations.csv` (Normalized 11-slot formation shapes)

  * `demo/data/clubs.csv` (Phase 5D 12-column registry; 106 records / 28 league labels; historical audit and 23 club rosters remain outstanding)

  * `demo/data/players_2006_with_price.csv` (`res://data/players_2006_with_price.csv` — active runtime dataset with `Club`, `League`, `Nationality`, `Stamina`, and `Playstyle 1..3`)

  * `Players_Database/players_2006_with_price.csv` (retained separate preventive/source copy)

  * `demo/data/PlaystylesDescription.txt` (22-Playstyle reference; gameplay integration planned in Phase 7E)

* **GDScript UI & Scenes (`demo/`)**:

  * `demo/CareerShell.gd` / `demo/CareerShell.tscn` (Global route shell, header, modals, toast)

  * `demo/AudioManager.gd` & `demo/assets/audio/` (planned Phase 5G menu/career music, stadium ambience, commentary, goal announcements & full-time audio controller)

  * `demo/DashboardView.gd` / `demo/DashboardView.tscn` (Career Dashboard)

  * `demo/SquadView.gd` / `demo/SquadView.tscn` (Phase 4B Interactive 2D Pitch, drag-and-drop + click-swap GK-only Slot `0` lock, XI/Bench/Reserves, Roles & Kit Numbers)

  * `demo/TacticsView.gd` / `demo/TacticsView.tscn` (Phase 4B Permanent Club Formation & Tactical Sliders)

  * `demo/YouthView.gd` / `demo/YouthView.tscn` (Phase 4B Youth Academy Facilities, Coaches & Prospect Promotion)

  * `demo/InboxView.gd` / `demo/InboxView.tscn` (Split-pane News & Inbox)

  * `demo/PlayerProfileModal.gd` / `demo/PlayerProfileModal.tscn` (Universal 4-mode player modal with Goalkeeper 6-stat display: `Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, `Physical`)

  * `demo/CareerRegressionTest.gd` (current source contains Phase 3 baseline Suites 1–8; Phase 4/5D regression coverage is still to be added)

* **Documentation**:

  * `ROADMAP.md` (canonical project roadmap)

  * `DEVELOPMENT_STATUS.md` (this current engineering handoff)

### Duplicate / Legacy Backup Files Recommended for Removal in Phase 4C

* `demo/DashboardView_2.gd` / `demo/DashboardView_2.tscn`

* `demo/InboxView_2.gd` / `demo/InboxView_2.tscn`

* `demo/PlayerProfileModal_2.gd` / `demo/PlayerProfileModal_2.tscn`

* No numbered roadmap/status backup documents are present in the repository root at the reviewed commit; keep `ROADMAP.md` and `DEVELOPMENT_STATUS.md` as the canonical documents.

* *Cleanup Rule*: Run `rg "_2\.(gd|tscn)" demo/` to verify zero active `.tscn` `ext_resource` dependencies remain, then remove stale `*_2` files before committing Phase 4C.

## 6. Phase 4C Build & Test Verification Matrix

### Step 1 — Recompile C++23 GDExtension (Windows MinGW/GCC 16.1.0 UCRT64)

```powershell
scons platform=windows target=template_debug api_version=4.7 -j6
```

* **Expected Artifact**: `demo/bin/libfootball.windows.template_debug.x86_64.dll` updated with Phase 4A `CareerState` method bindings (`swap_squad_players` with strict GK-only Slot `0` guard, `_player_to_dict` GK 6-stat keys, `set_club_roles`, `set_player_jersey_number`, `get_club_tactics`, `set_club_tactics`, `get_youth_academy_info`, `upgrade_youth_facility`, `assign_youth_coach`, `promote_youth_player`) plus the `4aa3a2a4b227006758f2421c1042657c921b3fe6` player metadata loader and Age-37 save normalization.

### Step 2 — Automated Regression Verification (`demo/CareerRegressionTest.gd`)

| Suite | Scope | Key Assertions | 
| ----- | ----- | ----- | 
| **Suites 1–8** | Phase 3A–3D Baseline | Deterministic bootstrap, atomic Save/Load, `DashboardView` auto-refresh, `InboxView` chip reset & `"board"` -> `"club"`, `PlayerProfileModal` centering & 4 modes. | 
| **Suite 9** | `SquadView`, GK Lock & GK 6-Stat Profile | `XI <-> Bench <-> Reserves` swaps, strict rejection of non-`GK` drag/swap into Slot `0` (`GK`) and `GK` into Slots `1–10`, `GK <-> GK` swap allowance, `is_gk` 6-stat dictionary export (`diving`, `reflex`, `positioning`, `reaction`, `kicking`, `physical`), positional OVR penalty calculation, Captain/Set-Piece/Jersey (`1–99`) persistence, and `MatchEngine` lineup sync. | 
| **Suite 10** | `TacticsView` & Scope Isolation | Formation + 6 slider mutations persist across route switches and Save/Load without leaking temporary in-match `TACTIC` changes. | 
| **Suite 11** | `YouthView` & Academy Growth | Facility/Scouting upgrades deduct `club_balance` and fail gracefully on insufficient funds; coach assignment scales weekly XP; 7-day `advance_day()` increments training XP without premature age `+1`; annual rollover increments age; `promote_youth_player()` moves prospect to senior squad + posts Inbox alert. | 
| **Suite 12** | Phase 4 Full Round-Trip | Mutate XI, roles, kit numbers, permanent tactics, youth upgrades, and promote a prospect -> `save_career()` -> `load_career()` -> verify exact state parity. | 

### Step 3 — Manual Cross-Viewport & Interactive QA Checklist

1. **Desktop Viewport (`1280x720`)**:

   * Open `Squad` tab -> verify 11 pitch markers match `formations.csv` geometry, attempt to drag and click-swap an outfield player onto the `GK` slot (confirm it is blocked with the GK-only warning banner), swap a bench `GK` onto Slot `0` (confirm allowed), edit Captain and jersey number, and inspect a `GK` in `PlayerProfileModal` to verify the 6 Goalkeeper stats (`Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, `Physical`).

   * Open `Tactics` tab -> switch formation (`4-3-3` -> `4-2-3-1`), adjust `Pressing` and `Tempo` sliders, verify stamina/risk tooltip updates, switch away and back to confirm persistence.

   * Open `Youth` tab -> upgrade Academy Facility (confirm top header balance pill decrements immediately), assign a youth coach, advance 7 days via `+1 Day` to verify weekly training progress ticks, and promote an eligible prospect (confirm unread Inbox badge increments and prospect appears in `Squad` reserves).

2. **Narrow / Mobile Viewport (`800x480` & Portrait)**:

   * Confirm `CareerShell` sidebar collapses into drawer (`< 780px`), `SquadView` pitch + bench stack cleanly without horizontal overflow, and `PlayerProfileModal` remains centered within viewport bounds.