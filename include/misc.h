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


#ifdef HAVE_SCHED
extern int set_thread_cpu(int index);
#else
#define set_cpu(x) ((void*)0)
#endif

#ifdef HAVE_CPUID
const int* get_mem_desc_keys(int* cnt);
const struct TLBDesc* get_tlb_desc(int key);
const struct CacheDesc* get_cache_desc(int key);
int hardware_prefetch_size();
double cpu_freq(int cpu);
void fake_use(void* ptr);
#else
#error "only support x86/64 platform now"
#endif

#ifdef HAVE_RDTSC
uint64_t get_stamp();
#endif

struct timeval;
uint64_t timeval_diff(const struct timeval* t1, const struct timeval* t2);


uint8_t number_to_power(uint64_t num);
uint64_t power_to_number(uint8_t power);

int walk(char* ptr, int len, int stride);
int page_walk(char* ptr, int page_count, int page_size, int stride, int max_stride);


/*
 *  Walk on pages one by one, visit exactly each page
 *  once. In ith page, we visit the (i) mod stride cache line
 *
 *  We manually template these so that most offset can be 
 *  constant...
 */
#define PAGE_WALK_NAME(page_size, cache_line, stride) \
  page_walk ## page_size ## cache_line ## stride

#define DEFINE_PAGE_WALK(page_size,cache_line,stride) \
  int PAGE_WALK_NAME(n, page_size, cache_line) (char* ptr, int page_cnt)\
{\
  register int sum = 0;\
  int i;\
  uint64_t off;\
\
  for (i = 0; i <= page_cnt - 8; i += 8, ptr += 8 * page_size) {\
    sum += *(ptr + (0 % n) * cache_line);\
    sum += *(ptr + 1 * page_size + (1 % n) * cache_line);\
    sum += *(ptr + 2 * page_size + (2 % n) * cache_line;\
    sum += *(ptr + 3 * page_size + (3 % n) * cache_line;\
    sum += *(ptr + 4 * page_size + (4 % n) * cache_line;\
    sum += *(ptr + 5 * page_size + (5 % n) * cache_line;\
    sum += *(ptr + 6 * page_size + (6 % n) * cache_line;\
    sum += *(ptr + 7 * page_size + (7 % n) * cache_line;\
  }\
  switch(page_cnt - i) {\
    case 7: sum += *(ptr + 7 * page_size + (7 % n) * cache_line);\
    case 6: sum += *(ptr + 6 * page_size + (6 % n) * cache_line);\
    case 5: sum += *(ptr + 5 * page_size + (5 % n) * cache_line);\
    case 4: sum += *(ptr + 4 * page_size + (4 % n) * cache_line);\
    case 3: sum += *(ptr + 3 * page_size + (3 % n) * cache_line);\
    case 2: sum += *(ptr + 2 * page_size + (2 % n) * cache_line);\
    case 1: sum += *(ptr + 1 * page_size + (1 % n) * cache_line);\
    case 0:\
  }\
  return sum;\
}\

#define DEFINE_PAGE_WALK64(page_size) \
  DEFINE_PAGE_WALK(page_size, 64, 0)\
  DEFINE_PAGE_WALK(page_size, 64, 1)\
  DEFINE_PAGE_WALK(page_size, 64, 2)\
  DEFINE_PAGE_WALK(page_size, 64, 3)\
  DEFINE_PAGE_WALK(page_size, 64, 4)\
  DEFINE_PAGE_WALK(page_size, 64, 5)\
  DEFINE_PAGE_WALK(page_size, 64, 6)\

#define DEFINE_PAGE_WALK32(page_size) \
  DEFINE_PAGE_WALK(page_size, 32, 0)\
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
