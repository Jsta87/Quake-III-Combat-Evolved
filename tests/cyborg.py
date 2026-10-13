#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('cyborg',ROOT/'scripts/convert-halo-cyborg.py');c=importlib.util.module_from_spec(s);s.loader.exec_module(c)
class ImportTests(unittest.TestCase):
 def test_e3_and_mac_shader_indices_are_not_assumed(self):
  self.assertEqual(c.shader_kind(r'digsite\characters\cyborg\99_e3\shaders\e3_99_armor'),'armor')
  self.assertEqual(c.shader_kind(r'digsite\characters\cyborg\99_e3\shaders\e3_99_visor'),'visor')
  self.assertEqual(c.shader_kind(r'digsite\characters\cyborg\00_mac\shaders\head visor'),'visor')
  self.assertIsNone(c.shader_kind('shaders/emblem'))
  with self.assertRaises(ValueError):c.shader_kind('shaders/unexpected')
 def test_iqm_rotation_and_attachment_roundtrip(self):
  p=c.p;bones=[{'name':'root','parent':-1},{'name':'bip01 r hand','parent':0}]
  rotated=((0,-1,0),(1,0,0),(0,0,1));frames=[[(p.IDENTITY,(1,2,3)),(p.IDENTITY,(4,0,0))],[(rotated,(3,2,1)),(p.IDENTITY,(4,0,0))]]
  bind=p.globals_for(bones,frames[0]);vertices=[{'position':v,'normal':(0,0,1),'uv':(0,0),'nodes':[0,-1],'node0_weight':1} for v in [(0,0,0),(1,0,0),(0,1,0)]]
  surface={'vertices':vertices,'triangles':[(0,2,1)],'shader':'test'};marker={'name':'tag_hand','node':1,'rotation':[0,0,.70710678,.70710678],'translation':[.1,.2,.3]}
  clips=[{'name':'test','first':0,'count':2,'fps':30}]
  blob=p.animated_iqm([surface],bones,bind,frames,[(bind,[0,1])],[marker],[0,1],80,clips,[[vertices],[vertices]])
  with tempfile.TemporaryDirectory() as directory:
   path=Path(directory)/'test.iqm';path.write_bytes(blob);decoded,_,hand=c.retail_animation(path,['root','bip01 r hand'])
  for row,expected in zip(decoded[1][0][0],rotated):
   for value,want in zip(row,expected):self.assertAlmostEqual(value,want,places=4)
  for a,b in zip(hand['translation'],marker['translation']):self.assertAlmostEqual(a,b,places=5)
  for a,b in zip(hand['rotation'],marker['rotation']):self.assertAlmostEqual(a,b,places=5)
 def test_outward_mesh_winding_and_bounds(self):
  vertices=[{'position':v,'normal':(0,0,1)} for v in [(0,0,0),(1,0,0),(0,1,0)]]
  self.assertEqual(c.strip_triangles([0,1,2],vertices),[[0,2,1]])
  self.assertEqual(c.strip_triangles([0,0,1],vertices),[])
  with self.assertRaises(ValueError):c.strip_triangles([0,1,9],vertices)
if __name__=='__main__':unittest.main()
