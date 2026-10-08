# Xbox gameplay import: build 01.10.12.2276

The owned Xbox UI and Blood Gulch maps are version-5 compressed caches, build
`01.10.12.2276` (NTSC-US build identifier), containing 983 and 1,806 tags.
The offline reader checks header/footer, bounded zlib decompression, identities,
dependencies and accessed pointers/reflexives. Zlib validates its checksum;
cache-header CRC is recorded but is not independently verified. Map files and
full extracted manifests remain ignored under `assets/halo/`.

The committed schema-3 profile contains extracted numeric values and converted
animation metadata. It contains no converted art. `data/halo-import-report.json`
records source map/hash, root tag hashes, source trigger/magazine/damage values,
converted fields and remaining adaptations. Root hashes cover root structures;
map hashes cover the source file, including reflexive data.

## Mapping and timings

| Quake slot | Halo weapon | Clip / full battery normal shots | Fire interval at maximum rate | Full / empty reload duration | Ready duration | Melee impact / duration |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| WP_MACHINEGUN | assault rifle | 60 | 67 ms | 2900 / 2900 | 1300 ms | 100 / 1233 ms |
| WP_SHOTGUN | shotgun | 12 | 1000 ms | 400 / 400 | 767 ms | 133 / 1200 ms |
| WP_ROCKET_LAUNCHER | rocket launcher | 2 | 2000 ms | 3700 / 4167 | 733 ms | 133 / 1733 ms |
| WP_RAILGUN | sniper rifle | 4 | 500 ms | 2767 / 3133 | 967 ms | 200 / 1200 ms |
| WP_PLASMAGUN | plasma rifle | 200 | 100 ms | none | 967 ms | 200 / 1200 ms |
| WP_LIGHTNING | plasma pistol | 500 | 33 ms | none | 467 ms | 133 / 1100 ms |
| WP_BFG | pistol | 12 | 300 ms | 2167 / 2233 | 1167 ms | 133 / 1500 ms |
| WP_GRENADE_LAUNCHER | needler | 20 | 100 ms | 2333 / 2333 | 767 ms | 200 / 1600 ms |

Shotgun reload values are per-shell. Plasma-pistol firing is release-driven;
its zero rate-of-fire tag uses a 33-ms minimum interval. Needler ramps 3→10
shots/s over approximately 500 ms; plasma rifle ramps 7→10 over 1,800 ms.
Rate intervals quantize to 30-Hz ticks. Trigger progress carries fractional
remainders and recovers when released or holstered.

Reload/ready/melee metadata is resolved by extracted animation names. Full and
empty reload durations are distinct; melee impact uses the first keyframe.
The build2342 reference uses animation-channel lookups and additional reload,
chamber and cancellation states; exact build2276 event timing still needs
validation and converted animation channels. Frame counts are timing inputs,
not evidence that the rendered Quake animation matches Halo.

## Units and behavior evidence

One Halo world unit maps to 80 Quake units: the .7-WU standing hull becomes 56
Quake units. This is a chosen arena adaptation, not an official conversion.
Cached projectile/jump velocities are WU per 30-Hz tick, so multiply by 30×80.
Directional movement speeds are WU/s; multiply by 80. Reference biped code
converts absolute acceleration to per-tick velocity increments, requiring
30×80 when expressing acceleration in Quake units/s².

| Movement quantity | Applied value |
| --- | ---: |
| Forward / backward / sideways | 180 / 160 / 160 units/s |
| Crouched forward / backward / sideways | 72 / 52 / 48 units/s |
| Ground / crouched / airborne acceleration | 768 / 384 / 84 units/s² |
| Jump impulse | 168 units/s |
| Gravity | 256.6929 projectile; 257 player units/s² |
| Collision radius; standing / crouched height | 16; 56 / 40 units |
| Standing / crouched eye height above feet | 49.6 / 28 units |
| Crouch camera transition | 200 ms |
| Slope falloff / cutoff | 20° / 45° |
| Uphill / downhill terminal scale | .65 / 1.25 |

Ground acceleration approaches the desired vector and also brakes to rest;
Quake ground friction and velocity snapping are bypassed in Halo movement.
No input in the air preserves horizontal momentum. Collision sweeps, stepping,
water movement, hazards, damage impulses, fall/landing behavior and some camera
smoothing still belong to Quake. A controlled jump fixture measures about
54.91 units above standing origin; retail comparison is still required.

Runtime formulas were checked against
[halo-ce-universal](https://github.com/cybersecurity/halo-ce-universal/tree/76b1898ee14e6fb58e0412acc183da509c10e001),
commit `76b1898ee14e6fb58e0412acc183da509c10e001`, **build2342**, which differs from
the uploaded build2276. Reference source is CC0; ioquake3 remains GPL2-or-later.
Factual binary layouts derive from Invader commit
`696830ff80af227e84e7237c2ef26eb2301ed110`; retained layout attribution/license
is under `data/halo-layout/`. The extractor is GPL3-only.

## Projectile and damage model

Bullets, pellets, rockets and plasma use authoritative finite travel. Curved
sweeps use up to eight-ms substeps, gravity and constant deceleration between
the tagged initial/final speeds. Air/water select their own damage ranges and
gravity. Maximum range is cumulative path distance, rather than a timeout
estimated from launch speed. The plasma rifle's zero maximum range expires at
the end of its slowdown. A 60-second safety lifetime remains for other weapon
projectiles. Bounce remainder within a substep is approximate.

Impact damage uses normalized speed when initial/final velocities differ.
Damage-effect **minimum** is the low-scale damage, while the two upper-bound
values are the random full-strength bounds:

`minimum × (1−scale) + random(lower, upper) × scale`.

This corrects the old zero-damage shotgun interpretation: near pellets randomize
18–25, and at the end of the 120–240-unit slowdown they approach **8**, not zero.
The build2342 source preserves an air/water maximum-damage-distance bug. This
implementation uses tagged air values and does not assert that bug exists in2276.
Integer damage rounding remains a Quake adaptation.

Rocket/hand-grenade blasts give full-strength random damage inside the inner
radius, interpolate to minimum damage over the outer band and stop at the
outer cutoff. Quake hull center substitutes for Halo's animated bounding
sphere. Arena wall occlusion and direct-hit exclusion are retained. Needle
supercombine keeps the earlier fixed group blast model pending further parity
work. Quake self-damage and knockback rules are not yet fully replaced.

All 33 weapon projectile material responses are extracted. Arena world surfaces
map to stone (2), `SURF_METALSTEPS` to thick metal (7), players to cyborg armor
(21) or energy shield (22), after damage. Potential response angle/speed bounds,
flags and skip fraction select reflect/penetrate/attach behavior. Bullet
penetration applies initial friction; Needler grazing ricochet uses normal and
tangential loss. Arena surfaces do not expose every Halo material. Water is
selected as a medium during travel; collision/media entry effects, angular
ricochet noise, charged-specific responses and shield-hit-material subtleties
still need validation.

## Batteries, precision and grenade behavior

Battery state uses millionth-age units. Primary costs are 5,000 for plasma rifle
and 2,000 for plasma pistol; charged pistol costs 110,000. Partial battery
remainder survives shots, manual/death drops and pickup. Ammo counts derive
from remaining energy for compatibility; the HUD displays battery percentage.
There is no magazine reload or ammo-box refill. Cooling retains fixed-point
heat and permille battery-age approximation. Fully charged hold pauses cooling;
release fires the EMP secondary. Overheat heat thresholds and imported recovery
animation duration gate firing while melee/grenades remain available.

Right-click cycles authoritative zoom. Magnum/rocket use 2×, sniper 2×/8×.
Scoped sniper error is zero according to its trigger flag; shot events capture
that decision so later zoom changes cannot affect a shot. Damage and actions
clear zoom. Rendered FOV/sensitivity use magnification, with user base FOV retained.

Precision rays intersect a temporary ellipsoid in the upper hull. This avoids
counting the entire upper-body width as head and resolves the coarse hull entry
point to a head point. It remains an approximation: actual animated head nodes,
pose geometry and model collision belong to the asset-conversion milestone.
Magnum shield-break/exact-depletion and sniper exception rules are preserved.

Both hand grenades launch at the biped's 10-WU/s (800 units/s), in the aim
direction, from the camera plus the imported .05-WU upward origin offset.
The old upward velocity bias is removed. Stone/metal bounce coefficients use
extracted material normal/tangential friction. Frag's 500-ms timer starts on
first bounce. Plasma bounces on arena surfaces, sticks to players or settles
on the floor, then starts its 2,000-ms timer. Moving/rotating attachments remain
serial/spawn-safe. Grenade release keyframes, two-of-each spawn adaptation and
the ten-second unarmed safety expiry still need retail validation.

## Reproduce and verify

```sh
python3 scripts/extract-halo.py assets/halo/bloodgulch.map --output assets/halo/bloodgulch-values.json
python3 scripts/extract-halo.py assets/halo/ui.map --index-only --output assets/halo/ui-index.json
python3 scripts/import-halo-profile.py assets/halo/bloodgulch-values.json --ui-index assets/halo/ui-index.json
python3 scripts/generate-profile.py
python3 tests/profile.py
python3 tests/halo-extraction.py
```

Project protocol **94** carries heat, charge, spread, rate, battery, zoom, camera
transition and recovery timers. Rebuild engines and native/QVM modules together.
The fourteen shell test suites and two Python suites cover controlled shared
prediction, server damage/travel/materials/blasts, drop/pickup conservation,
network serialization and malformed input. Builds and offscreen startup do not
establish remote multiplayer or retail parity.

The next milestone can focus on converted models, animations, textures, HUD and
sound. Animation channels and animated collision are dependencies for the
remaining presentation/event/hitbox work. No additional upload is needed to
begin inspecting the already supplied UI and Blood Gulch asset dependencies.
