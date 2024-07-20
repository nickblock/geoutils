from pyassimp import load
import subprocess
import os
import create_test_osm_file
import tempfile
import re
import shutil

def run(args):

  out = subprocess.run(args, capture_output=True)

  if out.returncode == 0:
    print(f"Failed to run {args[0]}")
  
    print(out.stdout)

    return False
  
  return True

def setup():

  test_data_dir = os.path.join(tempfile.gettempdir(), "geoutils_test")
  if(os.path.exists(test_data_dir)):
    shutil.rmtree(test_data_dir)
    os.mkdir(test_data_dir)
  test_file = os.path.join(test_data_dir, "test.osm")

  print(f"Writing test file {test_file}")

  create_test_osm_file.execute([51.514853,-0.104486,51.531354,-0.065948], 0.0002, 10.0, test_file)

  return [test_data_dir, test_file]


if not os.path.exists("osm2assimp"):
  print("Failed to find osm2assimp, have you built the project and added the 'build' directory to your path?")
  exit(1)


[test_data_dir, test_file] = setup()

#test split data
result = run(["osmsplit", "-i", test_file, "-o", test_data_dir, "-s", "1", "-l", "4"])

assert result

regex = re.compile('test[0-1]{4}.osm.pbf')

test_output_files = [f for f in os.listdir(test_data_dir) if re.match(regex, f)]

assert len(test_output_files) == 16



