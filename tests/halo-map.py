#!/usr/bin/env python3
"""BSP topology and bounded cache tests without proprietary input data."""
import importlib.util,struct,unittest
from pathlib import Path
s=importlib.util.spec_from_file_location('world',Path(__file__).resolve().parents[1]/'scripts/convert-halo-map.py');w=importlib.util.module_from_spec(s);s.loader.exec_module(w)
class MapTests(unittest.TestCase):
 def triangle(self):
  b=w.BSP();b.mesh('qce/test', [((0,0,0),(0,0),(0,0,1)),((100,0,0),(1,0),(0,0,1)),((0,100,0),(0,1),(0,0,1))],[0,1,2],True);return b
 def test_collision_planes_and_order(self):
  b=self.triangle();first,count,shader=struct.unpack('<3i',b.l[8]);planes=[]
  for i in range(first,first+count):
   ix,_=struct.unpack_from('<2i',b.l[9],i*8);planes.append(struct.unpack_from('<4f',b.l[2],ix*16))
  self.assertEqual([p[:3] for p in planes[:6]],[(-1,0,0),(1,0,0),(0,-1,0),(0,1,0),(0,0,-1),(0,0,1)])
  inside=lambda p:all(w.dot(n[:3],p)<=n[3]+1e-5 for n in planes)
  self.assertTrue(inside((20,20,-1)));self.assertFalse(inside((20,20,1)));self.assertFalse(inside((20,20,-3)));self.assertFalse(inside((80,80,-1)))
 def test_plane_pairs_and_dedup(self):
  b=self.triangle()
  for i in range(0,len(b.l[2]),32):
   p=struct.unpack_from('<4f',b.l[2],i);q=struct.unpack_from('<4f',b.l[2],i+16);self.assertEqual(p,tuple(-v for v in q))
  n=len(b.l[2]);self.assertEqual(b.plane((1,0,0),0),0);self.assertEqual(len(b.l[2]),n)
 def test_bsp_lumps(self):
  b=self.triangle();out=b.finish([{'classname':'worldspawn','gridsize':'512 512 512'}]);self.assertEqual(out[:8],b'IBSP'+struct.pack('<i',46))
  for i in range(17):
   start,length=struct.unpack_from('<2i',out,8+i*8);self.assertGreaterEqual(start,144);self.assertLessEqual(start+length,len(out));self.assertEqual(start%4,0)
  self.assertEqual(len(b.l[13]),104);self.assertEqual(len(b.l[10]),3*44)
 def test_degenerate_collision_ignored(self):
  b=w.BSP();b.brush([(0,0,0)]*3,b.shader('test'));self.assertEqual(b.brushes,0)
 def test_winding_follows_normals(self):
  b=w.BSP();b.mesh('test',[((0,0,0),(0,0),(0,0,1)),((100,0,0),(0,0),(0,0,1)),((0,100,0),(0,0),(0,0,1))],[0,2,1],True)
  self.assertEqual(struct.unpack('<3i',b.l[11]),(0,2,1));self.assertEqual(b.brushes,1)
 def test_surface_splitting(self):
  v=list(range(1200));parts=w.a.split_surface(v,[list(range(i,i+3)) for i in range(0,1200,3)],'test')
  self.assertEqual(len(parts),2);self.assertTrue(all(len(p['vertices'])<1000 and len(p['triangles'])*3<6000 for p in parts))
 def test_bsp_pointer_stays_in_cache(self):
  class Cache:
   def check(self,o,n):
    if o<0 or o+n>100:raise w.a.halo.CacheError('outside')
  obj=w.World.__new__(w.World);obj.c=Cache();obj.start=20;obj.size=40;obj.magic=1000
  self.assertEqual(obj.ptr(1005,10),25)
  for p,n in ((999,1),(1040,1),(1000,41)):
   with self.assertRaises(w.a.halo.CacheError):obj.ptr(p,n)
if __name__=='__main__':unittest.main()
