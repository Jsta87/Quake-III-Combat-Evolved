# Playtest follow-up

Rebuild both the engine and game modules. Protocol 96 requires matching clients
and servers; old protocol-95 binaries cannot connect. Regenerate your local media:

```bash
./scripts/build.sh client
./scripts/build.sh server
python3 scripts/convert-halo-world.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch \
  --pk3 assets/baseq3/zzz-qce-halo.pk3
./scripts/run-client.sh
```

The world converter assumes the base and weapon-animation converters have already
run, as described in LOCAL-TESTING.md. A fresh conversion through the full animation
converter also runs the world stage automatically. Restart the game after conversion.

- Weapon firing audio now plays from fire events, independently of reload/ready
  animation audio. The presentation stage repairs all fire aliases, including rockets.
- The visor uses an environment projection of its source reflection
  cubemap. Other reflection materials remain deferred to avoid unmasked glare. Unsupported transparent layers use an actual clear
  bitmap instead of a white quad. Halo register combiner parity remains unfinished.
- Pickups are 1.5 times their imported size by default. Tune
  `cg_qceWorldWeaponScale` (0.5–3); held weapons retain their source dimensions.
- First-person depth-hack models use a 0.1-unit near clip in both renderers;
  world geometry and crosshairs retain the world projection.
- Spartans use rifle, pistol, and missile stances with weapon-specific melee and
  reload tracks. Grip translation and orientation now use the authored right-hand
  marker. Normal state changes blend over six 30-Hz ticks (200 ms); idle-to-idle
  weapon-class changes use one tick (33 ms). Both capture an outgoing keyframe. Aim overlays, left-hand weapon IK and capturing the full interpolated outgoing pose remain
  unfinished; these are not yet a perfect reproduction of retail poses.
- Melee, carried-weapon switching and successful explicit weapon pickups interrupt
  reloads. Loaded ammunition stays if insertion already committed; interruption
  before insertion leaves the old magazine. Total ammunition is conserved.
  Full/empty animation choice stays latched after insertion.

## Grenades

**T** selects frag/plasma; **G** throws. Console equivalents:
`qce_grenade_type` and `+button13` / `-button13`.

The imported Xbox cache gives both types an 800-unit/s throw speed. Frag arms at
first impact and detonates after 500 ms; plasma arms at rest or a player stick and
then detonates after 2,000 ms. Gravity, material bounce coefficients, damage and
blast radii come from the profile. Plasma follows the attached player's movement
and rotation and detaches safely if that player respawns. Stationary grenades no
longer spin visually. Bounce, throw, flight and explosion sounds use local Halo
media. Explosions now use imported fire/plasma cloud and smoke bitmaps with color
preserved, rather than Quake's circular rail disc.

Release uses keyframe 8 of the Spartan throw animation: 267 ms after input,
rounded to the next server frame. It uses the current camera/aim at release,
with the source lateral origin offset; dying during windup releases at reduced
power. First-person clips have no release keyframe, which previously obscured
this timing.

Explosion textures are atlas sprites, not full images. The converter now crops
180 individual sprites from the authored sequences. Frag starts with 35
particles; plasma starts with 10 and continues emitting from its source creation
rate. Per-particle hold/transition times,
colors, scale and angular rates come from the three source states. Fire changes
to smoke; plasma fades through its longer energy-smoke state. Emitter fade,
explosion ejection, air drag and buoyancy also use source values;
particle motion updates on a fixed 30-Hz clock. Numeric provenance
is in `data/halo-presentation-timing.json`. Re-import with:

```bash
python3 scripts/import-halo-presentation.py assets/halo/bloodgulch.map
```

Quake maps still lack Halo weather/wind and media behavior. Minimum-count
particle replenishment, secondary sparks, decals and lights remain unfinished.
Grounded ejection uses a nearby Quake floor trace. A 10-second unarmed
safety expiry also remains an adaptation. Retail comparison is needed before
claiming exact Halo grenade behavior.

For an isolated visual/audio check on a cheats-enabled map, use
`qce_effecttest frag` or `qce_effecttest plasma`. This creates a local explosion
120 units ahead of the camera without dealing damage.

## Sound calibration

The converter applies definition gain × permutation gain and the pitch-range
playback rate once to runtime aliases. Raw decoded WAVs remain unchanged, so
re-running conversion does not progressively attenuate the audio. Fire, grenade,
footstep, pickup and animation-event aliases share this process. Calibration
metadata includes source gain/rate, peaks and clipped-sample counts in the local
manifest. The current cache produced 109 calibrated aliases with no clipping.

Halo sound-class volume priorities, distance/cone attenuation, randomized pitch,
occlusion and the Xbox reverb/mixer are not yet reproduced. Headless sound
registration/PCM checks cannot validate perceptual mixing; listening comparisons
against Xbox build 2276 remain necessary.

## Large matches

Start a 128-slot server with:

```bash
./scripts/run-server.sh +set sv_maxclients 128
```

128 is the compiled client limit, not an unlimited dynamic protocol. Client arrays,
player configstrings and bot slots use that bound. Snapshot history is allocated separately from the fixed map hunk. Bot AI allocation
scales with the client bound. Reliable queues hold 512 commands to tolerate roster
bursts, and the minimum zone allocation is 128 MiB for the larger client structures. Game-state
storage and message capacity were increased, and fragment offsets are treated as unsigned. The legacy
32-bit selective entity mask safely excludes players beyond its range; QCE does not
use that optional mask API.

Score messages carry four rows per reliable command, so the roster is not silently
truncated by the 1,024-byte string limit. Crowded scoreboards display 16 rows per
page. **Page Down / Page Up** change pages; `qce_scorepage auto` follows your rank.
Your own row remains visible on other pages. Team identity is shown by helmet color.

128-player performance and Internet bandwidth have not been benchmarked. The
snapshot still caps visible entities at 256; congested scenes can exhaust it.

## Reload insertion provenance

`data/halo-reload-timing.json` records frame numbers and derivation. AR, sniper and
pistol timing uses the animated magazine's return to its seated position; rockets
use the animated launcher tubes. This geometric estimate is distinct from a
confirmed retail ammo notification. Needler and shotgun lack a suitable return track and retain
completion/per-shell behavior. To recompute from the owned source map:

```bash
python3 scripts/import-halo-reload-timing.py assets/halo/bloodgulch.map
python3 scripts/generate-profile.py
```

## Validation in this pass

Native and QVM baseq3/missionpack builds passed. Gameplay, reload cancellation,
score chunking through client 127, protocol round trips and 60,000-byte fragment
reassembly tests passed. Asset fixtures passed and all 1,838 packaged files passed
CRC/hash checks. Native OpenGL 2 and QVM OpenGL 1 smoke runs completed; screenshots
verified the visor, plasma material, weapon poses and scoreboard paging. A dedicated
server started 128 bots and retained all 128 across a full map reload without
allocation, reliable-queue or bot-handle errors. This verifies capacity and
startup, not performance with 128 remote human clients. Audio files and dispatch
were checked using a dummy sound device; perceptual mix/volume still needs playtesting.

The presentation follow-up additionally passes server release tests (before/at
the source keyframe, current aim/origin, death and spectator handling),
particle phase/air-physics tests and source-header freshness checks. Native
OpenGL 2 and QVM OpenGL 1 both rendered isolated frag/plasma effects and shut down
without runtime errors. The rebuilt local PK3 contains 2,018 verified files,
including 180 atlas crops and 109 gain/rate-calibrated sound aliases. Sound was
checked with a dummy audio device; no listening comparison is claimed.
