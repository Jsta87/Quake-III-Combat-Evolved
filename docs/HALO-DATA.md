# Halo CE Xbox data and calibration

## Upload after pushing the code

Start with these files from the same official Xbox Halo CE release:

1. `bloodgulch.map`: first source for multiplayer weapon, damage, projectile,
   grenade, player and vehicle tags, plus referenced resources present in it.
2. `ui.map`: useful for interface resources and checking the map set/version.
3. A short note giving disc region (NTSC-U, PAL, NTSC-J if known), edition,
   language and any known build/version. Say if the maps were modified.

Keep the original filenames. If a file exceeds 32 MiB, upload every numbered
part of a split 7z archive, with each part below that limit. A small plain-text
listing of the original maps directory is helpful. No full ISO or executable
is needed for this first pass. If dependency inspection finds missing resource
files or tags, we will request those specific files; one multiplayer map is
not guaranteed to include every weapon/vehicle/model needed for the whole game.
Vehicles are deferred, so additional campaign/multiplayer maps can wait until
we identify what Blood Gulch contains. Keep these local assets out of Git.

## Offline extraction, engine-native runtime

Extract tags once for the selected reference build; do not load Xbox map/tag
formats during a Quake match. The planned data pipeline is:

1. Inspect map headers/version, integrity and tag/resource dependency tables.
2. Preserve raw values and identifiers in a local extraction manifest, with
   map SHA-256, tag path/group, tag hash, source fields and reference build.
3. Resolve referenced damage, projectile, animation, physics and global tags.
   A weapon tag alone does not contain every relevant combat rule.
4. Convert units and timings into the versioned gameplay profile, recording
   conversion assumptions and field mappings. Generate shared engine constants
   for both server and predicted client; commit the profile/generator, not maps.
5. Convert needed art/audio separately into Quake-compatible assets and align
   presentation with gameplay timing. Raw Xbox models/textures/animations do
   not become Quake assets merely by copying their tags.
6. Compare resulting behavior with the chosen retail release, then lock a
   reviewed reference profile. Retain extraction provenance for later updates.

The offline Xbox reader and supported scalar import are now implemented for
the uploaded build 2276 maps. See [HALO-IMPORT.md](HALO-IMPORT.md) for applied
values, reproducible commands, hashes, conversion assumptions and remaining
behavior/art work. The profile is mixed, not a validated reference profile.
Both supplied maps contain the dependencies needed for the current weapon pass.
Texture/audio conversion and static weapon MD3 exports are now implemented;
see [ASSET-CONVERSION.md](ASSET-CONVERSION.md) for commands and remaining work.

## What is needed to convert accurately

- The exact retail map set/build is essential. The available decompilation
  reference is Xbox build 2342 and is not automatically identical to retail.
- The source field units and semantics: distances/world scale, velocities,
  accelerations, angles, probability/spread, damage units and shield/health
  multipliers. Record the meaning before converting, not just a numeric value.
- A chosen Halo-to-Quake distance scale, checked against player height,
  movement speed, jump apex/time, melee reach and projectile travel. With scale
  `S` Quake units per Halo world unit: distance and velocity multiply by `S`,
  acceleration also multiplies by `S` when time remains seconds.
- Time fields converted according to their source representation: seconds
  multiply by 1000 for milliseconds; tick counts require the verified source
  simulation frequency. Preserve source ticks in the extraction manifest.
- Behavior around animation events, reload completion, melee contact and weapon
  recovery. Tags/animation timing and engine logic can both affect these.
- Targeted reference recordings or measurements later: run/crouch, jump arc,
  reload/fire cadence, shield recharge, body/head damage and grenade flight.
  Quake's simulation cadence and integer state quantization require tolerances.

You can upload the two maps and version note first. We can inspect and extract
before asking for any missing dependencies. Broader multiplayer validation,
vehicles and arena adjustments remain scheduled for later work.
