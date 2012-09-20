
set autoscale
unset log
unset label
set terminal postscript eps enhanced
set output "cache.eps"
set xtic auto
set ytic auto
set title "Cache Hit Ratio"
set xlabel "probing page count"
set ylabel "hit ratio(%)"


#plot "cache-hit-rate" using 1:5 title "happy"
plot "l1-hit-rate" using 1:2 title "L1 hit rate" with linespoints lt 1, \
	"l2-hit-rate" using 1:2 title "L2 hit rate" with points lw 1, \
	"l3-hit-rate" using 1:2 title "L3 hit rate" with lines lw 1, \
	"mem-hit-rate" using 1:2 title "Mem hit rate" with points
