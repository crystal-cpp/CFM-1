# CFM-1 Development Status & Engineering Handoff

**Last Updated**: 2026-11-26

**Reviewed Source Baseline**: c6a53faa41a10cecd23c17c51b391898a94172b *(PreMatch displays the fixture stadium explicitly; see aa8a3d for standalone identity reset)*; includes bead855 *(TacticalScreen pitch, tactics and substitutions follow the user's side)*; includes c2fed675 *(selected clash-resolved kit color handoff)*, 6a01300 *(remaining defensive chance-state checks converted to user-relative logic)*, c53eeed *(user-relative defensive challenge/chance handling)*, cf007db *(user-relative chance creation, shooting, scoring, pass reception and telemetry)*, 2217579 *(CareerState club identity, kit color and user/opponent context handoff)*, f6d6877 *(PreMatch HOME/AWAY banner and user-squad/opponent-scouting layout)*, and b6078a9 / c31f700 *(substitution and defensive implementation foundation)*.

**Current Phase**: Phase 5

**Current Milestone**: **Phase 5F is implemented and user-verified; Phase 5G implementation is COMPLETE (acceptance QA deferred)** across offside, restarts, fouls/cards, tackle injuries and HUD wiring. Phase 5J scope is expanded to include NHHA72528-gated visibility for former Tuesday League club/player records, SquadView blank-space deselection, highest-OVR Starting XI default captaincy with manual override persistence, and canonical per-player weekly wages backed by a `Wage` CSV column. Offside-specific deterministic test hooks are now present; boundary/foul/injury regression cases still have missing-hook false positives. Build/Godot runtime acceptance remain deferred; no successful build or runtime pass is claimed. Phase 5E remains implemented and user-verified, including CareerState-backed substitutions, defensive AI, movement, stamina/attribute coupling, halftime and HOME/AWAY identity integration. The user confirms that Phase 5F is finished in the repository, including stamina persistence/calendar recovery, injury lifecycle and squad/UI/Save-Load integration, dribbling balance, slide tackles/loose-ball outcomes, and GK Rush command-to-engine wiring with responsive two-axis goalkeeper movement. This status is based on the user's confirmation; no independent assistant-run clean build or automated regression suite is claimed. Phase 5D source implementation is complete, while its acceptance and historical-data audit remain outstanding.

## Latest Match Runtime Corrective Follow-up — 2026-11-22

Source-only corrective changes have been committed after the Phase 5E implementation baseline:

- MatchEngine now seeds the controlled player from the managed fixture side (HOME slot 9 / AWAY slot 20), exposes user-side index/chance helpers to GDScript, and rejects HOME/AWAY-local index remapping in direct selection and pass targets. Player indexes passed across the GDScript/C++ boundary are now expected to be absolute simulation indexes.
- Chance-start, interactive-event and chance-end notifications are queued during native simulation and flushed after the simulation step. Full-time waits for an active chance to finish; halftime flushes pending chance notifications after its transition.
- CareerState and MatchScene reassert the managed side after loading fixture lineups/benches. MatchPitch no longer overwrites preloaded club formations with demo defaults, resets its cached chance state between matches, guards stale pointer/chance callbacks, deduplicates GUI/unhandled input processing, and disconnects MatchEngine signals on tree exit.
- MatchHUD now guards duplicate engine signal connections and disconnects them on tree exit. TacticalScreen was inspected and already consumes the engine's user-side squad/bench snapshots, so no additional ownership branch was added there. PreMatchView already writes the managed XI and formation to the fixture side selected by managed club ID.
- **Verification status: user-confirmed working.** After these source changes, the user reported that the problem was fixed. The HOME/AWAY ownership, user-side controls and match-flow issues covered by this follow-up are now recorded as resolved based on the user's test. No independent assistant-run Windows UCRT64 build or automated runtime suite is claimed.

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
| **Phase 4C: Phase 4 Build, Youth-Coach Recruitment & Acceptance Suite** | Both | **Ready to Run; youth-coach recruitment planned** | **Pending / not current focus** | Recompile DLL + run Suites 1–12 + manual cross-viewport QA + `*_2` cleanup. The existing Phase 4C roadmap scope also owns youth-coach recruitment from `youth_coaches.csv`, including hiring, specialty assignment, weekly wages, affordability/duplicate guards and Save/Load. Phase 5G source implementation is complete; its separate verification work is tracked below. |
| **Phase 5A: Season Calendar & ScheduleView Foundation** | Both | **Implemented in repository** | **Runtime QA pending** | July–May calendar, July pre-season friendlies, domestic league fixture generation, and tabbed ScheduleView (`My Club Fixtures | League Matchdays | Results Archive`). Latest commit `018eef5` adds responsive flow layouts and compact labels/tooltips. |
| **Phase 5B-1: Domestic Competition Foundation** | Both | **UI + source integration fix committed** | **Godot runtime QA pending** | `CompetitionView.gd` now prefers the active `CareerState.get_leagues_and_clubs()` registry, normalizes the canonical CSV schema for fallback loading, and calls the no-argument `get_league_standings()` API while adapting `club_name` / `played` / `points` fields to the view schema. Verify that clubs render for the managed league and other selected leagues after a fresh career. UCL fixture generation remains separate/deferred. |
| **Phase 5B-2: CompetitionsView Integration & Responsive UI** | GDScript | **Implemented in repository** | **Source inspection complete; runtime QA pending** | `CareerShell.gd` calls `_competitions_view.refresh_competitions()` when navigating to Competitions, in `refresh_all_views()`, and in `_on_standings_updated()` (commit `d43ba52`). `CompetitionView.gd` registry/standings integration follow-up is committed in `e2f9351`; commit `626f2d5` fixes the initial dropdown selection caused by the view being constructed before CareerState injection. It now selects the managed club's live league standings, preserves intentional league changes, and reselects the managed league when switching careers. `ScheduleView` responsive layout refinements are in `018eef5`. Verify match-result and AI-result updates plus desktop/narrow viewport behavior in Godot. UCL fixture generation, qualification and authoritative results remain deferred. |
| **Phase 5C: Pre-Match Center & Post-Match Pipeline** | GDScript / CareerState | **Integration committed** | **Source inspection complete; runtime QA pending** | Dashboard / Schedule requests open `PreMatchView`; `MatchScene.launch_career_match()` applies the context before kickoff, passes primary kit colors to 2D outfield markers, commits the result after the final-whistle chance deferral, and displays `PostMatchModal.show_report(summary)`. Continue returns to CareerShell and refreshes views. Validate the full flow and result mapping in Godot. |
| **Phase 5C-2: Main Menu & Application Startup / Career Entry Flow** | GDScript / Scene Architecture | **Implemented in repository** | **Source inspection complete; runtime QA pending** | `demo/project.godot` starts `res://app_root.tscn`; `AppRoot.gd` owns `CareerState`, persistent `MatchScene`, `CareerShell`, `MainMenuView` and `NewCareerView`, with New / Continue / Load / Settings / Exit actions, automatic Dashboard entry, load-error handling and an unsaved-progress confirmation. Settings currently exposes fullscreen only; audio settings remain Phase 5K. Validate all transitions in Godot. |
| **Phase 5D-1: Club Registry Schema & Player CSV Metadata** | C++23 / CSV | **Implemented in source; dataset coverage complete** | **Historical/data QA pending** | Current source-level audit: 156 club rows, 13 columns (including Stadium), 28 leagues, 156 unique ClubIDs, 1,716 player rows, all player Club values matched, and all 156 clubs have at least one imported player. Historical accuracy as of 15 July 2006 remains unverified. NHHA72528 code and unlock UI are present; reward mapping still needs QA. |
| **Phase 5D: Historical Club Data & Structured Career Bootstrap** | C++23 / GDScript / CSV | **Source implementation complete** | **Build, regression, runtime and historical-data acceptance pending** | Includes canonical ClubRecord/ClubID handling, all club metadata propagation, CSV parsing hardening, league-isolated fixtures/standings, bye-aware odd-team scheduling, pre-season pairing correction, career parser/test cleanup, and regression coverage for 156 clubs / 28 leagues, 11A7 ClubID 1098, 21-team La Liga, no odd-round double-booking and single-club Allsvenskan. Latest commit `0ea259c` adds stadium metadata, formation-aware XI selection, hidden youth-prospect lifecycle handling, match lineup/name integration and PreMatch formation/context synchronization. This records source implementation, not a successful local build or runtime pass. The 15 July 2006 historical-data audit remains open. |
| **Phase 5E: MatchEngine Realism, Stamina, Defensive Attributes & Movement** | C++23 / GDScript | **Implemented; user-verified** | **User testing confirmed; reported bugs resolved** | Includes stamina drain/telemetry/report handoff; attribute/OVR-weighted attacking chances; attribute-, fatigue-, role- and tactic-aware defensive tackles, duels, interceptions, blocks and clearances; defensive positioning, pressing, runner tracking, recovery runs and defensive-line behavior; movement steering; goalkeeper corridor handling; fitness-aware PreMatch selection; halftime flow; CareerState-backed lineup/bench payloads; authoritative live substitutions with five-sub/three-window limits and halftime allowance; post-match stamina/condition write-back; fixture-order club IDs/names and clash-resolved kit colors in MatchEngine; explicit user/opponent context aliases; user-relative chance triggers, opponent shooting, pass reception, scoring gates, defensive chance pacing and match telemetry; PreMatch HOME/AWAY fixture labels with user squad on the left and opponent scouting on the right; TacticalScreen user-side squad/bench/tactics/substitution handling; MatchHUD/MatchPitch labels sourced from MatchEngine. The user reports the previous match identity/control bugs are fixed and the current 5E behavior is working. No independent assistant-run build or automated simulation suite is claimed. |
| **Phase 5F: Stamina Persistence, Recovery, Injury & Match-Control Refinements** | C++23 / GDScript | **Implemented; user-verified** | **User confirms Phase 5F is finished in the repository** | Includes post-match stamina persistence and configurable daily recovery; injury generation, severity/countdown/clearance, matchday availability guards, injury-aware UI/Inbox and Save/Load; attribute/fatigue-aware dribbling balance; physical slide-tackle and loose-ball behavior; and GK Rush command-to-engine integration with responsive two-axis movement. The previously documented GK Rush blocker is superseded by the user's completion confirmation. No independent assistant-run clean build or automated regression suite is claimed. |
| **Phase 5G: Match Laws, Offside, Restarts & Disciplinary System** | C++23 / GDScript | **Complete — implementation milestone** | **Acceptance QA deferred; regression runner repair pending** | Current source includes intended-receiver offside checks at pass initiation, teammate kick-time offside flags, shared pass-flight and shot-flight active-involvement checks (ball contact/opponent interference), loose-ball advantage checks, own-half/level/restart exceptions, and _trigger_offside() which clears the active chance and awards an indirect free kick. Deliberate outfield and goalkeeper clearances reset the previous offside snapshot. Current source search shows identified offside stoppage paths funnel through _trigger_offside(...), which requests an indirect free kick; ordinary foul awards separately request direct free kicks. Goal kicks/corners/throw-ins use formal restart state/timer/signals; foul/free-kick coverage includes standing and slide challenge paths; yellow accumulation, second-yellow/direct-red, DOGSO heuristic, sent-off filtering and injury telemetry/CareerState handoff exist. Injury probability is independent of card probability; tackle injury generation currently targets non-GK managed-team players. MatchHUD/MatchScene restart labels, defensive-control gating, event commentary, injury summary handoff, pause-state/pause-icon synchronization, kit synchronization for both teams, CareerShell pause/visibility handling and [REF] commentary tagging are wired in source. demo/Phase5GRegressionTest.gd and F9 hook exist. Offside-specific tests now use bound deterministic hooks for both HOME and AWAY and assert position rules separately from active involvement (passive presence, ball contact and opponent interference); end-to-end probes drive the real BALL_IN_PASS and BALL_IN_SHOT update branches and assert the opponent receives an indirect free kick. The stoppage test uses the actual get_restart_state() enum (free kick = 4), is_restart_indirect() and is_restart_for_home() APIs. The shared involvement check now runs during both pass and shot flight. Remaining boundary/foul/injury scenarios still have unbound debug_* methods or fallback-only assertions; their restart getters and throw-in enum expectation also need correction. Deferred verification: add deterministic loose-ball advantage/no-follow-up-shot tests; repair remaining false-positive cases; verify restart sides/spots/takers/timers, card/DOGSO/dismissal behavior, injury mapping and full Phase 5F lifecycle; run MinGW UCRT64 SCons build and Godot 4.7 runtime/manual QA. These items do not change the source-implementation status above. No build or runtime tests were run in this audit. |
| **Phase 5H: Relative Team-OVR Chance Frequency & Goalkeeper Intelligence** | C++23 / GDScript | **Partial foundation only** | **Planned** | Team OVR is imported from selected lineups and individual attributes influence match actions, but relative OVR-based attack/defend chance spawn frequency is not implemented as specified. CareerState exports GK six-stat data, but MatchEngine's player parser does not consume dedicated Diving/Reflex/Positioning/Reaction values; keeper behavior remains largely corridor/anchor based. Add relative-OVR chance pacing, keeper-stat-scaled diving and dynamic ball-aware positioning. Also align Main Menu, CareerShell/main management views and Match UI to the Pre-Match Center's layout and visual language, applying the shared natural color palette defined in Phase 5M; hide leagues with fewer than 8 distinct valid registered clubs from the New Career league selector (7 or fewer hidden; 8 or more visible) without deleting the underlying leagues/clubs. Test 7-, 8- and 9-club cases and confirm hidden leagues remain in the registry. These UI/filter requirements are planned, not implemented. Also merge Tuesday League into the single canonical Premier League identity and include 11A7 (ClubID 1098) in Premier League fixtures, standings, results, New Career selection and season progression. Keep league membership, New Career eligibility and competition participation independent of redeem-code activation. The separate visibility lock for former Tuesday League club/player records is specified in Phase 5J and must use stable record identity/origin metadata after migration; unrelated custom-player/Transfer Market unlocks remain isolated. Preserve all club/player IDs, roster links and metadata; prevent duplicate memberships, fixtures, match IDs and standings rows; test fresh careers and legacy-save migration. Explicitly include 11A7 in Champions League participant/fixture generation and progression without duplicate UCL entries. UCL-wide generation/progression remains under existing 5B work. These migration and UCL additions are planned, not implemented. |
| **Phase 5I: Stoppage Time & Knockout Extra Time** | C++23 | **Not implemented in audited source** | **Planned** | Current match clock has normal-time/halftime flow; no added-time countdown or knockout extra-time state was found. Add 2–5 minutes of stoppage time per half with a dead-ball whistle rule, two 15-minute extra-time halves for tied knockout semi-finals/finals and approximately 5% stamina recovery at the extra-time interval; add penalties if required. |
| **Phase 5J: MatchConclusion, Match Timeline & Youth Academy Data Integrity** | GDScript / C++23 | **Existing report foundation; requested screen not present** | **Planned** | Build a dedicated conclusion view with Player of the Match, every scorer and goal minute, match events and CareerShell return; remove the post-match R shortcut and skip-token UI/reward behavior. Resolve `[HOME]` / `[AWAY]` commentary placeholders to actual fixture home/away club names using authoritative match identity (not user-control side), shared with Phase 5K. Also automatically top up each club's academy roster to at least 9 prospects (more than 8) whenever it has fewer than 9. Generate only the missing number of prospects, automatically generating each player's name from the club's country/data and OVR within the plausible 65–68 youth range, with valid positions and unique IDs. Remove the unsynchronized extra Vietnamese-name injection. Remove only these six unwanted fixed academy prospects: Nguyen Van Son (GK), Doan Van Tuan (CB), Tran Hoang Long (LB), Pham Duc Huy (CM), Le Minh Quang (CAM), and Nguyen Tien Phat (ST). Trace their hard-coded/default/seeded source and prevent reinsertion after refresh, day progression, season rollover or Save/Load; do not delete legitimate senior/database players or other valid prospects merely because a name matches. Add persistence, duplicate-prevention and country/name consistency tests; cover academy counts 0, 1, 6, 8, 9 and above 9, verify only the missing count is generated, and confirm the six fixed entries never reappear. |
| **Phase 5K: Audio System, Stadium Atmosphere, Commentary & Goal Celebrations** | Both | **Spec Ready** | **Scheduled (Phase 5)** | Menu/career background music, match crowd ambience, match-importance stadium intensity, contextual commentary (GOAL/SAVE/FOUL/CARD/SUB) with every `[HOME]` / `[AWAY]` token resolved to actual fixture club names using the shared Phase 5J identity source, distinct home/away goal reactions, 3.0–5.0s touchline goal celebration run, and full-time whistle/crowd audio. |
| **Phase 5M: Main UI Color Palette & Natural Visual Identity** | GDScript / Godot Theme | **Newly scoped** | **Planned** | Establish a cohesive, restrained, human-designed palette for Main Menu and CareerShell/management views. Define shared color tokens for surfaces, borders, text, actions, selection and semantic states; avoid synthetic neon gradients, glow effects, excessive saturation and decorative color noise. Keep pitch-green purposeful, preserve club kit identity and maintain accessible contrast. Phase 5H applies this palette across Match UI and Pre-Match consistency; Phase 6C uses it for typography/player-card refinements rather than defining a competing palette. Desktop and narrow/mobile visual QA required. |
| **Phase 6A: Club Management, Finances & Board** | Both | Placeholder Views | Scheduled | Separated financial ledgers, board confidence, stadium/facility upgrades. | 
| **Phase 6B: Senior Squad Training Mechanism & `Training` Tab** | Both | **Spec Ready** | **Scheduled (Phase 6)** | Dedicated `Training` tab with 4 positional group focus programs (Attackers, Midfielders, Defenders, Goalkeepers) and potential/age-aware OVR growth. | 
| **Phase 6C: Global UI & Typography Design System** | GDScript | **Spec Ready** | **Scheduled (Phase 6)** | Custom typography hierarchy, redesigned high-clarity player cards, and disciplined non-AI-slop dark-navy aesthetic. |\n| **Phase 6D: Match-Control Integration Polish & Three-Playstyle Attribute Display** | C++23 / GDScript | **Newly scoped** | **Scheduled (Phase 6)** | Complete any remaining input/UI integration and regression polish for the Phase 5H goalkeeper intelligence without duplicating Phase 5F gameplay; display `Playstyle 1`, `Playstyle 2`, and `Playstyle 3` in player attribute/profile views. UI display is separate from Phase 7E's gated gameplay integration. | 
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

   * Academy Infrastructure panel (Facility/Scouting tiers, weekly upkeep, balance-gated upgrade buttons), existing four-specialty coach assignment panel, Prospect Roster table (`OVR`, `Potential Range`, `Dev Progress %`, `Condition`, `Age`), `PlayerProfileModal` (`"YOUTH"` mode) trigger, and one-click promotion action.
   * **Not yet implemented:** club-level youth-coach recruitment and wage management. Add the canonical `demo/data/youth_coaches.csv` with the exact header `Name,Skill,Weekly Wage`. `Name` identifies the coach, `Skill` is an integer from `0` to `100`, and `Weekly Wage` is the recurring club cost. Load candidates from CSV rather than hard-coding records in GDScript. Allow clubs to hire and release coaches, assign hired staff to Technical / Physical / Tactical / Goalkeeping specialties, and scale youth-development effectiveness by coach Skill (higher skill = stronger contribution, within progression caps). Validate affordability before hiring, prevent duplicate hires/assignments, deduct each wage exactly once per weekly financial/calendar tick, and persist the hired roster, assignments and wage obligations through Save/Load.

5. **`demo/CareerShell.gd` Integration**:

   * Replaced placeholder cards for `"squad"`, `"tactics"`, and `"youth"` with live view instances refreshing on `_ready()` and `visibility_changed`.

### C. Retained Phase 3B/3C UI Conventions (Enforced Across All Views)

* **`Label` Theme Overrides**: Always use `"normal"` `StyleBoxFlat` overrides (never `"panel"`) for pills, badges, and status tags.

* **Modal Centering**: Always configure `anchor_left/top/right/bottom = 0.5` with symmetric negative/positive offsets and `GROW_DIRECTION_BOTH`.

* **Route Normalization**: `"board"` resolves to `"club"` in both `InboxView` and `CareerShell.navigate_to()`.

## 3. Upcoming Engineering Blueprints & Technical Dependencies (Phases 5–7)

> **Status Note**: Phase 5A–5C source integrations remain subject to Godot runtime QA; authoritative UCL fixture generation and progression are deferred. Phase 5C-2 is implemented in source, but runtime QA remains pending. Phase 5D source fixes include canonical club dictionary mapping, stable ClubID/metadata propagation, league isolation, odd-team bye handling, pre-season pairing correction, CareerState parser cleanup and duplicate-test cleanup. Current CSV inventory is 156 clubs across 28 league labels (13 columns including Stadium), 1,716 player rows, and 15 custom 11A7 roster entries. The 15 July 2006 historical-data audit remains open. Phase 5E source implementation is complete and user-verified. Phase 5F is now recorded as implemented and user-verified based on the user's confirmation: stamina persistence/recovery, injury lifecycle, dribbling balance, slide tackles/loose balls and GK Rush two-axis movement are complete. The earlier source-audit note identifying GK Rush as a blocker is superseded. No independent assistant-run clean build or automated regression suite is claimed for 5F. Phase 5G implementation milestone is complete; deterministic regression-harness repair, build and Godot runtime checks remain deferred.

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
- Current source-level audit of demo/data/clubs.csv: **156 data rows**, **13 columns** including Stadium, **28 distinct league labels**, 156 unique ClubIDs, no duplicate club names and **12 single-club leagues**.
- Current source-level audit of demo/data/players_2006_with_price.csv: **1,716 player rows**; every non-empty player Club value matches a registry club, and all **156 clubs have at least one imported player row**. No club currently relies on a missing-roster fallback according to this CSV membership check.
- The 13-column club schema includes ClubID, Club, League, ShortCode, Country, Reputation, TotalBalance, TransferBudget, WageBudget, DefaultFormation, PrimaryColor, SecondaryColor and Stadium. Player data retains Stamina and Playstyle metadata; do not duplicate the Stamina column.
- Historical correctness of all clubs, league labels, memberships, finances, formations and kit colours as of **15 July 2006** is not yet independently verified. NHHA72528 reward mapping also remains subject to QA. The Tuesday League / 11A7 domestic and Champions League migration, including redeem-code conflict checks, is scoped under Phase 5H.

**5D-2 — Structured Bootstrap & League-Scoped Fixture Generation [IMPLEMENTED IN SOURCE; ACCEPTANCE QA PENDING]**
- **New Career league registry follow-up:** hardened `find_col()` to normalize spaces/underscores/hyphens in CSV headers and made `load_clubs_from_csv()` fail when it loads zero valid club rows. `NewCareerView.gd` now prints the playable league count/names at startup. This is a source-level fix/diagnostic; rebuild the MinGW GDExtension and confirm the real leagues appear before marking runtime QA complete.
- **Canonical ClubRecord integration:** CareerState consumes the canonical club dictionary fields (club, reputation, total_balance, transfer_budget, wage_budget, default_formation, primary_color, secondary_color) and uses normalized club-name matching.
- **Stable identity and metadata:** CSV ClubIDs are preserved through CareerState and New Career UI; club metadata is propagated into Team and career save data. NewCareerView accepts canonical club / club_name fields and does not fabricate sequential IDs.
- **Finance accessor correction:** CareerState uses Team::GetBudget() rather than directly accessing the private TransferBudget member.
- **League isolation and scheduling:** fixture and standings participants are selected from the managed club's league only. Single-club leagues do not borrow other leagues' clubs. Odd-sized round-robin scheduling uses a bye sentinel, and pre-season pairings avoid scheduling a club twice in the same round.

- **CareerState parser cleanup:** removed the orphaned duplicate career-start block and duplicate get_header_summary() / get_club_info() definitions that had caused global-scope parser errors and redefinitions.
- **Regression coverage:** CareerRegressionTest.gd includes Phase 5D checks for the 156-club / 28-league registry, 11A7 ClubID 1098, 21-team La Liga, no double-booking in the tested odd-sized league, and single-club Allsvenskan isolation. The older duplicate declarations of _test_career_initialization() and _test_subsystem_queries_and_bridges() were removed in commit c5cd7b6c44eda90581ad91b8c44363c6dd62fab6. The test suite has not yet been rerun after that parser fix.
- **Relevant source-fix commits:** 0c25cc8, 97bd023, 40fb353, 760aeaf, f363c28, 5504b33, 2204a80 and c5cd7b6. These commits establish source changes only; they do not establish a successful local build or runtime test.

**Remaining 5D acceptance steps**
1. Run the Windows MinGW/UCRT64 SCons build: scons platform=windows target=template_debug api_version=4.7.
2. Open the Godot 4.7 project, confirm scripts parse, run CareerRegressionTest.gd with F10, and fix any runtime or assertion failures.
3. Manually verify New Career League -> Club selection, displayed club metadata, player-to-club assignment, league-only standings/fixtures, odd-team byes, pre-season pairings and Save/Load persistence.
4. Complete the historical 15 July 2006 data audit. Dataset coverage is complete at the club level, but historical correctness is not yet certified.

### Phase 5E User-Side Identity & HOME/AWAY UI Integration Follow-Up [IMPLEMENTED & USER-VERIFIED]

* `CareerState::prepare_match_context()` exposes explicit `user_*` and `opponent_*` fields (club ID/name, OVR, formation, XI and bench) alongside fixture-order `home_*` / `away_*` fields.
* `CareerState::prepare_next_match_payload()` sets MatchEngine's managed-club side, fixture club IDs/names and kit colors. If primary kits clash, the away kit uses the away club's secondary color. PreMatch's selected alternate away kit is applied to MatchEngine and the pitch.
* MatchEngine's live-position chance trigger, opponent auto-shoot check, manual-dribble ownership, pass reception chance trigger, interception/tackle/clearance chance cancellation, scoring gate, chance expiry, defensive chance time scale and match-stat identity fields now follow `user_is_home`, not a fixed home-team assumption.
* PreMatchView keeps the fixture banner in strict fixture order: left = `home_club` with `HOME`, right = `away_club` with `AWAY`; the stadium is displayed explicitly. Its left panel is always the managed user's squad and tactics; its right panel is opponent scouting with expected XI, formation and OVR. A primary-kit clash forces the alternate away kit and disables toggling back to the clashing color.
* TacticalScreen uses `get_user_squad_snapshot()`, `get_user_bench_snapshot()`, `can_make_user_substitution()`, `is_user_home()` and `apply_substitution(is_user_home(), ...)`. Its pitch preview and tactical controls also follow the user's side when the managed club is away.
* MatchHUD and MatchPitch read club display names from MatchEngine rather than placeholder team labels.
* The standalone non-career Next Match route explicitly resets MatchEngine's user side and club names before kickoff, preventing a prior away-user career match from leaking into the default HOME/AWAY fixture.

**QA status:** the user has now confirmed the reported HOME/AWAY ownership and match-control problems are fixed and the behavior is working. Treat the 5E identity follow-up as user-verified. This is based on the user's own successful test; it does not imply an independent build or automated regression run by the assistant.

### Phase 5E — MatchEngine Simulation Realism, Attribute-Coupled Defense, Stamina, Movement & Match Identity [IMPLEMENTED & USER-VERIFIED]

**Implementation snapshot (reviewed through c31f7007, b6078a95 and 041b2865):**

* Stamina drain is scaled using each simulation player's imported `starting_stamina`; end-match home/away/combined stamina arrays are exported and passed to the stamina report.
* Attacking chance trigger probability and wide-chance xG use player attributes and team OVR.
* Defensive contests couple individual attributes, fatigue, role/position and tactical context across tackles, duels, interceptions, shot blocks and dedicated clearances. Role-aware bonuses distinguish CB, fullback, CDM, CM, goalkeeper and attacking roles; defensive positioning/awareness proxies influence shape and decisions.
* Defensive AI includes improved positioning, pressing selection, runner tracking, recovery runs, tackle cooldowns, probabilistic tackle/interception/block/loose-ball contests and tuned defensive-line behavior.
* Steering includes acceleration/deceleration/turn response; goalkeeper near-ball handling is restricted to the goal corridor.
* PreMatchView splits the CareerState roster into the configured Starting XI and actual bench, displays fitness/OVR, and supports player swaps. CareerState prepares both lineups and bench payloads for MatchEngine.
* TacticalScreen synchronizes live squad and bench snapshots from MatchEngine. MatchEngine exposes authoritative substitution APIs and enforces five substitutions across three stoppage windows, with halftime substitutions allowed. Subbed-on/off identity, final XI and stamina/condition are synchronized into career result handling.
* MatchPitch displays role + surname labels and club-specific goal banners from MatchScene context. Halftime flow, HUD refresh hooks and post-match stamina write-back are present.
* The redundant bench state removed in 041b286 does not remove the operative live-substitution APIs or their underlying bench storage.

**Implementation and verification status:** the agreed Phase 5E feature scope is complete, and the user has confirmed that the previously reported match bugs are resolved and the current match behavior is working. Record 5E as user-verified. This status reflects the user's successful testing; no separate assistant-run clean SCons build or automated comparative simulation suite is claimed.

**Preserved behavior / integration notes:** the primary visible Skip button remains (only a duplicate hidden control was removed); skip signal/backend token logic is preserved. `KEY_R` remains the post-match Next Match shortcut, not an Advanced Dribbling binding. Preserve chance-gated scoring, tactics, direct-control APIs and career/match handoff.

**Header/source integrity:** current `src/CareerState.h` correctly declares `CareerState : public Node`, while `src/match_engine.h` owns the MatchEngine declaration. The previously reported header regression is resolved in source.

1. **Dependencies & Partial Support**:

   * Depends on Phase 5D loading the CSV `Stamina` attribute and club kit colours onto `Team` / `Player`.

   * Partially supported by Phase 2A's chance-gated state machine (`CHANCE_NONE` / `HOME_ATTACK` / `AWAY_ATTACK`), stamina/tactics coupling, and direct control bindings in `MatchEngine.cpp`.

2. **Implemented Deliverables**:

   * **CSV `Stamina`-Coupled Match Drain (`MatchEngine.cpp`)**: Scales per-tick sprint/pressing stamina depletion inversely with each player's imported CSV `Stamina` value ($1\text{--}99$) and exports exact ending `current_stamina` at full-time.

   * **Attribute & OVR-Driven Attack/Defense Chances (`MatchEngine.cpp`)**: Evaluates `Shooting`, `Pace`, `Dribbling`, `Passing`, `Defending`, `Physical`, and positional sub-weights alongside `OVR` so individual attributes shape shot selection, dribbling, interceptions, and tackles without making `OVR` deterministic.

   * **Scaled Opponent Defensive AI & Balanced Chance Probability**: Scales defensive compactness, pressing triggers, and lane coverage with opponent `OVR` and tactics while keeping weaker teams competitive.

   * **Real-Time Believable Movement**: Replaces instant point-to-point movement with continuous 2D acceleration, deceleration, turn-rate inertia, and `Pace`/stamina-scaled top speed across mouse and touchscreen inputs.

   * **Match Input Mapping Audit**: Current `MatchScene.gd` maps `KEY_R` to `_request_next_match()` after full time; it is not an Advanced Dribbling binding. Preserve the Next Match shortcut and verify it is unavailable/harmless during live play. Do not remove internal dribble boost timers as part of an input cleanup.

### Phase 5F — Stamina Persistence, $\approx 30\%$/Day Recovery, Injury System & Match-Control Refinements `[IMPLEMENTED & USER-VERIFIED]`

1. **Dependencies & Partial Support**:

   * Depends on Phase 5E exporting post-match `current_stamina` and match injury events, and integrates with `CareerState::advance_day()`, `SquadView`, `PreMatchView`, `InboxView` (which already has an `"INJURY"` filter chip), and `save_career()` / `load_career()`.

2. **Planned Deliverables**:

   * **Stamina Persistence & Configurable Calendar Recovery**: Preserves post-match stamina without automatic full resets and recovers `stamina_recovery_rate_per_day` (default `0.30` / $30\%$ of max stamina per in-game calendar day, tunable in `0.25–0.30` range, capped at $100\%$).

   * **World Soccer Champs-Inspired Injury System**: Match-sustained injuries with four severity tiers: Minor Injury (3–5 days), Moderate Injury (7–14 days), Major Injury (30–60 days), and Severe Injury (120–180 days / 4–6 months). A Severe Injury applies a one-time 5–10% current market-value reduction. Daily calendar countdown in `advance_day()`, squad selection lock while injured, `"INJURY"` Inbox alerts and JSON save persistence.

   * **User-Controlled Dribbling Pace & Defensive Recovery Balance (`MatchEngine`)**:
     * Balance the user's attacker and opposing defenders using their individual `Pace` attributes and current stamina/fatigue during attacking chances; avoid a universal dribble-speed boost that ignores player stats.
     * When the user commands a long-distance dribble, a faster or fresher defender must be able to accelerate, catch the attacker and win the ball. A defender with lower effective pace should not automatically outrun a faster attacker.
     * Preserve responsive close control while tuning acceleration, top speed, turning, ball-carrying slowdown and defensive recovery. Add tests for multiple attacker/defender `Pace` matchups, stamina levels, and short- versus long-distance dribbles.

   * **Sliding Challenge & Loose-Ball Behavior (`MatchEngine`)**: The controlled defender performs a short slide toward the ball. Contact sends the ball loose rather than guaranteeing possession; the ball may reach an opposing attacker, who can recover it. Verify timing, distance, attributes and deflection outcomes.

   * **GK Rush Behavior (`MatchEngine`)**: The managed goalkeeper leaves the goal area and moves toward the ball to attempt a catch or clearance. Poor timing can leave the goal unguarded and allow an opponent to score. Verify HOME/AWAY keeper selection, catches, clearances and contested-ball outcomes.


### Phase 5G — Match Laws, Offside, Restarts & Disciplinary System [IMPLEMENTATION COMPLETE; ACCEPTANCE QA DEFERRED]
1. Detect offside at the instant the ball is played, using the second-last opponent, halfway line and ball position. Track all potentially offside attackers and penalize only when a player in an offside position becomes actively involved (interferes with play/opponent or gains an advantage). Preserve the kick-time snapshot; cover own-half, level-position and restart/deflection exceptions. **When offside is confirmed, immediately end the currently active attacking/defensive chance, cancel pending pass/shot/chance resolution so play cannot continue to a shot or goal, and award an indirect free kick to the opposing team at the applicable offside restart location.**
2. Award a goal kick when the attacking side last touches the ball before it crosses the goal line outside the goal; award a corner when the defending side last touches it. Award throw-ins on touchline exits. A throw-in is a separate restart, not an attack/defend chance. Track last touch, taker, restart team and clock pause/resume; apply offside exceptions for goal kicks, corners and throw-ins.
3. Implement and audit tactical foul detection across standing/slide/contact tackle paths, correct direct/indirect free-kick transitions, yellow accumulation, second-yellow dismissal and direct-red dismissal. Dismissed players cannot participate in subsequent play.
4. Opponent tackles/slide tackles can very rarely injure the user's attacker. Keep injury risk low and fatigue/contact dependent; injury outcome must be independent from disciplinary outcome and connect through the Phase 5F career lifecycle.
5. **Acceptance**: offside/onside/level/own-half/active-involvement/exception cases; both-team confirmation that the active chance ends immediately, pending actions are cancelled, no shot/goal follows, and opponent receives correct indirect free kick; both-team goal kicks/corners/throw-ins; throw-ins never create chances by themselves; foul coverage across tackle types; card accumulation/dismissals and sent-off exclusion; rare injury handoff/persistence and injury/card independence; dedicated regression runner, MinGW build and Godot runtime QA.
### Phase 5H — Relative Team-OVR Chance Frequency & Goalkeeper Intelligence
1. Use actual matchday Starting XI OVRs. The higher-OVR team receives a moderately higher share/frequency of attacking chances and fewer defensive chances; reverse this for the weaker team. Blend OVR with tactics, fatigue, player matchups and bounded randomness; preserve underdog counters and upsets.
2. Pass actual Diving, Reflex, Positioning and Reaction values into MatchEngine. Diving/Reflex influence save reach, reaction and save probability; do not substitute outfield stats for diving.
3. Keeper AI adjusts forward slightly while the ball is in the opponent half, tracks laterally, and retreats toward the goal/appropriate angle when play enters its own half or danger zone. Avoid unrealistic wandering and counter-attack exposure.
4. **Acceptance**: equal/small/large OVR gaps with HOME/AWAY swapped; chance-share tests; keeper attribute comparisons against identical shots; movement during opponent-half possession, own-half attacks, wide play and turnovers.
5. **Cross-Screen Visual Consistency**: Use the Pre-Match Center as the visual reference for layout consistency, then apply the shared natural color palette and visual direction defined in Phase 5M across the Main Menu, CareerShell/management views and Match UI. Align typography, component styling, spacing and overall visual language while preserving screen-specific workflows and information hierarchy and adapting layouts to desktop and narrow/mobile viewports. This is a planned UI consistency deliverable; no implementation or visual QA is claimed.
6. **Tuesday League -> Premier League Consolidation & Redeem-Code Conflict Audit**: Merge the former Tuesday League into the single canonical Premier League identity, including 11A7 (ClubID 1098). Update `clubs.csv` or apply one explicit migration and normalize the league key across CareerState, New Career, CompetitionView, fixtures, standings, schedules and saves. Remove the active Tuesday League domestic entry and old 11A7 league/New Career gate. Preserve all ClubIDs, player IDs, roster links, club metadata, kits and finances; each club must belong to exactly one domestic league. Rebuild affected fixtures/standings and prevent duplicate clubs, fixtures, match IDs or rows; do not discard any Tuesday League clubs or players. Audit `RedeemCodeModal`, `NHHA72528` and related reward/eligibility checks: league membership and competition participation must not depend on redeem-code activation, while any separate custom-player/Transfer Market unlock remains isolated. Test fresh careers, legacy-save migration, exact-once 11A7 membership, no active Tuesday League entry, and redeem-code behavior.
7. **UEFA Champions League Participation for 11A7**: Explicitly register 11A7 as a Champions League participant after its Premier League migration and include it in UCL fixture generation and competition progression. Ensure ScheduleView/CompetitionsView show its fixtures and prevent duplicate UCL registrations, fixtures or match IDs. Keep domestic and continental competition registrations distinct. UCL-wide generation/qualification/progression remains part of the existing Phase 5B work; 5H guarantees 11A7 is included when generated.

### Phase 5I — Stoppage Time & Knockout Extra Time
1. Add approximately 2–5 minutes at the end of each half, informed by goals, substitutions, injuries and delays. After minimum added time, do not whistle while the ball remains active; finish the current passage when it reaches a genuine dead-ball/out-of-play state, with a hard safety cap.
2. Only eligible knockout semi-finals/finals (or explicitly flagged knockout fixtures) tied after 90 minutes play two 15-minute extra-time halves. League matches do not enter extra time. Recover approximately 5% maximum stamina once at the interval, capped at 100%.
3. If still tied after 120 minutes, run a penalty shootout where competition rules require a winner; persist shootout outcome separately from match score.
4. **Acceptance**: 2–5 minute bounds; live-ball continuation at 45/90; league draws end after stoppage; knockout progression through 105/120; one-time 5% recovery; shootout persistence.

### Phase 5J — Dedicated MatchConclusion & Match Timeline
1. Add demo/MatchConclusion.gd (and .tscn if appropriate) as the full-time view, replacing or superseding PostMatchModal / MatchHUD's generic report. Preserve MatchScene -> CareerShell navigation.
2. Remove Play Next Match [R] and the KEY_R post-match shortcut. Return to CareerShell instead of starting another fixture. Remove skip-token balance/progress/reward UI and retire the user-facing skip-token economy; retained legacy methods must not grant post-match rewards.
3. Award Player of the Match using a transparent contribution score covering goals, assists, key passes, goalkeeper saves, tackles, interceptions, blocks and overall influence. Show name, position, club and key stats.
4. Show every goal in chronological order with exact scorer name/minute and optional assist; distinguish normal/stoppage/extra-time goals. Use real MatchEngine goal events, never infer scorer from team score.
5. Include final score, clubs, key statistics, cards and injuries. Persist events/performance through CareerState commit/save/load; do not fabricate events for legacy skipped matches.
6. **NHHA72528 visibility gate for former Tuesday League records**: Until the user successfully enters/redeems `NHHA72528` in the Codes flow, hide all designated former Tuesday League club and player records from relevant club/player selection, browsing and search surfaces. Use stable ClubID/player identity or explicit origin/unlock metadata rather than `League == "Tuesday League"`, because Phase 5H migrates those records into the canonical Premier League. Redemption unlocks visibility. This gate affects presentation/access only; it must not block domestic membership, fixtures, standings, season progression or Champions League participation.
7. **SquadView deselection**: When a player is selected for position swapping, clicking an unoccupied/blank area of the SquadView pitch or roster surface clears the selection without changing player positions.
8. **Default captain**: Each team's captain defaults to the highest-OVR player in its Starting XI only when no explicit captain choice is saved. Once manually changed, preserve that captain through refreshes, lineup recalculation and Save/Load.
9. **Per-player weekly wage**: Add a `Wage` column to `demo/data/players_2006_with_price.csv` and keep `Players_Database/players_2006_with_price.csv` synchronized. Load, expose and persist the canonical per-player weekly wage through PlayerDatabase/CareerState/player dictionaries. Squad/profile/contract views and weekly payroll must use the same field. Phase 6A's Wage Budget ledger consumes these values; Phase 7B contract negotiations update this canonical `Wage` field rather than creating a duplicate.
10. **Contract-year synchronization**: Treat the `Contract` column in `demo/data/players_2006_with_price.csv` as the canonical per-player contract-years value and keep `Players_Database/players_2006_with_price.csv` synchronized. Load and expose it through PlayerDatabase/CareerState/player dictionaries. Squad/profile/contract displays, contract expiry calculations and weekly/season progression must use this same value. Contract renewals and Phase 7B transfer negotiations must write the agreed duration back to this `Contract` field, rather than only changing UI state or a duplicate property. Preserve the Phase 7F age-37 contract-duration cap and persist updates through Save/Load.
11. **Acceptance**: POTM tie-breaks/GK awards; multiple scorers/assists; no-goal draw; stoppage/extra-time goal minutes; cards/injuries; save/load; no post-match R shortcut or skip-token UI/reward; code-gated former Tuesday League club/player visibility before/after redemption and across migration/Save-Load; no competition participation lock; blank-space deselection without swaps; highest-OVR Starting XI captain default for both teams and manual override persistence; Wage CSV schema/parity, parsing, player display, weekly payroll and Save/Load; Contract CSV schema/parity, parsing, display, renewal/negotiation write-back, expiry/age-37 cap and Save/Load.
### Phase 5K — Audio System, Stadium Atmosphere, Contextual Commentary & Goal Celebrations

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

### Phase 5M — Main UI Color Palette & Natural Visual Identity Refinement

1. Establish shared color tokens for the Main Menu and CareerShell/management views: backgrounds, surfaces, borders, primary/secondary text, muted metadata, primary/secondary actions, focus/selection and semantic success/warning/error states.
2. Refine the palette toward a restrained, natural, human-designed football-management aesthetic. Avoid synthetic neon gradients, glow effects, excessive saturation and decorative color noise; use pitch-green purposefully and preserve club kit colors as team identity.
3. Apply tokens through shared Godot theme resources/style helpers rather than isolated hard-coded colors. Maintain readable contrast and coherent desktop/narrow-mobile behavior.
4. Phase 5H consumes this palette when aligning Match UI and Pre-Match styling; Phase 6C uses it for typography and player-card refinements without introducing a competing color system.
5. **Acceptance**: Review Main Menu, New Career, Dashboard, Squad, Tactics, Youth, Transfers, Pre-Match and Match screens as one product. Confirm consistent surface/text/action/state colors, readable contrast, restrained accents, no neon-gradient/glow clutter, preserved kit identity and desktop/narrow-mobile visual quality.

### Phase 6B–6D — Senior Squad Training, Global UI/Typography, Match Controls & Player Attributes

1. **Dependencies & Partial Support**:

   * **Training (`Phase 6B`)**: Builds on Phase 4A's weekly XP progression pattern and Phase 6A's `Training Complex` facility tier, extending `CareerShell` with a `"training"` route (`demo/TrainingView.gd` / `.tscn`) and saving assignments/XP in `<slot>.json`.

   * **UI & Typography (`Phase 6C`)**: Builds on Phase 3B/3C's `StyleBoxFlat` `"normal"` `Label` rules and centered modal standards across all views.

2. **Planned Deliverables**:

   * **Dedicated `Training` Tab (`TrainingView`)**: UI-only positional group session assignment for **Attackers (`ST, CF, LW, RW`)**, **Midfielders (`CDM, CM, CAM, LM, RM`)**, **Defenders (`CB, LB, RB`)**, and **Goalkeepers (`GK`)**, driving gradual, potential- and age-differentiated attribute and OVR growth over in-game calendar days.

   * **UI & Typography Design System**: Bundled clean geometric/condensed font stack + tabular numerals, redesigned high-clarity player cards, and strict avoidance of AI-slop aesthetics (no heavy gradients, glowing drop-shadows, over-rounded cards, or generic dashboard clutter).

### Phase 6D & Phase 7E/7F — Player-Facing Playstyle Display, Gated Gameplay & Retirement

* **Phase 6D (Match-Control Polish & Player-Facing Display)**: Completes any remaining input/UI integration checks for goalkeeper behaviors specified in Phase 5F/5H, without duplicating their gameplay implementation, and exposes the three existing `Playstyle 1..3` metadata fields in player attribute/profile views. This does not grant gameplay effects or alter eligibility.\n\n* **Phase 7E (`PlaystylesDescription.txt`)**: Unlocks the 22 Playstyles exclusively for senior players with `BaseOVR > 96 && !IsAcademyProduct`, wiring `Playstyle 1..3` metadata into `MatchEngine` behaviors and the relevant player UI.

* **Phase 7F (Mandatory Age-37 Retirement)**: Reads and writes contract-years through the canonical Phase 5J `Contract` CSV field. Builds on the committed `RetirementAge = 37` data/save normalization to add contract length clamping ($\min(5, \max(1, 37 - \text{Age}))$), `"Retiring at End of Season"` status badges at `Age == 36`, July 1 rollover removal of `Age >= 37` players from active squads/market/match payloads, automatic Best-XI rebalance (preserving `GK` in Slot `0`), and a `"BOARD"` Inbox retirement report.

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

  * Verify `KEY_R` remains the post-match Next Match shortcut, does not trigger during live play, and does not interfere with pass/shot/tackle/dribble controls; verify real-time steering movement velocity ramps.

  * `GOAL_CELEBRATION` `3.0–5.0s` touchline run timer & kickoff reset verification, contextual event emission (`GOAL`, `SAVE`, `FOUL`, `SUBSTITUTION`), home vs. away goal announcement routing, and `AudioManager` menu/match bus crossfade checks.

* **Phase 6 & 7 Regression Suites**:\n\n  * Phase 6D: verify Phase 5H goalkeeper intelligence and normal keeper return behavior; Phase 5F GK Rush is complete. Verify all three Playstyle fields render in order and empty values do not produce broken UI.

  * `TrainingView` 4-group assignment persistence, OVR recalculation after attribute XP thresholds, and age/potential growth differentiation.

  * 6-criteria Transfer Search (`Name`, `Club`, `Age`, `Nationality`, `League`, `Position`), `NHHA72528` former-Tuesday-League visibility gate (shared Phase 5J implementation), $> 96$ Base OVR Playstyles gate, and July 1 Age-37 mandatory retirement.

## 5. File Inventory & Cleanup Checklist

### Active Project Files

* **C++23 GDExtension Core (`src/`)**:

  * `src/match_engine.h` / `src/match_engine.cpp` (Phase 1–2C simulation engine; Phase 5E user-verified stamina/attribute/AI/movement implementation in `30e281a`, HUD/MatchScene follow-ups in `9a1e9a5` and `0822a95`; stray-token compile blocker in `_compute_tactical_target` was removed in `52729a`; remaining clearance/defender coupling and balance verification; planned Phase 5K `GOAL_CELEBRATION` 3–5s state & event hooks; planned Phase 7E Playstyle hooks)

  * `src/CareerState.h` / `src/CareerState.cpp` (Phase 3A & Phase 4A central career state, GK-only Slot `0` guard, GK 6-stat export, player metadata & Age-37 save normalization; planned Phase 5D `clubs.csv` & Codes; planned Phase 5F/5G/6B/7E/7F systems)

  * `src/mainplayer.h` / `src/mainplayer.cpp` (GK 6-stat attribute accessors, player metadata, `RetirementAge = 37`; planned `Stamina`, `CurrentStamina`, `IsInjured`, `IsRetired`) & `src/team.h` / `src/team.cpp`

* **Data & CSV Registries (`demo/data/` & `Players_Database/`)**:

  * `demo/data/formations.csv` (Normalized 11-slot formation shapes)

  * `demo/data/clubs.csv` (Phase 5D 13-column registry including Stadium; 156 records / 28 league labels / 12 single-club leagues; historical audit remains outstanding)

  * `demo/data/players_2006_with_price.csv` (`res://data/players_2006_with_price.csv` — active runtime dataset with `Club`, `League`, `Nationality`, `Stamina`, and `Playstyle 1..3`)

  * `Players_Database/players_2006_with_price.csv` (retained separate preventive/source copy)

  * `demo/data/PlaystylesDescription.txt` (22-Playstyle reference; gameplay integration planned in Phase 7E)

* **GDScript UI & Scenes (`demo/`)**:

  * `demo/CareerShell.gd` / `demo/CareerShell.tscn` (Global route shell, header, modals, toast)

  * `demo/AudioManager.gd` & `demo/assets/audio/` (planned Phase 5K menu/career music, stadium ambience, commentary, goal announcements & full-time audio controller)
  * `demo/MatchConclusion.gd` / `.tscn` (planned Phase 5J full-time summary, POTM, goal timeline and CareerShell return)

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
| **Suite 11** | `YouthView` & Academy Growth | Facility/Scouting upgrades deduct `club_balance` and fail gracefully on insufficient funds; coach assignment scales weekly XP; 7-day `advance_day()` increments training XP without premature age `+1`; annual rollover increments age; `promote_youth_player()` moves prospect to senior squad + posts Inbox alert; youth-coach hiring, wages, affordability/duplicate guards and Save/Load. | 
| **Suite 12** | Phase 4 Full Round-Trip | Mutate XI, roles, kit numbers, permanent tactics, youth upgrades, and promote a prospect -> `save_career()` -> `load_career()` -> verify exact state parity. | 

### Step 3 — Manual Cross-Viewport & Interactive QA Checklist

1. **Desktop Viewport (`1280x720`)**:

   * Open `Squad` tab -> verify 11 pitch markers match `formations.csv` geometry, attempt to drag and click-swap an outfield player onto the `GK` slot (confirm it is blocked with the GK-only warning banner), swap a bench `GK` onto Slot `0` (confirm allowed), edit Captain and jersey number, and inspect a `GK` in `PlayerProfileModal` to verify the 6 Goalkeeper stats (`Diving`, `Reflex`, `Positioning`, `Reaction`, `Kicking`, `Physical`).

   * Open `Tactics` tab -> switch formation (`4-3-3` -> `4-2-3-1`), adjust `Pressing` and `Tempo` sliders, verify stamina/risk tooltip updates, switch away and back to confirm persistence.

   * Open `Youth` tab -> upgrade Academy Facility (confirm top header balance pill decrements immediately), assign a youth coach, advance 7 days via `+1 Day` to verify weekly training progress ticks, and promote an eligible prospect (confirm unread Inbox badge increments and prospect appears in `Squad` reserves).

2. **Narrow / Mobile Viewport (`800x480` & Portrait)**:

   * Confirm `CareerShell` sidebar collapses into drawer (`< 780px`), `SquadView` pitch + bench stack cleanly without horizontal overflow, and `PlayerProfileModal` remains centered within viewport bounds.