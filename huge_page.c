/******************************************************************************

 @File Name : {PROJECT_DIR}/huge_page.c

 @Creation Date : 18-09-2012

 @Created By: Zhai Yan

 @Purpose :

*******************************************************************************/

#include "debug.h"
#include "config.h"
#include "misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>

static int test_sequential_walking(char* pages, int nstride, int iter, int stride, uint64_t* timep)
{
  int sum = 5;
  uint64_t begin_u = get_usec();
  sum += walk(pages, stride, iter, nstride / 2);
  *timep = get_usec() - begin_u;
  return sum;
}

static int test_random_walking(char* pages, int64_t total_size, int nstride, int iter, uint64_t* timep)
{
  int sum = 6, i, j;
  int64_t begin_u = get_usec();
  for (i = 0; i < iter; i++) {
    for (j = 0; j <= nstride - 4; j += 4) {
      int r1 = rand() % (total_size - sizeof(int));
      int r2 = rand() % (total_size - sizeof(int));
      int r3 = rand() % (total_size - sizeof(int));
      int r4 = rand() % (total_size - sizeof(int));
      sum += *(int*) (&pages[r1]);
      sum += *(int*) (&pages[r2]);
      sum += *(int*) (&pages[r3]);
      sum += *(int*) (&pages[r4]);
    }
    int r;
    switch(nstride - j) {
      case 3: r = rand() % (total_size - sizeof(int)); sum += *(int*)(&pages[r]);
      case 2: r = rand() % (total_size - sizeof(int)); sum += *(int*)(&pages[r]);
      case 1: r = rand() % (total_size - sizeof(int)); sum += *(int*)(&pages[r]);
    }
  }
  *timep = get_usec() - begin_u;
  return sum;
}


int main(int argc, char** argv)
{
  int64_t page_size = HUGE_PAGE_SIZE;
  if (argc < 2 || strcmp(argv[1], "small") == 0)
    page_size = PAGE_SIZE;

  /*
   *  It is not quite reasonable to allocate even larger chunk of data
   */
  int64_t mem_size;
  int sum = 0;

  for (mem_size = 2048 * 1024; mem_size <= 64 * 1024 * 2048LL; mem_size *= 8) {
    int npage = mem_size / page_size;
    int iter, stride, i;
    for (iter = 1; iter <= 16; iter *= 2) {
      const int strides[] = {4, 4096, 32768, 2048 * 1024};
      const int nstride = sizeof(strides) / sizeof(strides[0]);
      int inner_loop[] = {4, 16, 32, 512};
      for (i = 0; i < nstride; i++) {
        int stride = strides[i], k;
        uint64_t time_inner = 0, time_out, begin_u;

        begin_u = get_usec();
        for (k = 0; k < inner_loop[i]; k++) {
          uint64_t tmp;
          char* pages = alloc_raw_pages(npage, page_size);
          sum += test_random_walking(pages, mem_size, iter, mem_size / stride, &tmp);
          free_raw_pages(pages, npage, page_size);
          time_inner += tmp;
        }
        time_out = get_usec() - begin_u;
        fprintf(stderr, "rand %"PRId64" %d %d %"PRId64 " %"PRId64"\n", mem_size, iter, stride, time_inner / inner_loop[i], time_out/inner_loop[i]);

        time_inner = 0;
        begin_u = get_usec();
        for (k = 0; k < inner_loop[i]; k++) {
          uint64_t tmp;
          char* pages = alloc_raw_pages(npage, page_size);
          sum += test_sequential_walking(pages, mem_size / stride, iter, stride, &tmp);
          free_raw_pages(pages, npage, page_size);
          time_inner += tmp;
        }
        time_out = get_usec() - begin_u;
        fprintf(stderr, "seq %"PRId64" %d %d %"PRId64 " %"PRId64"\n", mem_size, iter, stride, time_inner / inner_loop[i], time_out/inner_loop[i]);
      }
      printf("%d %d %d\n", mem_size, iter, nstride);
    }
  }
  printf("#%x\n", sum);
  return 0;
}
