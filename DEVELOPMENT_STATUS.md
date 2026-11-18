# CFM-1 Development Status & Engineering Handoff

**Last Updated**: 2026-11-18  
**Reviewed Repository Baseline**: `c1c4d7451820057762afdf3d8d50562654cce492` *(latest code/roadmap commit reviewed for this update; Phase 4C verification remains pending)*  
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
| **Phase 4A: C++ Squad, Tactics & Youth Bindings** | C++23 | **Complete** | **Pending Phase 4C** | Swaps, GK/ID guards, fit OVR, roles/numbers, tactics, youth training/aging/promo. |
| **Phase 4B: `SquadView`, `TacticsView`, `YouthView` UI** | GDScript | **Complete** | **Pending Phase 4C** | Procedural views wired into `CareerShell.gd` and `PlayerProfileModal.gd`. |
| **Phase 4C: Phase 4 Build & Acceptance Suite** | Both | **Ready to Run** | **Active (Next)** | Recompile DLL + run Suites 1–12 + manual cross-viewport QA + `*_2` cleanup. |
| **Phase 5A–5C: Schedule, Competitions & Pre-Match** | Both | Placeholder Views | Scheduled | Scaffolded as route cards inside `CareerShell.gd`. |
| **Phase 5D: `res://data/clubs.csv` & Player Metadata Merge** | Both | **Spec Ready** | **Scheduled (Phase 5)** | Canonical club/league registry, New Career `League -> Club` selection, and `Club`, `League`, `Nationality`, `Playstyle 1..3` CSV metadata. |
| **Phase 6: Club Management, Finances & Board** | Both | Placeholder Views | Scheduled | Separated financial ledgers, board confidence, stadium/facility upgrades. |
| **Phase 7A–7D: Transfers, Search & Reputation** | Both | Placeholder Views | Scheduled | Valuation, fee/wage negotiations, transfer windows, six-field player search, AI transfers, loyalty, and reputation. |
| **Phase 7E: Exclusive $> 96$ Base OVR Playstyles** | Both | **Spec Ready** | **Scheduled (Phase 7)** | 22 Playstyles (`PlaystylesDescription.txt`) gated strictly to `BaseOVR > 96` non-academy players. |

---

## 2. Phase 4 Implementation Summary & Phase 3 Patch Baseline

### A. Phase 4A C++23 Backend Additions (`src/CareerState.h` / `src/CareerState.cpp`)
1. **Squad Roster & Positional Fit Engine**:
   * Added `swap_squad_players(club_id, slot_a, slot_b)` supporting `Starting XI (0–10)`, `Bench (11–17)`, and `Reserves (18+)` swaps with single-GK enforcement on slot `0` and duplicate-`player_id` rejection.
   * Added positional compatibility evaluator (`NATURAL` = $1.00\times$, `COMPATIBLE` = $0.95\times$, `OUT_OF_POSITION` = $0.85\times$ effective OVR) synced to the club's active `formations.csv` slot tags.
   * Added `set_club_roles(club_id, roles_dict)` (Captain, Vice-Captain, Penalties, Free Kicks, Corners) and `set_player_jersey_number(club_id, player_id, number)` enforcing unique `1–99` kit numbers.
2. **Permanent Club Tactics & `MatchEngine` Handoff**:
   * Added `get_club_tactics(club_id)` and `set_club_tactics(club_id, tactics_dict)` persisting `formation_id`, `mentality`, `pressing`, `defensive_line`, `passing_style`, `width`, and `tempo` on `Team` and inside atomic JSON saves—isolated from temporary in-match `TACTIC` overrides.
   * Wired `CareerState` lineup, jersey numbers, and permanent tactics directly into `MatchEngine` team initialization.
3. **Youth Academy Infrastructure, Coaching & Dual-Cycle Progression**:
   * Added `get_youth_academy_info(club_id)` and `upgrade_youth_facility(club_id, upgrade_type)` for Academy Facility Level (`1–5`) and Scouting Network Level (`1–5`), enforcing `club_balance` checks and updating weekly upkeep.
   * Added `assign_youth_coach(club_id, specialty, coach_id)` across four specialties (`Technical`, `Physical`, `Tactical`, `Goalkeeping`) that scale weekly attribute XP gains.
   * Separated **weekly training XP ticks** (executed every 7 days in `advance_day()`) from **annual season rollover** (age `+1`, debut age eligibility update, and potential range narrowing).
   * Verified `promote_youth_player(player_id)` atomically removes the prospect from `YouthAcademy`, appends to senior `Team`, assigns a senior wage/contract, and posts a `"BOARD"` (`"club"`) Inbox confirmation.

### B. Phase 4B GDScript Views & Shell Integration (`demo/`)
1. **`demo/SquadView.gd` / `demo/SquadView.tscn`**:
   * Procedural 2D pitch rendering Starting XI nodes at normalized `formations.csv` coordinates alongside a 7-slot Bench and Reserves table.
   * Interactive two-click/tap swap workflow with color-coded positional fit highlights (Pitch-Green natural, Amber compatible, Coral out-of-position), inline Captain/Set-Piece and Jersey Number (`1–99`) controls, and `PlayerProfileModal` (`"SQUAD"` mode) inspection.
2. **`demo/TacticsView.gd` / `demo/TacticsView.tscn`**:
   * Formation selector (`4-3-3`, `4-2-3-1`, `4-4-2`, `3-5-2`, `5-3-2`, etc.) with live 2D shape preview and 6 tactical sliders/selectors (`Mentality`, `Pressing`, `Defensive Line`, `Passing Style`, `Width`, `Tempo`) displaying live stamina-drain and tactical risk tooltips.
3. **`demo/YouthView.gd` / `demo/YouthView.tscn`**:
   * Academy Infrastructure panel (Facility/Scouting tiers, weekly upkeep, balance-gated upgrade buttons), Coaching Staff assignment panel, Prospect Roster table (`OVR`, `Potential Range`, `Dev Progress %`, `Condition`, `Age`), `PlayerProfileModal` (`"YOUTH"` mode) trigger, and one-click promotion action.
4. **`demo/CareerShell.gd` Integration**:
   * Replaced placeholder cards for `"squad"`, `"tactics"`, and `"youth"` with live view instances refreshing on `_ready()` and `visibility_changed`.

### C. Retained Phase 3B/3C UI Conventions (Enforced Across All Views)
* **`Label` Theme Overrides**: Always use `"normal"` `StyleBoxFlat` overrides (never `"panel"`) for pills, badges, and status tags.
* **Modal Centering**: Always configure `anchor_left/top/right/bottom = 0.5` with symmetric negative/positive offsets and `GROW_DIRECTION_BOTH`.
* **Route Normalization**: `"board"` resolves to `"club"` in both `InboxView` and `CareerShell.navigate_to()`.

---

## 3. Upcoming Engineering Blueprints: Phase 5D & Phase 7E

> The items in this section are specifications for future work unless their implementation is explicitly marked complete in the status matrix. Their presence here does not mean the systems are already implemented or tested.

### Phase 5D — `res://data/clubs.csv`, New Career League Selection & Player Metadata Merge
1. **Files to Create / Update**:
   * **`demo/data/clubs.csv` (`res://data/clubs.csv`)**: Canonical club registry (`ClubID,Club,League,ShortCode,Country,Reputation,TotalBalance,TransferBudget,WageBudget,DefaultFormation,PrimaryColor,SecondaryColor`). Replaces hardcoded `DEFAULT_CLUB_NAMES[16]` and prevents random club creation.
   * **`Players_Database/players_2006_with_price.csv` & `demo/data/players_2006_with_price.csv`**: Add `Club`, `League`, `Nationality`, and `Playstyle 1..3` columns; club values must match the canonical `Club` entries in `res://data/clubs.csv` or use `Free Agent`.
   * **`src/mainplayer.h` & `src/mainplayer.cpp`**: Add `ClubName`, `BaseOVR`, and `IsAcademyProduct` fields and accessors.
   * **`src/team.h` & `src/team.cpp`**: Store `ShortCode`, `LeagueName`, `Country`, `Reputation`, `TransferBudget`, `WageBudget`, and kit colors loaded from `clubs.csv`.
   * **`Players_Database/player_database.h` & `Players_Database/player_database.cpp`**: Parse `Club`, `League`, `Nationality`, and `Playstyle 1..3` header columns dynamically in `load_from_csv()` and add `load_clubs_from_csv()` for `res://data/clubs.csv`.
   * **`src/CareerState.h` & `src/CareerState.cpp`**:
     * In `start_new_career()`: Load `res://data/clubs.csv` first to create exact `Team` records, implement the New Career `League -> Club -> Manager Name` selection flow, map players by their CSV `Club` value (routing unmatched/empty rows to Free Agents), run positional Best-XI sorting (`GK` locked to Slot `0`), and serialize `"csv_club_name"`, `"base_ovr"`, `"is_academy_product"`, `"league"`, and `"short_code"` in `save_career()` / `load_career()`.
   * **GDScript Views (`CareerShell.gd`, `DashboardView.gd`, `SquadView.gd`, `CompetitionsView.gd`, `TransfersView.gd`, `PlayerProfileModal.gd`)**: Consume `clubs.csv` short codes, league names, and merged player `club_name`, `league`, and `nationality` metadata.
2. **Adding Custom Players & Classmates**:
   * Append custom player rows with `Club` and `League` values matching the canonical entries in `res://data/clubs.csv`, plus the player's `Nationality`.
   * Any custom player authored with base attributes yielding $\text{BaseOVR} > 96$ (`97–99` OVR) automatically qualifies for **Phase 7E Playstyles**.


**Roadmap alignment update (2026-11-18):**
* Phase 5D now uses `res://data/clubs.csv` with the canonical header `ClubID,Club,League,ShortCode,Country,Reputation,TotalBalance,TransferBudget,WageBudget,DefaultFormation,PrimaryColor,SecondaryColor`.
* New Career is planned as a `League -> Club -> Manager Name` selection flow, populated only from the registry; do not create random or unregistered clubs.
* The player CSV metadata scope is `Club`, `League`, `Nationality`, and `Playstyle 1`, `Playstyle 2`, `Playstyle 3` (not just a `Club` column).
* Phase 7A–7D includes a six-field Transfer Search: `Name`, `Club`, `Age`, `Nationality`, `League`, and `Position`. This is planned functionality, not a claim that the transfer market is implemented.

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
   * `Players_Database/player_database.cpp`: Parse pipe-delimited `Playstyles` column (or auto-assign 2–3 role-matched Playstyles for `BaseOVR > 96` players when omitted).
   * `src/MatchEngine.h` / `.cpp`: Apply Playstyle decision weights, shot curves, pass lane detection, tackle selection, sprint dribbling, and GK rush-out/deflection modifiers during match simulation.
   * `src/CareerState.cpp`: Include `"playstyles"`, `"base_ovr"`, and `"is_academy_product"` in `prepare_next_match_payload()`, `_player_to_dict()`, `save_career()`, and `load_career()`.
   * `demo/PlayerProfileModal.gd` & `demo/SquadView.gd`: Render Playstyle badges and descriptions from `PlaystylesDescription.txt` on eligible $> 96$ Base OVR players.

---

## 4. File Inventory & Cleanup Checklist

### Active Project Files
* **C++23 GDExtension Core (`src/`)**:
  * `src/MatchEngine.h` / `src/MatchEngine.cpp` (Phase 1–2C simulation engine; planned Phase 7E Playstyle hooks)
  * `src/CareerState.h` / `src/CareerState.cpp` (Phase 3A & Phase 4A central career state; planned Phase 5D `clubs.csv` integration; planned Phase 7E serialization)
  * `src/mainplayer.h` / `src/mainplayer.cpp` & `src/team.h` / `src/team.cpp`
* **Data & CSV Registries (`demo/data/` & `Players_Database/`)**:
  * `demo/data/formations.csv` (Normalized 11-slot formation shapes)
  * `demo/data/clubs.csv` (planned Phase 5D deterministic club/league registry; not yet present in the reviewed data directory)
  * `Players_Database/players_2006_with_price.csv` & `demo/data/players_2006_with_price.csv` (Phase 5D planned `Club`, `League`, `Nationality`, and `Playstyle 1..3` metadata)
  * `demo/data/PlaystylesDescription.txt` (22-Playstyle reference; gameplay integration remains planned)
* **GDScript UI & Scenes (`demo/`)**:
  * `demo/CareerShell.gd` / `demo/CareerShell.tscn` (Global 10-route shell, header, modals, toast)
  * `demo/DashboardView.gd` / `demo/DashboardView.tscn` (Career Dashboard)
  * `demo/SquadView.gd` / `demo/SquadView.tscn` (Phase 4B Interactive 2D Pitch, XI/Bench/Reserves, Roles & Kit Numbers)
  * `demo/TacticsView.gd` / `demo/TacticsView.tscn` (Phase 4B Permanent Club Formation & Tactical Sliders)
  * `demo/YouthView.gd` / `demo/YouthView.tscn` (Phase 4B Youth Academy Facilities, Coaches & Prospect Promotion)
  * `demo/InboxView.gd` / `demo/InboxView.tscn` (Split-pane News & Inbox)
  * `demo/PlayerProfileModal.gd` / `demo/PlayerProfileModal.tscn` (Universal 4-mode player modal)
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
* **Expected Artifact**: `demo/bin/libfootball.windows.template_debug.x86_64.dll` updated with Phase 4A `CareerState` method bindings (`swap_squad_players`, `set_club_roles`, `set_player_jersey_number`, `get_club_tactics`, `set_club_tactics`, `get_youth_academy_info`, `upgrade_youth_facility`, `assign_youth_coach`, `promote_youth_player`).

### Step 2 — Automated Regression Verification (`demo/CareerRegressionTest.gd`)
| Suite | Scope | Key Assertions |
| :---: | :--- | :--- |
| **Suites 1–8** | Phase 3A–3D Baseline | Deterministic bootstrap, atomic Save/Load, `DashboardView` auto-refresh, `InboxView` chip reset & `"board"` -> `"club"`, `PlayerProfileModal` centering & 4 modes. |
| **Suite 9** | `SquadView` & Lineup Engine | `XI <-> Bench <-> Reserves` swaps, single-GK guard on slot `0`, duplicate-ID rejection, positional OVR penalty calculation, Captain/Set-Piece/Jersey (`1–99`) persistence, and `MatchEngine` lineup sync. |
| **Suite 10** | `TacticsView` & Scope Isolation | Formation + 6 slider mutations persist across route switches and Save/Load without leaking temporary in-match `TACTIC` changes. |
| **Suite 11** | `YouthView` & Academy Growth | Facility/Scouting upgrades deduct `club_balance` and fail gracefully on insufficient funds; coach assignment scales weekly XP; 7-day `advance_day()` increments training XP without premature age `+1`; annual rollover increments age; `promote_youth_player()` moves prospect to senior squad + posts Inbox alert. |
| **Suite 12** | Phase 4 Full Round-Trip | Mutate XI, roles, kit numbers, permanent tactics, youth upgrades, and promote a prospect -> `save_career()` -> `load_career()` -> verify exact state parity. |

### Step 3 — Manual Cross-Viewport & Interactive QA Checklist
1. **Desktop Viewport (`1280x720`)**:
   * Open `Squad` tab -> verify 11 pitch markers match `formations.csv` geometry, swap a starter with a bench player, verify natural/compatible/out-of-position badge colors, edit Captain and jersey number, and inspect a player in `PlayerProfileModal`.
   * Open `Tactics` tab -> switch formation (`4-3-3` -> `4-2-3-1`), adjust `Pressing` and `Tempo` sliders, verify stamina/risk tooltip updates, switch away and back to confirm persistence.
   * Open `Youth` tab -> upgrade Academy Facility (confirm top header balance pill decrements immediately), assign a youth coach, advance 7 days via `+1 Day` to verify weekly training progress ticks, and promote an eligible prospect (confirm unread Inbox badge increments and prospect appears in `Squad` reserves).
2. **Narrow / Mobile Viewport (`800x480` & Portrait)**:
   * Confirm `CareerShell` sidebar collapses into drawer (`< 780px`), `SquadView` pitch + bench stack cleanly without horizontal overflow, and `PlayerProfileModal` remains centered within viewport bounds.