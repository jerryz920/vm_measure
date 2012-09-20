
for ((i=2;i<=1024;i+=4))
do
  echo -n "$i "
	grep 'dTLB\|r1008' tlb-mem-perf.$i | grep -v misses \
	  | awk '{print $1}' | tr '\n' ' ' | awk ' {print $2 " " $1} '
done
