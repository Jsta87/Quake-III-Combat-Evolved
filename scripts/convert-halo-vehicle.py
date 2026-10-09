#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Warthog moving parts and calibrated source audio; imported by map converter."""
import importlib.util,json,math,struct,io,wave
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('vehicle_pose',ROOT/'scripts/animate-halo-weapons.py');p=importlib.util.module_from_spec(s);s.loader.exec_module(p)
def export(cache,assets,model):
 meta=json.loads((assets.output/f'model-sources/{p.a.safe_name(model["path"])}.json').read_text())
 bind=p.globals_for(meta['bones'],[(p.qmatrix(b['rotation']),tuple(t*meta['scale'] for t in b['translation'])) for b in meta['bones']])
 node_groups={6:'wheel',1:'turret-base',7:'turret-base',8:'turret-gun',13:'turret-barrels',
  2:'suspension-left-back',9:'suspension-left-back',3:'suspension-left-front',10:'suspension-left-front',
  4:'suspension-right-back',11:'suspension-right-back',5:'suspension-right-front',12:'suspension-right-front',
  14:'left-back',15:'left-front',16:'right-back',17:'right-front'}
 groups={k:[] for k in ('body',*dict.fromkeys(node_groups.values()))}
 pivots={k:bind[n][1] for n,k in node_groups.items()}
 # Arms pivot at their hull hinge, not at their child wheel carrier.
 for n in (2,3,4,5):pivots[node_groups[n]]=bind[n][1]
 pivots['turret-base']=bind[1][1];pivots['body']=(0,0,0)
 for part in meta['parts']:
  shader=f'qce/halo/{meta["id"]:08x}/{part["geometry"]}_{part["part"]}'
  partition={k:[] for k in groups}
  for tri in part['triangles']:
   weights={k:0. for k in groups}
   for vi in tri:
    v=part['vertices'][vi]
    for n,w in zip(v['nodes'],(v['node0_weight'],1-v['node0_weight'])):weights[node_groups.get(n,'body')]+=w
   partition[max(weights,key=weights.get)].append(tri)
  for key,tris in partition.items():
   if not tris:continue
   origin=pivots[key];verts=[{**v,'position':[v['position'][i]-origin[i] for i in range(3)]} for v in part['vertices']]
   groups[key].extend(p.a.split_surface(verts,tris,shader))
 for key,surfaces in groups.items():
  if not surfaces:raise p.a.halo.CacheError(f'Missing Warthog moving part {key}')
  assets.write(f'models/qce/halo/warthog/{key}.md3',p.a.md3(surfaces),{'source':model,'moving_part':key})
 assets.write('models/qce/halo/warthog/rig.json',json.dumps({'pivots':pivots,'source':model['path'],'scale':meta['scale']},indent=2).encode(),{'role':'moving-part audit'})
 # Use the same Halo material conversion as the weapons, including glass.
 exported={r['id'] for r in assets.records if 'index' in r}
 classes={'soso':'ShaderModel','sgla':'ShaderTransparentGlass','sotr':'ShaderTransparentGeneric','schi':'ShaderTransparentChicago','smet':'ShaderTransparentMeter'}
 for part in meta['parts']:
  tag=next(t for t in cache.index if t['path']==part['shader'] and t['class'] in classes)
  kind=classes[tag['class']]
  for key,field in p.a.LAYOUT['structs'][kind]['fields'].items():
   deps=[]
   if field['type']=='TagDependency':deps=[struct.unpack_from('<I',cache.data,tag['offset']+field['offset']+12)[0]]
   elif field['type']=='TagReflexive' and key=='maps':
    for entry in assets.reflexive(kind,key,tag['offset'],field['struct']):
     ofs=p.a.LAYOUT['structs'][field['struct']]['fields']['map']['offset'];deps.append(struct.unpack_from('<I',cache.data,entry+ofs+12)[0])
   for ident in deps:
    dep=cache.tags.get(ident)
    if dep and dep['class']=='bitm' and ident not in exported:assets.texture(dep);exported.add(ident)
 p.materials(cache,assets,[{'id':meta['id'],'source_mesh':f'model-sources/{p.a.safe_name(model["path"])}.json'}])
 audio_sources={}
 for alias,path in {'engine-load':'sound\\sfx\\vehicles\\warthog_engine5','turret-fire':'sound\\sfx\\weapons\\chaingun\\fire','engine-start':'sound\\sfx\\vehicles\\warthog_engine_start','engine':'sound\\sfx\\vehicles\\warthog_engine','engine-stop':'sound\\sfx\\vehicles\\warthog_engine_stop','suspension':'sound\\sfx\\impulse\\impacts\\warthog_suspension'}.items():
  tag=next(t for t in cache.index if t['class']=='snd!' and t['path']==path)
  before=len(assets.records);assets.sound(tag);records=assets.records[before:]
  if not records:raise p.a.halo.CacheError(f'No audio for {path}')
  audio,calibration=assets.calibrated_sound(records[0]);audio_sources[alias]=audio;assets.write(f'sound/qce/halo/warthog/{alias}.wav',audio,{'source':tag,'calibration':calibration})
 for seat,name in enumerate(('driver','rider','gunner')):
  for phase in ('enter','exit'):
   path=f'sound\\sfx\\impulse\\animations\\cyborg\\w_{name}_{phase}'
   tag=next(t for t in cache.index if t['class']=='snd!' and t['path']==path);before=len(assets.records);assets.sound(tag)
   records=assets.records[before:]
   if not records:raise p.a.halo.CacheError(f'Missing seat audio {path}')
   audio,calibration=assets.calibrated_sound(records[0]);assets.write(f'sound/qce/halo/warthog/seat-{seat}-{phase}.wav',audio,{'source':tag,'calibration':calibration})
 # Quake's common looping-sound ABI has no independent pitch/gain input.
 # Pre-render small RPM bands, blending the source engine and load layers.
 def pcm(blob):
  with wave.open(io.BytesIO(blob),'rb') as w:
   if w.getnchannels()!=1 or w.getsampwidth()!=2:raise p.a.halo.CacheError('Engine PCM must be mono 16-bit')
   data=w.readframes(w.getnframes());return w.getframerate(),struct.unpack('<'+'h'*(len(data)//2),data)
 rate,idle=pcm(audio_sources['engine']);load_rate,load=pcm(audio_sources['engine-load'])
 for band in range(17):
  rpm=band/16;pitch=1+rpm*.6;count=max(1,round(len(idle)/pitch));out=[]
  def sample(data,pos):
   lo=int(pos)%len(data);f=pos-int(pos);return data[lo]*(1-f)+data[(lo+1)%len(data)]*f
  for i in range(count):
   value=sample(idle,i*len(idle)/count)*(1-.35*rpm)+sample(load,i*load_rate/rate*pitch)*(.3*rpm)
   out.append(max(-32768,min(32767,round(value))))
  blob=io.BytesIO()
  with wave.open(blob,'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(rate);w.writeframes(struct.pack('<'+'h'*count,*out))
  assets.write(f'sound/qce/halo/warthog/engine-rpm-{band:02}.wav',blob.getvalue(),{'role':'engine RPM band','rpm':rpm,'pitch':pitch,'limitations':'prototype pitch/load envelope'})
 for alias,path in {'dust':'effects\\particles\\air\\bitmaps\\smoke cloud','rock':'effects\\particles\\solid\\bitmaps\\gravel'}.items():
  tag=next(t for t in cache.index if t['class']=='bitm' and t['path']==path);assets.texture(tag)
  r=next(r for r in assets.records if r.get('id')==tag['id'] and r.get('index')==0);w,h,pixels=p.read_tga(assets.output/r['outputs'][0])
  seq=assets.reflexive('Bitmap','bitmap group sequence',tag['offset'],'BitmapGroupSequence')
  sprites=assets.reflexive('BitmapGroupSequence','sprites',seq[0],'BitmapGroupSprite') if seq else []
  if sprites:
   q=sprites[0];bi=struct.unpack_from('<h',cache.data,q)[0];w,h,pixels=p.read_tga(assets.output/f'textures/qce/halo/{p.a.safe_name(tag["path"])}/{bi:03}.tga')
   l,r,t,b=struct.unpack_from('<4f',cache.data,q+8);x0,x1,y0,y1=round(l*w),round(r*w),round(t*h),round(b*h)
   pixels=b''.join(pixels[(y*w+x0)*4:(y*w+x1)*4] for y in range(y0,y1));w,h=x1-x0,y1-y0
  assets.write(f'textures/qce/warthog/{alias}.tga',p.a.tga(w,h,pixels),{'source':tag})
 assets.write('scripts/qce-warthog-effects.shader','\n'.join(f'qce/warthog/{alias}\n{{\n cull none\n {{ map textures/qce/warthog/{alias}.tga\n blendFunc blend\n rgbGen vertex\n alphaGen vertex }}\n}}' for alias in ('dust','rock')).encode(),{'role':'tire particles'})
