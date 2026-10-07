# Xbox map import: build 01.10.12.2276

Both uploaded maps have Xbox cache version 5, build `01.10.12.2276`, matching
Invader's Xbox NTSC-US build identifier. Blood Gulch contains 1,806 tags and UI
983. The offline reader validates header/footer, bounded zlib decompression,
tag identities, dependency IDs and accessed pointers/reflexives. The zlib
stream checksum is checked by decompression; the cache-header CRC is recorded
but not independently verified. Files remain under ignored `assets/halo/`.

## Applied in the game

| Quake slot | Halo weapon | Magazine | Maximum reserve | Fire interval used | Reload interval used |
| --- | --- | ---: | ---: | ---: | ---: |
| Machinegun | Assault rifle | 60 | 600 | 67 ms | 3,400 ms |
| Shotgun | Shotgun | 12 | 60 | 1,000 ms | 400 ms per shell |
| Rocket launcher | Rocket launcher | 2 | 8 | 2,000 ms | 5,000 ms |
| Railgun | Sniper rifle | 4 | 24 | 500 ms | 3,133 ms, provisional animation-derived fallback |
| Plasma gun | Plasma rifle | Battery | None | 100 ms, final rate | No magazine reload |
| Lightning gun | Plasma pistol | Battery | None | 33 ms rate cap; charging still pending | No magazine reload |
| BFG | Magnum | 12 | 120 | 300 ms | 2,170 ms |
| Grenade launcher | Needler | 20 | 80 | 100 ms, final rate | 1,000 ms |

Capacities, initial ammunition and nonzero reload times come from magazine
fields. Reserve limits exclude the loaded magazine, including partially loaded
magazines. Full-ammo pickups do not consume weapons. Battery weapons cannot
recharge from Quake ammo boxes or automatic duplicate pickups; E replaces the
selected battery weapon and drops its remaining energy. The current battery
representation is shot equivalents (plasma rifle 200, plasma pistol 500) from
trigger age-per-round. Heat and charged consumption are implemented; exact
fractional battery age and Halo battery presentation remain pending.
Quake grenade-launcher ammo boxes now supply Needler ammunition.

Fire intervals convert the final rate to whole 30 Hz ticks, then integer
milliseconds. They are rate caps, not proof of exact trigger/animation behavior.
Fire-rate ramps still need simulation. Plasma pistol tap/charge release is implemented. Shotgun reload adds one
shell at a time and permits firing to interrupt once a shell is available.
The sniper's magazine reload time is zero: its 94-frame empty reload animation
provides a provisional 30 frames/s fallback rather than an instant reload.
Full-versus-empty timings and animation event interpretation are not yet matched.

The imported multiplayer unit is `characters\cyborg_mp\cyborg_mp`, not the
single-player cyborg. Its collision tag provides 75 body vitality, 75 shield
vitality, a six-second stun/recharge delay and four seconds for full recharge.
Fractional regeneration carries remainders across frames so 75/4 seconds does
not drift with frame cadence. Displayed shield points remain integers.

Magnum damage is 25 before material modifiers, with shield multiplier 1 and
cyborg-armor multiplier 1.5. Its headshot bonus requires shields to be zero
after that same shot. Regression tests verify two body shots leave 25 shields,
then a third headshot kills on exact depletion. A third body shot removes the
remaining shields without the headshot bonus. The sniper uses 101 damage and
its shielded-headshot policy. Classification still uses Quake collision-box
head zones, not Halo animated hitboxes.

Shotgun uses 15 projectiles, initial angular spread and the first damage upper
bound (18), with shield multiplier 0.5. Other ranged damage/material scalars,
initial projectile velocities, explosion damage/radius and melee damage scalars
are imported where supported. Quake integer health/shield damage rounding remains an adaptation.
Variable damage ranges, distance falloff and
material-response behavior remain in the extraction/report for future work.

Frag uses the tag's 500 ms timer after the first bounce, without resetting it
on later bounces. Sticky plasma uses two seconds after coming to rest. Grenade
explosion damage is 120; outer radii are converted from tags. Throw impulses,
bounce material responses, inner-radius falloff, attached damage and spawn
counts remain incomplete. A ten-second airborne safety expiry is an explicit
engine adaptation rather than a Halo tag value.

## Reproduce and audit

```sh
python3 scripts/extract-halo.py assets/halo/bloodgulch.map --output assets/halo/bloodgulch-values.json
python3 scripts/extract-halo.py assets/halo/ui.map --index-only --output assets/halo/ui-index.json
python3 scripts/import-halo-profile.py assets/halo/bloodgulch-values.json --ui-index assets/halo/ui-index.json
python3 scripts/generate-profile.py
```

`data/halo-import-report.json` records applied fields, raw numeric values,
assumptions and parity gaps. The profile records source map SHA-256 and root
structure hashes for referenced tags; a root hash does not cover its reflexive
arrays/resources, which remain covered by the map hash. Extraction manifests
with full decoded numeric structures stay local. UI is indexed for later
presentation work; its resources have not been converted into Quake art.

A scale of 80 Quake units per Halo world unit maps the tag's 0.7-unit standing
hull to the existing 56-unit Quake hull. This is a chosen arena adaptation,
not an official conversion. Cached projectile velocity is per tick and is
multiplied by 30 and by the scale. Angular spread is converted from radians
into Quake ray offsets. Imported maximum ranges now bound bullet/shotgun rays
and plasma/needle flight lifetimes. Bullet travel, projectile acceleration, gravity,
random spread distribution and simulation cadence are not yet Halo-equivalent.

## Remaining work before claiming parity

The profile deliberately has `status: mixed`. The next behavior work is
exact plasma/Needler acquisition/material response and animation events, damage ranges
and falloff, sustained spread/recovery, finite projectile behavior, reload and
melee animation events, and movement/camera/collision calibration. Quake models, animations, icons and sounds
remain. Retail comparison and human/network validation are still required.
Vehicles and arena adaptations remain deferred as agreed. No additional map
files are needed for the current weapon dependency pass.

## Needler behavior pass

Needles acquire a visible living opponent at launch, turn at the imported
90 degrees/second while sightlines remain clear, and expire after the imported
20-world-unit range at their current constant speed. On contact they attach
without immediate damage, follow translation and rotation, then deal the
10-damage attached effect after 750 ms. Respawn and entity serial checks stop
an old attachment or tracking lock from affecting a replacement player.

Seven live needles attached to the same target trigger one 60-damage,
80-Quake-unit outer-radius supercombine. Attachments count across shooters.
Consumed groups cannot combine twice. Remaining needles still apply their
individual attached damage through the normal shield/protection rules.

The seven-needle threshold follows `MAXIMUM_COMBINING_PROJECTILES = 6` and
its seventh-attachment check in the build-2342 reference:
[projectiles.c](https://github.com/cybersecurity/halo-ce-universal/blob/76b1898ee14e6fb58e0412acc183da509c10e001/source/items/projectiles.c).
This is behavior evidence from a different build, not proof of retail-2276
parity. The 0.95 acquisition dot-product cone, world-contact attachment,
constant projectile speed, and grouped detonation on the next server frame
are explicit adaptations. Halo material reflections and randomized scheduling
of remaining attached needles are pending. Quake plasma visuals remain.

Run `./scripts/test-needles.sh` for attachment damage, grouping, launch
acquisition, turn bounds, walls, lifetime and target/owner reuse regression tests.

## Plasma heat and charging

The uploaded tags set rifle heat to 8% per shot and base cooling to 30%/second;
pistol heat is 16% per normal shot with 65%/second cooling. Both overheat at
100% and recover only below 25%. Rifle cooling slows by up to 20% as its
battery drains. Cooling runs on both carried weapons and heat survives
manual/death drops; a world pickup accounts for elapsed cooling time.
Heat is fixed-point in 0.01-percent units, with a fractional cooling remainder.
Battery age for the recovery penalty is derived from shot-equivalent ammo
and quantized to 0.1%. Animation-state recovery delays remain pending.

A short plasma-pistol press fires its primary bolt on release. Holding for
at least 600 ms then releasing fires the secondary bolt: raw damage70,
health multiplier0.6, shield multiplier1, initial speed1200 Quake units/second,
range3200, and tagged guided turn rate29 degrees/second. It consumes55 normal
shot equivalents (11% battery), capped at the remaining energy, and adds100%
heat. Holding a full charge pauses cooling. Swapping, melee, grenade input,
death or leaving normal player movement cancels charge. Charge animation,
45-second overcharge state transitions and exact input timing need retail
comparison. Constant velocity and the current acquisition cone remain
adaptations shared with the Needler tracker.

The charged damage tag has EMP side effect3. The build-2342 reference
[damage.c](https://github.com/cybersecurity/halo-ce-universal/blob/76b1898ee14e6fb58e0412acc183da509c10e001/source/objects/damage.c)
clears remaining shields even when the bolt's normal damage would leave
shields up. Normal shield absorption still determines health overflow: a
70-damage charged bolt clears75 shields without damaging health; with30
shields it applies the unabsorbed40 base damage at the armor multiplier.
God mode, friendly-fire rules and armor-bypass behavior are preserved.
Integer vitality and float-to-integer damage rounding remain parity gaps.

Heat, remainders, overheat locks and active charge time replicate in player
state. The engine therefore uses **QCE protocol91** and disables legacy
connections/old-demo decoding. Rebuild both engines and game modules together,
including for stock-combat mode. This is a project protocol number, not an
ioquake3 compatibility version. `scripts/test-network-state.sh` exercises
actual delta encoding/decoding through charged, cooling and reset states.
`test-heat.sh` exercises real Pmove release firing and actual weapon pickups.
