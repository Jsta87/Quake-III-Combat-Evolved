#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 -I"$root/engine/code/qcommon" -I"$root/engine/code/game" "$root/tests/aim.c" "$root/engine/code/qcommon/q_math.c" -lm -o "$root/build/tests/aim"
"$root/build/tests/aim"
