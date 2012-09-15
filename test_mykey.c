/******************************************************************************

 @File Name : {PROJECT_DIR}/test.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :

*******************************************************************************/

#include "misc.h"
#include <stdio.h>

int main()
{
  int mykey_cnt;
  const int* mykeys = get_mem_desc_keys(&mykey_cnt);
  printf("key count %d\n", mykey_cnt);
  while (mykey_cnt-- > 0) {
    const struct CacheDesc* c = get_cache_desc(mykeys[mykey_cnt]);
    const struct TLBDesc* t = get_tlb_desc(mykeys[mykey_cnt]);

    printf("key %x - ", mykeys[mykey_cnt]);
    if (c->level != -1) {
      printf("cache level %d:\n size %d\n set %d\n cache line %d\n data cache? %d\n",
          c->level, c->size, c->group_size, c->line_size, c->data_cache);
    } else if (t->level != -1) {
      printf("tlb level %d:\n size %d\n set %d\n data tlb? %d\n",
          t->level, t->size, t->group_size, t->data_tlb);
      int tmp = t->page_size_cnt;
      printf(" supported pages:\n");
      while (tmp-- > 0)
        printf(" %d ", t->page_size[tmp]);
      puts("");
    }

  }

  printf("cpu freq %f, prefetch %d\n", cpu_freq(0), hardware_prefetch_size());


  return 0;
}
