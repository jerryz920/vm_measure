/******************************************************************************

 @File Name : {PROJECT_DIR}/a.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :

*******************************************************************************/

#include "config.h"
#include "misc.h"

#include <unistd.h>
#include <sys/tpyes.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * return the page count used. For each of the page, we will future visit only 1 cache line
 * inside. We use bit count to describe size.
 *
 * When we force it to miss, then we should walk (uncontrollable_set + 1) * (way + 1) cache line
 */
static int uncontrollable_set(int cache_size, int way, int page_size, int cache_line)
{
  /*
   *  Cache line per way also means the number of associative set
   */
  int total_ass_set = cache_size - number_to_power(way) - cache_line;
  int page_ass_set = page_size - cache_line;
  if (total_ass_set < page_ass_set)
    return 0;
  else
    return power_to_number((total_ass_set - page_ass_set) - 1);
}

/*
 *  Measure the TLB size, unit is KB
 *
 *  Observation:
 *    When measuring the TLB size, we want to keep the cache missing or hit. And walking sequence
 *    should be somewhat "cyclic", rather than block. This means, we visit one page, then next
 *    visit the following page rather than stay in the same one. This will effectively use the
 *    cache capacity to force TLB miss
 *
 *    One thing complicate is that pages at user space is not physical continuous. The result is
 *    that we might be able to control our hehaviors visiting some of the associative set inside.
 *
 *    Here due to my observations to current TLB/Cache settings, we make following assumptions:
 *    1.)
 *    2.)
 */
static int measure_small_tlb_size(int level, int prev_tlb_size, int upper, int page_size)
{
  return 0;
}

static int measure_large_tlb_size(int level, int prev_tlb_size, int upper, int page_size)
{
  return 0;
}

int main()
{
  int l1_size = measure_small_tlb_size(1, 0, l1_cache_size(), 4096);
  int l2_size = measure_small_tlb_size(2, l1_size, l2_cache_size(), 4096);
  int l1_size_huge = measure_large_tlb_size(2, 0, l2_cache_size(), 2048 * 1024);
  int l2_size_huge = measure_large_tlb_size(2, l1_size, l2_cache_size(), 2048 * 1024);


  return 0;
}
