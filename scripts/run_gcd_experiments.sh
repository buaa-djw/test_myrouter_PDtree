#!/usr/bin/env bash
set -e
for c in g++ cmake python3; do command -v $c >/dev/null || { echo "$c missing"; exit 1; }; done
mkdir -p build
cd build
cmake ..
make -j$(nproc)
cd ..
./build/pdtree_experiment --config configs/baseline.json
./build/pdtree_experiment --config configs/proposed_default.json
./build/pdtree_experiment --config configs/proposed_high_hbt_rc.json
python3 scripts/plot_3d_nets.py --root results/gcd_baseline
python3 scripts/plot_3d_nets.py --root results/gcd_proposed_default
python3 scripts/plot_3d_nets.py --root results/gcd_proposed_high_hbt_rc
find results -name summary.rpt -print
