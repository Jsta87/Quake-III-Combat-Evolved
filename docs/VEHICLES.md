# Warthog and Blood Gulch prototype

This pass adds a drivable Warthog and an offline converter for your Xbox
build-2276 Blood Gulch cache. Original assets and generated PK3/BSP files stay
local under `assets/`; Git contains the implementation and conversion tools.

From the repository folder, after the existing weapon/player conversion:

```sh
export PATH="$PWD/.tools/bin:$PATH"
./scripts/build.sh client
python3 scripts/convert-halo-map.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/map \
  --pk3 assets/baseq3/zz-qce-bloodgulch.pk3
QCE_MAP=qce_bloodgulch ./scripts/run-client.sh
```

Press **E** near a Warthog to take the first available seat: driver, passenger,
then rear seat. Press E again to exit. Holding E doesn't immediately eject you;
release and press again. **W/S** accelerate, brake and reverse; **A/D** steer.
Steering follows the vehicle heading. The mouse controls the chase camera.
Weapons and grenade throws are disabled while seated in this first prototype.
Exit is refused if the surrounding space is blocked. Death, disconnect and
respawn clear seat ownership.

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

The vehicle state is server authoritative. Explicit seat/entity fields are
replicated in QCE protocol **99**; rebuild the engine and every game module
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

Warthog wheel/suspension animation, seated Spartan poses, engine audio, turret
fire/aiming, run-over damage, flipping and bots driving vehicles are not
implemented in this prototype. The rear seat can be occupied but does not fire.
The model is a static source bind pose. The source speed values do not establish
that its current physical handling matches retail Halo.

The layout facts are checked against the GPL-3.0 Reclaimer HEK definitions:
https://github.com/Sigmmma/reclaimer/tree/master/reclaimer/hek/defs
(`sbsp.py`, `scnr.py`, `senv.py`, `sotr.py`, `vehi.py`). Reclaimer is not a runtime
or conversion dependency.

## Checks

```sh
./scripts/test-vehicles.sh
./scripts/test-network-state.sh
./scripts/test-weapon-presentation.sh
python3 tests/halo-map.py
```

The tests cover speed/braking across tick rates, terrain-tangent movement, seat ownership, blocked and
forced exits, entry restrictions, replicated vehicle state, BSP plane pairing,
box-collision halfspaces, winding, surface limits and pointer bounds. Shield
fade and corpse timeline regressions and weapon sway are also covered by the
presentation suite. In-game handling remains a playtest requirement.
