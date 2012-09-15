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
#endif
