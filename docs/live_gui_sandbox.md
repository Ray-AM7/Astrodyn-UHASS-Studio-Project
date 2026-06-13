# Live GUI Sandbox

The first starter project used separate example programs that exported CSVs. The next step is a single interactive app:

```bash
sudo apt install -y libsfml-dev
cmake -S . -B build
cmake --build build
./build/orbit_sandbox
```

The GUI dynamically integrates and renders trajectories while the simulation is running. CSV export is optional and happens when you press `X`.

## Controls

- `1`: Earth LEO scenario
- `2`: Earth + Moon perturbation scenario
- `3`: Sun + Jupiter + asteroid scenario
- `4`: moving planet flyby scenario
- `5`: Earth LEO finite-burn sandbox
- `F1`: explicit Euler
- `F2`: symplectic Euler
- `F3`: velocity Verlet
- `F4`: RK4
- `Space`: pause/resume
- `R`: reset current scenario
- `C`: clear trails
- `+` / `-`: increase/decrease simulation speed
- `Up` / `Down`: zoom in/out
- `Left` / `Right`: pan camera horizontally
- `PageUp` / `PageDown`: pan camera vertically
- `B`: apply +50 m/s prograde impulsive burn
- `N`: apply -50 m/s retrograde impulsive burn
- `T`: toggle continuous prograde finite thrust
- `E`: toggle first body's gravity
- `M` or `J`: toggle second body's gravity if present
- `X`: export current trajectory log to `outputs/csv/orbit_sandbox_export.csv`
- `Esc`: quit

## What this app is

This is not yet a full mission designer. It is the bridge between the separate examples and the future Copernicus-like program. It gives you one live sandbox where you can:

- choose a scenario,
- choose the numerical method,
- toggle gravity sources,
- apply simple burns,
- watch trajectories update live,
- and export the trajectory after running.

## What should come after this

The next architectural step is to replace hardcoded scenarios with editable scenario files and a real trajectory segment system:

```text
Scenario
├── Bodies
├── Spacecraft
├── Integrator settings
├── Maneuver events
└── Output settings
```

Then later:

```text
Trajectory
├── CoastSegment
├── ImpulsiveBurnSegment
├── FiniteBurnSegment
└── Event detection
```
