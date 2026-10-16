#!/usr/bin/env python3
"""Automatic skin transfer math must survive nontrivial blended transforms."""
import importlib.util,random,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('alpha',Path(__file__).resolve().parents[1]/'scripts/convert-alpha-spartan.py');a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
class TransferTests(unittest.TestCase):
 def test_nearest_matches_exhaustive_search(self):
  rng=random.Random(1234);points=[(tuple(rng.uniform(-20,20) for _ in range(3)),i) for i in range(100)];tree=a.tree(points[:])
  for _ in range(30):
   point=tuple(rng.uniform(-30,30) for _ in range(3));distance,index=a.nearest(tree,point)
   expected=min(points,key=lambda x:sum((x[0][i]-point[i])**2 for i in range(3)))
   self.assertEqual(index,expected[1]);self.assertGreaterEqual(distance,0)
 def test_inverse_skin_transform(self):
  matrix=((.8,-.3,.1),(.3,.8,0),(.1,0,.9));point=(11,-7,3)
  unposed=a.p.rotate(a.inverse(matrix),a.p.rotate(matrix,point))
  for got,want in zip(unposed,point):self.assertAlmostEqual(got,want,places=6)
 def test_degenerate_transform_fails(self):
  with self.assertRaises(ValueError):a.inverse(((0,0,0),(0,0,0),(0,0,0)))
if __name__=='__main__':unittest.main()
