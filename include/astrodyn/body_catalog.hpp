#pragma once

#include "astrodyn/sandbox_world.hpp"

namespace astrodyn {

SimObject2D make_sun();
SimObject2D make_mercury(double phase_rad);
SimObject2D make_venus(double phase_rad);
SimObject2D make_earth(double phase_rad);
SimObject2D make_moon_around_earth(const SimObject2D& earth, double phase_rad);
SimObject2D make_mars(double phase_rad);
SimObject2D make_phobos_around_mars(const SimObject2D& mars, double phase_rad);
SimObject2D make_deimos_around_mars(const SimObject2D& mars, double phase_rad);
SimObject2D make_ceres(double phase_rad);

SimObject2D make_spacecraft_near_earth_leo(const SimObject2D& earth, double altitude_m, double phase_rad);

SandboxWorld2D make_empty_world();
SandboxWorld2D make_solar_system_inner_preset();
SandboxWorld2D make_earth_sandbox_preset();

} // namespace astrodyn