# Halo combat presentation and inventory

Rebuild the engine and every module together: this pass uses QCE protocol **98**.
Older clients, servers and demos cannot share this player-state layout.
Generated media remains in ignored local asset folders; no original game assets
are added to Git.

## Player colors

```text
set qce_colorRGB "40 180 70"
set qce_visorRGB "30 160 255"
```

Both accept three whole RGB components from 0 through 255 and are archived userinfo
settings. Visor color updates independently of armor and team armor colors. Invalid
visor input falls back to `255 190 30`. The server always assigns that gold visor
to bots, even if bot userinfo requests another color. The existing static HUD helmet
portrait retains its baked gold visor; the animated player model uses the new color.

The exporter builds separate body/visor skins and neutralizes the visor's source
texture luminance before applying RGB. Both renderer paths use the same skin pass,
including cloak/powerup rendering. Glass reflection still approximates Halo's GPU
material with Quake shader stages.

## Held weapons and switch popup

```text
set gv_maxHeldWeapons 2
map_restart 0
```

`1` through `8` set the capacity for the eight implemented Halo weapons. `0` enables
all-weapons inventory. Two is the default. The limit applies on respawn/restart;
changing a variant never silently discards guns already held. `gv_save`/`gv_load`
include this option, and older version-1 variant files load with the previous
two-weapon default. `qce_status` lists all eight inventory positions.

Each carried gun preserves its own magazine, reserve, battery, heat, recovery,
spread and firing-rate state through switching, dropping and pickup. Prediction
and network serialization include every slot. `give weapons` fills available
slots; no gauntlet is granted in Halo combat. Empty guns remain selectable.
The Quake switch popup is hidden for capacities 1–8 and shown for the `0` variant
or stock Quake combat.

## Projectiles, crosshairs, targeting and shields

The owner-local converter now packages the actual Needler needle and rocket
projectile meshes. Attached needles keep their impact orientation. Plasma rifle,
plasma pistol and charged plasma-pistol bolts use the retail `mgs2` light-volume
lengths, sample counts, radii, radius exponents and ARGB endpoints. Those tags have
null bitmap references: these are volumes rather than the Quake plasma-ball sprite.
Camera-facing samples form one MD3 model per bolt to bound renderer entity usage.
The radial kernel approximates Halo's volume rasterizer. Glass needles carry their
authored reflection tint. Impact flashes use converted Halo energy particles and
Halo plasma/needle impact sounds; rocket impacts share the current frag effect
rather than a complete independent retail rocket particle system.

Each weapon's aim reticle is cropped from its Xbox HUD bitmap sequence with its
source placement and size. Enemy targeting colors the reticle red; allies are green
in team modes, and neutral reticles are blue. There is no Quake pickup-size pulse.
Center-ray targeting supplies direct-hit colors; the assisted red state also uses
CE's full autoaim-strength condition. HUD warnings and all scope decorations are
not yet a complete replacement for the entire Halo weapon HUD.

Projectile autoaim follows recovered CE `aim_assist.c`: nearest point on an aim
pill, pill-width correction, plateau through half the tagged angle/range, linear
attenuation beyond that, ranking by autoaim then magnetism, line-of-sight and team
rejection, zoom-scaled angles/ranges, camera/muzzle parallax correction and a final
deviation-cone limit. It does not turn the player's camera. Sniper assistance is
zoom-only; the rocket's tagged zero autoaim angle produces no bullet correction.
Source pill width is 0.08 Halo units (6.4 game units).

This is not a claim of proven retail parity: the targeting pill currently uses
Quake collision dimensions instead of the animated pelvis/head matrices; controller
stick slowdown and moving-target magnetism are separate, pending input work.
Source-reference functions and tag parameters are reproduced, but comparison with
retail gameplay is still needed, particularly for crouched and animated targets.

Successful shield damage emits an impact effect while shields remain, or one
shield-break event when that hit depletes them. Headshots/backsmacks resolve before
the event is chosen, including exact depletion and lethal hits. Damage to an
already unshielded player emits no shield effect. Animated players get a short gold
shield material pass; the local player gets a faint source shield-impact flash and
imported shield-hit/depletion audio. Breaks also produce a converted depletion
sprite. The shell's additive shader, 250/600 ms presentation windows, and simplified
break burst require retail calibration; Halo GPU combiners are not reproduced.

## Bots

Bots use the same grenade/melee inputs and cooldown/latch rules as people. Melee
requires a living enemy in reach, a forward aim cone, and a clear trace. Grenades
require clear sight, a viable ballistic arc, a safe distance and no nearby teammate
in the blast zone. Plasma aims at the body for sticking; frag aims near the feet.
Bots can choose an available grenade type and hold aim through the release delay.
Inputs persist across a short firing cooldown without creating duplicate throws.
Bot aim/navigation and weapon selection still build on Quake's AI.

## Generate and verify

The normal asset workflow automatically calls `convert-halo-combat.py` from the
world conversion stage. To refresh just these outputs after a full conversion:

```sh
python3 scripts/convert-halo-combat.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch \
  --pk3 assets/baseq3/zzz-qce-halo.pk3
```

Keep the game closed while rewriting its PK3. Source tag records, parameter values,
limitations and hashes appear in the local manifest. The small numeric aim header
is generated into the repository; media stays ignored.

Validation includes native/QVM builds for baseq3 and missionpack, 22 shell regression
suites, texture/audio and animation Python fixtures, native and QVM graphical
smokes, and sustained 128-bot server runs. Headless audio verifies registration and
dispatch, not audible retail sound balance. Manual testing remains necessary for
reticle feel, shield material timing, impact effects and controller behavior.

References: [Halo aim assistance](https://github.com/cybersecurity/halo-ce-universal/blob/main/source/game/aim_assist.c),
[weapon HUD](https://github.com/cybersecurity/halo-ce-universal/blob/main/source/interface/hud_weapon.c),
and [Invader tag layouts](https://github.com/SnowyMouse/invader/tree/696830ff80af227e84e7237c2ef26eb2301ed110/src/tag/hek/definition).
Adapted code and conversion routines retain GPL attribution.
