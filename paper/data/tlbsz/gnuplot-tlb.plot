
set autoscale
unset log
unset label
set terminal postscript eps enhanced font "Arial Black, 16"
set output "tlb.eps"
set xtic auto
set ytic auto
set xlabel "Probing Page Count" font "Arial Black, 18"
set ylabel "Miss Ratio(%)" font "Arial Black, 18"

#plot "cache-hit-rate" using 1:5 title "happy"
plot "tlb-miss-rate" using 1:2 title "L1 miss rate" with linespoints lt 1
