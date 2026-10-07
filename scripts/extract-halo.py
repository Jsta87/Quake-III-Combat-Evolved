#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounds-checked, offline Xbox CE cache reader. Does not export art or execute map data."""
import argparse,hashlib,json,math,struct,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
LAYOUT=json.loads((ROOT/'data/halo-layout/xbox.json').read_text())
CLASSES={'weap':'Weapon','proj':'Projectile','jpt!':'DamageEffect','matg':'Globals','bipd':'Biped','coll':'ModelCollisionGeometry','effe':'Effect','antr':'ModelAnimations'}
WEAPONS={'WP_MACHINEGUN':'weapons\\assault rifle\\assault rifle','WP_SHOTGUN':'weapons\\shotgun\\shotgun','WP_ROCKET_LAUNCHER':'weapons\\rocket launcher\\rocket launcher','WP_RAILGUN':'weapons\\sniper rifle\\sniper rifle','WP_PLASMAGUN':'weapons\\plasma rifle\\plasma rifle','WP_LIGHTNING':'weapons\\plasma pistol\\plasma pistol','WP_BFG':'weapons\\pistol\\pistol','WP_GRENADE_LAUNCHER':'weapons\\needler\\mp_needler'}
class CacheError(ValueError):pass
class XboxMap:
 def __init__(self,path):
  self.path=Path(path);packed=self.path.read_bytes()
  if len(packed)<2048 or packed[:4]!=b'daeh' or packed[2044:2048]!=b'toof':raise CacheError('Invalid cache header/footer')
  version,size,_,offset,length=struct.unpack_from('<5I',packed,4)
  if version!=5:raise CacheError(f'Expected Xbox version 5, got {version}')
  if not 2048<=size<=128*1024*1024:raise CacheError('Invalid decompressed size')
  if len(packed)==size:self.data=packed;compressed=False
  else:
   d=zlib.decompressobj()
   try:body=d.decompress(packed[2048:],size-2048+1)
   except zlib.error as e:raise CacheError('Invalid compressed cache stream') from e
   if len(body)!=size-2048 or not d.eof or d.unconsumed_tail:raise CacheError('Decompression length/stream mismatch')
   if any(d.unused_data):raise CacheError('Nonzero trailing compressed data')
   self.data=packed[:2048]+body;compressed=True
  self.tag_offset=offset;self.tag_length=length;self.check(offset,length)
  self.header={'filename':self.path.name,'name':self.cstring(32,32),'build':self.cstring(64,32),'version':version,'compressed':compressed,'file_size':len(packed),'decompressed_size':size,'sha256':hashlib.sha256(packed).hexdigest(),'header_crc32':f'{struct.unpack_from("<I",packed,100)[0]:08x}','header_crc_verified':False}
  if self.header['build']!='01.10.12.2276':raise CacheError('This reader is currently validated for build 2276 only')
  array,_,_,count=struct.unpack_from('<4I',self.data,offset)
  if count>65535 or self.data[offset+32:offset+36]!=b'sgat':raise CacheError('Invalid Xbox tag header')
  self.base=array-36;self.tags={};self.index=[]
  for i in range(count):
   entry=self.pointer(array+i*32,32);group=self.data[entry:entry+4][::-1].decode('ascii')
   ident,pathptr,dataptr,indexed=struct.unpack_from('<4I',self.data,entry+12)
   if ident&65535!=i or ident in self.tags or indexed:raise CacheError('Invalid/indexed Xbox tag entry')
   pathoff=self.pointer(pathptr,1);name=self.cstring(pathoff,1024)
   dataoff=None if group=='sbsp' and dataptr==0 else self.pointer(dataptr,1)
   tag={'id':ident,'class':group,'path':name,'offset':dataoff};self.tags[ident]=tag;self.index.append(tag)
  self.header['tag_count']=count
 def check(self,offset,size):
  if offset<0 or size<0 or offset>len(self.data)-size:raise CacheError('Read outside decompressed cache')
 def pointer(self,pointer,size):
  offset=pointer-self.base+self.tag_offset;self.check(offset,size)
  if offset<self.tag_offset or offset+size>self.tag_offset+self.tag_length:raise CacheError('Pointer outside tag data')
  return offset
 def cstring(self,offset,limit):
  self.check(offset,1);end=self.data.find(b'\0',offset,min(offset+limit,len(self.data)))
  if end<0:raise CacheError('Unterminated string')
  return self.data[offset:end].decode('ascii')
 def decode(self,name,offset,depth=0):
  if depth>6:raise CacheError('Reflexive nesting too deep')
  layout=LAYOUT['structs'][name];self.check(offset,layout['size']);result={}
  for key,f in layout['fields'].items():
   pos=offset+f['offset'];typ=f['type'];size=f['size']
   if typ=='TagDependency':
    ident=struct.unpack_from('<I',self.data,pos+12)[0]
    if ident==0xffffffff:result[key]=None
    else:
     if ident not in self.tags:raise CacheError(f'Missing dependency {ident:08x}')
     tag=self.tags[ident];result[key]={'id':ident,'class':tag['class'],'path':tag['path']}
   elif typ=='TagReflexive':
    count,pointer=struct.unpack_from('<2I',self.data,pos)
    if count>65535:raise CacheError('Reflexive count exceeds limit')
    child=LAYOUT['structs'].get(f['struct'])
    if not count:result[key]=[]
    elif child:
     start=self.pointer(pointer,count*child['size']);result[key]=[self.decode(f['struct'],start+i*child['size'],depth+1) for i in range(count)]
    else:result[key]={'count':count,'unparsed_struct':f['struct']}
   elif typ=='TagString':result[key]=self.cstring(pos,32)
   else:
    formats={'float':'f','Angle':'f','Fraction':'f','Point3D':'f','Vector3D':'f','ColorARGB':'f','Index':'h','int16':'h','uint16':'H','uint32':'I','uint8':'B','int8':'b','int32':'i','Euler2D':'f','Vector2D':'f','TagDataOffset':'I','Quaternion':'f'}
    fmt=formats[typ];n=size//struct.calcsize(fmt);v=struct.unpack_from('<'+fmt*n,self.data,pos)
    if any(isinstance(x,float) and not math.isfinite(x) for x in v):raise CacheError(f'Nonfinite field {name}.{key}')
    result[key]=v[0] if n==1 else list(v)
  return result
 def tag(self,path,group=None):
  matches=[t for t in self.index if t['path']==path and (not group or t['class']==group)]
  if len(matches)!=1:raise CacheError(f'Expected one tag for {path}, got {len(matches)}')
  t=matches[0];name=CLASSES[t['class']];offset=t['offset'];size=LAYOUT['structs'][name]['size']
  self.pointer(offset-self.tag_offset+self.base,size)
  return {**t,'root_sha256':hashlib.sha256(self.data[offset:offset+size]).hexdigest(),'values':self.decode(name,offset)}
 def dependency(self,dep):return self.tag(dep['path'],dep['class']) if dep else None
 def report(self):
  weapons={}
  for slot,path in WEAPONS.items():
   if slot=='WP_GRENADE_LAUNCHER' and not any(t['path']==path for t in self.index):path='weapons\\needler\\needler'
   w=self.tag(path,'weap');weapons[slot]=w
   w['projectiles']=[]
   for trigger in w['values']['triggers']:
    p=self.dependency(trigger['projectile']);w['projectiles'].append(p)
    if p:
     p['effects']={k:self.dependency(v) for k,v in p['values'].items() if isinstance(v,dict) and v.get('class')=='effe'}
     p['damage_effects']={k:self.dependency(v) for k,v in p['values'].items() if isinstance(v,dict) and v.get('class')=='jpt!'}
   w['melee']=self.dependency(w['values']['player melee damage'])
   w['animations']=self.dependency(w['values']['first person animations'])
  globals_tag=self.tag('globals\\globals','matg');g=globals_tag['values']
  player=self.dependency(g['multiplayer information'][0]['unit'])
  if player:player['collision']=self.dependency(player['values']['collision model'])
  grenades=[]
  for entry in g['grenades']:
   p=self.dependency(entry['projectile']);grenades.append({'globals':entry,'projectile':p})
   if p:p['effects']={k:self.dependency(v) for k,v in p['values'].items() if isinstance(v,dict) and v.get('class')=='effe'}
   if p:p['damage_effects']={k:self.dependency(v) for k,v in p['values'].items() if isinstance(v,dict) and v.get('class')=='jpt!'}
  selected=[p for w in weapons.values() for p in w['projectiles'] if p]+[globals_tag,player,player['collision']]+[g['projectile'] for g in grenades]
  for tag in selected:
   for effect in tag.get('effects',{}).values():
    if effect:
     effect['damage_effects']=[self.dependency(part['type']) for event in effect['values']['events'] for part in event['parts'] if part.get('type') and part['type']['class']=='jpt!']
  return {'schema_version':1,'map':self.header,'layout_reference':{'repository':LAYOUT['source'],'commit':LAYOUT['commit']},'weapons':weapons,'globals':globals_tag,'player':player,'grenades':grenades}
def main():
 p=argparse.ArgumentParser();p.add_argument('map',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--index-only',action='store_true');args=p.parse_args()
 cache=XboxMap(args.map);result={'map':cache.header,'tags':cache.index} if args.index_only else cache.report()
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
 print(f"Read {cache.header['name']} build {cache.header['build']}: {len(cache.index)} tags -> {args.output}")
if __name__=='__main__':main()
