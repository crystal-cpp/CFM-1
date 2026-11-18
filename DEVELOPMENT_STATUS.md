# CFM-1 Development Status & Engineering Handoff

**Last Updated**: 2026-11-18  
**Reviewed Repository Baseline**: `b62b97bc0e659e634d8206893776ae09d3beab16` *(Expand roadmap and add regression runner; build verification pending)*  
**Current Phase**: Phase 4 — Team Management, Permanent Tactics & Youth Academy UI  
**Current Milestone**: **Phase 4A & 4B Implemented -> Entering Phase 4C (SCons Build, Automated Regression Suites 9–12 & Manual Verification)**

---

## 1. Milestone & Subsystem Status Matrix

| Subsystem / Milestone | Layer | Implementation Status | Verification Status | Notes |
| :--- | :---: | :---: | :---: | :--- |
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
| **Phase 4C: Phase 4 Build & Acceptance Suite** | Both | **Ready to Run** | **Active (Next)** | Recompile DLL + run Suites 1–12 + manual cross-viewport QA + `*_2` cleanup. |
| **Phase 5A–5C: Schedule, Competitions & Pre-Match** | Both | Placeholder Views | Scheduled | Scaffolded as route cards inside `CareerShell.gd`. |
| **Phase 5D: Historical 15 July 2006 `clubs.csv`, Player Metadata & Codes** | Both | **Partially Implemented** | **Build Verification Pending** | Runtime CSV loader now reads `Club`, `League`, `Nationality`, and `Playstyle 1..3`, and normalizes `RetirementAge = 37` in CSV import and career saves (`res://data/players_2006_with_price.csv` is the runtime dataset; `Players_Database/players_2006_with_price.csv` is retained as a separate preventive/source copy). Historical **15 July 2006** `clubs.csv` + kit colours, existing CSV `Stamina` attribute binding, New Career `League -> Club` selection, `Tuesday League` / `11A7` lock, and `NHHA72528` Codes modal remain planned. |
| **Phase 5E: `MatchEngine` Realism, Stamina Drain & Controls Cleanup** | Both | **Spec Ready** | **Scheduled (Phase 5)** | CSV `Stamina`-driven match drain, attribute-coupled attack/defense chances, scaled opponent defensive AI, balanced chance probability, real-time steering movement, and removal of Advanced Dribbling (`R`). |
| **Phase 5F: Stamina Persistence, $\approx 30\%$/Day Recovery & Injury System** | Both | **Spec Ready** | **Scheduled (Phase 5)** | Post-match stamina preservation, configurable $\approx 30\%$/day calendar recovery, and World Soccer Champs-inspired injury generation & calendar recovery. |
| **Phase 6A: Club Management, Finances & Board** | Both | Placeholder Views | Scheduled | Separated financial ledgers, board confidence, stadium/facility upgrades. |
| **Phase 6B: Senior Squad Training Mechanism & `Training` Tab** | Both | **Spec Ready** | **Scheduled (Phase 6)** | Dedicated `Training` tab with 4 positional group focus programs (Attackers, Midfielders, Defenders, Goalkeepers) and potential/age-aware OVR growth. |
| **Phase 6C: Global UI & Typography Design System** | GDScript | **Spec Ready** | **Scheduled (Phase 6)** | Custom typography hierarchy, redesigned high-clarity player cards, and disciplined non-AI-slop dark-navy aesthetic. |
| **Phase 7A–7D: Transfers, Search & Reputation** | Both | Placeholder Views | Scheduled | Valuation, fee/wage negotiations, transfer windows, six-field player search, AI transfers, loyalty, and reputation. |
| **Phase 7E: Exclusive $> 96$ Base OVR Playstyles** | Both | **Spec Ready** | **Scheduled (Phase 7)** | 22 Playstyles (`PlaystylesDescription.txt`) gated strictly to `BaseOVR > 96` non-academy players. |
| **Phase 7F: Mandatory Age-37 Retirement Pipeline** | Both | **Partially Implemented (Data/Save)** | **Scheduled (Phase 7)** | `RetirementAge = 37` normalization committed in CSV loader & saves; contract length clamping, `Age == 36` badge, July 1 rollover removal + Best-XI rebalance, and Board retirement report remain scheduled. |

---

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

---

## 3. Upcoming Engineering Blueprints & Technical Dependencies (Phases 5–7)

> **Status Note**: Except for the Phase 5D CSV metadata loader and Age-37 save normalization committed at `4aa3a2a4b227006758f2421c1042657c921b3fe6` (marked *Build Verification Pending*), every feature in Section 3 is **Planned / Not Started**. None of these remaining systems have been implemented, compiled, or verified yet.

### Phase 5D — Historical 15 July 2006 `res://data/clubs.csv`, Player CSV Metadata (`Stamina` + `Club`/`League`/`Nationality`) & Codes (`NHHA72528`)
1. **Current Implementation vs. Remaining Dependencies**:
   * **Committed (`4aa3a2a4b227006758f2421c1042657c921b3fe6` — Build Verification Pending)**:
     * `PlayerDatabase::load_from_csv()` now resolves core and optional columns by header name, reads `Club`, `League`, `Nationality`, `Playstyle 1..3`, and optional `Price`, preserves metadata on each `Player`, and normalizes `RetirementAge = 37`.
     * `CareerState` exports player metadata in `_player_to_dict()` and persists/normalizes `RetirementAge = 37` in `save_career()` / `load_career()`.
     * `res://data/players_2006_with_price.csv` (`demo/data/players_2006_with_price.csv`) is the runtime dataset; `Players_Database/players_2006_with_price.csv` is intentionally retained as a separate preventive/source copy and does not need to mirror the runtime file row-for-row.
   * **Remaining Data-Preparation Requirement (Blocking Phase 5D)**:
     * `demo/data/clubs.csv` (`res://data/clubs.csv`) does not yet exist in the repository and must be authored and audited for **15 July 2006** pre-season accuracy (`ClubID,Club,League,ShortCode,Country,Reputation,TotalBalance,TransferBudget,WageBudget,DefaultFormation,PrimaryColor,SecondaryColor`), including balanced historical budgets/reputations, valid `formations.csv` formations, and primary/secondary kit colours.
   * **Existing CSV `Stamina` Column Mapping**:
     * `res://data/players_2006_with_price.csv` already contains a `Stamina` column; `PlayerDatabase::load_from_csv()` must bind that existing `Stamina` column directly onto `Player::Stamina` (`1–99`) without adding a duplicate CSV column.
2. **Remaining Planned Deliverables**:
   * **Historical 15 July 2006 Start (`NewCareerView.gd` & `CareerState::start_new_career()`)**: Anchors new career saves to **`2006-07-15` (Season `2006/07`)**, renders the `League -> Club -> Manager Name` selector strictly from `res://data/clubs.csv` (eliminating `DEFAULT_CLUB_NAMES[16]` and preventing random club creation), routes CSV players into their matching `Team` (unmatched rows to Free Agents `club_id = 0`), and passes `PrimaryColor` / `SecondaryColor` kit colours into `MatchEngine` match presentation (with automatic away clash-kit switching).
   * **Codes Mechanism (`NHHA72528`) & `Tuesday League` / `11A7` Lock**: Adds `Codes` button in `CareerShell` + `RedeemCodeModal` backed by `CareerState::redeem_promo_code("NHHA72528")` to unlock `Tuesday League -> 11A7` globally and credit `+$50.0M` Club Balance, `+$35.0M` Transfer Budget, `+$250K/wk` Wage Budget, `+5` Skip Tokens, and `+10` Club Reputation.
   * **Custom Players & Classmates**: Supports appending custom player rows in `res://data/players_2006_with_price.csv` (`Club = "11A7"`, `League = "Tuesday League"`, or any canonical `clubs.csv` club/league), with $\text{BaseOVR} > 96$ players qualifying for Phase 7E Playstyles.

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

---

## 4. Technical Risks & Required Regression Tests (Phases 5–7)

### Key Technical Risks to Monitor
1. **CSV `Stamina` vs. `Physical` Mapping**: `PlayerAttributes` currently stores 6 core attributes (`Pace`, `Shooting`, `Passing`, `Dribbling`, `Defending`, `Physical`). Binding the existing CSV `Stamina` column onto a dedicated `int Stamina` field on `Player` ensures match endurance scales accurately without distorting `Physical` duel calculations or GK 6-stat mappings.
2. **Match Stamina & Recovery Balance**: If `MatchEngine` drains $> 75\%$ stamina in a single 90-minute match while recovery is $30\%$/day, congested 3-day fixture turnarounds could leave starters below $60\%$ stamina unless `BaseDrain` and `stamina_recovery_rate_per_day` are tuned together.
3. **Real-Time Movement Responsiveness vs. Inertia**: Excessive turn-rate or deceleration damping in `MatchEngine` could make direct user movement feel sluggish; turning/acceleration constants must be profiled on both mouse-click and Android virtual stick inputs.
4. **Injury & Age-37 Retirement Squad Soft-Lock Edge Cases**: If multiple injuries or July 1 retirements thin out a squad (especially `GK`s), `CareerState` must ensure a club always retains at least 11 available players (including a `GK` in Slot `0`) via emergency youth/reserve call-up fallback so matches cannot soft-lock.

### Planned Regression Suites for Upcoming Phases
* **Phase 5 Regression Suites**:
  * Historical **15 July 2006** `clubs.csv` load (`NewCareerView` `League -> Club`) & kit colour propagation to `MatchEngine`.
  * CSV `Stamina` column differential drain test (90-rated `Stamina` player vs. 50-rated `Stamina` player over identical 90-minute simulation).
  * Post-match stamina persistence + 3-day `advance_day()` recovery ($+30\%$/day capped at $100\%$) + Save/Load round-trip.
  * Injury lifecycle test (sustained in match -> blocked in `SquadView`/`PreMatchView` -> daily countdown in `advance_day()` -> cleared on day `0` -> Save/Load persistence).
  * `KEY_R` Advanced Dribbling removal check (no action trigger, intact pass/shot/tackle/dribble controls) and real-time steering movement velocity ramp verification.
* **Phase 6 & 7 Regression Suites**:
  * `TrainingView` 4-group assignment persistence, OVR recalculation after attribute XP thresholds, and age/potential growth differentiation.
  * 6-criteria Transfer Search (`Name`, `Club`, `Age`, `Nationality`, `League`, `Position`), `NHHA72528` unlock gate, $> 96$ Base OVR Playstyles gate, and July 1 Age-37 mandatory retirement.

---

## 5. File Inventory & Cleanup Checklist

### Active Project Files
* **C++23 GDExtension Core (`src/`)**:
  * `src/MatchEngine.h` / `src/MatchEngine.cpp` (Phase 1–2C simulation engine; planned Phase 5E stamina/attribute/AI/movement/R-removal; planned Phase 7E Playstyle hooks)
  * `src/CareerState.h` / `src/CareerState.cpp` (Phase 3A & Phase 4A central career state, GK-only Slot `0` guard, GK 6-stat export, player metadata & Age-37 save normalization; planned Phase 5D `clubs.csv` & Codes; planned Phase 5F/6B/7E/7F systems)
  * `src/mainplayer.h` / `src/mainplayer.cpp` (GK 6-stat attribute accessors, player metadata, `RetirementAge = 37`; planned `Stamina`, `CurrentStamina`, `IsInjured`, `IsRetired`) & `src/team.h` / `src/team.cpp`
* **Data & CSV Registries (`demo/data/` & `Players_Database/`)**:
  * `demo/data/formations.csv` (Normalized 11-slot formation shapes)
  * `demo/data/clubs.csv` (planned Phase 5D historical **15 July 2006** club/league & kit colour registry; data preparation required)
  * `demo/data/players_2006_with_price.csv` (`res://data/players_2006_with_price.csv` — active runtime dataset with `Club`, `League`, `Nationality`, `Stamina`, and `Playstyle 1..3`)
  * `Players_Database/players_2006_with_price.csv` (retained separate preventive/source copy)
  * `demo/data/PlaystylesDescription.txt` (22-Playstyle reference; gameplay integration planned in Phase 7E)
* **GDScript UI & Scenes (`demo/`)**:
  * `demo/CareerShell.gd` / `demo/CareerShell.tscn` (Global route shell, header, modals, toast)
  * `demo/DashboardView.gd` / `demo/DashboardView.tscn` (Career Dashboard)
  * `demo/SquadView.gd` / `demo/SquadView.tscn` (Phase 4B Interactive 2D Pitch, drag-and-drop + click-swap GK-only Slot `0` lock, XI/Bench/Reserves, Roles & Kit Numbers)
  * `demo/TacticsView.gd` / `demo/TacticsView.tscn` (Phase 4B Permanent Club Formation & Tactical Sliders)
  * `demo/YouthView.gd` / `demo/YouthView.tscn` (Phase 4B Youth Academy Facilities, Coaches & Prospect Promotion)
  * `demo/InboxView.gd` / `demo/InboxView.tscn` (Split-pane News & Inbox)
  * `demo/PlayerProfileModal.gd` / `demo/PlayerProfileModal.tscn` (Universal 4-mode player modal with Goalkeeper 6-stat display: `Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, `Physical`)
  * `demo/CareerRegressionTest.gd` (Automated regression Suites 1–12)
* **Documentation**:
  * `ROADMAP.md` (canonical project roadmap)
  * `DEVELOPMENT_STATUS.md` (this current engineering handoff)

### Duplicate / Legacy Backup Files Recommended for Removal in Phase 4C
* `demo/DashboardView_2.gd` / `demo/DashboardView_2.tscn`
* `demo/InboxView_2.gd` / `demo/InboxView_2.tscn`
* `demo/PlayerProfileModal_2.gd` / `demo/PlayerProfileModal_2.tscn`
* No numbered roadmap/status backup documents are present in the repository root at the reviewed commit; keep `ROADMAP.md` and `DEVELOPMENT_STATUS.md` as the canonical documents.
* *Cleanup Rule*: Run `rg "_2\.(gd|tscn)" demo/` to verify zero active `.tscn` `ext_resource` dependencies remain, then remove stale `*_2` files before committing Phase 4C.

---

## 6. Phase 4C Build & Test Verification Matrix

### Step 1 — Recompile C++23 GDExtension (Windows MinGW/GCC 16.1.0 UCRT64)
```powershell
scons platform=windows target=template_debug api_version=4.7 -j6
```
* **Expected Artifact**: `demo/bin/libfootball.windows.template_debug.x86_64.dll` updated with Phase 4A `CareerState` method bindings (`swap_squad_players` with strict GK-only Slot `0` guard, `_player_to_dict` GK 6-stat keys, `set_club_roles`, `set_player_jersey_number`, `get_club_tactics`, `set_club_tactics`, `get_youth_academy_info`, `upgrade_youth_facility`, `assign_youth_coach`, `promote_youth_player`) plus the `4aa3a2a4b227006758f2421c1042657c921b3fe6` player metadata loader and Age-37 save normalization.

### Step 2 — Automated Regression Verification (`demo/CareerRegressionTest.gd`)
| Suite | Scope | Key Assertions |
| :---: | :--- | :--- |
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