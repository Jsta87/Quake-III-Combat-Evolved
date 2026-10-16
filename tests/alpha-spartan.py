#!/usr/bin/env python3
"""Regression checks for bind-space skin transfer and animated hand movement."""
import importlib.util,random,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('alpha',Path(__file__).resolve().parents[1]/'scripts/convert-alpha-spartan.py');a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
class TransferTests(unittest.TestCase):
 def test_reference_variant_and_texture_origin(self):
  self.assertEqual(a.DEFAULT_VARIANT,'99_armor')
  self.assertEqual(a.gltf_uv((.25,.1)),(.25,.1))
  self.assertEqual(a.gltf_uv((.8,.9)),(.8,.9))
 def test_nearest_matches_exhaustive_search(self):
  rng=random.Random(1234);points=[(tuple(rng.uniform(-20,20) for _ in range(3)),i) for i in range(100)];tree=a.tree(points[:])
  for _ in range(30):
   point=tuple(rng.uniform(-30,30) for _ in range(3));distance,index=a.nearest(tree,point)
   expected=min(points,key=lambda x:sum((x[0][i]-point[i])**2 for i in range(3)))
   self.assertEqual(index,expected[1]);self.assertGreaterEqual(distance,0)
 def test_transfer_preserves_arms_down_bind_pose(self):
  # A hand beside the hip must keep its own hand influence and bind position;
  # matching a rifle pose or unposing it here strands the hand at the hip.
  hand={'nodes':[1,-1],'node0_weight':1}
  vertex=a.bind_vertex((0,10,30),(1,0,0),(0,0),hand)
  self.assertEqual(vertex['position'],(0,10,30));self.assertEqual(vertex['normal'],(1,0,0))
  identity=a.p.IDENTITY;bind=[(identity,(0,0,0)),(identity,(0,10,30))]
  raised=[(identity,(0,0,0)),(identity,(20,5,65))]
  posed=a.p.fast_skin(vertex,a.p.skin_matrices(bind,raised))
  self.assertEqual(tuple(posed['position']),(20,5,65))
  self.assertEqual(vertex['position'],(0,10,30))
 def test_neighbor_blend_softens_reference_boundaries(self):
  vertices=[((-1,0,0),{'nodes':[0,-1],'node0_weight':1}),((1,0,0),{'nodes':[1,-1],'node0_weight':1})]
  weights=a.nearest_weights(a.tree(vertices),(0,0,0))
  self.assertEqual(set(weights['nodes']),{0,1});self.assertAlmostEqual(weights['node0_weight'],.5)
 def test_thigh_cannot_inherit_adjacent_hand(self):
  bones=[{'name':'bip01 l thigh'},{'name':'bip01 l hand'}]
  lookup=a.tree([((0,7,25),{'nodes':[1,-1],'node0_weight':1}),((0,6,25),{'nodes':[0,-1],'node0_weight':1})])
  skin=a.nearest_weights(lookup,(0,6.8,25),bones=bones)
  self.assertEqual(skin['nodes'],[0,-1]);self.assertEqual(skin['node0_weight'],1)
 def test_backpack_near_forearm_stays_on_torso(self):
  bones=[{'name':'bip01 spine1'},{'name':'bip01 r forearm'}]
  positions=[(-6,-6,36),(-6,-7,36),(-6,-6,39),(0,0,0),(1,0,0),(0,1,0),(0,0,1)]
  vertices=[{'position':v,'nodes':[1,-1],'node0_weight':1} for v in positions]
  a.rigid_accessories(vertices,[(0,1,2),(3,4,5),(4,5,6)],bones)
  self.assertTrue(all(vertices[i]['nodes'][0]==0 for i in range(3)))
 def test_shared_visors_are_filtered_to_selected_model(self):
  positions=[(0,y,z) for y,z in [(5,50),(5.5,50),(5,51),(10,50),(10.5,50),(10,51),(17,50),(17.5,50),(17,51)]]
  self.assertEqual(a.visor_indices(positions,list(range(9)),8.75,11.48),[3,4,5])
 def test_uv_seams_share_component_assignment(self):
  points=[(0,0,0),(1,0,0),(0,1,0),(1,0,0),(0,1,0),(1,1,0)]
  self.assertEqual(len(a.mesh_components(points,[(0,1,2),(3,4,5)])),1)
 def test_lookup_uses_mesh_bind_positions(self):
  hip={'position':(0,0,30),'nodes':[0,-1],'node0_weight':1}
  hand={'position':(0,10,30),'nodes':[1,-1],'node0_weight':1}
  lookup=a.tree(a.bind_reference([{'vertices':[hip,hand]}]))
  # In rifle idle the hand moves to (20,5,65); the uploaded hand remains down.
  _,match=a.nearest(lookup,(0,10,30))
  self.assertEqual(match['nodes'][0],1)
if __name__=='__main__':unittest.main()
