/******************************************************************************

 @File Name : {PROJECT_DIR}/cache.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :

*******************************************************************************/

#include "config.h"
#include <stdio.h>


/*
 *  Use huge table support to measure the cache size
 */
static int measure_cache()
{
  char* ptr = alloc_huge_warm_page(4, (HUGE_PAGE_SIZE));
  int total = 0, i, walk_size = get_cache_line();

  for (walk_size = get_cache_line(); walk_size < HUGE_PAGE_SIZE; walk_size *= 2) {
    int j;
    uint64_t begin = get_usec();
    for (j = 0; j < 1000; j++) {
      total += walk(ptr, walk_size, get_cache_line());
    }
    uint64_t end = get_usec();
    printf("%f\n", end - begin / 1000.0)
  }
  /* just for reference */
  return total;
}

int main()
{
  measure_cache();
  return 0;
}

