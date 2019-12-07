#!/usr/bin/env bats

@test "setup  context" {
  
  rm -rf testdata
  mkdir testdata

  # a large area around old street london
  ./create_test_osm_file.py testdata/test.osm --extents " 51.514853,-0.104486,51.531354,-0.065948" 

  [ -f testdata/test.osm ]
}

@test "split test data" {

  mkdir -p testdata

  ./build/osmsplit -i testdata/test.osm -o testdata -s 1 -l 4

  [ `ls -al testdata/test*.osm.pbf | wc -l` -eq 16 ]
}

@test "split into S2 cells" {

  ./build/osms2split -i testdata/test.osm -o testdata -l 10

  [ -f testdata/s2_4876030000000000.osm.pbf ]
  [ -f testdata/s2_4876050000000000.osm.pbf ]
  [ -f testdata/s2_48761b0000000000.osm.pbf ]
  [ -f testdata/s2_48761d0000000000.osm.pbf ]
}

@test "convert to geom" {

  ./build/osm2assimp -i testdata/test0000.osm.pbf -o testdata/test0000.osm.obj

  [ -f testdata/test0000.osm.obj ]
  [ `grep "Mesh" testdata/test0000.osm.obj | wc -l` -eq 1008 ]
}

@test "s2util" {

  [ `./build/s2util s2_4876030000000000.osm.pbf --c` = "51.473,-0.0468724" ]
}
