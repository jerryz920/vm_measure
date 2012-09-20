
for ((i = 0 ; i <= 1025; i++))
do
  if test -f cache-mem-perf.$i
  then
    echo -n "$i "
    grep 'r..D1' cache-mem-perf.$i | awk  '{print $1}'  | sed 's/,//g' | tr '\n' ' '
    echo
  fi
done > caches
