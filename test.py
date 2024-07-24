import pyassimp
import subprocess
import os
import create_test_osm_file
import tempfile
import re
import shutil
import unittest
import logging

logger = logging.getLogger(__name__)

def runProcess(args):

  logger.info(f"Running: {" ".join(args)}")

  out = subprocess.run(args, capture_output=True)

  if out.returncode != 0:
    print(f"Failed to run '{" ".join(args)}'")
  
    print(out.stdout)

    return False
  
  return True

class GeoUtilsProcesses(unittest.TestCase):

  @staticmethod
  def getTestDir():
    return os.path.join(tempfile.gettempdir(), "geoutils_test")
  
  @staticmethod
  def getTestFile():
    return os.path.join(GeoUtilsProcesses.getTestDir(), "test.osm")
  
  @staticmethod
  def getTestCoords():
    return [-0.085415,51.522852,-0.076432,51.528441]

  @classmethod
  def setUpClass(self):

    
    if(os.path.exists(self.getTestDir())):
      shutil.rmtree(self.getTestDir())
      os.mkdir(self.getTestDir())
    
    logging.basicConfig(filename=os.path.join(GeoUtilsProcesses.getTestDir(), "log.txt"), level=logging.INFO)

    [self.numBuildings, self.numHighways ] = create_test_osm_file.execute(self.getTestCoords(), 0.0002, 10.0, self.getTestFile())

    logger.info(f"buildings {self.numBuildings} highways {self.numHighways}")


    logger.info(f"Writing test file {self.getTestFile()}")

    if not os.path.exists("osm2assimp"):
      logger.error("Failed to find osm2assimp, have you built the project and added the 'build' directory to your path?")
      exit(1)

  def test_OsmSplit(self):

    #test split data
    result = runProcess(["osmsplit", "-i", self.getTestFile(), "-o", GeoUtilsProcesses.getTestDir(), "-s", "1", "-l", "4"])

    self.assertTrue(result)

    regex = re.compile('test[0-1]{4}.osm.pbf')

    test_output_files = [f for f in os.listdir(GeoUtilsProcesses.getTestDir()) if re.match(regex, f)]

    self.assertEqual(len(test_output_files), 16)

  def test_SplitS2Cells(self):

    result = runProcess(["osms2split", "-i", self.getTestFile(), "-o", GeoUtilsProcesses.getTestDir(), "-l", "12"])

    self.assertTrue(result)

    self.assertTrue(os.path.exists(os.path.join(GeoUtilsProcesses.getTestDir(), "s2_48761cb000000000.osm.pbf")))
    self.assertTrue(os.path.exists(os.path.join(GeoUtilsProcesses.getTestDir(), "s2_48761cd000000000.osm.pbf")))

  def test_Osm2Assimp(self):

    outputFile = os.path.join(GeoUtilsProcesses.getTestDir(), "extents.fbx")

    result = runProcess([
      "osm2assimp", "-i", self.getTestFile(), "-o", outputFile, "-z", "-u", "3.5", "-c", "2", "-r"
    ])

    self.assertTrue(result)

    self.assertTrue(os.path.exists(outputFile))

    with pyassimp.load(outputFile ,file_type=None, processing=pyassimp.postprocess.aiProcess_Triangulate) as scene:
      logger.info(" num meshes " + str(scene.mNumMeshes))
      
      self.assertEqual(scene.mNumMeshes, self.numBuildings + self.numHighways)

if __name__ == "__main__":
    
    unittest.main()