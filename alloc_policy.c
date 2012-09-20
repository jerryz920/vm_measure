/******************************************************************************

 @File Name : {PROJECT_DIR}/alloc_policy.c

 @Creation Date : 17-09-2012

 @Created By: Zhai Yan

 @Purpose :

  Test the allocation policy for the system allocator

*******************************************************************************/


/*
 *  Tasks:
 *   check whether system on demand zero out allocated pages
 *    random read / sequential read at free/busy time
 */

#include "debug.h"
#include "misc.h"
#include "config.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/*
 *  Define the walk iterations
 */
#define CHECK_ALLOC_ITER 100
#define ALLOC_SIZE  (1024 * 1024 * 1024)

static int test_on_demand_allocate()
{

/*
 *  Repeatly, we map a page, and visit at stride of PAGE_SIZE, unmap it. To
 *  ammortize the impact of mmap/mmumap, we allocate large page size
 */
  int i, j, sum = 0, npage = ALLOC_SIZE / PAGE_SIZE;
  char* pages;

  uint64_t total_unmap = 0;

  uint64_t begin_u = get_usec();
  uint64_t begin_n = get_nsec();
  for (i = 0; i < CHECK_ALLOC_ITER; i++) {
    pages = alloc_raw_pages(npage, PAGE_SIZE);
    sum += walk(pages, PAGE_SIZE, 1, ALLOC_SIZE / PAGE_SIZE / 2);
    uint64_t tmp1 = get_usec();
    free_raw_pages(pages, npage, PAGE_SIZE);
    total_unmap += get_usec() - tmp1;
  }
  uint64_t end_u = get_usec();
  uint64_t end_n = get_nsec();
  fprintf(stderr, "with map: %.3f %.3f %.3f\n", 1000.0 * (end_u - begin_u) / CHECK_ALLOC_ITER,
                                            1.0 * (end_n - begin_n) / CHECK_ALLOC_ITER,
                                       1000.0 * total_unmap / CHECK_ALLOC_ITER
      );

  uint64_t total_time_u = 0;
  total_unmap = 0;
  for (i = 0; i < CHECK_ALLOC_ITER; i++) {
    pages = alloc_raw_pages(npage, PAGE_SIZE);
    begin_u = get_usec();
    sum += walk(pages, PAGE_SIZE, 1, ALLOC_SIZE / PAGE_SIZE / 2);
    end_u = get_usec();
    uint64_t tmp1 = get_usec();
    for (j = 0; j < npage; j++)
      pages[j * PAGE_SIZE] = 1;
    free_raw_pages(pages, npage, PAGE_SIZE);
    total_unmap += get_usec() - tmp1;
    total_time_u += end_u - begin_u;
  }

  uint64_t total_time_n = 0;
  for (i = 0; i < CHECK_ALLOC_ITER; i++) {
    pages = alloc_raw_pages(npage, PAGE_SIZE);
    begin_n = get_nsec();
    sum += walk(pages, PAGE_SIZE, 1, ALLOC_SIZE / PAGE_SIZE / 2);
    end_n = get_nsec();
    free_raw_pages(pages, npage, PAGE_SIZE);
    total_time_n += end_n - begin_n;
  }

  fprintf(stderr, "without map: %.3f %.3f %.3f\n", 1000.0 * total_time_u / CHECK_ALLOC_ITER,
                                            1.0 * total_time_n / CHECK_ALLOC_ITER,
                                          1000.0 * total_unmap / CHECK_ALLOC_ITER);

  pages = alloc_raw_pages(npage, PAGE_SIZE);
  begin_u = get_usec();
  begin_n = get_nsec();
  sum += walk(pages, PAGE_SIZE, CHECK_ALLOC_ITER, ALLOC_SIZE / PAGE_SIZE / 2);
  end_u = get_usec();
  end_n = get_nsec();

  fprintf(stderr, "regular1: %.3f %.3f\n", 1000.0 * (end_u - begin_u) / CHECK_ALLOC_ITER,
                                            1.0 * (end_n - begin_n) / CHECK_ALLOC_ITER
      );
  free_raw_pages(pages, npage, PAGE_SIZE);

  pages = alloc_raw_pages(npage, PAGE_SIZE);
  begin_u = get_usec();
  begin_n = get_nsec();
  for (i = 0; i < CHECK_ALLOC_ITER; i++) 
    sum += walk(pages, PAGE_SIZE, 1, ALLOC_SIZE / PAGE_SIZE / 2);
  end_u = get_usec();
  end_n = get_nsec();

  fprintf(stderr, "regular2: %.3f %.3f\n", 1000.0 * (end_u - begin_u) / CHECK_ALLOC_ITER,
                                            1.0 * (end_n - begin_n) / CHECK_ALLOC_ITER
      );
  free_raw_pages(pages, npage, PAGE_SIZE);

}

static char* consume_memory(int64_t npage, int page_size)
{
  char* consume = alloc_raw_pages(npage, page_size);

  /*
   *  Force allocating the memory
   */
  if (!consume) {
    perror("consume");
    exit(1);
  }
  int sum = walk(consume, page_size, 3, npage / 2);
  sum += walk(consume, CACHE_LINE, 1, npage * page_size / CACHE_LINE / 2);

  printf("#sum %d\n", sum);
  return consume;
}

static int small_alloc(int npage, int page_size)
{
  int sum = 0, i;
  uint64_t begin = get_usec();
  for (i = 0; i < CHECK_ALLOC_ITER; i++) {
    char* ptr = alloc_pages(i, npage);
    sum += (int64_t)ptr;
    free(ptr);
  }
  fprintf(stderr, "small %.3lf\n", 1000.0 *  (get_usec() - begin) / CHECK_ALLOC_ITER);
  return sum;
}

int main()
{
  int sum = test_on_demand_allocate();

  int64_t free_mem = free_mem_count();
  int64_t free_pages = free_mem / PAGE_SIZE;
  printf("free memory %lx, free pages %d\n", free_mem, free_pages);
  /*
   *  Left only with 5/4 memory that can be allocated
   */
  int64_t take_away = (free_pages - ALLOC_SIZE / PAGE_SIZE / 4 * 5);
  printf("take away %lx\n", take_away);
  char* consume = consume_memory(take_away, PAGE_SIZE);
  sum = test_on_demand_allocate();
  free_raw_pages(consume, take_away, PAGE_SIZE);
  printf("#%d\n", sum);

  return 0;
}
