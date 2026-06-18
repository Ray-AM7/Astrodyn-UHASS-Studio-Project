# Astrodyn Sim

Astrodyn Sim is an interactive 2D astrodynamics sandbox for building, visualizing, and experimenting with orbital systems. The current Phase 1 version focuses on a clean object-oriented simulation framework, live visualization, editable objects, preset orbital systems, and basic spacecraft/test-particle maneuver controls.

The long-term goal is to grow this into a trajectory-design and orbital maneuvering tool with finite-burn planning, intercept/repositioning simulations, reachability/probability tracing, Monte Carlo studies, and eventually GNC-style modules.

## Current Features

* Interactive 2D simulation viewport using SFML
* Dear ImGui interface for buttons, fields, object editing, and simulation controls
* Mouse-based navigation

  * Scroll wheel zoom
  * Right-click drag pan
  * Left-click object/label selection
* Starfield background for visual navigation
* Barycenter origin marker
* Labels that remain visible even when objects are too small to see at true scale
* True-size rendering option for physical body radii
* Editable object properties through GUI fields
* Preset loading

  * Blank sandbox
  * Earth-Moon system
  * Solar System preset
* Object creation from GUI
* Circular orbit initialization when creating new objects near a selected body
* Gravity source toggle per object
* Affected-by-gravity toggle per object
* Dynamic/fixed-body toggle
* Trail rendering
* Basic collision handling

  * Small objects can merge/stick into much larger bodies
  * Comparable objects use a simple elastic collision response
* Universe-level propagation methods

  * Symplectic Euler
  * Velocity Verlet
  * RK4
* Relative kinematics display

  * Velocity relative to barycenter
  * Velocity relative to strongest gravity source
  * Tangential velocity vector and tangential speed
  * Spacecraft velocity relative to origin and target bodies
* Basic maneuver controls for spacecraft and test particles

  * Impulsive prograde/retrograde burns
  * Continuous prograde burn toggle
  * Editable burn acceleration

## Object Model

Every object in the simulation inherits from the abstract `SpaceObject` base class.

Current object hierarchy:

```text
SpaceObject
├── LargeBody
├── SmallBody
├── SpaceCraft
└── TestParticle
```

### SpaceObject

Base properties include:

* Position vector
* Velocity vector
* Momentum vector
* Spin angular momentum placeholder
* Orbital angular momentum calculation
* Gravity enabled toggle
* Affected-by-gravity toggle
* Dynamic/fixed toggle
* Collision toggle
* Trail toggle
* Origin body index
* Target body index
* Continuous burn settings

### LargeBody

Used for planets, stars, and moons.

Additional properties:

* Mass
* Radius
* Atmosphere radius
* Atmosphere density
* Average temperature

### SmallBody

Used for asteroids, comets, and small natural bodies.

Additional properties:

* Mass
* Density
* Ice content
* Average temperature

### SpaceCraft

Used for spacecraft and controlled vehicles.

Additional properties:

* Mass
* Battery/charge fraction
* Fuel
* Engine impulse
* Thrust
* Drag coefficient
* Sun vector placeholder
* Earth vector placeholder
* Star vector placeholder
* Origin body
* Target body

### TestParticle

Used for future Monte Carlo and statistical tracing workflows.

Additional properties:

* Particle area
* Area density
* Shape ID
* Monte Carlo / Statistical Tracing mode toggle

## Presets

### Blank Sandbox

Starts with an empty universe.

### Earth-Moon System

Loads:

* Earth
* Moon
* A spacecraft in low Earth orbit

The Earth is fixed at the initial origin, and the Moon is initialized at an approximate real Earth-Moon orbital distance.

### Solar System

Loads:

* Sun
* Mercury
* Venus
* Earth
* Moon
* Mars
* Phobos
* Deimos
* Ceres
* Jupiter
* Saturn
* Uranus
* Neptune

The Solar System preset uses approximate real masses, radii, semi-major axes, and eccentricities. Angular positions are not date-accurate yet. This is a simplified 2D orbital sandbox, not a full ephemeris model.

## Controls

Most controls are available through the Dear ImGui interface.

Mouse controls:

```text
Left click        Select object or label
Right click drag  Pan camera
Scroll wheel      Zoom around cursor
```

Keyboard controls currently kept for maneuver testing:

```text
Space  Pause/play
B      Prograde impulsive burn on selected spacecraft/test particle
N      Retrograde impulsive burn on selected spacecraft/test particle
T      Toggle continuous burn on selected spacecraft/test particle
```

## Build Instructions

### Dependencies

Install required build tools and SFML:

```bash
sudo apt update
sudo apt install -y build-essential cmake libsfml-dev
```

Dear ImGui and ImGui-SFML are vendored in:

```text
external/imgui
external/imgui-sfml
```

Recommended compatible versions:

```text
imgui      v1.89.9
imgui-sfml v2.6
SFML       2.6.1
```

### Build

```bash
rm -rf build
cmake -S . -B build
cmake --build build -j4
./build/astrodyn_app
```

## Current Limitations

This is a Phase 1 simulation framework, not a high-fidelity mission analysis tool yet.

Current limitations:

* 2D only
* Presets are approximate, not date-matched ephemerides
* No SPICE/JPL Horizons integration yet
* No atmosphere drag model yet
* No thrust direction control beyond simple prograde/retrograde behavior
* No fuel depletion model yet
* No spacecraft attitude dynamics yet
* No Monte Carlo or statistical tracing implementation yet
* No finite-burn trajectory planner yet
* No GNC, EKF, PID, or LQR modules yet

## Planned Next Steps

Future development goals include:

* Real ephemeris import using SPICE or JPL Horizons
* 3D vectors and quaternions
* Finite-burn maneuver simulation
* Propellant usage and delta-v accounting
* Intercept/repositioning guidance
* Monte Carlo uncertainty propagation
* Statistical tracing and reachability visualization
* Spacecraft attitude dynamics
* Sensor models
* EKF/state estimation
* PID/LQR control
* Trajectory tracking
* Public demo plots and writeup

## Project Direction

Astrodyn Sim is intended to become a practical orbital maneuvering and trajectory sandbox, especially for rapid repositioning, finite-burn vehicle behavior, and propulsion-aware orbital operations. Phase 1 establishes the object model, visual interface, editable simulation state, and basic propagation engine needed for those later features.
