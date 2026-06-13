# Current build scope

This repo starts with trajectory simulation only:

1. Two-body propagation
2. Integrator comparison
3. Orbit diagnostics
4. Impulsive burns
5. Hohmann transfers
6. Finite burns with mass depletion
7. Basic multibody dynamics
8. Gravity-assist-style flyby demo

Not included yet:

- trajectory search
- reachability
- Monte Carlo
- CR3BP manifolds
- orbit determination
- EKF
- GNC
- 6-DOF spacecraft simulation
- mission design UI

The point is to get a trustworthy dynamics kernel before adding optimization and control.
