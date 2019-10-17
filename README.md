# Geoutils

A set of tools for converting Open Street Map data into 3d geometry, also incorporating S2 Cells.

## Deps

geos 3.5.2 (no newer)

http://download.osgeo.org/geos/geos-3.5.2.tar.bz2

build + install 


## Building

In root directory create a build folder

`mkdir build`

cd into into it and build with cmake

`cmake ..`

On systems that dont have c++ filesystem (MacOS), you need to avoid one of the tools:

`cmake -DBUILD_OSMSPLIT=OFF ..`

## Convert OSM to obj

`osm2assimp -i example.osm -o example.obj`