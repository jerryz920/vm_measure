#!/bin/bash
 
 
 
split -l 5 -a 1 -d large_rand_stat_128m_fixed  large_rand_stat_128m_fixed_
split -l 5 -a 1 -d large_rand_stat_16m_fixed   large_rand_stat_16m_fixed_
split -l 5 -a 1 -d large_rand_stat_2m_fixed    large_rand_stat_2m_fixed_
split -l 5 -a 1 -d small_rand_stat_128m_fixed  small_rand_stat_128m_fixed_
split -l 5 -a 1 -d small_rand_stat_16m_fixed   small_rand_stat_16m_fixed_
split -l 5 -a 1 -d small_rand_stat_2m_fixed    small_rand_stat_2m_fixed_
