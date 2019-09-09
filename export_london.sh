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

osms2split -i $1 -o $2 -l $3 -x

for filename in $2/*; do
  coords=`s2util $filename | sed 's/ //g'`
  geomFile=`echo $filename | sed s/\.osm/\.obj/`
  osm2assimp -i $filename -o $geomFile -p $coords -r
done