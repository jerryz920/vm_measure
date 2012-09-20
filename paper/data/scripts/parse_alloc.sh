#!/bin/bash

rm -f map_time first_walk_time regular_time_*

grep 'with map' $1 | sed 's/with map *://' > map_time
grep 'without map' $1 | sed 's/without map *://' > first_walk_time
grep 'regular1' $1 | sed 's/regular1 *://' > regular_time_1
grep 'regular2' $1 | sed 's/regular2 *://' > regular_time_2

chmod +w map_time first_walk_time regular_time_1 regular_time_2

awk ' NR%2==0{print $1 }' map_time > map_time_full
awk ' NR%2==0{print $2 }' map_time > map_time_full_nsec
awk ' NR%2==0{print $3 }' map_time > unmap_time_no_clean

awk ' NR%2==0{print $1 }' first_walk_time > fwalk_time
awk ' NR%2==0{print $2 }' first_walk_time > fwalk_time_nsec
awk ' NR%2==0{print $3 }' first_walk_time > unmap_time_clean

awk ' NR%2==0{print $1 }' regular_time_1 >> regular_walk_time
awk ' NR%2==0{print $2 }' regular_time_1 >> regular_walk_time_nsec

awk ' NR%2==0{print $1 }' regular_time_2 >> regular_walk_time
awk ' NR%2==0{print $2 }' regular_time_2 >> regular_walk_time_nsec

awk ' NR%2==1{print $1 }' map_time > map_time_full_busy
awk ' NR%2==1{print $2 }' map_time > map_time_full_nsec_busy
awk ' NR%2==1{print $3 }' map_time > unmap_time_no_clean_busy
awk ' NR%2==1{print $1 }' first_walk_time > fwalk_time_busy
awk ' NR%2==1{print $2 }' first_walk_time > fwalk_time_nsec_busy
awk ' NR%2==1{print $3 }' first_walk_time > unmap_time_clean_busy
awk ' NR%2==1{print $1 }' regular_time_1 >> regular_walk_time_busy
awk ' NR%2==1{print $2 }' regular_time_1 >> regular_walk_time_nsec_busy
awk ' NR%2==1{print $1 }' regular_time_2 >> regular_walk_time_busy
awk ' NR%2==1{print $2 }' regular_time_2 >> regular_walk_time_nsec_busy
rm -f map_time first_walk_time regular_time_*

