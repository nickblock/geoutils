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

  ./build/osm2assimp -i testdata/test0000.osm.pbf -o testdata/test0000.osm.obj -r

  [ -f testdata/test0000.osm.obj ]
  [ `grep "Mesh" testdata/test0000.osm.obj | wc -l` -gt 1000 ]
}

@test "convert with extents" {

  #these coordinates should produce an obj file 1km in width and depth, using the mercator projection
  ./build/osm2assimp -i testdata/test.osm -o ./testdata/extents.obj -e -0.085415,51.522852,-0.076432,51.528441 -z

  [ -f ./testdata/extents.obj ]
  [ "`./build/ext/assimp/tools/assimp_cmd/assimpd info ./testdata/extents.obj | grep -i 'Maximum point'`" = "Maximum point      (1005.099976 984.254028 30.000000)" ]
}

@test "export s2 cell" {

  ./build/osm2assimp -i testdata/test.osm -o testdata/48761cafc0000000.dae -s 48761cafc0000000 

  [ "`./build/ext/assimp/tools/assimp_cmd/assimpd info ./testdata/48761cafc0000000.dae | grep -i 'Maximum point'`" = "Maximum point      (156.870972 30.000000 246.121613)" ]
  [ "`./build/ext/assimp/tools/assimp_cmd/assimpd info ./testdata/48761cafc0000000.dae | grep -i 'Minimum point'`" = "Minimum point      (-165.954987 0.000000 -236.954605)" ]

}

@test "s2util" {

  [ `./build/s2util s2_4876030000000000.osm.pbf --c` = "51.473,-0.0468724" ]
}
