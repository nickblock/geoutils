#!/bin/bash

function get_assimp_info() {
  assimpd info $1
}

function maxline() {
  echo "$1" | grep "Maximum point"
}

function minline() {
  echo "$1" | grep "Minimum point"
}

function get_decimal() {
  echo "$1" | grep -Eo '[+-]?[0-9]+([.][0-9]+)?'
}

function get_dimensions() {

  assimp_info=$(get_assimp_info "$1")

  maxstr=$(maxline "$assimp_info")

  minstr=$(minline "$assimp_info")

  min_nums=$(get_decimal "$minstr")
  max_nums=$(get_decimal "$maxstr")

  arr_min=($min_nums)
  arr_max=($max_nums)

  res=""
  for i in {0..2}; do
    res="$res $(echo "${arr_max[$i]} - ${arr_min[$i]}" | bc)"
  done
  echo $res
}

#params input1 - input2 < diff
#returns 1 if comparison is greater than diff
function compare_dec() {
  diff=$(echo "$1 - $2" | bc)
  neg=$(echo "$diff < 0" | bc)
  if [ $neg == 1 ]; then 
    diff=$(echo "$diff*-1.0" | bc)
  fi
  gt=$(echo "$diff > $3" | bc)
  echo $gt
}

#params v1_0 v1_1 v1_2 v2_0 v2_1 v2_2 epsilon
function compare_vec3() {

  gt=$(compare_dec $1 $4 $7)
  echo "$gt"
  return
  if [ $gt == "1" ]; then
    echo "1"
    return
  fi
  gt=$(compare_dec $2 $5 $7)
  if [ $gt == "1" ]; then
    echo "1"
    return
  fi
  gt=$(compare_dec $3 $6 $7)
  if [ $gt == "1" ]; then
    echo "1"
    return
  fi
  echo "0"
}

# dim=$(get_dimensions $1)

# echo $(compare_dec 10.0 9.8 1.0) 
# if [ $(compare_dec 10.0 9.8 1.0) == "0" ]; then
#   echo "passed"
# else
#   echo "failed"
# fi
# echo $(compare_vec3 $dim 1000.0 1000.0 100.0 20.0)

# get_dimensions $1
