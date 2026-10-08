#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="$root/.tools/bin:$PATH"
# Prefer installed development headers. Otherwise build SDL inside this checkout.
if [[ -z "${QCE_SDL_PREFIX:-}" ]] && pkg-config --exists sdl2; then
 "$root/scripts/build.sh" client
 exit 0
fi
# Reuse an existing cloud installation without making local clones depend on it.
if [[ -z "${QCE_SDL_PREFIX:-}" && -d /workspace/.qce-sdl ]]; then
 export QCE_SDL_PREFIX=/workspace/.qce-sdl
 "$root/scripts/build.sh" client
 exit 0
fi
prefix="${QCE_SDL_PREFIX:-$root/.tools/sdl}"
source_dir="$root/.tools/sdl-src"
build_dir="$root/.tools/sdl-build"
if [[ ! -d "$source_dir" ]]; then
 git clone --depth 1 --branch release-2.32.8 https://github.com/libsdl-org/SDL.git "$source_dir"
fi
[[ "$(git -C "$source_dir" rev-parse HEAD)" == 98d1f3a45aae568ccd6ed5fec179330f47d4d356 ]] || { echo "Unexpected SDL source revision" >&2; exit 1; }
cmake -S "$source_dir" -B "$build_dir" -G Ninja \
 -DCMAKE_INSTALL_PREFIX="$prefix" -DSDL_TEST=OFF -DSDL_STATIC=OFF
cmake --build "$build_dir" --parallel "${QCE_BUILD_JOBS:-4}"
cmake --install "$build_dir"
export QCE_SDL_PREFIX="$prefix"
"$root/scripts/build.sh" client
