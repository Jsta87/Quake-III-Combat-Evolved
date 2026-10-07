#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="$root/.tools/bin:$PATH"
# Local SDL development build; no root/system package changes are needed.
if [[ ! -d /workspace/.qce-sdl-src ]]; then
 git clone --depth 1 --branch release-2.32.8 https://github.com/libsdl-org/SDL.git /workspace/.qce-sdl-src
fi
[[ "$(git -C /workspace/.qce-sdl-src rev-parse HEAD)" == 98d1f3a45aae568ccd6ed5fec179330f47d4d356 ]] || { echo "Unexpected SDL source revision" >&2; exit 1; }
cmake -S /workspace/.qce-sdl-src -B /workspace/.qce-sdl-build -G Ninja \
 -DCMAKE_INSTALL_PREFIX=/workspace/.qce-sdl -DSDL_TEST=OFF -DSDL_STATIC=OFF
cmake --build /workspace/.qce-sdl-build --parallel "${QCE_BUILD_JOBS:-4}"
cmake --install /workspace/.qce-sdl-build
"$root/scripts/build.sh" client
