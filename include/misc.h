/******************************************************************************

 @File Name : {PROJECT_DIR}/include/misc.h

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :

  auxiliary tools for testing vm system

*******************************************************************************/
#ifndef ZY_TEST_H
#define ZY_TEST_H

#include "config.h"

#include <stdint.h>
#include <sys/types.h>

struct CacheDesc {
  int level;
  int size;
  int group_size;
  int inclusive;
  int line_size;
  int data_cache;
};

struct TLBDesc {
  int level;
  int size;
  int group_size;
  int page_size_cnt;
  int page_size[TLB_SUPPORT_PAGE_SIZE_CNT];
  int data_tlb;
};

struct CPUDesc {
  double freq;
  int prefetch_size;
  /* adding other parameters */
};

struct HugePage {
  int shmid;
  int pcnt;
  int psize;
  char* addr;
};

struct PageGroup {
  union {
    struct {
      char* start;
      int16_t len;
      int exist:1;
    };
    char mem[16];
  };
};

/***********************************************************************
 *  Scheduler Interface
 ***********************************************************************/
#ifdef HAVE_SCHED
extern int set_thread_cpu(int index);
#else
#define set_cpu(x) ((void*)0)
#endif

/***********************************************************************
 *  Timing Support
 ***********************************************************************/

#ifdef HAVE_RDTSC
uint64_t get_stamp();
#endif

struct timeval;
struct timespec;

uint64_t time_diff(const struct timeval* t1, const struct timeval* t2);
uint64_t spec_diff(const struct timespec* t1, const struct timespec* t2);
uint64_t get_usec();
uint64_t get_nsec();

/***********************************************************************
 *  CPU Interface
 ***********************************************************************/
#ifdef HAVE_CPUID
const int* get_mem_desc_keys(int* cnt);
const struct TLBDesc* get_tlb_desc(int key);
const struct CacheDesc* get_cache_desc(int key);
int hardware_prefetch_size();
double cpu_freq(int cpu);
#else
#error "only support x86/64 platform now"
#endif


/***********************************************************************
 *              Memory Manipulation Interface
 ***********************************************************************/
/*
 *  Memory information 
 */
int64_t free_mem_count();
int64_t unused_mem_count();
int64_t buffered_mem_count();
int64_t cached_mem_count();
int64_t total_mem_count();
int64_t page_in_core(char* addr, int npage, int page_size);

/*
 *  Allocation/Deallocation
 */
char* alloc_pages(int cnt, int size);
struct HugePage* alloc_huge_pages(int cnt, int size);
char* alloc_raw_pages(int cnt, int size);
void free_raw_pages(char* ptr, int cnt, int size);
void free_huge_page(struct HugePage* page);

/*
 *  Specific for small walk to test cache way
 */
int walk(const char* ptr, int stride, int loop, int nway);

/*
 *  This will walk loop * period * nstride memory location.
 *    The location visited would be repeatedly at
 *    ptr + j * period + k * stride, here k >= 0 && k < nstride, j >= 0 && j < nperiod
 *
 */
int period_walk(const char* ptr, int loop, int nperiod, int period, int nstride, int stride);

/*
 *  Linked list style walk
 */
int page_walk_setup(char* ptr, int nperiod, int period, int nstride, int stride, int tail_stride) ;
int linked_page_walk(char* page, int loop, int npages);

/*
 *  A group walk is a walk with several page groups,
 *  each group is a segment of continous memory.
 *  During walking, the inner_group_size specified
 *  how to walk within a group. And the inter_group_stride means before start a walk in a group, the offset to be add to the start address in a group
 *
 */
int group_walk(struct PageGroup* pg, int max_line, int loop, int inter_group_stride, int inner_group_stride);

/*
 * @DEPRECATED
 *  use period_walk instead
 *
 *  Walk on pages one by one, visit exactly each page
 *  once. In ith page, we visit the (i) mod n cache line
 *
 *  We manually template these so that most offset can be 
 *  constant...
 */
#if 0
#define PAGE_WALK_NAME(page_size, cache_line, n) \
  page_walk_ ## page_size ## _ ## cache_line ## _ ## n

#define DEFINE_PAGE_WALK(page_size,cache_line, n) \
  int PAGE_WALK_NAME(page_size, cache_line, n) (char* ptr, int page_cnt)\
{\
  register int sum = 0;\
  int i;\
  uint64_t off;\
\
  for (i = 0; i <= page_cnt - 8; i += 8, ptr += 8 * page_size) {\
    sum += *(int*)(ptr + (0 % n) * cache_line);\
    sum += *(int*)(ptr + 1 * page_size + (1 % n) * cache_line);\
    sum += *(int*)(ptr + 2 * page_size + (2 % n) * cache_line);\
    sum += *(int*)(ptr + 3 * page_size + (3 % n) * cache_line);\
    sum += *(int*)(ptr + 4 * page_size + (4 % n) * cache_line);\
    sum += *(int*)(ptr + 5 * page_size + (5 % n) * cache_line);\
    sum += *(int*)(ptr + 6 * page_size + (6 % n) * cache_line);\
    sum += *(int*)(ptr + 7 * page_size + (7 % n) * cache_line);\
  } \
  switch(page_cnt - i) {\
    case 7: sum += *(int*)(ptr + 7 * page_size + (7 % n) * cache_line);\
    case 6: sum += *(int*)(ptr + 6 * page_size + (6 % n) * cache_line);\
    case 5: sum += *(int*)(ptr + 5 * page_size + (5 % n) * cache_line);\
    case 4: sum += *(int*)(ptr + 4 * page_size + (4 % n) * cache_line);\
    case 3: sum += *(int*)(ptr + 3 * page_size + (3 % n) * cache_line);\
    case 2: sum += *(int*)(ptr + 2 * page_size + (2 % n) * cache_line);\
    case 1: sum += *(int*)(ptr + 1 * page_size + (1 % n) * cache_line);\
  }\
  return sum;\
}\

#define DEFINE_PAGE_WALK64(page_size) \
  DEFINE_PAGE_WALK(page_size, 64, 1 )\
  DEFINE_PAGE_WALK(page_size, 64, 2 )\
  DEFINE_PAGE_WALK(page_size, 64, 3 )\
  DEFINE_PAGE_WALK(page_size, 64, 4 )\
  DEFINE_PAGE_WALK(page_size, 64, 5 )\
  DEFINE_PAGE_WALK(page_size, 64, 6 )\
  DEFINE_PAGE_WALK(page_size, 64, 7 )\

#define DEFINE_PAGE_WALK32(page_size) \
  DEFINE_PAGE_WALK(page_size, 32, 1)\
  DEFINE_PAGE_WALK(page_size, 32, 2)\
  DEFINE_PAGE_WALK(page_size, 32, 3)\
  DEFINE_PAGE_WALK(page_size, 32, 4)\
  DEFINE_PAGE_WALK(page_size, 32, 5)\
  DEFINE_PAGE_WALK(page_size, 32, 6)\
  DEFINE_PAGE_WALK(page_size, 32, 7)\

#define CALL_PAGE_WALK(page_size, cache_line, n, ptr, page_cnt) \
      PAGE_WALK_NAME(page_size, cache_line, n)(ptr, page_cnt)
#endif

/***********************************************************************
 *  Misc
 ***********************************************************************/
#define ALIGN_PTR(ptr, size) (typeof(ptr)) ((uint64_t)((((char*) ptr) + size - 1)) & (~(uint64_t)size))

uint8_t number_to_power(uint64_t num);
uint64_t power_to_number(uint8_t power);
void fake_use(void* ptr);

static inline int min(int x, int y)
{
  return x < y? x: y;
}

#endif
