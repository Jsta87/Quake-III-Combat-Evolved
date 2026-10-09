# Warthog and Blood Gulch prototype

This pass adds a drivable Warthog and an offline converter for your Xbox
build-2276 Blood Gulch cache. Original assets and generated PK3/BSP files stay
local under `assets/`; Git contains the implementation and conversion tools.

From the repository folder, after the existing weapon/player conversion:

```sh
export PATH="$PWD/.tools/bin:$PATH"
./scripts/build.sh client
python3 scripts/animate-halo-weapons.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch \
  --pk3 assets/baseq3/zzz-qce-halo.pk3 --materials-only
python3 scripts/convert-halo-map.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/map \
  --pk3 assets/baseq3/zz-qce-bloodgulch.pk3
QCE_MAP=qce_bloodgulch ./scripts/run-client.sh
```

Press **E** near the driver's side, passenger's side or rear to take the
nearest free seat. All three seats have separate source entry and seated poses. Passenger poses
follow rifle, pistol or rocket-launcher weapon class.
Press E again to exit. Release E between uses. **Mouse yaw steers**, **W** drives
forward, **S** reverses, and **Space** brakes. Braking overrides held throttle.
The driver's view aligns with the vehicle when boarding. A and D have no driving action.
The mouse also controls the chase camera. Passengers can use their carried weapon;
the rear gunner uses mouse aim and held fire for the turret. Turret controls become
active after boarding finishes. Its carried weapon and ammunition are preserved.
Exit plays the source seat animation before releasing ownership. The seat stays
reserved and its controls are disabled during the animation. The server checks
the route and landing space before starting and again before releasing; a newly
blocked exit returns the occupant to the seat. Death, disconnect and respawn
clear seat ownership.

Reconvert **both** packages when updating from the first vehicle prototype. The
Spartan package now contains seat and aim clips; the map/vehicle package contains
separate wheels, steering wheel, source materials, engine/suspension audio and
source dust/gravel sprites, articulated suspension/turret parts, RPM bands and
seat transition audio. The `--materials-only` command above preserves the
existing first-person weapon animations while refreshing these world assets.

Useful console commands:

```text
qce_vehicle_spawn    // create a Warthog ahead of you; requires sv_cheats
qce_vehicle_enter   // enter a nearby Warthog
qce_vehicle_exit
qce_vehicle_status  // current vehicle, seat, speed and health
```

The launcher uses `devmap`, which enables development cheats. Spawn in an open
area, then walk toward the new Warthog before entering. Blood Gulch also has two
Warthogs at its original scenario placements. Vehicle bodies currently have
500 prototype health and respawn after 15 seconds when destroyed; these values
are tuning choices, not a claim about retail Halo CE vehicle durability.

The vehicle state is server authoritative. Seat identity, exit phase and animation time are
replicated in QCE protocol **100**; rebuild the engine and every game module
on both ends. Mixed older builds are incompatible. The hull uses swept box
collision, four ground probes, terrain alignment, gravity, acceleration and
braking. The source Warthog tag supplies forward/reverse speed and acceleration
values, converted from 30-Hz ticks using the project's 80-unit world scale.
Steering, hull dimensions, terrain contact and chase-camera distances are
prototype approximations.

## Conversion and current limits

The converter reads the Xbox structure BSP's separate cache block rather than
assuming it is ordinary tag memory. It validates pointers, material/vertex
ranges and triangle indices. World vertices, normals, UVs, diffuse textures,
transparent material textures, multiplayer starts and Warthog placements come
from the supplied map. Render surfaces are split to fit both Quake renderers.
Collision consists of thin convex prisms behind solid terrain triangles, with
axial bounds and box-trace bevels. It emits IBSP version 46 directly; no external
map compiler is required. The generated manifest records the input checksum,
scale, map statistics and limitations.

This is a vehicle test map, not a finished level port. Retail collision BSP,
lightmaps, atmosphere, scenery, weapon placements and teleporter scripting still
need conversion. A procedural blue sky and coarse lighting grid are used.
There is one visibility cluster, so performance has not been optimized.
Blood Gulch has **no AAS bot navigation** yet; use human players for vehicle
testing. Existing Quake maps retain their bot navigation.

Wheels spin using the source wheel circumference, front wheels steer and the
steering wheel rotates about its source axle. Wheel-contact heights determine
pitch and roll even where collision normals are flat. Source driver/passenger/
gunner poses anchor to the model's seat markers. Driver aim changes the head
while keeping the authored steering grip. Passenger aim uses the source rifle,
pistol and missile bounds; gunner aim uses its five-column, three-row source
screen. Aim pitch currently uses three bands, with interpolation in yaw.
Entry and exit play their source clips and frame-zero sound cues. Animation
start times are replicated, so joining clients do not restart an old entry.

The turret yaw is unrestricted and its pitch is limited to the source vehicle's
15 degrees down / 35 degrees up. Hull slope is included in muzzle orientation.
The source trigger ramps from 8 to 15 rounds/second in one second, spins down
in four seconds, and grows spread from 1 to 2 degrees over three seconds.
Rounds are finite-travel projectiles: initial speed 23,440 units/sec, final speed
800, gravity and air/water slowdown come from the source projectile. Damage uses
the source 16–24 range and 16 minimum, with equal shield/health scales and no
headshot kill flag. Source material stop/pass/reflect responses are retained for
materials the game can classify. Every third shot can draw a tracer, anchored to
the actual muzzle. Tracer artwork and impact particles still use the shared bullet
presentation. Muzzle position is aligned to the converted barrel tip; the retail
weapon marker is not available in this converted model metadata.

The four suspension arms and wheel carriers pivot at the source hinges. Ground
traces drive wheel travel within the source extension/compression poses: rear
-30/+15 degrees, front +30/-15. Tires extend in the air and compress on landing.
Visual wheel travel remains a damped approximation; chassis support now uses the source spring/mass force equations.

The source engine start, loop, stop, load, suspension, turret-fire and seat
transition sounds are converted. Seventeen pre-rendered RPM bands vary engine
pitch and blend the load recording; acceleration and speed drive a filtered RPM
envelope. This avoids changing Quake's sound ABI and works with both backends,
but band changes and the pitch/load envelope still need listening calibration. Tire dust and gravel use
source bitmaps with a bounded procedural emitter; emission timing/material choice
still needs calibration against Halo. Player collision uses a smaller central
body so the doors remain accessible; terrain collision sweeps the source physics mass-point centers instead of a large axis-aligned box. Ground support probes the 15 mass-point radii. Quake BSP contact queries remain an adaptation of Halo collision-feature queries.

Run-over damage, flipping and bots driving vehicles remain pending. The source speed values do not establish
that the current physical handling matches retail Halo.

The layout facts are checked against the GPL-3.0 Reclaimer HEK definitions:
https://github.com/Sigmmma/reclaimer/tree/master/reclaimer/hek/defs
(`sbsp.py`, `scnr.py`, `senv.py`, `sotr.py`, `vehi.py`). Reclaimer is not a runtime
or conversion dependency.

## Checks

```sh
./scripts/test-vehicles.sh
./scripts/test-iqm-tags.sh
python3 tests/vehicle-physics.py
./scripts/test-network-state.sh
./scripts/test-weapon-presentation.sh
python3 tests/halo-map.py
```

The tests cover speed/braking across tick rates, force-supported settling, powered motion, airborne crest momentum, A/Space controls, seat ownership, blocked and
forced exits, source-timed exit reservation/control lockout, turret boarding gates,
spin-up/down, aim limits, finite turret projectile damage/travel, suspension bounds,
entry restrictions, replicated vehicle phase/time/RPM/aim, BSP plane pairing,
box-collision halfspaces, winding, surface limits and pointer bounds. Shield
fade and corpse timeline regressions and weapon sway are also covered by the
presentation suite. In-game handling remains a playtest requirement.

## Handling and turret corrections

The solver was studied against `vehicles.c` (`update_human_jeep_physics`) and
`physics.c` (mass-point contacts, friction, inertia and integration) in
[halo-ce-universal at f479e349](https://github.com/cybersecurity/halo-ce-universal/tree/f479e34914604df5a22a2bdb0b38f180a1f702d8/source).
That project supplies CC0 reconstructed code; the port is adapted to Quake's
collision API rather than claiming a byte-identical Xbox engine implementation.

`scripts/extract-halo-vehicle-physics.py` reads the owned build-2276 cache and
produces `bg_qce_vehicle_profile.generated.h`: all 15 mass points, their weights,
radii, powered-wheel groups, anisotropic friction, center of mass and inertia.
At 80 Quake units per Halo world unit, support depth is 12 units, ground damping
is 1.5/s and air friction is 0.15/s. The 612-unit/s forward speed is a powered-wheel
target, not a forced chassis speed. Halo gravity, contact forces and angular
momentum determine actual movement; leaving a crest preserves momentum. Ground
and air forces integrate in steps no larger than 1/120 second.

Terrain sweeps slide over contact planes. The ground probes currently use downward
rays to approximate radius/feature contacts; material-specific friction, water
buoyancy and Halo's complete angular collision response remain unported. Retail
handling parity still needs comparison playtests, especially steep slopes,
wall impacts and rollover recovery.

The local turret and gunner pose share predicted view-angle aim; remote aim
interpolates snapshots with angle wrapping. Gunner poses interpolate between
both yaw and pitch keys with quaternion blending, including attachment tags.
Held fire reads the current seated command. Muzzle clearance ignores the riders'
standing collision boxes while projectile traces still hit players. Mouse1 is
explicitly bound to fire in `qce-controls.cfg`.

Rebuild the **whole client** for the pose-renderer ABI change; replacing only the
cgame DLL/QVM is insufficient. No asset reconversion is needed if the existing
packages already contain articulated turret parts and seat/aim clips. A console
warning identifies missing turret parts in an older package. `qce_vehicle_status`
now reports chassis position, actual velocity and wheel contact bits in addition
to motor speed and turret shot count.

## Explosions and firing audio

Explosive damage now adds linear velocity and tipping angular velocity to vehicles,
using the source `vehicle_accelerate` rule (`cross(up, acceleration) * pi`). Xbox
Warthog acceleration scale is 0.3; frag/plasma acceleration is 4, rocket is 6.
The Quake adaptation scales this impulse with damage falloff. Walls, ignored
entities and explicit no-knockback flags retain their existing behavior.
Ordinary bullets do not apply this blast impulse. Wheel contacts and inertia
then evolve the hull pose, allowing it to be thrown and rolled rather than
only changing a rendered angle. Full retail rollover/collision parity remains
an open playtest item.

The local gunner hears firing as a local weapon-channel sound, independent of
bullet impact distance. Other listeners hear it spatialized at the turret muzzle.
Impact audio remains spatialized at the impact.

To update an existing clone, pull **before** rebuilding:

```sh
git checkout work
git pull --ff-only origin work
export PATH="$PWD/.tools/bin:$PATH"
./scripts/build.sh client
QCE_MAP=qce_bloodgulch ./scripts/run-client.sh
```
