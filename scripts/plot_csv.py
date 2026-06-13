#!/usr/bin/env python3
"""
Generic plotter for Astrodyn CSV files.
Usage:
  python scripts/plot_csv.py outputs/csv/two_body_compare.csv --kind orbit
  python scripts/plot_csv.py outputs/csv/two_body_compare.csv --kind energy
  python scripts/plot_csv.py outputs/csv/finite_burn_raise.csv --kind mass
"""

import argparse
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt


def savefig(path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    plt.tight_layout()
    plt.savefig(path, dpi=180)
    print(f"wrote {path}")


def plot_orbit(df: pd.DataFrame, out: Path):
    plt.figure(figsize=(8, 8))
    for name, g in df.groupby("object"):
        plt.plot(g["x_m"] / 1000, g["y_m"] / 1000, label=name, linewidth=1.2)
    plt.scatter([0], [0], s=20, marker="o", label="origin")
    plt.axis("equal")
    plt.xlabel("x [km]")
    plt.ylabel("y [km]")
    plt.title("Trajectory")
    plt.legend()
    plt.grid(True, alpha=0.3)
    savefig(out)


def plot_energy(df: pd.DataFrame, out: Path):
    plt.figure(figsize=(9, 5))
    for name, g in df.groupby("object"):
        if g["energy_J_kg"].abs().max() > 0:
            e0 = g["energy_J_kg"].iloc[0]
            plt.plot(g["t_s"] / 3600, (g["energy_J_kg"] - e0) / abs(e0), label=name)
    plt.xlabel("time [hr]")
    plt.ylabel("relative energy error")
    plt.title("Specific orbital energy drift")
    plt.legend()
    plt.grid(True, alpha=0.3)
    savefig(out)


def plot_radius(df: pd.DataFrame, out: Path):
    plt.figure(figsize=(9, 5))
    for name, g in df.groupby("object"):
        plt.plot(g["t_s"] / 3600, g["radius_m"] / 1000, label=name)
    plt.xlabel("time [hr]")
    plt.ylabel("radius [km]")
    plt.title("Radius vs time")
    plt.legend()
    plt.grid(True, alpha=0.3)
    savefig(out)


def plot_mass(df: pd.DataFrame, out: Path):
    plt.figure(figsize=(9, 5))
    for name, g in df.groupby("object"):
        if g["mass_kg"].max() > 0:
            plt.plot(g["t_s"], g["mass_kg"], label=name)
    plt.xlabel("time [s]")
    plt.ylabel("mass [kg]")
    plt.title("Spacecraft mass vs time")
    plt.legend()
    plt.grid(True, alpha=0.3)
    savefig(out)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path)
    parser.add_argument("--kind", choices=["orbit", "energy", "radius", "mass"], default="orbit")
    parser.add_argument("--out", type=Path, default=None)
    args = parser.parse_args()

    df = pd.read_csv(args.csv)
    out = args.out
    if out is None:
        out = Path("outputs/figures") / f"{args.csv.stem}_{args.kind}.png"

    if args.kind == "orbit":
        plot_orbit(df, out)
    elif args.kind == "energy":
        plot_energy(df, out)
    elif args.kind == "radius":
        plot_radius(df, out)
    elif args.kind == "mass":
        plot_mass(df, out)


if __name__ == "__main__":
    main()
