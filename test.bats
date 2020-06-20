#!/usr/bin/env bats

source assimp_info.sh

test_output_folder=/data/test
build_folder=/home/nick/Documents/metrolysis/server/native/build/thirdparty/geoutils/


@test "setup  context" {
  
  rm -rf $test_output_folder
  mkdir -p $test_output_folder

  # a large area around old street london
  ./create_test_osm_file.py $test_output_folder/test.osm --extents " 51.514853,-0.104486,51.531354,-0.065948" 

  [ -f $test_output_folder/test.osm ]
}

@test "split test data" {

  mkdir -p $test_output_folder

  "$build_folder"/osmsplit -i $test_output_folder/test.osm -o $test_output_folder -s 1 -l 4

  [ `ls -al $test_output_folder/test*.osm.pbf | wc -l` -eq 16 ]
}

@test "split into S2 cells" {

  "$build_folder"/osms2split -i $test_output_folder/test.osm -o $test_output_folder -l 10

  [ -f $test_output_folder/s2_4876030000000000.osm.pbf ]
  [ -f $test_output_folder/s2_4876050000000000.osm.pbf ]
  [ -f $test_output_folder/s2_48761b0000000000.osm.pbf ]
  [ -f $test_output_folder/s2_48761d0000000000.osm.pbf ]
}

@test "convert to geom" {

  "$build_folder"/osm2assimp -i $test_output_folder/test0000.osm.pbf -o $test_output_folder/test0000.osm.obj -r 

  [ -f $test_output_folder/test0000.osm.obj ]
  [ `grep "Mesh" $test_output_folder/test0000.osm.obj | wc -l` -eq 2 ]
}

@test "convert with extents" {

  #these coordinates should produce an obj file 1km in width and depth, using the mercator projection
  "$build_folder"/osm2assimp -i $test_output_folder/test.osm -o $test_output_folder/extents.obj -e -0.085415,51.522852,-0.076432,51.528441 -z

  dimensions=$(get_dimensions "$test_output_folder/extents.obj")


  [ -f $test_output_folder/extents.obj ]
  [ $(compare_vec3 $dimensions 1000.0 1000.0 100.0 20.0) == "0" ]
}

@test "export s2 cell" {

  "$build_folder"/osm2assimp -i $test_output_folder/test.osm -o $test_output_folder/48761cafc0000000.dae -s 48761cafc0000000

  dimensions=$(get_dimensions "$test_output_folder/48761cafc0000000.dae" )

  [ $(compare_vec3 $dimensions 320.0 30.000000 480.0 40.0) == "0" ]

}

@test "s2util" {

  [ `"$build_folder"/s2util s2_4876030000000000.osm.pbf --c` = "51.473,-0.0468724" ]
}
