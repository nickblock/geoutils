#!/usr/bin/env bats

@test "download test data" {

  mkdir -p testdata
  cd testdata
  
  if [ !-f greater-london-latest.osm.pbf ]; then 
    wget http://download.geofabrik.de/europe/great-britain/england/greater-london-latest.osm.pbf
  fi

  [ -f greater-london-latest.osm.pbf ]
}

@test "split test data" {

  cd testdata

  ../build/osmsplit -i greater-london-latest.osm.pbf -o . -s 1000 -l 4

  [ -f greater-london-latest0000.osm.pbf ]
  [ -f greater-london-latest0001.osm.pbf ]
  [ -f greater-london-latest0010.osm.pbf ]
  [ -f greater-london-latest0011.osm.pbf ]
  [ -f greater-london-latest0100.osm.pbf ]
  [ -f greater-london-latest0101.osm.pbf ]
  [ -f greater-london-latest0110.osm.pbf ]
  [ -f greater-london-latest0111.osm.pbf ]
  [ -f greater-london-latest1000.osm.pbf ]
  [ -f greater-london-latest1001.osm.pbf ]
  [ -f greater-london-latest1010.osm.pbf ]
  [ -f greater-london-latest1011.osm.pbf ]
  [ -f greater-london-latest1100.osm.pbf ]
  [ -f greater-london-latest1101.osm.pbf ]
  [ -f greater-london-latest1110.osm.pbf ]
  [ -f greater-london-latest1111.osm.pbf ]
}

@test "convert to geom" {

  cd testdata 

  ../build/osm2assimp -i greater-london-latest0000.osm.pbf -o greater-london-latest0000.osm.obj

  [ -f greater-london-latest0000.osm.obj ]
}