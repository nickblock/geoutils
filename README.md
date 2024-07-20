# Geoutils

A set of tools for converting Open Street Map data into 3d geometry, also incorporating S2 Cells.

## Building

Install and use VcPkg

https://learn.microsoft.com/en-gb/vcpkg/get_started/get-started?pivots=shell-bash

generate cmake build with ninja

>    cmake --preset=default

build using 12 cores

>    cmake --build build --parallel 12

## S2 Cell Split

Starting with an osm data file, download an area from https://www.openstreetmap.org/, eg old_street.osm

split the osm file into files comprising of S2 cells by specifying to input file, an output folder and an S2 cell level:

 >   osms2split -i old_street.osm -o testadata -l 14

the output folder will now contain a number of osm files for each S2 cell.

## Convert OSM to obj

convert osm data files into geometry

 >   osm2assimp -i example.osm -o example.obj

## Run Tests

Tests consist of running the various utilities and assessing the output files that get prtoduced.
The test script generates input osm files to test upon.

Requires pyassimp
> pip install pyassimp

pyassimp requires assimp dynamic lib created by vcpkg: 

> export PATH=PATH:`pwd`/build/vcpkg_installed/x64-windows/bin

Add path to build folder so the test script can run the utilties

> export PATH=$PATH:`pwd`/build

Run tests
> python test.py