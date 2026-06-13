#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build
cmake --build build

./build/two_body_compare
./build/hohmann_transfer
./build/finite_burn_raise
./build/sun_jupiter_asteroid
./build/gravity_assist_demo

python3 scripts/plot_csv.py outputs/csv/two_body_compare.csv --kind orbit
python3 scripts/plot_csv.py outputs/csv/two_body_compare.csv --kind energy
python3 scripts/plot_csv.py outputs/csv/hohmann_transfer.csv --kind orbit
python3 scripts/plot_csv.py outputs/csv/finite_burn_raise.csv --kind orbit
python3 scripts/plot_csv.py outputs/csv/finite_burn_raise.csv --kind mass
python3 scripts/plot_csv.py outputs/csv/sun_jupiter_asteroid.csv --kind orbit
python3 scripts/plot_csv.py outputs/csv/gravity_assist_demo.csv --kind orbit
