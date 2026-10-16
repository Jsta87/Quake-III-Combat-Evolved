#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Rig the user's Sketchfab glTF collection to the local retail Spartan skeleton.
Requires Pillow for PNG decoding. Generated art remains owner-local.
"""
import argparse,importlib.util,json,math,struct,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('cyborg',ROOT/'scripts/convert-halo-cyborg.py');c=importlib.util.module_from_spec(s);s.loader.exec_module(c);p=c.p
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
def inverse(r):
 a,b,d=r
 def cross(x,y):return (x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0])
 cols=[cross(b,d),cross(d,a),cross(a,b)];det=sum(a[i]*cols[0][i] for i in range(3))
 if abs(det)<1e-8:raise ValueError('Degenerate skin transform')
 return tuple(tuple(cols[j][i]/det for j in range(3)) for i in range(3))
def convert(source,retail,output,pk3,variant='armormac'):
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
 skins=p.skin_matrices(bind,p.globals_for(bones,frames[0]));reference=[]
 for part in meta['parts']:
  for vertex in part['vertices']:reference.append((p.fast_skin(vertex,skins)['position'],vertex))
 lookup=tree(reference);raw=[xyz for primitive,_ in selected for xyz in accessor(primitive['attributes']['POSITION'])]
 low=[min(v[i] for v in raw) for i in range(3)];high=[max(v[i] for v in raw) for i in range(3)]
 refPositions=[v[0] for v in reference];refLow=[min(v[i] for v in refPositions) for i in range(3)];refHigh=[max(v[i] for v in refPositions) for i in range(3)]
 scale=(refHigh[2]-refLow[2])/(high[2]-low[2]);center=(low[1]+high[1])/2;files={}
 def write(name,data):
  files[name]=data;dest=output/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
 surfaces=[];visors=[];errors=[]
 for primitive,name in selected:
  if primitive.get('mode',4)!=4:raise ValueError('Only triangle primitives supported')
  positions=accessor(primitive['attributes']['POSITION']);normals=accessor(primitive['attributes']['NORMAL']);uv=accessor(primitive['attributes']['TEXCOORD_0']);verts=[]
  for xyz,normal,tex in zip(positions,normals,uv):
   position=(xyz[0]*scale,(xyz[1]-center)*scale,(xyz[2]-low[2])*scale+refLow[2]);distance,ref=nearest(lookup,position);errors.append(math.sqrt(distance))
   nodes=ref['nodes'];weight=ref['node0_weight'];rr=[[0]*3 for _ in range(3)];tt=[0,0,0]
   for node,w in zip(nodes,(weight,1-weight)):
    if not w:continue
    r,t=skins[node]
    for i in range(3):
     tt[i]+=t[i]*w
     for j in range(3):rr[i][j]+=r[i*3+j]*w
   inv=inverse(rr);rest=p.rotate(inv,tuple(position[i]-tt[i] for i in range(3)));n=p.rotate(inv,normal);length=math.sqrt(sum(x*x for x in n));n=tuple(x/length for x in n)
   verts.append({'position':rest,'normal':n,'uv':(tex[0],1-tex[1]),'nodes':nodes,'node0_weight':weight})
  indices=[v[0] for v in accessor(primitive['indices'])];tris=[]
  for i in range(0,len(indices),3):
   tri=indices[i:i+3];a,b,d=[verts[j] for j in tri];x=[b['position'][k]-a['position'][k] for k in range(3)];y=[d['position'][k]-a['position'][k] for k in range(3)];cross=(x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0])
   if sum(cross[k]*a['normal'][k] for k in range(3))>0:tri[1],tri[2]=tri[2],tri[1]
   tris.append(tri)
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
 report={'variant':variant,'clips':len(clips),'vertices':len(raw),'triangles':sum(len(s['triangles']) for s in surfaces),'max_transfer_distance':max(errors),'mean_transfer_distance':sum(errors)/len(errors),'rigging':'nearest posed retail vertex, inverse blended skin transform','credit':CREDIT}
 write('cyborg-alpha-manifest.json',(json.dumps(report,indent=2)+'\n').encode())
 if pk3:
  with zipfile.ZipFile(pk3,'w',zipfile.ZIP_DEFLATED) as z:
   for name,data in files.items():z.writestr(name,data)
 print(json.dumps(report))
if __name__=='__main__':
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('zip',type=Path);a.add_argument('--variant',choices=('armormac','99_armor','armor','armor01','e3_99_armor'),default='armormac');a.add_argument('--retail-output',type=Path,required=True);a.add_argument('--output',type=Path,required=True);a.add_argument('--pk3',type=Path,required=True);args=a.parse_args();convert(args.zip,args.retail_output,args.output,args.pk3,args.variant)
