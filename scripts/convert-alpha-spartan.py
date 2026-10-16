#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Rig the user's Sketchfab glTF collection to the local retail Spartan skeleton.
Requires Pillow for PNG decoding. Generated art remains owner-local.
"""
import argparse,importlib.util,json,math,struct,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('cyborg',ROOT/'scripts/convert-halo-cyborg.py');c=importlib.util.module_from_spec(s);s.loader.exec_module(c);p=c.p
DEFAULT_VARIANT='99_armor'  # Tall visor, chest harness, shoulder 3 and missile pod in the reference.
CREDIT='This work is based on "Characters>Spartans>Halo 1 Alpha" (https://sketchfab.com/3d-models/charactersspartanshalo-1-alpha-e7c7949a1e344db4b4fedcdb8f7b5e9e) by Bornstellar Makes Eternallasting (https://sketchfab.com/jameslucino117), licensed under CC-BY-4.0 (https://creativecommons.org/licenses/by/4.0/). Rigging, scale and materials adapted for QCE.'
def tree(points,depth=0):
 if not points:return None
 axis=depth%3;points.sort(key=lambda v:v[0][axis]);i=len(points)//2
 return points[i],axis,tree(points[:i],depth+1),tree(points[i+1:],depth+1)
def nearest(node,point,best=(float('inf'),None)):
 if node is None:return best
 value,axis,left,right=node;distance=sum((point[i]-value[0][i])**2 for i in range(3))
 if distance<best[0]:best=distance,value[1]
 delta=point[axis]-value[0][axis];first,second=(left,right) if delta<0 else (right,left)
 best=nearest(first,point,best)
 return nearest(second,point,best) if delta*delta<best[0] else best
def nearest_weights(node,point,count=12,bones=None):
 """Blend nearby bind influences so a reference vertex boundary cannot tear a seam."""
 import heapq
 candidates=[];serial=0
 def visit(node):
  nonlocal serial
  if node is None:return
  value,axis,left,right=node;distance=sum((point[i]-value[0][i])**2 for i in range(3))
  serial+=1;item=(-distance,-serial,value[1])
  if len(candidates)<count:heapq.heappush(candidates,item)
  elif distance<-candidates[0][0]:heapq.heapreplace(candidates,item)
  delta=point[axis]-value[0][axis];visit(left if delta<0 else right)
  if len(candidates)<count or delta*delta<-candidates[0][0]:visit(right if delta<0 else left)
 visit(node);influences={}
 for neg,_,vertex in candidates:
  strength=1/max(.04,-neg)
  for bone,weight in zip(vertex['nodes'],(vertex['node0_weight'],1-vertex['node0_weight'])):
   if bone>=0 and weight:influences[bone]=influences.get(bone,0)+strength*weight
 if bones:
  # Hands sit beside the thighs in bind space. Spatial proximity alone must
  # never make a thigh follow a hand, or bridge the left/right lower legs.
  side='bip01 '+('l' if point[1]>=0 else 'r')+' '
  for bone in list(influences):
   name=bones[bone]['name'];limb=any(part in name for part in ('thigh','calf','foot'))
   if point[2]<32 and abs(point[1])<8:
    if any(part in name for part in ('hand','forearm','upperarm','clavicle','head','neck')):del influences[bone];continue
    if limb and not name.startswith(side):del influences[bone];continue
   if point[2]<20 and abs(point[1])<8 and (not limb or not name.startswith(side)):influences.pop(bone,None)
  if not influences:
   if count<256:return nearest_weights(node,point,count*2,bones)
   raise ValueError(f'No anatomically valid bind influence at {point}')
 chosen=sorted(influences,key=lambda n:(-influences[n],n))[:2];total=sum(influences[n] for n in chosen)
 return {'nodes':chosen+[-1]*(2-len(chosen)),'node0_weight':influences[chosen[0]]/total}

def mesh_components(positions,triangles):
 # UV and hard-normal seams duplicate vertices; weld only for rig assignment.
 parent=list(range(len(positions)));at={}
 def root(i):
  while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
  return i
 for i,v in enumerate(positions):
  key=tuple(round(x,5) for x in v)
  if key in at:parent[root(i)]=root(at[key])
  else:at[key]=i
 for tri in triangles:
  for i in tri[1:]:parent[root(i)]=root(tri[0])
 groups={}
 for i in range(len(positions)):groups.setdefault(root(i),[]).append(i)
 return list(groups.values())

def rigid_accessories(vertices,triangles,bones):
 # Separate gloves and backpack must not inherit a nearby limb's weights.
 # Determine accessory regions only for detached pieces; body joints stay blended.
 names={b['name']:i for i,b in enumerate(bones)}
 for group in mesh_components([v['position'] for v in vertices],triangles):
  if len(group)>len(vertices)/2:continue
  center=[sum(vertices[i]['position'][k] for i in group)/len(group) for k in range(3)]
  low=[min(vertices[i]['position'][k] for i in group) for k in range(3)]
  high=[max(vertices[i]['position'][k] for i in group) for k in range(3)]
  joint=None
  if abs(center[1])>8 and high[2]<32:joint='bip01 '+('l' if center[1]>0 else 'r')+' hand'
  elif center[0]<-5 and low[2]>33 and abs(center[1])<8:joint='bip01 spine1'
  if joint in names:
   for i in group:vertices[i]['nodes']=[names[joint],-1];vertices[i]['node0_weight']=1

def visor_indices(positions,indices,low_y,high_y):
 """Shared collection visor triangles must stay within the selected body's band."""
 return [v for i in range(0,len(indices),3)
         if all(low_y<=positions[j][1]<=high_y for j in indices[i:i+3])
         for v in indices[i:i+3]]

def gltf_uv(uv):
 # glTF and Quake's normalized TGA loader both address v=0 at the top row.
 # tga() already writes bottom-up rows for the loader; flipping UVs flips the atlas.
 return tuple(uv)

def bind_reference(parts):
 """The source model JSON already stores mesh vertices in skeleton bind space."""
 return [(v['position'],v) for part in parts for v in part['vertices']]

def bind_vertex(position,normal,uv,reference):
 """Transfer influences without pre-applying or undoing an animation pose."""
 return {'position':tuple(position),'normal':tuple(normal),'uv':uv,
         'nodes':list(reference['nodes']),'node0_weight':reference['node0_weight']}

def convert(source,retail,output,pk3,variant=DEFAULT_VARIANT):
 from PIL import Image
 import io
 archive=zipfile.ZipFile(source);g=json.loads(archive.read('scene.gltf'));blob=archive.read('scene.bin')
 def accessor(index):
  a=g['accessors'][index];v=g['bufferViews'][a['bufferView']];types={5126:'f',5123:'H',5125:'I',5121:'B'};sizes={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}
  fmt='<'+types[a['componentType']]*sizes[a['type']];stride=v.get('byteStride',struct.calcsize(fmt));offset=v.get('byteOffset',0)+a.get('byteOffset',0)
  if offset+(a['count']-1)*stride+struct.calcsize(fmt)>len(blob):raise ValueError('glTF accessor outside buffer')
  return [struct.unpack_from(fmt,blob,offset+i*stride) for i in range(a['count'])]
 selected=[]
 for mesh in g['meshes']:
  for primitive in mesh['primitives']:
   name=g['materials'][primitive['material']]['name']
   if name==variant or (variant in ('99_armor','e3_99_armor') and name==variant.replace('armor','visor')):selected.append((primitive,name))
 if not selected:raise ValueError('Variant not found')
 meta=next(json.loads(f.read_text()) for f in (retail/'model-sources').glob('cyborg_*.json'))
 bones=meta['bones'];bind=p.globals_for(bones,[(p.qmatrix(b['rotation']),tuple(x*meta['scale'] for x in b['translation'])) for b in bones])
 frames,clips,marker=c.retail_animation(retail/'models/qce/halo/player/spartan.iqm',[b['name'] for b in bones])
 # Both meshes are authored arms-down in model bind space. Matching against
 # rifle-idle vertices assigns hands to the hips and then bakes that mismatch
 # into the inverse skin transform. Animation must only be applied at runtime.
 reference=bind_reference(meta['parts'])
 lookup=tree(reference);raw=[xyz for primitive,_ in selected for xyz in accessor(primitive['attributes']['POSITION'])]
 low=[min(v[i] for v in raw) for i in range(3)];high=[max(v[i] for v in raw) for i in range(3)]
 refPositions=[v[0] for v in reference];refLow=[min(v[i] for v in refPositions) for i in range(3)];refHigh=[max(v[i] for v in refPositions) for i in range(3)]
 scale=(refHigh[2]-refLow[2])/(high[2]-low[2]);center=(low[1]+high[1])/2;files={}
 def write(name,data):
  files[name]=data;dest=output/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
 # The collection stores three models' visors together in a shared primitive.
 # Select only triangles inside this body's lateral band; do not omit the Mac visor.
 if variant in ('armormac','armor','armor01'):
  selected += [(pr,'visor') for mesh in g['meshes'] for pr in mesh['primitives'] if g['materials'][pr['material']]['name']=='visor']
 surfaces=[];visors=[];errors=[]
 for primitive,name in selected:
  if primitive.get('mode',4)!=4:raise ValueError('Only triangle primitives supported')
  positions=accessor(primitive['attributes']['POSITION']);normals=accessor(primitive['attributes']['NORMAL']);uv=accessor(primitive['attributes']['TEXCOORD_0']);verts=[]
  indices=[v[0] for v in accessor(primitive['indices'])]
  if name=='visor':indices=visor_indices(positions,indices,low[1],high[1])
  used=set(indices)
  for vi,(xyz,normal,tex) in enumerate(zip(positions,normals,uv)):
   if vi not in used:verts.append(None);continue
   position=(xyz[0]*scale,(xyz[1]-center)*scale,(xyz[2]-low[2])*scale+refLow[2]);distance,ref=nearest(lookup,position);errors.append(math.sqrt(distance))
   verts.append(bind_vertex(position,normal,gltf_uv(tex),nearest_weights(lookup,position,bones=bones)))
  tris=[]
  for i in range(0,len(indices),3):
   tri=indices[i:i+3];a,b,d=[verts[j] for j in tri];x=[b['position'][k]-a['position'][k] for k in range(3)];y=[d['position'][k]-a['position'][k] for k in range(3)];cross=(x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0])
   if sum(cross[k]*a['normal'][k] for k in range(3))>0:tri[1],tri[2]=tri[2],tri[1]
   tris.append(tri)
  # Compact first: a shared visor primitive contains unused vertices from other models.
  compact=p.a.split_surface(verts,tris,'temporary')
  for piece in compact:rigid_accessories(piece['vertices'],piece['triangles'],bones)
  verts=[v for piece in compact for v in piece['vertices']]
  tris=[];offset=0
  for piece in compact:
   tris.extend([[i+offset for i in tri] for tri in piece['triangles']]);offset+=len(piece['vertices'])
  pieces=p.a.split_surface(verts,tris,'qce/alpha/'+('visor' if 'visor' in name else 'armor'));surfaces+=pieces;visors += ['visor' in name]*len(pieces)
 material=next(g['materials'][pr['material']] for pr,name in selected if 'armor' in name);ext=material['extensions']['KHR_materials_pbrSpecularGlossiness'];texture=g['textures'][ext['diffuseTexture']['index']];uri=g['images'][texture['source']]['uri'];image=Image.open(io.BytesIO(archive.read(uri))).convert('RGBA');write('textures/qce/alpha/diff.tga',p.a.tga(image.width,image.height,image.tobytes()))
 posed=p.PoseFrames(surfaces)
 for frame in frames:posed.poses.append([p.skin_matrices(bind,p.globals_for(bones,frame))]*len(surfaces))
 write('models/qce/halo/player/cyborg.iqm',p.animated_iqm(surfaces,bones,bind,frames,[(bind,list(range(len(bones))))]*len(surfaces),[marker],list(range(len(bones))),80,clips,posed))
 for suffix,which in [('body',False),('visor',True)]:write('models/qce/halo/player/cyborg-'+suffix+'.skin',('\n'.join(f'part{i},'+('qce/alpha/visor' if visor else 'qce/alpha/armor') if visor==which else f'part{i},qce/halo/invisible' for i,visor in enumerate(visors))+'\n').encode())
 head=[];headFlags=[]
 for surface,visor in zip(surfaces,visors):
  triangles=[t for t in surface['triangles'] if all(bones[surface['vertices'][i]['nodes'][0]]['name']=='bip01 head' for i in t)]
  pieces=p.a.split_surface(surface['vertices'],triangles,surface['shader']);head+=pieces;headFlags += [visor]*len(pieces)
 if head:
  center=[(min(v['position'][i] for s in head for v in s['vertices'])+max(v['position'][i] for s in head for v in s['vertices']))/2 for i in range(3)]
  for surface in head:surface['vertices']=[{**v,'position':[v['position'][i]-center[i] for i in range(3)]} for v in surface['vertices']]
  write('models/qce/halo/player/cyborg-head.md3',p.a.md3(head))
  for suffix,which in [('body',False),('visor',True)]:write('models/qce/halo/player/cyborg-head-'+suffix+'.skin',('\n'.join(f'part{i},'+('qce/alpha/visor' if visor else 'qce/alpha/armor') if visor==which else f'part{i},qce/halo/invisible' for i,visor in enumerate(headFlags))+'\n').encode())
 shader='''qce/alpha/armor
{
 { map textures/qce/alpha/diff.tga
 rgbGen lightingDiffuse }
 { map $whiteimage
 blendFunc GL_DST_COLOR GL_ZERO
 rgbGen entity }
 { map $whiteimage
 blendFunc GL_SRC_ALPHA GL_ONE
 rgbGen const ( .35 .35 .35 )
 alphaGen lightingSpecular }
}
qce/alpha/visor
{
 { map $whiteimage
 rgbGen entity }
 { map $whiteimage
 blendFunc GL_SRC_ALPHA GL_ONE
 rgbGen const ( .65 .65 .65 )
 alphaGen lightingSpecular }
}
'''
 write('scripts/qce-cyborg.shader',shader.encode());write('models/qce/halo/player/cyborg-source.txt',('alpha-'+variant+'\n').encode());write('credits/alpha-spartan.txt',CREDIT.encode())
 report={'variant':variant,'clips':len(clips),'vertices':sum(len(s['vertices']) for s in surfaces),'triangles':sum(len(s['triangles']) for s in surfaces),'max_transfer_distance':max(errors),'mean_transfer_distance':sum(errors)/len(errors),'rigging':'blended retail bind-space influences with rigid glove/backpack pieces','credit':CREDIT}
 write('cyborg-alpha-manifest.json',(json.dumps(report,indent=2)+'\n').encode())
 if pk3:
  with zipfile.ZipFile(pk3,'w',zipfile.ZIP_DEFLATED) as z:
   for name,data in files.items():z.writestr(name,data)
 print(json.dumps(report))
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('zip',type=Path);a.add_argument('--variant',choices=('armormac','99_armor','armor','armor01','e3_99_armor'),default=DEFAULT_VARIANT);a.add_argument('--retail-output',type=Path,required=True);a.add_argument('--output',type=Path,required=True);a.add_argument('--pk3',type=Path,required=True);args=a.parse_args();convert(args.zip,args.retail_output,args.output,args.pk3,args.variant)
