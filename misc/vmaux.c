/******************************************************************************

 @File Name : {PROJECT_DIR}/vmaux.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :
   Helper routines that helps measure the virtual memory

*******************************************************************************/

#include "misc.h"
#include <time.h>

#ifdef HAVE_SCHED

#define __GNU_SOURCE
#include <sched.h>

/*
 *  Set the cpu affinity for process
 */
void set_cpu(int cpu)
{


}
#endif

/*
 *  High precision stamp
 */
uint64_t get_stamp()
{
}

/*
 *  Read the CPU frequency
 */
uint64_t get_cpu_freq()
{
}


/*
 *  
 */
uint64_t time_diff(const struct timeval* t1, const struct timeval* t2)
{
}


/***************************************************
 *  CPU dependent features
 *    Currently just support x86/64 platform with
 *    CPUID
 ***************************************************/

#ifdef HAVE_CPUID

struct CacheDesc {
  int size;
  int group_size;
  int inclusive;
};

struct TLBCache {
  int size;
  int group_size;
  int page_size;
  int data_tlb;
};

static struct TLBCache tlb_descs[MAX_DESC




#else
#error "only support x86/64 platform now"
#endif




