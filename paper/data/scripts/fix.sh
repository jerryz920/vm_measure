#!/bin/bash
awk '{fix='134217728'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}' large_rand_stat_128m > large_rand_stat_128m_fixed
awk '{fix='134217728'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}' large_seq_stat_128m  > large_seq_stat_128m_fixed
awk '{fix='134217728'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}' small_rand_stat_128m > small_rand_stat_128m_fixed
awk '{fix='134217728'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}' small_seq_stat_128m  > small_seq_stat_128m_fixed
awk '{fix='16777216'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}'  large_rand_stat_16m  > large_rand_stat_16m_fixed
awk '{fix='16777216'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}'  small_rand_stat_16m  > small_rand_stat_16m_fixed
awk '{fix='16777216'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}'  small_seq_stat_16m   > small_seq_stat_16m_fixed
awk '{fix='16777216'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}'  large_seq_stat_16m   > large_seq_stat_16m_fixed
awk '{fix='2097152'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}'   large_seq_stat_2m    > large_seq_stat_2m_fixed
awk '{fix='2097152'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}'   large_rand_stat_2m   > large_rand_stat_2m_fixed
awk '{fix='2097152'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}'   small_rand_stat_2m   > small_rand_stat_2m_fixed
awk '{fix='2097152'/$2; print $1 " " $2 " " $3/$1/fix " " $4 /$1/fix}'   small_seq_stat_2m   > small_seq_stat_2m_fixed
