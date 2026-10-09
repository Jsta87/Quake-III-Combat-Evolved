#!/usr/bin/env python3
"""Animation/skinning/material fixtures; no copyrighted map data required."""
import importlib.util
import math
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
ROOT=Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('anim',ROOT/'scripts/animate-halo-weapons.py');a=importlib.util.module_from_spec(s);s.loader.exec_module(a)

class AnimationTests(unittest.TestCase):
 def test_quaternion_hierarchy_and_cycles(self):
  r=a.qmatrix((0,0,math.sqrt(.5),math.sqrt(.5)))
  v=a.rotate(r,(1,0,0));self.assertAlmostEqual(v[0],0);self.assertAlmostEqual(v[1],-1)
  result=a.globals_for([{'parent':-1},{'parent':0}],[(r,(10,0,0)),(a.IDENTITY,(2,0,0))])
  self.assertAlmostEqual(result[1][1][0],10);self.assertAlmostEqual(result[1][1][1],-2)
  with self.assertRaises(a.a.halo.CacheError):a.globals_for([{'parent':1},{'parent':0}],[(a.IDENTITY,(0,0,0))]*2)
 def fixture(self):
  animation={'node count':2,'frame count':2,'frame size':12,'flags':0,'offset to compressed data':0,'node rotation flag data':[0,0],'node transform flag data':[2,0],'node scale flag data':[0,0]}
  # Node zero: all default. Node one: quaternion/scale default, translation dynamic.
  default=struct.pack('<4h3ff4hf',0,0,0,32767,1,2,3,1,0,0,0,32767,1)
  frames=struct.pack('<6f',4,5,6,7,8,9)
  return animation,default,frames
 def test_default_dynamic_tracks(self):
  desc,default,frames=self.fixture();result=a.decode_tracks(desc,default,frames)
  self.assertEqual(result[0][0][1],(1,2,3));self.assertEqual(result[0][1][1],(4,5,6));self.assertEqual(result[1][1][1],(7,8,9))
  self.assertEqual(result[1][0][0],a.IDENTITY)
 def test_malformed_track_buffers_masks_and_floats(self):
  desc,default,frames=self.fixture()
  for d,df,f in [(desc,default[:-1],frames),(desc,default,frames[:-1]),({**desc,'node transform flag data':[4,0]},default,frames),({**desc,'flags':1},default,frames),(desc,default,struct.pack('<6f',float('nan'),0,0,0,0,0))]:
   with self.assertRaises(a.a.halo.CacheError):a.decode_tracks(d,df,f)
 def test_bind_inverse_and_weighted_skin(self):
  v={'position':[12,0,0],'normal':[1,0,0],'nodes':[0,1],'node0_weight':.25}
  bind=[(a.IDENTITY,(10,0,0)),(a.IDENTITY,(0,0,0))];pose=[(a.IDENTITY,(20,0,0)),(a.IDENTITY,(0,4,0))]
  result=a.skin_vertex(v,bind,pose);fast=a.fast_skin(v,a.skin_matrices(bind,pose))
  self.assertEqual(result['position'],[14.5,3,0]);self.assertEqual(fast['position'],result['position']);self.assertEqual(fast['normal'],result['normal'])
  identity=a.skin_vertex(v,bind,bind);self.assertEqual(identity['position'],v['position'])
 def test_marker_follows_bone_rotation_and_translation(self):
  marker={'name':'tag_flash','node':0,'translation':(1,0,0),'rotation':(0,0,0,1)}
  r=a.qmatrix((0,0,math.sqrt(.5),math.sqrt(.5)))
  tags=a.pose_tags([marker],[(r,(5,0,0))],80)
  self.assertAlmostEqual(tags['tag_flash'][1][0],5);self.assertAlmostEqual(tags['tag_flash'][1][1],-80)
 def test_multiframe_md3_tags_bounds_and_order(self):
  v={'position':[1,2,3],'normal':[0,0,1],'uv':[0,0]};v2={**v,'position':[2,4,6]}
  surfaces=[{'vertices':[v]*3,'triangles':[[0,1,2]],'shader':'qce/test'}]
  frames=[[[v]*3],[[v2]*3]];tags=[{'tag_flash':(a.IDENTITY,(1,2,3))},{'tag_flash':(a.IDENTITY,(4,5,6))}]
  data=a.animated_md3(surfaces,frames,tags);header=struct.unpack_from('<9i',data,72)
  self.assertEqual(header[1:4],(2,1,1));self.assertEqual(header[-1],len(data))
  self.assertEqual(struct.unpack_from('<3f',data,header[6]+112+64),(4,5,6))
  surface=header[7];s=struct.unpack_from('<10i',data,surface+68);self.assertEqual(s[1],2)
  self.assertEqual(struct.unpack_from('<3h',data,surface+s[-2]+24),(128,256,384))
 def test_tangents_orthogonal_mirrored_and_degenerate_uv(self):
  vertices=[{'position':p,'normal':[0,0,1],'uv':uv} for p,uv in [([0,0,0],[0,0]),([1,0,0],[1,0]),([0,1,0],[0,1])]]
  surface={'vertices':vertices,'triangles':[[0,1,2]]}
  self.assertEqual(a.mesh_tangents(surface),[[1,0,0,1]]*3)
  vertices[1]['uv']=[-1,0];self.assertEqual(a.mesh_tangents(surface),[[-1,0,0,-1]]*3)
  for v in vertices:v['uv']=[0,0]
  for t in a.mesh_tangents(surface):
   self.assertAlmostEqual(sum(x*x for x in t[:3]),1);self.assertEqual(t[2],0)
 def test_iqm_skeleton_channels_weights_and_attachment(self):
  r=a.qmatrix((0,0,math.sqrt(.5),math.sqrt(.5)))
  c=a.transform_channels((r,(1,2,3)))
  rebuilt=a.qmatrix((-c[3],-c[4],-c[5],c[6]))
  for i in range(3):
   for j in range(3):self.assertAlmostEqual(rebuilt[i][j],r[i][j])
  v={'position':[1,0,0],'normal':[0,0,1],'uv':[0,0],'nodes':[0,0],'node0_weight':1.}
  surfaces=[{'vertices':[v]*3,'triangles':[[0,1,2]],'shader':'qce/test'}]
  bind=[(a.IDENTITY,(0,0,0))];local=[bind,[(r,(2,0,0))]]
  marker={'name':'tag_flash','node':0,'translation':[1,0,0],'rotation':[0,0,0,1]}
  data=a.animated_iqm(surfaces,[{'name':'root','parent':-1}],bind,local,[(bind,[0])],[marker],[0],1,[{'name':'idle','first':0,'count':2,'fps':30}],[[[v]*3]]*2)
  h=struct.unpack_from('<27I',data,16);self.assertEqual(h[0],2);self.assertEqual(h[1],len(data));self.assertEqual(h[13],3);self.assertEqual(h[19],2)
  joint=struct.unpack_from('<Ii10f',data,h[14]+96);self.assertEqual(joint[1],0);self.assertEqual(joint[2:5],(1,0,0))
  text=data[h[4]:h[4]+h[3]];self.assertEqual(text[joint[0]:].split(b'\0')[0],b'tag_flash')
  arrays=[struct.unpack_from('<5I',data,h[9]+20*i) for i in range(h[7])]
  tangent=next(x for x in arrays if x[0]==3);self.assertEqual(tangent[2:4],(7,4))
  weight=next(x for x in arrays if x[0]==5);self.assertEqual(data[weight[4]:weight[4]+4],bytes((255,0,0,0)))
  self.assertEqual(len(data[h[21]:h[21]+h[19]*h[20]*2]),h[19]*h[20]*2)
 def test_material_channel_semantics_and_energy_split(self):
  base=bytes((200,100,50,255));multi=bytes((10,128,220,99))
  d,e,s=a.material_layers(base,multi);self.assertEqual(s[:3],bytes((10,)*3));self.assertEqual(e[:3],bytes((100,50,25)))
  self.assertTrue(all(abs(d[i]+e[i]-base[i])<=1 for i in range(3)))
  self.assertEqual(a.material_layers(base,multi,False)[2][:3],bytes((220,)*3))
 def test_mask_resampling_preserves_normalized_uv(self):
  pixels=bytes((0,0,0,255,200,100,50,255))
  self.assertEqual(a.resample_rgba(pixels,2,1,1,1),bytes((100,50,25,255)))

 def test_runtime_loop_keeps_valid_indices_and_bounds_retail_links(self):
  self.assertEqual(a.runtime_loop({'loop_frame':3,'count':10}),3)
  for index in (-1,10,34):self.assertEqual(a.runtime_loop({'loop_frame':index,'count':10}),0)
 def test_sound_event_aliases_source_frames_and_bounds(self):
  with tempfile.TemporaryDirectory() as tmp:
   data=bytearray(1024);struct.pack_into('<II',data,84,1,120);struct.pack_into('<I',data,132,42)
   struct.pack_into('<f',data,240,0.5);struct.pack_into('<II',data,352,1,400)
   struct.pack_into('<f',data,448,1.0);struct.pack_into('<II',data,460,1,500);struct.pack_into('<f',data,536,1.0)
   tag={'id':42,'path':'sound\\reload','class':'snd!','offset':200}
   cache=SimpleNamespace(data=data,tags={42:tag},pointer=lambda p,n:p)
   assets=a.a.Assets(cache,Path(tmp));assets.write('sound/raw.wav',a.a.wav(struct.pack('<3h',1000,-1000,0),1,22050),{})
   clip={'sound_index':0,'sound_frame':6,'count':30}
   clips=[None]*len(a.ACTIONS);clips[4]=clip
   records=[{'id':42,'range':0,'permutation':0,'outputs':['sound/raw.wav']}]
   events=a.animation_sounds(cache,{'offset':0},clips,assets,records,'machinegun')
   self.assertEqual(events[4]['frame'],6);self.assertEqual(events[4]['source'],tag)
   with a.a.wave.open(str(Path(tmp)/events[4]['outputs'][0]),'rb') as stream:
    self.assertEqual(struct.unpack('<3h',stream.readframes(3)),(500,-500,0))
   raw=(Path(tmp)/'sound/raw.wav').read_bytes()
   first,_=assets.calibrated_sound(records[0]);second,_=assets.calibrated_sound(records[0])
   self.assertEqual(first,second);self.assertEqual(raw,(Path(tmp)/'sound/raw.wav').read_bytes())
   self.assertTrue((Path(tmp)/'models/qce/halo/view/machinegun.events').read_text().startswith('2 '+str(len(a.ACTIONS))))
   clip['sound_frame']=30
   with self.assertRaises(a.a.halo.CacheError):a.animation_sounds(cache,{'offset':0},clips,assets,records,'machinegun')
 def test_ammo_skins_keep_other_materials_and_select_tens_units(self):
  with tempfile.TemporaryDirectory() as tmp:
   tag={'id':42,'path':'weapons\\assault rifle\\fp\\bitmaps\\numbers_plate'}
   assets=a.a.Assets(SimpleNamespace(index=[tag]),Path(tmp))
   for digit in range(10):assets.write('textures/qce/halo/'+a.a.safe_name(tag['path'])+f'/{digit:03}.tga',b'fixture',{})
   parts=[{'shader':'weapon\\numbers','geometry':0,'part':i,'vertices':[{'position':[0,y,0]}]} for i,y in enumerate((1,-1))]
   meta={'id':123,'parts':parts}
   surfaces=[{'shader':f'qce/halo/0000007b/0_{i}'} for i in range(2)]+[{'shader':'qce/hands'}]
   report=a.ammunition_skins(assets,surfaces,meta,'machinegun')
   self.assertEqual(report['skins'],61)
   for ammo in (0,9,10,59,60):
    lines=(Path(tmp)/f'models/qce/halo/view/machinegun_{ammo}.skin').read_text().splitlines()
    self.assertEqual(lines,[f'part0,qce/halo/digits/{ammo//10}',f'part1,qce/halo/digits/{ammo%10}','part2,qce/hands'])
   self.assertIsNone(a.ammunition_skins(assets,surfaces,meta,'shotgun'))

if __name__=='__main__':unittest.main()
