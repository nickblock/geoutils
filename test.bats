#!/usr/bin/env bats

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

load assimp_info.sh

setup() {

  if [ -z "$TEST_DATA_DIR" ]; 
  then
    export TEST_DATA_DIR=/tmp
  fi
}

@test "setup  context" {

  # a large area around old street london
  $BATS_TEST_DIRNAME/create_test_osm_file.py $TEST_DATA_DIR/test.osm --extents " 51.514853,-0.104486,51.531354,-0.065948" 

  [ -f $TEST_DATA_DIR/test.osm ]
}

@test "split test data" {

  mkdir -p $TEST_DATA_DIR

  cmd="osmsplit -i $TEST_DATA_DIR/test.osm -o $TEST_DATA_DIR -s 1 -l 4"
  echo $cmd
  run $cmd
  
  [ "$status" -eq 0 ]
  [ `ls -al $TEST_DATA_DIR/test*.osm.pbf | wc -l` -eq 16 ]
}

@test "split into S2 cells" {

  cmd="osms2split -i $TEST_DATA_DIR/test.osm -o $TEST_DATA_DIR -l 10"
  echo $cmd
  run $cmd

  [ "$status" -eq 0 ]
  [ -f $TEST_DATA_DIR/s2_4876030000000000.osm.pbf ]
  [ -f $TEST_DATA_DIR/s2_4876050000000000.osm.pbf ]
  [ -f $TEST_DATA_DIR/s2_48761b0000000000.osm.pbf ]
  [ -f $TEST_DATA_DIR/s2_48761d0000000000.osm.pbf ]
}

@test "convert to geom" {

  cmd="osm2assimp -i $TEST_DATA_DIR/test0000.osm.pbf -o $TEST_DATA_DIR/test0000.osm.obj -r"
  echo $cmd
  run $cmd

  [ "$status" -eq 0 ]
  [ -f $TEST_DATA_DIR/test0000.osm.obj ]
  [ `grep "Mesh" $TEST_DATA_DIR/test0000.osm.obj | wc -l` -eq 2 ]
}

@test "convert with extents" {

  #these coordinates should produce an obj file 1km in width and depth, using the mercator projection
  cmd="osm2assimp -i $TEST_DATA_DIR/test.osm -o $TEST_DATA_DIR/extents.obj -e -0.085415,51.522852,-0.076432,51.528441 -z -u 3.5"

  run $cmd
  echo $cmd

  dimensions=$(get_dimensions "$TEST_DATA_DIR/extents.obj")

  [ "$status" -eq 0 ]
  [ -f $TEST_DATA_DIR/extents.obj ]
  [ $(compare_vec3 $dimensions 1000.0 1000.0 100.0 20.0) == "0" ]
}

@test "export s2 cell" {

  cmd="osm2assimp -i $TEST_DATA_DIR/test.osm -o $TEST_DATA_DIR/48761cafc0000000.obj -s 48761cafc0000000 -z -r -g -a -c 1"
  echo $cmd
  run $cmd

  dimensions=$(get_dimensions "$TEST_DATA_DIR/48761cafc0000000.obj" )

  # echo $dimensions 2>&3 

  [ "$status" -eq 0 ]
  [ $(compare_vec3 $dimensions 2668.0 1828.0 30.0 40.0) == "0" ]

}

@test "s2util" {

  cmd="s2util s2_4876030000000000.osm.pbf -c"
  echo $cmd
  run $cmd
  echo "s2Utils = [$output]" 2>&3

  [ "$status" -eq 0 ]
  [ "$output" = "Cell Center = 51.473,-0.0468724" ]
}
