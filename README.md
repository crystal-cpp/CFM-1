# Crystal Football Manager 1 (CFM-1)

**Dream. Manage. Compete. Build Your Legacy.**

Crystal Football Manager 1 is an open-source, 2D football management and simulation game built with **Godot 4.7** and a **C++23 GDExtension**. It aims to combine realistic football match simulation with deep squad management, tactical decisions, youth development, and long-term managerial progression.

CFM-1 is currently in active development. Its core match simulation and several career-management foundations are implemented, while broader career systems are being developed incrementally.

---

## Project Overview

| Information | Details |
|---|---|
| Project | Crystal Football Manager 1 |
| Abbreviation | CFM-1 |
| Genre | Football Management / Sports Simulation |
| Game Engine | Godot 4.7 |
| Core Language | C++23 |
| UI & Presentation | GDScript |
| Target Platforms | Windows and Android |
| Visual Style | 2D |
| Development Stage | Alpha / Active Development |
| License | See repository license |

## Key Features

### Implemented Foundations

- **2D Real-Time Match Simulation** — A top-down football match engine featuring 22-player simulation, tactical positioning, player movement, and ball interactions.
- **Player Attributes & Positioning** — Attribute-based player profiles, positional compatibility, and formation-driven positioning.
- **Direct Match Controls** — Player movement, passing, through balls, shooting, switching players, and tackling.
- **Match Statistics** — Match telemetry and statistical tracking.
- **Career Management Architecture** — A centralized C++ `CareerState` responsible for managing career data and game progression.
- **Save & Load System** — Persistent career data with atomic JSON save/load functionality.
- **Squad Management** — Starting XI, substitutes, reserves, player swapping, squad roles, and jersey number management.
- **Permanent Tactical Management** — Custom formations and team instructions, kept separate from temporary in-match tactical adjustments.
- **Youth Academy** — Youth development infrastructure, coaching assignments, prospect progression, and promotion into the senior squad.
- **Career Interface** — A unified career navigation system with dashboard, inbox, squad, tactics, youth, and player profile interfaces.

### Planned Features

The following systems are part of the development roadmap and are not yet fully implemented:

- **Fixtures & Competitions** — Calendar, league tables, competition tracking, and season archives.
- **Pre-Match & Post-Match Systems** — Match preparation, result processing, player statistics, and match reports.
- **Club Management** — Club finances, transfer budgets, wage budgets, board objectives, and stadium upgrades.
- **Transfer Market** — Player valuations, transfer negotiations, contracts, and AI-driven market activity.
- **Manager Progression** — Manager reputation, career history, achievements, and opportunities to manage other clubs.
- **Advanced Player Systems** — Dynamic development, loyalty, and special Playstyles for eligible players.
- **Cross-Platform Release** — Further Windows and Android testing, optimization, and packaging.

For detailed implementation progress, see [DEVELOPMENT_STATUS.md](DEVELOPMENT_STATUS.md). For the long-term development plan, see [ROADMAP.md](ROADMAP.md).

---

## Technology Stack

| Component | Technology |
|---|---|
| Game Engine | Godot 4.7 |
| Simulation & Management Core | C++23 |
| Godot Integration | GDExtension |
| Interface & Input | GDScript |
| Build System | SCons |
| C++ Compiler | GCC / MinGW UCRT64 |
| Data Storage | CSV and JSON |

### Architecture Philosophy

CFM-1 follows a 2D-first architecture:

- **C++** handles core simulation, player and team data, career management, and gameplay calculations.
- **GDScript** handles interface rendering, navigation, user interaction, and input handling.
- **Godot** provides the application framework, 2D presentation, and platform integration.

The project prioritizes modular systems, reusable components, deterministic data handling, and separation between gameplay logic and presentation.

A 3D player presentation system is postponed and is not a dependency of the current development roadmap.

---

## Getting Started

### Requirements

The current development environment uses:

- Godot 4.7.
- Python with SCons installed.
- MSYS2 MinGW UCRT64.
- GCC 16.1.0 or a compatible compiler.
- Git.

### Clone the Repository

```bash
git clone https://github.com/crystal-cpp/CFM-1.git
cd CFM-1
```

### Build the C++ GDExtension

Open an MSYS2 MinGW UCRT64 terminal in the project directory and run:

```bash
scons platform=windows target=template_debug api_version=4.7
```

After a successful build, open the project using Godot 4.7.

> **Note:** Build configuration and extension dependencies may change during development. Refer to the project files and development documentation if the build process changes.

---

## Project Structure

The repository is organized around the following major components:

| Directory / File | Responsibility |
|---|---|
| `src/` | Core C++ gameplay and career-management systems |
| `Players_Database/` | Player database and CSV loading |
| `demo/` | Godot interface, scenes, scripts, and game presentation |
| `demo/data/` | Game data and configuration files |
| `ROADMAP.md` | Long-term development roadmap |
| `DEVELOPMENT_STATUS.md` | Current implementation status and engineering handoff |

The project structure may evolve as additional systems are introduced.

---

## Development Roadmap

CFM-1 is developed through incremental milestones:

| Phase | Development Focus | Status |
|---|---|---|
| Phase 1 | Match Engine Foundation | Complete |
| Phase 2 | Match Gameplay, Controls & Tactical Simulation | Complete |
| Phase 3 | Career State, Save/Load & Global Interface | Complete |
| Phase 4 | Squad Management, Permanent Tactics & Youth Academy | Implementation Complete; Verification Ongoing |
| Phase 5 | Fixtures, Competitions & Pre-Match Systems | Planned |
| Phase 6 | Club Management, Finances & Board Objectives | Planned |
| Phase 7 | Transfers, Contracts, Reputation & Playstyles | Planned |
| Phase 8 | Full Acceptance Testing & Multi-Platform Release | Planned |

For detailed milestones, technical specifications, and acceptance criteria, visit [ROADMAP.md](ROADMAP.md).

---

## Contributing

CFM-1 is an open-source project, and contributions are welcome.

Before contributing:

1. Read the project roadmap and development status.
2. Check existing issues before proposing changes.
3. Keep changes modular and consistent with the existing architecture.
4. Avoid introducing unnecessary dependencies.
5. Test changes before submitting a pull request.
6. Document important architectural decisions and changes.

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

---

## Project Status

**CFM-1 is currently an experimental Alpha project under active development.**

Features, interfaces, data structures, and gameplay systems may change between development versions. Not all planned systems are currently available, and the game should not yet be considered a complete football management experience.

Feedback, bug reports, and constructive suggestions are appreciated.

---

## License

See the repository's [LICENSE](LICENSE) file for licensing terms and conditions.

---

**Crystal Football Manager 1**

*Dream. Manage. Compete. Build Your Legacy.*
