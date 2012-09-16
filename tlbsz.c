/******************************************************************************

 @File Name : {PROJECT_DIR}/a.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :


*******************************************************************************/

#include "config.h"
#include "misc.h"
#include "debug.h"

#include <inttypes.h>
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

/***********************************************************************
 *    Part1: Measuring the parameters of different level caches
 ***********************************************************************/
struct CacheDesc measured_cache_info[4]; /* we don't see any level 4 cache currently, so we use 4 */

struct MemWalkCb {
  char* ptr;
  int loop;
  int nperiod;
  int period;
  int nstride;
  int stride;
};

static void init_mem_walk_cb(struct MemWalkCb* cb, char* pg, int total_size, int stride, int loop)
{
  cb->ptr = pg;
  cb->loop = loop;
  cb->nperiod = 1;
  cb->period = 0;
  cb->stride = stride;
  cb->nstride = total_size / stride;
}

static void init_mem_walk_cb_period(struct MemWalkCb* cb, char* pg, int loop, int nperiod, int period, int nstride, int stride)
{
  cb->ptr = pg;
  cb->loop = loop;
  cb->nperiod = nperiod;
  cb->period = period;
  cb->stride = stride;
  cb->nstride = nstride;
}

static int do_mem_walk(struct MemWalkCb* cb, double* timep)
{
  int i, sum = 0;
  if (cb->nperiod <= 1) {
    sum += walk(cb->ptr, cb->stride, 1, cb->nstride);
    uint64_t begin = get_usec();
    sum += walk(cb->ptr, cb->stride, cb->loop, cb->nstride);
    uint64_t end = get_usec();
    *timep = 1.0 * (end - begin) / cb->loop/ cb->nstride / 2;
  } else {
    sum += period_walk(cb->ptr, 1, cb->nperiod, cb->period, cb->nstride, 
        cb->stride);
    uint64_t begin = get_usec();
    sum += period_walk(cb->ptr, cb->loop, cb->nperiod, cb->period, cb->nstride, 
        cb->stride);
    uint64_t end = get_usec();
    *timep = 1.0 * (end - begin) / cb->loop / cb->nstride / cb->nperiod /2;
  }
  return sum;
}

static int cache_walk(struct HugePage* pg, int walk_size, int64_t loop, double* timep)
{
  struct MemWalkCb cb;
  init_mem_walk_cb(&cb, pg->addr, walk_size, CACHE_LINE, loop);
  /* walk to warm the cache and TLB, so later can go miss directly */
  int sum = do_mem_walk(&cb, timep);
  printf("%d %"PRId64" %.5f\n", walk_size, loop, *timep);
  return sum;
}

static int do_measure_cache_info(int level, int previous_size, int total_mem)
{
  int walk_size;
  int sum = 100;
  struct HugePage* base_page = alloc_huge_pages(total_mem / HUGE_PAGE_SIZE + 1,
    HUGE_PAGE_SIZE);
  if (!base_page) {
    ERR("can not allocate %d huge pages\n", total_mem / HUGE_PAGE_SIZE);
    exit(1);
  }

  double walk_time_prev = -1;
  double walk_time_curr = 0;
  printf("cache %d\n", level);
  for (walk_size = previous_size? previous_size * 2: 4096;
      walk_size < total_mem; walk_size *= 2) {
    sum += cache_walk(base_page, walk_size, total_mem / walk_size * 10, 
        &walk_time_curr);
    if (walk_time_prev > 0 && walk_time_curr > walk_time_prev * 1.5) {
      measured_cache_info[level].data_cache = 1;
      measured_cache_info[level].level = level;
      measured_cache_info[level].size = walk_size; /* we are actually testing 2*walk_size, so use this size*/
      measured_cache_info[level].line_size = CACHE_LINE;
      break;
    }
    walk_time_prev = walk_time_curr;
  }
  free_huge_page(base_page);
  return sum;
}


static int cache_way_walk(struct HugePage* pg, int test_way, int cache_size, double* timep)
{
  struct MemWalkCb cb;
  init_mem_walk_cb(&cb, pg->addr, cache_size * test_way, cache_size, 8000000/test_way);
  int sum = 0;
  sum += do_mem_walk(&cb, timep);
  printf("%d %.5lf\n", test_way, *timep);
  return sum;
}

static int do_measure_cache_way(int level, int size, int prev_size, int total_mem)
{
  int test_way;
  int sum = 100;
  struct HugePage* base_page = alloc_huge_pages(total_mem / HUGE_PAGE_SIZE + 1,
    HUGE_PAGE_SIZE);
  if (!base_page) {
    ERR("can not allocate %d huge pages\n", total_mem / HUGE_PAGE_SIZE);
    exit(1);
  }
  
  double walk_time_prev = -1;
  double walk_time_curr = 0;
  printf("cache way %d\n", level);
  int last = 1;
  for (test_way = prev_size? prev_size / 2 + 1: 1; test_way <= 32; test_way += 1) {
    if (size % test_way != 0)
      continue;
    sum += cache_way_walk(base_page, test_way, size, &walk_time_curr);
    if (walk_time_prev > 0 && walk_time_curr > walk_time_prev * 1.2) {
      /*
       *  The reason to divide 2 is because we visit twice as large line. And we also
       *  need to remove the impact from previous level.
       */
      measured_cache_info[level].group_size = size / CACHE_LINE / ((last) * 2 - prev_size);
      break;
    }
    walk_time_prev = walk_time_curr;
    last = test_way;
  }
  if (test_way > 32)
    measured_cache_info[level].group_size = 32;
  free_huge_page(base_page);
  return sum;
}

static void measure_cache_info()
{
  int sum = 0;
  sum += do_measure_cache_info(1, 0, TOTAL_MEM_SMALL);
  sum += do_measure_cache_info(2, measured_cache_info[1].size, TOTAL_MEM_SMALL);
  sum += do_measure_cache_info(3, measured_cache_info[2].size, TOTAL_MEM_SMALL);
  printf("cache size %d %d %d\n",
      measured_cache_info[1].size,
      measured_cache_info[2].size,
      measured_cache_info[3].size);
  sum += do_measure_cache_way(1, measured_cache_info[1].size, 0, TOTAL_MEM_SMALL);
  sum += do_measure_cache_way(2, measured_cache_info[2].size, 
      measured_cache_info[1].size / measured_cache_info[1].group_size / CACHE_LINE,
      TOTAL_MEM_SMALL);
  printf("cache way %d %d\n",
      measured_cache_info[1].group_size,
      measured_cache_info[2].group_size);
  printf("#useless sum %d\n", sum);
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
 *  Method:
 *    When measuring the TLB size, we want to keep the cache missing or hit. And walking sequence
 *    should be somewhat "cyclic", rather than block. This means, we visit one page, then next
 *    visit the following page rather than stay in the same one. This will effectively use the
 *    cache capacity to force TLB miss
 *
 *  Observation:
 *    1.) if L1-TLB has N entry, and L1 cache has K way M sets of cache line, then
 *       N << min(M, (page_size / cache_line)) * K
 *    2.) if L2-TLB has N2 entry, L1 cache has K way M sets, L2 cache has K2 way M2 sets, then
 *      at least (K + K2) * min(M, (page_size / cache_line)) pages can be visited without
 *      L2 cache miss.
 *    3.) if this is still not the case, we try page probing:
 *      We select x * K2 pages for testing L2 miss at same offset. The chance
 *      it has misses would be C(K2, x * K2) / M2^K2. In such way, we can use
 *      following strategy:
 *        3.1) assume we are testing Y pages for TLB missing, here Y > K + K2, or Y is found in previous setup.
 *          allocate exactly one page for buffer
 *        3.2) for i = 0; Y > 0 && i < min(M, page_size/cache_line); i++
 *          map some pages, find the maximum pages Yi that would not miss L2 on the same offset visiting
 *          mark Yi (here Yi has a good chance to be larger than K + K2, but smaller than Y, at least
 *          K + 1)
 *          Y -= Yi
 *          goto 3.2)
 *        3.3) for selected Yi pages, visit them at offset i * cache_line. It should not fail L2, but L1
 *          should fail. If Y is TLB number, we can observe difference compared with Y - 1 setting.
 *        3.4) the expectation of this method will cover E tlb entries measurement, where
 *          E = Exp{Yi} * min(M, page_size / cache_line), and Exp{Yi} can approach K2 * (M2 - 1). Together
 *          with coefficient min(M, page_size / cache_line), this should cover any known TLB
 *          size.
 *
 *      one detail is how to implement the list of Yi, we could allocate half a page, and for condition in
 *      3.2, we just use min(M, page_size/cache_line) / 2 as condition. then we have luxury to store
 *      page_size / <ptr,len>. Given the fact cache_line is usually 32/64byte, <ptr,len> can be easily
 *      stored on the half page without disturbing other cache line. It should occupy a L1-TLB entry, and
 *      when we make walking, the cost could be ammortized in Yi memory operations
 *
 *
 *  Measuring huge tlb is easy since it's physical continous inside. So we just walk through all the 
 *  cache line and things are solved.
 *
 *  Measuring tlb associative degree is also easy. We just test how many ways there are, that is, set 
 *  a way, and observe when the length of walking will no longer cause larger overhead. Visiting would
 *  be limited to measured page size
 *
 */

/*
 *  Record the TLB information
 */
static struct TLBDesc measured_tlb_info[4];

/*
 *  General TLB walk routine. One thing is for sure, TLB entry size is usually power of 2, so it's easier to set the nperiod parameter.
 */
static int tlb_walk(char* pages, int walk_page_cnt, int page_size, int usable_set, int stride, int loop, double* timep)
{
  struct MemWalkCb cb;
  int nperiod, period, nstride;
  if (walk_page_cnt > usable_set) {
    nperiod = walk_page_cnt / usable_set;
    period = usable_set * page_size;
    nstride = usable_set;
  } else {
    nperiod = 1;
    period = 0;
    nstride = walk_page_cnt;
  }

  init_mem_walk_cb_period(&cb, pages, loop, nperiod, period, nstride, stride);
  int sum = do_mem_walk(&cb, timep);
  printf("%d %.5f\n", walk_page_cnt, *timep);
  return sum;
}

/*
 *  Try to measure the L1 TLB using method 1
 *  Assume we have known/measured the cache size/way
 */
static int do_measure_small_tlb1(int mem_total)
{
  int l1_cache_sz  = measured_cache_info[1].size;
  int l1_cache_way = measured_cache_info[1].size / measured_cache_info[1].group_size / CACHE_LINE;
  int usable_set = min(measured_cache_info[1].group_size, PAGE_SIZE / CACHE_LINE);

  int test_page, sum = 0, max_page_to_test = usable_set * l1_cache_way;
  double timeprev = -1;
  double timecurr = 0;

  char* pages = alloc_pages(max_page_to_test, PAGE_SIZE);
  /*
   *  This will only consume about 512 pages in total
   */
  for (test_page = 1; test_page < max_page_to_test; test_page++) {

    /*
     *  Provide a sequence
     */
    int loop = mem_total / test_page / PAGE_SIZE;
    sum += tlb_walk(pages, test_page, PAGE_SIZE, usable_set, PAGE_SIZE + CACHE_LINE, loop, &timecurr) ;
    if (timeprev > 0 && timecurr > timeprev * 1.25) {
      measured_tlb_info[1].level = 1;
      measured_tlb_info[1].size = test_page - 1;
      break;
    }
    timeprev = timecurr;
  }
  free(pages);
  return sum;
}


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
  set_thread_cpu(0);
  measure_cache_info();
  return 0;
}
