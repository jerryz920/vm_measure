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

#ifdef HAVE_SCHED
extern int set_thread_cpu(int index);
#else
#define set_cpu(x) ((void*)0)
#endif

#ifdef HAVE_CPUID

#else
#error "only support x86/64 platform now"
#endif

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

#endif
