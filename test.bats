#!/usr/bin/env bats

@test "setup  context" {
  
  rm -rf testdata
}

@test "split test data" {

  mkdir -p testdata

  ./build/osmsplit -i old_street.osm -o testdata -s 1 -l 4

  [ `ls -al testdata/old_street*.osm.pbf | wc -l` -eq 16 ]
}

@test "split into S2 cells" {

  ./build/osms2split -i old_street.osm -o testdata -l 10

  [ -f testdata/s2_4876030000000000.osm.pbf ]
  [ -f testdata/s2_4876050000000000.osm.pbf ]
  [ -f testdata/s2_48761b0000000000.osm.pbf ]
  [ -f testdata/s2_48761d0000000000.osm.pbf ]
}

@test "convert to geom" {

  ./build/osm2assimp -i testdata/old_street0000.osm.pbf -o testdata/old_street0000.osm.obj

  [ -f testdata/old_street0000.osm.obj ]
  [ `grep "Mesh" testdata/old_street0000.osm.obj | wc -l` -eq 48 ]
}

@test "s2util" {

  [ `./build/s2util s2_4876030000000000.osm.pbf --c` = "51.473,-0.0468724" ]
}
