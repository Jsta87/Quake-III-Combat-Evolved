#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/build/tests"
sdl_prefix="${QCE_SDL_PREFIX:-$root/.tools/sdl}"
if [[ -d "$sdl_prefix/include/SDL2" ]]; then
 sdl_flags=("-I$sdl_prefix/include/SDL2")
elif pkg-config --exists sdl2; then
 read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"
elif [[ -d /workspace/.qce-sdl/include/SDL2 ]]; then
 sdl_flags=(-I/workspace/.qce-sdl/include/SDL2)
else
 echo "SDL2 development headers required (see README.md)" >&2; exit 2
fi
for renderer in renderergl1 renderergl2; do
 flags=()
 if [[ "$renderer" == renderergl2 ]]; then flags=(-DTEST_GL2); fi
 cc -std=c99 -O2 -ffunction-sections -fdata-sections "${flags[@]}" \
  "${sdl_flags[@]}" \
  "$root/tests/iqm-tags.c" "$root/engine/code/$renderer/tr_model_iqm.c" \
  "$root/engine/code/qcommon/q_math.c" "$root/engine/code/qcommon/q_shared.c" \
  -Wl,--gc-sections -lm -o "$root/build/tests/iqm-tags-$renderer"
 "$root/build/tests/iqm-tags-$renderer"
done
