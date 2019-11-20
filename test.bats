#!/usr/bin/env bats

@test "split test data" {

  mkdir -p testdata

  ./build/osmsplit -i old_street.osm -o testdata -s 1000 -l 4

  [ `ls -al testdata/old_street*.osm.pbf | wc -l` -eq 16 ]
}

@test "convert to geom" {

  ./build/osm2assimp -i testdata/old_street0100.osm.pbf -o testdata/old_street0100.osm.obj

  [ -f testdata/old_street0100.osm.pbf ]
}