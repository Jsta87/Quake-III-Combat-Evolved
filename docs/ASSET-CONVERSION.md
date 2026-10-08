# Xbox asset conversion

`scripts/convert-halo-assets.py` converts the supplied build-2276 Xbox caches
into local Quake-compatible resources. It uses Python's standard library;
Blender, a Halo executable, and additional uploads are not needed for this pass.
Original maps, extracted art, mesh sidecars, manifests and PK3s stay under the
Git-ignored `assets/` directory. Only the converter, layouts, tests, client hooks
and documentation belong in Git.

The second conversion stage now enables animated first-person weapons with
Spartan arms, moving attachment points and layered materials. See
[Weapon presentation](WEAPON-PRESENTATION.md) for behavior and remaining limits.

## Reproduce

From the repository root:

```sh
python3 scripts/convert-halo-assets.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch \
  --pk3 assets/baseq3/zzz-qce-halo.pk3
python3 scripts/convert-halo-assets.py assets/halo/ui.map \
  --output assets/halo/converted/ui \
  --pk3 assets/halo/converted/ui.pk3
python3 scripts/animate-halo-weapons.py assets/halo/bloodgulch.map \
  --output assets/halo/converted/bloodgulch \
  --pk3 assets/baseq3/zzz-qce-halo.pk3
python3 tests/halo-assets.py
python3 tests/halo-animation.py
./scripts/build.sh client
./scripts/run-client.sh
```

If `QCE_DATA_DIR` selects another data directory, place the Blood Gulch PK3 in
its `baseq3/` directory. The UI package is exported for inspection, not installed
in the live game yet. Rebuild the client native/QVM modules to enable optional
Halo firing audio. Without the package, existing Quake firing sounds remain.
Remove the Blood Gulch PK3 to revert its presentation resources.

`--textures-only` and `--sounds-only` restrict exports to the named resource type.
The default pass additionally converts the selected weapon meshes. Conversion
can take a few minutes because texture/audio decoding is implemented in Python.
Only files recorded by the current run enter the package; stale extracted files
and mesh source sidecars do not. Package entry order, timestamps and permissions
are fixed for repeatability. Conversion errors stop the command; inspect its
exit status before installing a package.

## Converted resources

The supplied Blood Gulch cache produces **504 bitmap entries**, **899 complete
sound variations**, and **16 weapon MD3 models**: world and first-person geometry
for each of the eight agreed weapon slots. Bitmap cube faces are separate images;
these counts are source entries, not filesystem file counts. The UI cache
produces **379 bitmap entries** and **83 complete sound variations**.

Textures support DXT1/DXT3/DXT5, Xbox unswizzling, grayscale/alpha formats,
RGB565, ARGB1555/4444 and XRGB/ARGB8888. TGA rows are physically bottom-up:
ioquake3 ignores the TGA top-origin flag. Alpha channels are preserved. Only
the top mip is exported; Quake generates its own mipmaps. Cubemaps use Xbox
face-chain/padding layout and correct the second/third face ordering.

Xbox ADPCM is decoded into 16-bit PCM WAV at the original rate/channel count.
Each 36-byte channel block contains 64 decoded frames, including its predictor.
Split sound permutation chains are joined, independent variations remain
separate, and cycles or invalid indices are rejected. Source gain, pitch,
skip probability and attenuation fields are recorded without baking them into
samples. The client uses up to four firing variations per weapon. Plasma pistol
normal fire uses the referenced plasma fire resource. Halo gain/pitch selection,
attenuation, charged-shot sounds and animation-event audio remain to be integrated.

MD3s are **static inspection models**, not finished in-game replacements. Each
region uses its first permutation and highest available LOD. Triangle strips
are expanded with their original alternating winding, packed normals/UVs are
decoded, and positions use the profile's 80 Quake units per Halo world unit.
Surfaces split at ioquake3's renderer limits. The accompanying diffuse shaders
are deliberately simple and two-sided; they do not reproduce Halo materials.

For example, open the developer console in an arena and inspect the static
first-person assault rifle:

```text
testgun models/qce/halo/fp_3c512e26221960b7.md3
```

Use `testgun` without a filename to clear it. The generated manifest lists the
other model names. These static inspection models remain available. The second stage exports
compact skeletal IQM models for regular first-person gameplay; their names and
action bindings appear in `animated_weapons` in the manifest.

## Provenance and inspection

Each output directory contains `manifest.json`, with map SHA-256/build, resource
identities, source-buffer and output hashes, dimensions, formats, source offsets,
audio segment chains and durations, and weapon/model/shader/bitmap dependencies.
`model-sources/*.json` preserves the selected mesh positions, triangle topology,
UVs, normals, skin weights, skeleton/default transforms and buffer hashes for
subsequent animation conversion. These sidecars are excluded from the PK3.

Generated engine paths are sanitized and shortened with a source-path digest to
stay below Quake's 64-byte path limit. Map strings cannot create parent paths or
collide merely because spaces and underscores normalize alike. The parser retains
bounds checks and the existing build-2276 gate. Retail flagged resource offsets
are masked separately from virtual tag pointers.

Layout and format references are pinned to Invader commit
`696830ff80af227e84e7237c2ef26eb2301ed110` (GPL-3.0-only). No converter fetches
reference files or executes map contents at runtime.

## Remaining conversion work

- Compose additive movement/aim/ammunition overlays and schedule source animation
  sound events; refine transitions and retail view placement.
- Convert player geometry/animation and animated head/collision references.
- Reproduce multipurpose/detail/reflection/transparency shader behavior, including
  Xbox channel semantics and animated weapon displays.
- Integrate HUD graphics, crosshairs, scope masks and presentation timing.
- Refine source sound gain/pitch/distance and charge fades; connect impact and
  explosion audio/effects. Ready/reload/melee/charge/recovery events are connected.
- Convert projectile effects, particles, decals and grenade meshes; vehicles later.
- Add palette-backed bump and 3D textures. Blood Gulch explicitly skips 12 such
  bitmap entries; the UI cache skips five 3D entries. They are not converted to
  guessed colors or substituted silently.

Native/QVM builds, synthetic decoder/MD3 tests, independent TGA/WAV reads and
local renderer/sound loading validate the conversion pipeline. Retail visual,
audio and animation parity still requires comparison with the Xbox release.

## Validation recorded for this pass

- Fourteen synthetic tests pass, including both DXT5 alpha modes, packed Halo
  fractions, strip winding, renderer surface splitting, cube-face padding/order,
  chained audio/cycle rejection, TGA row orientation, bounds and package repeatability.
- Existing Halo extraction and gameplay profile checks pass.
- Native and QVM client builds pass. Offscreen q3dm1 smoke runs load Halo firing
  audio; the QVM run registers all 16 exported weapon MD3s and their shaders.
- Every packaged file matches its manifest SHA-256; all TGA/WAV files are readable
  by independent image/audio readers. PK3 integrity checks pass and repacking
  the recorded files produces byte-identical archives.
