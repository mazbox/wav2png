#!/usr/bin/env bash
# Build wav2png without cmake/cpm. All deps (dr_libs, stb) are vendored in src/.
set -euo pipefail

cd "$(dirname "$0")"

CXX="${CXX:-c++}"
OUT="${OUT:-wav2png}"

echo "Compiling $OUT..."
"$CXX" -O3 -std=c++17 -I include src/main.cpp -o "$OUT"

echo "Built ./$OUT"
