
set autoscale
unset log
unset label
set terminal postscript eps enhanced
set output "tlb.eps"
set xtic auto
set ytic auto
set title "L1 TLB Miss Ratio (Exclusive from L2 TLB Miss)"
set xlabel "probing page count"
set ylabel "miss ratio(%)"

#plot "cache-hit-rate" using 1:5 title "happy"
plot "tlb-miss-rate" using 1:2 title "L1 miss rate" with linespoints lt 1
