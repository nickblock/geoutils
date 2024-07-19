# Geoutils

A set of tools for converting Open Street Map data into 3d geometry, also incorporating S2 Cells.

## Building

Install and use VcPkg

https://learn.microsoft.com/en-gb/vcpkg/get_started/get-started?pivots=shell-bash

generate cmake build with ninja

    cmake --preset=default

build all cores

    cmake --build build --parallel `nproc`

## S2 Cell Split

Starting with an osm data file, download an area from https://www.openstreetmap.org/, eg old_street.osm

split the osm file into files comprising of S2 cells by specifying to input file, an output folder and an S2 cell level:

    osms2split -i old_street.osm -o testadata -l 14

the output folder will now contain a number of osm files for each S2 cell.

## Convert OSM to obj

convert osm data files into geometry

    osm2assimp -i example.osm -o example.obj

## Run Tests

Tests make use of https://github.com/bats-core/bats-core, a system for running tests in bash.
After building to the 'build' folder export PATH:

    export PATH=$PATH:`pwd`/build:

Run tests with:

    bats test.bats