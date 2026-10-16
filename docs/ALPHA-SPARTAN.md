# Mac 1999 player model

The uploaded `Characters>Spartans>Halo 1 Alpha` collection contains several
static models. `armormac` is the default chosen for the user's Mac 1999 model.
`convert-alpha-spartan.py` accepts the original ZIP and the locally converted
retail Spartan; it transfers weights between the two arms-down meshes in bind space and reuses
the retail animations and weapon attachment. Matching against a rifle-holding
animation pose is incorrect: it assigns arms to the hips/torso and leaves the
weapon floating while the hands stay down. The converter preserves uploaded
bind positions and applies animations only when posing the finished model.
Nearby bind influences are blended, with anatomical restrictions that prevent
thighs from following hands. Detached gloves and backpack pieces keep rigid
hand/torso assignments. This is an automatic rig; it needs visual playtesting and may need manual
weight cleanup around shoulders, elbows and hips. The upload supplies no
authored animations. It is not an original Halo engine animation rig.

Alternate `--variant` values are `99_armor`, `armor`, `armor01`, and
`e3_99_armor`. The collection stores the Mac visor together with two other models' visors.
The converter selects only its triangles and exports a separate RGB visor
surface, including in the HUD portrait. Armor uses entity RGB,
world lighting and a view-dependent Quake specular pass. Retail armor now draws
its specular pass after its color mask, so the paint no longer covers highlights.
The highlight intensity is calibrated for Quake's lobe. Xbox cubemap reflection
and physically based materials are not reproduced exactly by this shader.

Source: [Characters>Spartans>Halo 1 Alpha](https://sketchfab.com/3d-models/charactersspartanshalo-1-alpha-e7c7949a1e344db4b4fedcdb8f7b5e9e)
by [Bornstellar Makes Eternallasting](https://sketchfab.com/jameslucino117),
licensed under [CC-BY-4.0](https://creativecommons.org/licenses/by/4.0/).
QCE adapts scale, rigging and materials. The generated package includes this
attribution. Neither the uploaded source art nor generated art is tracked.
