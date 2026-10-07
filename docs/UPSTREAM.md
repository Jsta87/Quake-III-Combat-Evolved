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
supply rights to Microsoft's game assets. No Halo code or assets are imported
in this foundation. Neither game's commercial maps, models, textures, sounds,
or tag data are bundled.

Local changes add shared provisional movement and a generated gameplay profile,
shield/damage rules, two-slot inventory/reload/replacement, grenade selection
and attachment, melee lunge/backsmacks and precision exceptions. Native and QVM
modules share definitions; CMake tracks headers and gameplay profile inputs.
SDL2 development source is pinned to release 2.32.8
(`98d1f3a45aae568ccd6ed5fec179330f47d4d356`) and installed locally under
`/workspace/.qce-sdl`; SDL retains its upstream zlib license.
