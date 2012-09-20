
set autoscale
unset log
unset label
set terminal postscript eps enhanced
set output "time.eps"
set xtic auto
set ytic auto
set title "Memory Walk Time"
set xlabel "probing page count"
set ylabel "normalized memory latency(ns)"

#plot "cache-hit-rate" using 1:5 title "happy"
plot "timing" using 1:2 title "Memory Read Time" with linespoints lt 1
