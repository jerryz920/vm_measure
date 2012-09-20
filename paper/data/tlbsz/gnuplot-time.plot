
set autoscale
unset log
unset label
set terminal postscript eps enhanced "Arial Black, 16"
set output "time.eps"
set xtic auto
set ytic auto
set xlabel "probing page count" font "Arial Black, 18"
set ylabel "normalized memory latency(ns)" font "Arial Black, 18"

#plot "cache-hit-rate" using 1:5 title "happy"
plot "timing" using 1:2 title "Memory Read Time" with linespoints lt 1 lw 2
