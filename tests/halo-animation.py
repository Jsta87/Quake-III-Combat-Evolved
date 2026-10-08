#!/usr/bin/env python3
"""Animation/skinning/material fixtures; no copyrighted map data required."""
import importlib.util
import math
from pathlib import Path
import struct
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

if __name__=='__main__':unittest.main()
