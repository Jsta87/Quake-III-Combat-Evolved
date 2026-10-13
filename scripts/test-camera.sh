#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
cc -std=c99 -O2 "$root/tests/camera.c" -lm -o "$root/build/tests/camera"
"$root/build/tests/camera"
