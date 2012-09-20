#!/bin/bash

rm -f plotdata
for prefix in map_full unmap_clean unmap_no_clean fwalk regular_walk
do
  echo -n $prefix "	" >> plotdata
  for kind in "time" "time_busy"
  do
    fname=$prefix"_"$kind
    cat $fname | xargs ../scripts/avg.rb >> plotdata
    echo -n "	" >> plotdata
  done
  echo >> plotdata
done


