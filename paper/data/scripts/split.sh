#!/bin/bash
 
 
 
split -l 5 -a 1 -d large_seq_stat_128m_fixed  large_seq_stat_128m_fixed_
split -l 5 -a 1 -d large_seq_stat_16m_fixed   large_seq_stat_16m_fixed_
split -l 5 -a 1 -d large_seq_stat_2m_fixed    large_seq_stat_2m_fixed_
split -l 5 -a 1 -d small_seq_stat_128m_fixed  small_seq_stat_128m_fixed_
split -l 5 -a 1 -d small_seq_stat_16m_fixed   small_seq_stat_16m_fixed_
split -l 5 -a 1 -d small_seq_stat_2m_fixed    small_seq_stat_2m_fixed_
