#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mode="${1:-server}"
case "$mode" in
  server) client=OFF ;;
  client) client=ON ;;
  *) echo "Usage: $0 [server|client]" >&2; exit 2 ;;
esac
if [[ -d /workspace/.qce-sdl ]]; then
  export CMAKE_PREFIX_PATH="/workspace/.qce-sdl${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
fi
python3 "$root/scripts/generate-profile.py"
cmake -S "$root/engine" -B "$root/build/$mode" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_CLIENT="$client" \
  -DBUILD_SERVER=ON -DBUILD_GAME_LIBRARIES=ON -DBUILD_GAME_QVMS=ON \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF
cmake --build "$root/build/$mode" --parallel "${QCE_BUILD_JOBS:-4}"
