from pyassimp import load
import subprocess
import os
import create_test_osm_file
import tempfile
import re
import shutil
import unittest


def runProcess(args):

  out = subprocess.run(args, capture_output=True)

  if out.returncode != 0:
    print(f"Failed to run '{" ".join(args)}'")
  
    print(out.stdout)

    return False
  
  return True

class GeoUtilsProcesses(unittest.TestCase):

  def setUp(self):

    self.test_data_dir = os.path.join(tempfile.gettempdir(), "geoutils_test")
    if(os.path.exists(self.test_data_dir)):
      shutil.rmtree(self.test_data_dir)
      os.mkdir(self.test_data_dir)
    self.test_file = os.path.join(self.test_data_dir, "test.osm")

    print(f"Writing test file {self.test_file}")

    create_test_osm_file.execute([51.514853,-0.104486,51.531354,-0.065948], 0.0002, 10.0, self.test_file)


    if not os.path.exists("osm2assimp"):
      print("Failed to find osm2assimp, have you built the project and added the 'build' directory to your path?")
      exit(1)

  def test_OsmSplit(self):

    #test split data
    result = runProcess(["osmsplit", "-i", self.test_file, "-o", self.test_data_dir, "-s", "1", "-l", "4"])

    self.assertTrue(result)

    regex = re.compile('test[0-1]{4}.osm.pbf')

    test_output_files = [f for f in os.listdir(self.test_data_dir) if re.match(regex, f)]

    self.assertEqual(len(test_output_files), 16)

  def test_SplitS2Cells(self):

    result = runProcess(["osms2split", "-i", self.test_file, "-o", self.test_data_dir, "-l", "10"])

    self.assertTrue(result)

    self.assertTrue(os.path.exists(os.path.join(self.test_data_dir, "s2_4876030000000000.osm.pbf")))
    self.assertTrue(os.path.exists(os.path.join(self.test_data_dir, "s2_4876050000000000.osm.pbf")))
    self.assertTrue(os.path.exists(os.path.join(self.test_data_dir, "s2_48761b0000000000.osm.pbf")))
    self.assertTrue(os.path.exists(os.path.join(self.test_data_dir, "s2_48761d0000000000.osm.pbf")))


if __name__ == "__main__":
    
    unittest.main()