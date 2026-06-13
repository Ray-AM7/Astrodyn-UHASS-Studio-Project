# Astrodyn Starter

Astrodyn Starter is a C++/Python astrodynamics sandbox focused on **single-trajectory simulation**:

- two-body orbital propagation
- integrator comparison: explicit Euler, symplectic Euler, velocity Verlet, RK4
- classical orbit diagnostics
- impulsive burns and Hohmann transfer
- simple finite-burn propagation with mass depletion
- simple multibody Sun/Jupiter/asteroid dynamics
- a gravity-assist-style demonstration
- CSV output and Python plots

This starter intentionally does **not** include trajectory search, reachability, Monte Carlo, EKF, GNC, or 6-DOF spacecraft simulation yet.

## WSL2 setup

Install build tools and Python packages:

```bash
sudo apt update
sudo apt install -y build-essential cmake python3 python3-pip
python3 -m pip install --user pandas matplotlib
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run examples

```bash
./build/two_body_compare
./build/hohmann_transfer
./build/finite_burn_raise
./build/sun_jupiter_asteroid
./build/gravity_assist_demo
```

## Plot outputs

```bash
python3 scripts/plot_csv.py outputs/csv/two_body_compare.csv --kind orbit
python3 scripts/plot_csv.py outputs/csv/two_body_compare.csv --kind energy
python3 scripts/plot_csv.py outputs/csv/hohmann_transfer.csv --kind orbit
python3 scripts/plot_csv.py outputs/csv/finite_burn_raise.csv --kind mass
```

Or run everything:

```bash
bash scripts/run_all_examples.sh
```

## GitHub quick start

```bash
git init
git add .
git commit -m "Initial Astrodyn starter"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/astrodyn.git
git push -u origin main
```

Create the empty GitHub repo first on github.com, then run the commands above locally.

## Live interactive orbit sandbox

The original examples are still useful as small validation demos, but the project now also includes an optional live GUI app using SFML.

Install SFML in WSL2:

```bash
sudo apt install -y libsfml-dev
```

Build:

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/orbit_sandbox
```

The GUI lets you select scenarios, select the integrator, toggle gravity sources, apply simple prograde/retrograde burns, toggle finite thrust, pan/zoom, pause, and export the current trajectory to CSV.

See `docs/live_gui_sandbox.md` for controls.
