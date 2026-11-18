# CFM-1 Development Status & Engineering Handoff

**Last Updated**: 2026-11-18  
**Reviewed Repository Baseline**: `4aa3a2a4b227006758f2421c1042657c921b3fe6` *(metadata loader and age-37 save normalization committed; build verification pending)*  
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
| **Phase 5D: Player Metadata, Club Registry & Codes** | Both | **Partially Implemented** | **Build Verification Pending** | Runtime CSV loader now reads `Club`, `League`, `Nationality`, and `Playstyle 1..3`; metadata is included in player dictionaries and career saves. `res://data/players_2006_with_price.csv` remains the runtime dataset. `Players_Database/players_2006_with_price.csv` is intentionally retained as a separate preventive/source copy. `clubs.csv`, New Career selection, access lock, and code redemption remain planned. |
| **Phase 6: Club Management, Finances & Board** | Both | Placeholder Views | Scheduled | Separated financial ledgers, board confidence, stadium/facility upgrades. |
| **Phase 7A–7D: Transfers, Search & Reputation** | Both | Placeholder Views | Scheduled | Valuation, fee/wage negotiations, transfer windows, six-field player search, AI transfers, loyalty, and reputation. |
| **Phase 7E: Exclusive $> 96$ Base OVR Playstyles** | Both | **Spec Ready** | **Scheduled (Phase 7)** | 22 Playstyles (`PlaystylesDescription.txt`) gated strictly to `BaseOVR > 96` non-academy players. |
| **Phase 7F: Mandatory Age-37 Retirement Pipeline** | Both | **Spec Ready** | **Scheduled (Phase 7)** | Universal `RetirementAge = 37` cap, contract clamping, July 1 rollover removal + Best-XI rebalance, and Board retirement report. |

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

## 3. Upcoming Engineering Blueprints: Phase 5D, Phase 7E & Phase 7F

> The items in this section are specifications for future work unless their implementation is explicitly marked complete in the status matrix. Their presence here does not mean the systems are already implemented or tested.

### Phase 5D — Player Metadata Integration, `clubs.csv`, New Career League Selection & Codes (`NHHA72528`)
1. **Files to Create / Update**:
   * **`demo/data/clubs.csv` (`res://data/clubs.csv`)**: Canonical club registry (`ClubID,Club,League,ShortCode,Country,Reputation,TotalBalance,TransferBudget,WageBudget,DefaultFormation,PrimaryColor,SecondaryColor`). Replaces hardcoded `DEFAULT_CLUB_NAMES[16]` and prevents random club creation.
   * **Runtime player data (`demo/data/players_2006_with_price.csv`, loaded as `res://data/players_2006_with_price.csv`)**: Contains `Club`, `League`, `Nationality`, and `Playstyle 1..3`. `Players_Database/players_2006_with_price.csv` is a deliberately retained separate preventive/source copy; it is not required to mirror the runtime dataset row-for-row.
   * **`src/mainplayer.h` & `src/mainplayer.cpp`**: Add `ClubName`, `LeagueName`, `Nationality`, `BaseOVR`, `IsAcademyProduct`, `IsRetired`, `RetirementAge = 37`, and `Playstyles` fields and accessors.
   * **`src/team.h` & `src/team.cpp`**: Store `ShortCode`, `LeagueName`, `Country`, `Reputation`, `TransferBudget`, `WageBudget`, and kit colors loaded from `clubs.csv`.
   * **`Players_Database/player_database.h` & `Players_Database/player_database.cpp`**:
     * **Implemented, build verification pending:** `load_from_csv()` resolves core and optional columns by header name, reads `Club`, `League`, `Nationality`, `Playstyle 1..3`, and optional `Price`, and preserves metadata on each `Player`.
     * **Remaining:** add `load_clubs_from_csv()` for the future `res://data/clubs.csv` registry.
     * Imported players are normalized to `RetirementAge = 37`; legacy save loading also defaults and normalizes retirement age to `37`.
   * **`src/CareerState.h` & `src/CareerState.cpp`**:
     * In `start_new_career()`: Load `res://data/clubs.csv` first to create exact `Team` records, implement the New Career `League -> Club -> Manager Name` selection flow, map players by their CSV `Club` value (routing unmatched/empty rows to Free Agents), run positional Best-XI sorting (`GK` locked to Slot `0`), and serialize `"csv_club_name"`, `"base_ovr"`, `"is_academy_product"`, `"retirement_age"`, `"is_retired"`, `"league"`, `"nationality"`, `"short_code"`, `"playstyles"`, and `"redeemed_codes"` in `save_career()` / `load_career()`.
     * Implement `redeem_promo_code("NHHA72528")` to permanently unlock `Tuesday League` and `11A7` Club (and all `11A7` players) across New Career setup and Transfer Market searches while awarding `+$50.0M` Club Balance, `+$35.0M` Transfer Budget, `+$250K/wk` Wage Budget, `+5` Skip Tokens, and `+10` Club Reputation.
   * **GDScript Views (`NewCareerView.gd`, `RedeemCodeModal.gd`, `CareerShell.gd`, `DashboardView.gd`, `SquadView.gd`, `CompetitionsView.gd`, `TransfersView.gd`, `PlayerProfileModal.gd`)**: Consume `clubs.csv` short codes, league names, `Tuesday League` / `11A7` unlock state, and merged player `club_name`, `league`, and `nationality` metadata.
2. **Adding Custom Players & Classmates**:
   * Append custom player rows with `Club = "11A7"`, `League = "Tuesday League"` (or another canonical club/league from `res://data/clubs.csv`), plus the player's `Nationality`.
   * Any custom player authored with base attributes yielding $\text{BaseOVR} > 96$ (`97–99` OVR) automatically qualifies for **Phase 7E Playstyles**.

### Phase 7E — Exclusive $> 96$ Base OVR Playstyles System (`PlaystylesDescription.txt`)
1. **Strict Eligibility Gate**:
   * Playstyles are granted **exclusively** when `player.GetBaseOVR() > 96 && !player.GetIsAcademyProduct()`.
   * Academy players promoted from `YouthAcademy` (`IsAcademyProduct == true`) or normal players whose initial $\text{BaseOVR} \le 96$ never receive Playstyles even if trained past $96$ OVR.
2. **22 Supported Playstyles (`PlaystylesDescription.txt`)**:
   * *Scoring & Set Pieces*: `Finesse Expert`, `Clinical Finisher`, `Power Shot`, `Chip Shot`, `Penalty Expert`, `Dead Ball`.
   * *Passing & Vision*: `Tiki Taka`, `Bullet Pass`, `Whipped Cross`, `Incisive Through Pass`, `Driven Lob Through`.
   * *Defending*: `Anticipate`, `Guardian`, `Slide Tackle`.
   * *Dribbling & Physical*: `Rapid`, `Trickster`, `Bruiser`, `Accelerator`, `Relentless`.
   * *Aerial*: `Precision Header`.
   * *Goalkeeping*: `Deflector`, `Rush Out`.
3. **Files to Update in Phase 7E**:
   * `src/mainplayer.h` / `.cpp`: `uint32_t PlaystyleMask` + `std::vector<std::string> Playstyles` with `BaseOVR > 96 && !IsAcademyProduct` guard.
   * `Players_Database/player_database.cpp`: Parse `Playstyle 1..3` columns (or auto-assign 2–3 role-matched Playstyles for `BaseOVR > 96` players when omitted).
   * `src/MatchEngine.h` / `.cpp`: Apply Playstyle decision weights, shot curves, pass lane detection, tackle selection, sprint dribbling, and GK rush-out/deflection modifiers during match simulation.
   * `src/CareerState.cpp`: Include `"playstyles"`, `"base_ovr"`, and `"is_academy_product"` in `prepare_next_match_payload()`, `_player_to_dict()`, `save_career()`, and `load_career()`.
   * `demo/PlayerProfileModal.gd` & `demo/SquadView.gd`: Render Playstyle badges and descriptions from `PlaystylesDescription.txt` on eligible $> 96$ Base OVR players.

### Phase 7F — Mandatory Age-37 Retirement & Squad Turnover Pipeline
1. **Universal Age-37 Retirement Rule (`src/mainplayer.h`, `Players_Database/player_database.cpp`, `src/CareerState.cpp`)**:
   * Every player in the database (CSV imports from `players_2006_with_price.csv`, custom classmates/icons, Youth Academy graduates, and fallback/generated players) has a fixed mandatory retirement age of **`37`** (`retirement_age = 37`), overriding any legacy `36` defaults or per-row CSV `RetirementAge` variance.
   * Players at `Age == 36` display a `"Retiring at End of Season"` status badge in `PlayerProfileModal` and `SquadView` and are blocked from incoming transfer bids during their final season.
2. **Contract Length Cap (`NegotiationModal` & `CareerState`)**:
   * Maximum contract duration during transfer negotiations and contract renewals is clamped to $\min(5, \max(1, 37 - \text{Age}))$ years so every player's contract terminates at or before age `37`.
3. **Annual Rollover Retirement Execution (`CareerState::_process_annual_rollover()` on July 1)**:
   * During the July 1 season boundary (`_month == 7 && _day == 1`), after incrementing every player's age (`Age + 1`), any player reaching **`Age >= 37`** immediately triggers retirement (`player->SetRetired(true)`):
     * **Roster Removal & Best-XI Rebalance**: Automatically removes all retired players from every club's `Starting XI`, `Bench`, and `Reserves` (`Team::GetRoster()`), reassigns captaincy (`_captain_by_club`) if the retiring player was captain, and runs positional Best-XI sorting (`auto_pick_best_xi`) to fill vacated starting spots (ensuring Slot `0` remains a `GK`).
     * **Wage Bill Relief**: Deducts retired players' weekly wages from `_finances.weekly_wage_bill`.
     * **Market & Match Exclusion**: Filters `IsRetired() == true` players out of `TransfersView` search queries, AI transfer activity, and `MatchEngine` matchday payloads while keeping their historical profile records in `_players_by_id` for career stats lookup.
     * **Inbox Retirement Report**: Dispatches a `"BOARD"` Inbox summary listing every retiring player at the managed club alongside their final OVR and career appearances/goals/assists.
4. **Save/Load Normalization (`save_career` / `load_career`)**:
   * Serializes `"retirement_age": 37` and `"is_retired"` inside `<slot>.json` and normalizes legacy saves to `retirement_age = 37` on load.

---

## 4. File Inventory & Cleanup Checklist

### Active Project Files
* **C++23 GDExtension Core (`src/`)**:
  * `src/MatchEngine.h` / `src/MatchEngine.cpp` (Phase 1–2C simulation engine; planned Phase 7E Playstyle hooks)
  * `src/CareerState.h` / `src/CareerState.cpp` (Phase 3A & Phase 4A central career state, GK-only Slot `0` guard, GK 6-stat export; planned Phase 5D `clubs.csv` & Codes; planned Phase 7E/7F Playstyles & Age-37 retirement)
  * `src/mainplayer.h` / `src/mainplayer.cpp` (GK 6-stat attribute accessors, `BaseOVR`, `RetirementAge = 37`, `IsRetired`) & `src/team.h` / `src/team.cpp`
* **Data & CSV Registries (`demo/data/` & `Players_Database/`)**:
  * `demo/data/formations.csv` (Normalized 11-slot formation shapes)
  * `demo/data/clubs.csv` (planned Phase 5D deterministic club/league registry)
  * `Players_Database/players_2006_with_price.csv` & `demo/data/players_2006_with_price.csv` (Phase 5D planned `Club`, `League`, `Nationality`, and `Playstyle 1..3` metadata with `RetirementAge = 37`)
  * `demo/data/PlaystylesDescription.txt` (22-Playstyle reference; gameplay integration planned in Phase 7E)
* **GDScript UI & Scenes (`demo/`)**:
  * `demo/CareerShell.gd` / `demo/CareerShell.tscn` (Global 10-route shell, header, modals, toast)
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

## 5. Phase 4C Build & Test Verification Matrix

### Step 1 — Recompile C++23 GDExtension (Windows MinGW/GCC 16.1.0 UCRT64)
```powershell
scons platform=windows target=template_debug api_version=4.7 -j6
```
* **Expected Artifact**: `demo/bin/libfootball.windows.template_debug.x86_64.dll` updated with Phase 4A `CareerState` method bindings (`swap_squad_players` with strict GK-only Slot `0` guard, `_player_to_dict` GK 6-stat keys, `set_club_roles`, `set_player_jersey_number`, `get_club_tactics`, `set_club_tactics`, `get_youth_academy_info`, `upgrade_youth_facility`, `assign_youth_coach`, `promote_youth_player`).

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