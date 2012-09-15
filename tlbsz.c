/******************************************************************************

 @File Name : {PROJECT_DIR}/a.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :


*******************************************************************************/

#include "config.h"
#include "misc.h"
#include "debug.h"

#include <unistd.h>
#include <sys/types.h>
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
 *  Define the walking routines for walk. We just use 2MB pages on X86-64 platform.
 *  Although there are 1G page, we just don't use it right now. The theory to
 *  measure is the same
 */
#if CACHE_LINE == 64
DEFINE_PAGE_WALK64(4096)
DEFINE_PAGE_WALK64(2097152)
#elif CACHE_LINE == 32
DEFINE_PAGE_WALK32(4096)
DEFINE_PAGE_WALK32(2097152)
#else
#error "cache line is not typical configuration (64/32), double check it"
#endif

/***********************************************************************
 *    Part1: Measuring the parameters of different level caches
 ***********************************************************************/
struct CacheDesc measured_cache_info[4]; /* we don't see any level 4 cache currently, so we use 4 */

struct CacheWalkCb {
  char* ptr;
  int total_size;
  int stride;
};

static void init_cache_walk_cb(struct CacheWalkCb* cb, struct HugePage* pg, int walk_size, int cache_line)
{
  /*
   *  Check that huge page can still meet the requirement of the walk size
   */
  if (pg->pcnt * pg->psize < walk_size) {
    ERR("huge page size can not meet requirement, consider configuring more");
    exit(1);
  }
  cb->ptr = pg->addr;
  cb->stride = cache_line;
  cb->total_size = walk_size;
}

static int cache_walk(struct HugePage* pg, int walk_size, int loop, uint64_t* timep)
{
  int sum = 0, i;
  struct CacheWalkCb cb;
  init_cache_walk_cb(&cb, pg, walk_size, CACHE_LINE);
  /* walk to warm the cache and TLB, so later can go miss directly */
  walk(cb.ptr, cb.total_size, cb.stride);
  uint64_t begin = get_usec();
  for (i = 0; i < loop; i++)
    walk(cb.ptr, cb.total_size, cb.stride);
  uint64_t end = get_usec();
  *timep = end - begin;
  return sum;
}

static int do_measure_cache_info(int level, int previous_size, int total_mem)
{
  int walk_size;
  int sum = 0;
  struct HugePage* base_page = alloc_huge_pages(total_mem / HUGE_PAGE_SIZE,
    HUGE_PAGE_SIZE);
  if (!base_page) {
    ERR("can not allocate %d huge pages\n", total_mem / HUGE_PAGE_SIZE);
    exit(1);
  }

  double walk_time_prev = -1;
  double walk_time_curr = 0;
  printf("cache %d\n", level);
  for (walk_size = previous_size? previous_size * 2: 1;
      walk_size < total_mem; walk_size *= 2) {
    debug("walk on size %d\n", walk_size);
    sum += cache_walk(base_page, walk_size, total_mem / walk_size,
        &walk_time_curr);
    if (walk_time_prev > 0 && walk_time_curr > walk_time_prev * 2) {
      measured_cache_info[level].data_cache = 1;
      measured_cache_info[level].level = level;
      measured_cache_info[level].size = walk_size;
      measured_cache_info[level].line_size = CACHE_LINE;
    }
    printf("%d %.4f\n", walk_size, walk_time_curr);
    walk_time_prev = walk_time_curr;
  }
  return sum;
}



static void measure_cache_info()
{
  int sum = 0;
  sum += do_measure_cache_info(1, 0, TOTAL_MEM_SMALL);
  sum += do_measure_cache_info(2, measured_cache_info[1].size, TOTAL_MEM_SMALL);
  sum += do_measure_cache_info(3, measured_cache_info[2].size, TOTAL_MEM_SMALL);
  printf("useless sum %d\n", sum);
}


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
  measure_cache_info();
  return 0;
}
