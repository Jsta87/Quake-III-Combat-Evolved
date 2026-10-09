# Playtest fixes and development controls

Rebuild the client and regenerate the Halo package after updating this pass:

```sh
./scripts/build.sh client
python3 scripts/animate-halo-weapons.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch --pk3 assets/baseq3/zzz-qce-halo.pk3
./scripts/run-client.sh
```

Run the base asset conversion from LOCAL-TESTING.md first on a fresh checkout.
The animation stage now also converts the Spartan body, world weapons,
frag/plasma grenade models, armor color masks, and additional audio. Existing
conversions can be refreshed independently with `scripts/convert-halo-world.py`
using the same arguments. Maps and generated media remain local and ignored.
Client/server engines and modules must all use QCE protocol **96**.

## Changes

- Clockwise Halo meshes use Quake's default front-face culling. Their original
  outward normals are preserved. Two-sided source materials remain two-sided.
- Overheat presentation begins on the last shot, including the plasma pistol's
  charged shot. The final shot sound remains audible alongside cooling feedback.
- Melee lunge is disabled by default. The server replicates the optional setting
  to prediction; normal melee reach and the delayed damage keyframe are unchanged.
- Holding E before reaching a nearby weapon works in any direction within the
  existing 64-unit range, with line-of-sight and action checks. A successful
  pickup latches until release, preventing repeated swaps. `qce_swap` still works.
- Eight Halo world models replace pickup/dropped weapons and Spartan carried
  weapons. Halo pickups no longer receive Quake's extra 1.5x model enlargement.
- Development launchers use `devmap`, enabling cheats at map creation. Do not
  set `sv_cheats` directly; it is engine-managed and read-only. Use `map q3dm1`
  for a session without cheats and `devmap q3dm1` to re-enable them.
- Every player and bot uses the Spartan body when the converted package is
  installed and Halo combat is enabled. Source clips cover idle, four movement
  directions, crouch, airborne, melee, grenade throw and death.
- FFA armor colors replicate per player. Bots have distinct default colors.
  Team matches retain red/blue team colors. HUD and scoreboard portraits also
  use an extracted Spartan helmet instead of Quake faces.
- Hand grenades have their own network discriminator, so the Needler slot cannot
  hide them or supply its presentation. Grenades use visible source meshes,
  throw/bounce/explosion audio and a plasma loop/light. Explosion billboards use
  additive blending and a valid texture instead of a black square.
- Weapon pickup sounds and normal/metal/water footsteps use extracted Halo audio.

## Console controls

| Command | Behavior |
| --- | --- |
| `qce_color 255 40 80` | Set local armor RGB, each channel 0–255 |
| `qce_color_hex 20C0FF` | Set local armor with six hexadecimal digits |
| `qce_colorRGB` | Inspect the stored RGB userinfo value |
| `g_qceMeleeLunge 0` | Halo CE default, no lunge |
| `g_qceMeleeLunge 1` | Enable the optional lunge on this server |
| `g_qceMovementScale 1.1` | Development default: 10% larger movement distances |
| `g_qceMovementScale 1` | Original imported movement values |
| `cg_qceWeaponScale 0.9` | Development default: 10% smaller first-person meshes |
| `cg_qceWeaponScale 1` | Original imported first-person scale |
| `cg_thirdPerson 1` | Inspect the Spartan and carried world weapon |
| `god`, `noclip`, `give Shotgun` | Development cheats enabled by the launcher |

Movement scale is replicated in the player state at 1% precision (range 0.5–2).
It scales speed, acceleration, jump impulse and gravity together: movement/jump
travel increases without changing jump duration. It does not enlarge the hull,
weapon damage/ranges, world art or arena geometry. The import conversion stays
**80 Quake units per Halo unit**. The view scale is local presentation only.

## Remaining presentation limits

This is an initial playable Spartan implementation, not complete retail player
animation parity. Rifle poses are currently shared across weapon classes;
aim/replacement overlays, transitions, precise weapon grip calibration, source
root displacement and retail death physics need further work. The grenade
explosion is a temporary additive flash; the full Halo particle graph is not yet
converted. Voice, shield, vehicle and some impact sounds remain to be connected.
Source sound gain/pitch/attenuation and layered Halo reflections are still
approximated. Compare movement/view tuning on your desktop before treating the
new development defaults as final calibration.

## Validation in the cloud workspace

Client/server native and QVM builds passed for baseq3 and missionpack. Existing
gameplay fixtures passed for movement, inventory/swap, melee, heat/charge,
grenades and projectiles, including replicated movement/lunge/input settings.
Presentation fixtures cover immediate heat feedback, no replay when firing
cooldown ends, and valid/invalid RGB input. Renderer tag and Python
animation/asset/profile checks passed.

Offscreen native/OpenGL2 and QVM/OpenGL1 runs completed with Spartan RGB changes,
distinct bot colors, both grenade types, source audio dispatch, world weapons
and charged-shot cooling. A final QVM/OpenGL2 plasma-rifle run verified one heat
entry sound per episode. All 1,837 package records matched their hashes and ZIP
CRCs. Audio was decoded/registered/dispatched with a dummy device; desktop
listening and comparison to an Xbox remain necessary for sound calibration.

See [the playtest follow-up](PLAYTEST-FOLLOWUP.md) for protocol 96, 128 slots,
reload interruption, new material conversion and scoreboard controls.
