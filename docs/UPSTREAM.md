# Source provenance

The editable `engine/` tree was initially imported without modifications from
https://github.com/ioquake/ioq3 at commit
`83a776283bdb958f82db25554b5ed0966aaf6e49`.
Its upstream README and license notices are retained. Engine and game code
are GPL-2.0-or-later; see `engine/COPYING.txt` and per-file notices.

Halo reference: https://github.com/cybersecurity/halo-ce-universal at
`07302a866f8b36c162203fe58dcf632d4708d65c`.
This is a decompilation port of Xbox build 2342, not a drop-in ioquake3 library.
Its repository contains a CC0 declaration, but that declaration does not
supply rights to Microsoft's game assets. No Halo executable code or visual/audio assets are imported
in this foundation. Numeric combat scalars are now extracted from the user's
build 2276 maps; see HALO-IMPORT.md. Neither game's commercial maps, models, textures, sounds,
or binary tags are bundled.

Behavior work also uses reference commit `76b1898ee14e6fb58e0412acc183da509c10e001`
(build2342); build differences and adaptations are recorded in HALO-IMPORT.md.

Local changes add shared tag-driven movement and a generated gameplay profile,
shield/damage rules, two-slot inventory/reload/replacement, grenade selection
and attachment, melee lunge/backsmacks, precision/zoom, finite projectile motion,
material/damage curves, trigger ramps and fractional battery state. Native and QVM
modules share definitions; CMake tracks headers and gameplay profile inputs.
SDL2 development source is pinned to release 2.32.8
(`98d1f3a45aae568ccd6ed5fec179330f47d4d356`) and installed locally under
`/workspace/.qce-sdl`; SDL retains its upstream zlib license.

The offline Xbox layout table/reader use Invader schema references at
`696830ff80af227e84e7237c2ef26eb2301ed110`. Their GPL-3.0-only notice and
license are under `data/halo-layout/`; these tools are not linked into the game.
