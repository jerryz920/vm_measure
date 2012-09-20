#!/bin/bash

for t in seq rand
do
  for mem in 2097152 16777216 134217728
  do
    for iter in 1 2 4 8 16
    do
      for stride in 4 4096 32768 2097152
      do
	echo -n "$t $mem $iter $stride " >> stat
	cat ${1:-tlb}.$t.$mem.$iter.$stride | ../../scripts/avgf >> stat
      done
    done
  done
done
