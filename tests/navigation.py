#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import struct
import unittest
ROOT=Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('nav',ROOT/'scripts/build-bloodgulch-bots.py');n=importlib.util.module_from_spec(s);s.loader.exec_module(n)
class NavigationTests(unittest.TestCase):
 def test_md4_checksum(self):
  # RFC 1320 digest vectors, XORed as ioquake3's Com_BlockChecksum does.
  for data,digest in [(b'', '31d6cfe0d16ae931b73c59d7e0c089c0'),(b'a','bde52cb31de33e46245e05fbdbd6fb24'),(b'abc','a448017aaf21d8525fc10ae87aa6729d'),(b'1234567890'*8,'e33b4ddc9c38f2199c3e7b164fcc0536')]:
   words=struct.unpack('<4I',bytes.fromhex(digest));self.assertEqual(n.bsp_checksum(data),words[0]^words[1]^words[2]^words[3])
 def file(self):
  data=bytearray(124+96+44);struct.pack_into('<3I',data,0,0x53414145,5,123)
  struct.pack_into('<2i',data,12+7*8,124,96);struct.pack_into('<2i',data,12+9*8,220,44)
  for i in range(8,124):data[i]^=(i-8)*119 & 255
  return data
 def test_matching_map_and_bounds(self):
  self.assertEqual(n.validate_aas(self.file(),123),1)
  with self.assertRaises(ValueError):n.validate_aas(self.file(),456)
  with self.assertRaises(ValueError):n.validate_aas(self.file()[:-1],123)
if __name__=='__main__':unittest.main()
