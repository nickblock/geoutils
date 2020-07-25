#!/usr/bin/env bats

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

load assimp_info.sh

if [ ! -z "$TEST_DATA_DIR" ]; 
then
  echo "You must export TEST_DATA_DIR variable point to a folder to store generated test data, and make sure the folder exists"
fi

@test "setup  context" {

  # a large area around old street london
  $BATS_TEST_DIRNAME/create_test_osm_file.py $TEST_DATA_DIR/test.osm --extents " 51.514853,-0.104486,51.531354,-0.065948" 

  [ -f $TEST_DATA_DIR/test.osm ]
}

@test "split test data" {

  mkdir -p $TEST_DATA_DIR

  osmsplit -i $TEST_DATA_DIR/test.osm -o $TEST_DATA_DIR -s 1 -l 4

  [ `ls -al $TEST_DATA_DIR/test*.osm.pbf | wc -l` -eq 16 ]
}

@test "split into S2 cells" {

  osms2split -i $TEST_DATA_DIR/test.osm -o $TEST_DATA_DIR -l 10

  [ -f $TEST_DATA_DIR/s2_4876030000000000.osm.pbf ]
  [ -f $TEST_DATA_DIR/s2_4876050000000000.osm.pbf ]
  [ -f $TEST_DATA_DIR/s2_48761b0000000000.osm.pbf ]
  [ -f $TEST_DATA_DIR/s2_48761d0000000000.osm.pbf ]
}

@test "convert to geom" {

  osm2assimp -i $TEST_DATA_DIR/test0000.osm.pbf -o $TEST_DATA_DIR/test0000.osm.obj -r 

  [ -f $TEST_DATA_DIR/test0000.osm.obj ]
  [ `grep "Mesh" $TEST_DATA_DIR/test0000.osm.obj | wc -l` -eq 2 ]
}

@test "convert with extents" {

  #these coordinates should produce an obj file 1km in width and depth, using the mercator projection
  osm2assimp -i $TEST_DATA_DIR/test.osm -o $TEST_DATA_DIR/extents.obj -e -0.085415,51.522852,-0.076432,51.528441 -z

  dimensions=$(get_dimensions "$TEST_DATA_DIR/extents.obj")


  [ -f $TEST_DATA_DIR/extents.obj ]
  [ $(compare_vec3 $dimensions 1000.0 1000.0 100.0 20.0) == "0" ]
}

@test "export s2 cell" {

  osm2assimp -i $TEST_DATA_DIR/test.osm -o $TEST_DATA_DIR/48761cafc0000000.obj -s 48761cafc0000000

  dimensions=$(get_dimensions "$TEST_DATA_DIR/48761cafc0000000.obj" )

  [ $(compare_vec3 $dimensions 320.0 30.000000 480.0 40.0) == "0" ]

}

@test "s2util" {

  [ `s2util s2_4876030000000000.osm.pbf --c` = "51.473,-0.0468724" ]
}
