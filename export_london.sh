#!/bin/bash

if [ ! -f $1 ] ; then
  echo "'$1' input not found"
  exit 1
fi

if [ ! -d $2 ] ; then
  echo "'$2' output dir not found"
  exit 1
fi

rm $2/*
echo "input $1, output $2, level $3"

./build/osms2split -i $1 -o $2 -l $3 -x

inputFiles=""
num=0
for filename in $2/*; do
  # coords=`s2util $filename | sed 's/ //g'`
  # geomFile=`echo $filename | sed s/\.osm/\.obj/`
  inputFiles="$inputFiles,$filename"
  let num=num+1
done

echo $num

LD_LIBRARY_PATH=/usr/local/lib/ ./build/osm2assimp -i $inputFiles -o london.dae -r -p 51.507361,-0.127743