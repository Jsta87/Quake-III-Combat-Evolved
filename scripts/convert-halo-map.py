#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline Xbox build-2276 Blood Gulch -> IBSP46 vehicle test map.

Geometry uses the retail render mesh, with thin convex collision prisms.
This deliberately simple one-cluster map is a prototype, not a Halo BSP port.
Layout facts checked against Reclaimer's HEK sbsp/scnr/senv/vehi definitions:
https://github.com/Sigmmma/reclaimer (GPL-3.0).
"""
import argparse,importlib.util,json,math,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def load(name,file):
 s=importlib.util.spec_from_file_location(name,ROOT/'scripts'/file);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
a=load('map_assets','convert-halo-assets.py')
SCALE=80.0
MODEL='models/qce/halo/warthog.md3'
def sub(x,y):return tuple(a-b for a,b in zip(x,y))
def dot(x,y):return sum(a*b for a,b in zip(x,y))
def cross(x,y):return (x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0])
def norm(x):
 n=math.sqrt(dot(x,x));return tuple(t/n for t in x) if n>1e-7 else None
class World:
 def __init__(self,cache):
  self.c=cache;self.d=cache.data
  self.scenario=next(t['offset'] for t in cache.index if t['class']=='scnr')
  records=self.ref(self.scenario+1444,32,False)
  if len(records)!=1:raise a.halo.CacheError('Expected one structure BSP')
  self.start,self.size,self.magic=struct.unpack_from('<3I',self.d,records[0]);cache.check(self.start,self.size)
  self.root=self.ptr(self.u(self.start),648)
 def u(self,o):self.c.check(o,4);return struct.unpack_from('<I',self.d,o)[0]
 def ptr(self,p,n):
  o=p-self.magic+self.start;self.c.check(o,n)
  if o<self.start or o+n>self.start+self.size:raise a.halo.CacheError('BSP pointer outside structure cache')
  return o
 def ref(self,o,size,bsp=True):
  n,p=struct.unpack_from('<II',self.d,o)
  if n>65535:raise a.halo.CacheError('Oversized world reflexive')
  if not n:return []
  q=self.ptr(p,n*size) if bsp else self.c.pointer(p,n*size)
  return [q+i*size for i in range(n)]
 def meshes(self):
  tris=self.ref(self.root+248,6)
  for lm in self.ref(self.root+260,32):
   for m in self.ref(lm+20,256):
    sid=self.u(m+12);first,count=struct.unpack_from('<2i',self.d,m+20);nv=self.u(m+180)
    if not 0<nv<=65535 or first<0 or count<0 or first+count>len(tris):raise a.halo.CacheError('Invalid world material ranges')
    kind=struct.unpack_from('<H',self.d,m+196)[0]
    if kind!=3:raise a.halo.CacheError('Expected Xbox compressed world vertices')
    header=self.ptr(self.u(m+192),12);vo=self.ptr(self.u(header+4),nv*32);verts=[]
    for i in range(nv):
     x,y,z,normal,_,_,u,v=struct.unpack_from('<3f3I2f',self.d,vo+i*32)
     values=(x,y,z,u,v)
     if not all(math.isfinite(t) for t in values):raise a.halo.CacheError('Nonfinite world vertex')
     n=norm((a.unpack_fraction(normal,11),a.unpack_fraction(normal>>11,11),a.unpack_fraction(normal>>22,10))) or (0,0,1)
     verts.append(((x*SCALE,y*SCALE,z*SCALE),(u,v),n))
    indices=[]
    for t in tris[first:first+count]:
     tri=struct.unpack_from('<3H',self.d,t)
     if max(tri)>=nv:raise a.halo.CacheError('World triangle outside material vertices')
     indices.extend(tri)
    if count:yield sid,verts,indices
 def entities(self):
  result=[{'classname':'worldspawn','message':'Blood Gulch - QCE vehicle prototype','gridsize':'512 512 512'}]
  starts=self.ref(self.scenario+852,52,False)
  for p in starts:
   xyz=struct.unpack_from('<3f',self.d,p);angle=struct.unpack_from('<f',self.d,p+12)[0]
   if not all(math.isfinite(t) for t in (*xyz,angle)):raise a.halo.CacheError('Nonfinite spawn')
   result.append({'classname':'info_player_deathmatch','origin':' '.join(f'{v*SCALE+(24 if i==2 else 0):.3f}' for i,v in enumerate(xyz)),'angle':f'{math.degrees(angle):.3f}'})
  if len(result)<2:raise a.halo.CacheError('No multiplayer starts')
  palettes=self.ref(self.scenario+588,48,False)
  for p in self.ref(self.scenario+576,120,False):
   ix=struct.unpack_from('<h',self.d,p)[0]
   if not 0<=ix<len(palettes):raise a.halo.CacheError('Invalid vehicle palette')
   tag=self.c.tags.get(self.u(palettes[ix]+12))
   if not tag or tag['path']!='vehicles\\warthog\\warthog':continue
   xyz=struct.unpack_from('<3f',self.d,p+8);rotation=struct.unpack_from('<3f',self.d,p+20)
   result.append({'classname':'qce_warthog','origin':' '.join(f'{v*SCALE+(18 if i==2 else 0):.3f}' for i,v in enumerate(xyz)),'angle':f'{math.degrees(rotation[0]):.3f}'})
  return result
class BSP:
 def __init__(self):
  self.l=[bytearray() for _ in range(17)];self.planes={};self.shaders={};self.surfaces=0;self.brushes=0;self.bounds=[]
  self.plane((1,0,0),0)
 def plane(self,n,d):
  key=tuple(round(x,5) for x in (*n,d))
  if key not in self.planes:
   ix=len(self.l[2])//16;self.planes[key]=ix;self.planes[tuple(-x for x in key)]=ix+1
   self.l[2]+=struct.pack('<4f',*n,d)+struct.pack('<4f',*(-x for x in n),-d)
  return self.planes[key]
 def shader(self,name,solid=True):
  if name not in self.shaders:
   if len(name.encode())>=64:raise ValueError('Shader name too long')
   self.shaders[name]=len(self.shaders);self.l[1]+=struct.pack('<64s2i',name.encode(),4 if name=='qce/bloodgulch/sky' else 0,1 if solid else 0)
  return self.shaders[name]
 def brush(self,points,shader):
  n=norm(cross(sub(points[1],points[0]),sub(points[2],points[0])))
  if n is None:return
  # Halo render winding is corrected by vertex normals before this call.
  back=[tuple(p[i]-n[i]*2 for i in range(3)) for p in points];hull=points+back
  planes=[(n,dot(n,points[0])),(tuple(-x for x in n),-dot(n,back[0]))]
  for i in range(3):
   edge=norm(cross(sub(points[(i+1)%3],points[i]),n));planes.append((edge,dot(edge,points[i])))
  # Quake's box traces require axial bounds first (CM_LoadBrushes) and edge bevels.
  axial=[]
  for axis in range(3):
   for sign in (-1,1):
    normal=tuple(sign if j==axis else 0 for j in range(3));axial.append((normal,max(dot(normal,p) for p in hull)))
  bevel=[]
  for i in range(3):
   e=sub(points[(i+1)%3],points[i])
   for axis in ((1,0,0),(0,1,0),(0,0,1)):
    bn=norm(cross(e,axis))
    if bn is None:continue
    for sign in (1,-1):
     nn=tuple(x*sign for x in bn);dd=dot(nn,points[i])
     if all(dot(nn,p)<=dd+0.01 for p in hull):bevel.append((nn,dd))
  ids=[]
  for nn,dd in axial+planes+bevel:
   ix=self.plane(nn,dd)
   if ix not in ids:ids.append(ix)
  first=len(self.l[9])//8
  for ix in ids:self.l[9]+=struct.pack('<2i',ix,shader)
  self.l[8]+=struct.pack('<3i',first,len(ids),shader);self.brushes+=1
 def mesh(self,name,verts,indices,solid):
  shader=self.shader(name,solid);firstvert=len(self.l[10])//44;firstindex=len(self.l[11])//4
  for p,uv,n in verts:
   self.bounds.append(p);brightness=max(.3,.75+.25*n[2]);color=int(brightness*255)
   self.l[10]+=struct.pack('<10f4B',*p,*uv,0,0,*n,color,color,color,255)
  corrected=[]
  for i in range(0,len(indices),3):
   t=list(indices[i:i+3]);pts=[verts[j][0] for j in t];normal=cross(sub(pts[1],pts[0]),sub(pts[2],pts[0]))
   if dot(normal,verts[t[0]][2])<0:t[1],t[2]=t[2],t[1];pts=[verts[j][0] for j in t]
   # Quake draw triangles use the opposite winding to the collision normal.
   corrected.extend((t[0],t[2],t[1]))
   if solid:self.brush(pts,shader)
  self.l[11]+=struct.pack('<'+'i'*len(corrected),*corrected)
  self.l[13]+=struct.pack('<12i12f2i',shader,-1,3,firstvert,len(verts),firstindex,len(indices),-3,0,0,0,0,*([0.0]*12),0,0);self.surfaces+=1
 def finish(self,entities):
  if not self.bounds or self.brushes>32768 or self.surfaces>131072 or len(self.l[10])//44>524288 or len(self.l[11])//4>1048576:raise a.halo.CacheError('World exceeds Quake BSP limits or is empty')
  for k in (2,9):
   if len(self.l[k])//(16 if k==2 else 8)>131072:raise ValueError('Quake BSP plane/brushside limit exceeded')
  mins=[math.floor(min(p[i] for p in self.bounds))-128 for i in range(3)];maxs=[math.ceil(max(p[i] for p in self.bounds))+128 for i in range(3)]
  self.l[0]=bytearray(('\n'.join('{\n'+'\n'.join('"'+k+'" "'+v+'"' for k,v in e.items())+'\n}' for e in entities)+'\n\0').encode())
  self.l[3]+=struct.pack('<9i',0,-1,-1,*mins,*maxs)
  self.l[4]+=struct.pack('<12i',0,0,*mins,*maxs,0,self.surfaces,0,self.brushes)
  self.l[5]+=struct.pack('<'+'i'*self.surfaces,*range(self.surfaces));self.l[6]+=struct.pack('<'+'i'*self.brushes,*range(self.brushes))
  self.l[7]+=struct.pack('<6f4i',*mins,*maxs,0,self.surfaces,0,self.brushes)
  gridpoints=math.prod(math.floor(maxs[i]/512)-math.ceil(mins[i]/512)+1 for i in range(3))
  self.l[15]+=bytes([100,100,100,100,100,100,0,0])*gridpoints
  self.l[16]+=struct.pack('<2iB',1,1,1)
  header=bytearray(b'IBSP'+struct.pack('<i',46));data=bytearray();offset=144
  for lump in self.l:
   header+=struct.pack('<2i',offset+len(data),len(lump));data+=lump
   data+=b'\0'*((-len(data))%4)
  return bytes(header+data)
def convert(path,output,pk3):
 cache=a.halo.XboxMap(path)
 if cache.header['name']!='bloodgulch':raise a.halo.CacheError('Currently validated for Blood Gulch only')
 w=World(cache);assets=a.Assets(cache,output);bsp=BSP();shadertext=[];textures=set()
 def texture(tag):
  if tag['id'] not in textures:assets.texture(tag);textures.add(tag['id'])
  r=next((r for r in assets.records if r['id']==tag['id'] and r.get('index')==0),None)
  return r['outputs'][0] if r else '$whiteimage'
 for sid,verts,indices in w.meshes():
  tag=cache.tags.get(sid);name=f'qce/bloodgulch/{sid:08x}';image='$whiteimage';solid=True
  if tag and tag['class']=='senv':
   bitmap=cache.tags.get(w.u(tag['offset']+40+68+28+12))
   if bitmap:image=texture(bitmap)
  elif tag and tag['class']=='sotr':
   solid=False;maps=w.ref(tag['offset']+84,100,False)
   if maps:
    bitmap=cache.tags.get(w.u(maps[0]+28+12))
    if bitmap:image=texture(bitmap)
    us,vs,uo,vv=struct.unpack_from('<4f',cache.data,maps[0]+4)
    if not all(math.isfinite(t) for t in (us,vs,uo,vv)):raise a.halo.CacheError('Nonfinite material UV transform')
    verts=[(p,(uv[0]*(us or 1)+uo,uv[1]*(vs or 1)+vv),n) for p,uv,n in verts]
  elif not tag or tag['class'] in ('schi','scex','swat'):solid=False
  if name not in bsp.shaders:
   blend=' blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n depthWrite\n alphaFunc GE128\n' if not solid else ''
   shadertext.append(f'{name}\n{{\n cull none\n {{ map {image}\n{blend} rgbGen vertex }}\n}}\n')
  for part in a.split_surface(verts,[indices[i:i+3] for i in range(0,len(indices),3)],name):
   bsp.mesh(name,part['vertices'],[i for tri in part['triangles'] for i in tri],solid)
 # A simple procedural blue sky closes the unrendered Halo atmosphere volume.
 low=[min(p[i] for p in bsp.bounds)-256 for i in range(3)];high=[max(p[i] for p in bsp.bounds)+256 for i in range(3)];high[2]+=1024
 corners=[tuple(high[i] if n&(1<<i) else low[i] for i in range(3)) for n in range(8)]
 for quad in ((0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)):
  pts=[corners[j] for j in quad];nn=norm(cross(sub(pts[1],pts[0]),sub(pts[2],pts[0])))
  bsp.mesh('qce/bloodgulch/sky',[(p,(0,0),nn) for p in pts],[0,1,2,0,2,3],False)
 shadertext.append('qce/bloodgulch/sky\n{\n surfaceparm sky\n surfaceparm noimpact\n skyparms - 512 -\n { map $whiteimage\n rgbGen const ( 0.35 0.60 0.85 ) }\n}\n')
 vehicle=next(t for t in cache.index if t['class']=='vehi' and t['path']=='vehicles\\warthog\\warthog')
 model=cache.tags[w.u(vehicle['offset']+40+12)]
 # Export model dependencies with the existing bounds-checked Xbox model decoder.
 refs=assets.reflexive('Model','shaders',model['offset'],'ModelShaderReference')
 for p in refs:
  shader=cache.tags.get(w.u(p+12))
  if shader and shader['class']=='soso':
   bit=cache.tags.get(w.u(assets.field('ShaderModel','base map',shader['offset'])+12))
   if bit:texture(bit)
 models=assets.weapon_models({'vehicle':{'models':[model]}})
 old='scripts/qce-halo.shader';assets.files.pop(old,None)
 assets.write(MODEL,(output/models[0]['output']).read_bytes(),{'alias_of':models[0]['output']})
 vehicle_assets=load('vehicle_assets','convert-halo-vehicle.py');vehicle_assets.export(cache,assets,model)
 shadertext.append((output/old).read_text());assets.files.pop(old,None)
 entities=w.entities();assets.write('maps/qce_bloodgulch.bsp',bsp.finish(entities),{'role':'terrain prototype'})
 assets.write('scripts/qce-bloodgulch.shader','\n'.join(shadertext).encode(),{'role':'world and vehicle materials'})
 assets.write('scripts/qce-bloodgulch.arena',b'{ map qce_bloodgulch longname "Blood Gulch" type "ffa team" }\n',{'role':'map menu'})
 report={'source':cache.header,'scale':SCALE,'render_surfaces':bsp.surfaces,'collision_brushes':bsp.brushes,'planes':len(bsp.l[2])//16,'entities':entities,'vehicle_model':MODEL,'limitations':['One visibility cluster; no optimized PVS or bot AAS','Render triangles approximate collision; scenery and retail lightmaps not converted','Split Warthog parts with procedural wheels/steering; runtime wheeled physics is a prototype','Procedural blue sky and vertex lighting replace retail atmosphere/lightmaps']}
 output.mkdir(parents=True,exist_ok=True);(output/'map-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
 if pk3:assets.package(pk3)
 print(json.dumps({k:report[k] for k in ('render_surfaces','collision_brushes','planes','vehicle_model')}))
 return report
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('map',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--pk3',type=Path);args=p.parse_args();convert(args.map,args.output,args.pk3)
