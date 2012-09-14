/******************************************************************************

 @File Name : {PROJECT_DIR}/vmaux.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :
   Helper routines that helps measure the virtual memory

*******************************************************************************/

#include "misc.h"
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>


#ifdef HAVE_SCHED
#define __GNU_SOURCE
#include <sched.h>
#undef __GNU_SOURCE
#endif

/*
 *  Set the cpu affinity for calling thread
 */
int set_thread_cpu(int cpu)
{
  cpu_set_t target_cpu_set;
  CPU_ZERO(&target_cpu_set);
  CPU_SET(cpu, &target_cpu_set);
  return sched_setaffinity(0, sizeof(cpu_set_t), &target_cpu_set);
}

/*
 *  High precision stamp
 */
#ifdef HAVE_RDTSC
uint64_t get_stamp()
{
  int hi, lo;
  __asm__ __volatile__ (
      "rdtscp\n"
      "movl %%eax, %0\n"
      "movl %%edx, %1\n"
      : "=&g"(lo), "=g"(hi):: "eax", "edx");
  return (((uint64_t)hi) << 32) | lo;
}
#endif


/*
 *  
 */
uint64_t time_diff(const struct timeval* t1, const struct timeval* t2)
{
}


/***********************************************************************
 *  CPU dependent features
 *    Currently just support x86/64 platform with CPUID and RDTSC
 ***********************************************************************/
/*
 *  Records things from cpu id
 */
static struct TLBDesc tlb_descs[TLB_MAX_DESC];
static struct CacheDesc cache_descs[CACHE_MAX_DESC];
static struct CPUDesc cpu_info; /* we assume it's homogeneous architecture */
static int my_desc_keys[TLB_MAX_DESC + CACHE_MAX_DESC];
static int my_desc_key_cnt;


static void init_memory_descs() __attribute__((constructor));
static void init_cpu_descs() __attribute__((constructor));


/***********************************************************************
 *            Cache/TLB related configurations
 ***********************************************************************/
static void fill_cache_desc(int key, int level, int size, int group_size, int line_size, int is_data)
{
  cache_descs[key].level = level;
  cache_descs[key].size = size;
  cache_descs[key].group_size = group_size;
  cache_descs[key].inclusive = 0; /* don't consider inclusive cache right now*/
  cache_descs[key].line_size = line_size;
  cache_descs[key].data_cache = is_data;
}

static void fill_tlb_desc(int key, int level, int size, int group_size, int page_size, int is_data)
{
  tlb_descs[key].level = level;
  tlb_descs[key].size = size;
  tlb_descs[key].group_size = group_size;
  tlb_descs[key].data_tlb = is_data;
  add_tlb_page_size(key, page_size);
}

static void add_tlb_page_size(int key, int size)
{
  int pos = tlb_descs[key].page_size_cnt++;
  tlb_descs[key].page_size[pos] = size;
}

static void fill_reserve_cache_desc(int key)
{
  cache_descs[key].level = -1;
}

static void fill_reserve_tlb_desc(int key)
{
  tlb_descs[key].level = -1;
}

/*
 *  Decode a register value for memory descriptor.
 *  For information, please reference the CPUID
 *  document, branch 2 information and formats
 */
static int decode_one_register(int r, int* keys)
{
  int cnt = 0;
  uint32_t ur = r;
  if (r < 0)
    return 0;

next_desc:
  if (ur & 0xff) {
    *keys++ = ur & 0xff;
    cnt++;
  }
  /*
   *  This applies for positive number only
   */
  if (ur) goto next_desc;
  return cnt;
}


/*
 *  Use CPUID to fetch all the memory descriptors about
 *  cache and TLB information
 */
static void fetch_my_memory_descs()
{
  int eax, ebx, ecx, edx;
  uint8_t cnt;

  my_desc_key_cnt = 0;
more_descs:
  /*
   *  Here we force all the variables to be memory location, to avoid
   *  early clobber things.
   */
  __asm__ __volatile__(
      "movl $2, %%eax\n"
      "cpuid\n"
      "movl %%eax, %0\n"
      "movl %%ebx, %1\n"
      "movl %%ecx, %2\n"
      "movl %%edx, %3\n"
      "movb %%al, %4\n"
      : "=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx), "=m"(cnt), "=m"(cnt)
      :
      : "eax", "ebx", "ecx", "edx"
      );

  /*
   *  Decoding keys, clear the LSB of eax
   */
  eax &= 0xffffff00;
  /*
   *  Don't worry about the expression, Only one sequence point inside
   *  each += expression. So it's not undefined behavior here.
   */
  my_desc_key_cnt += decode_one_register(eax, &my_desc_keys[my_desc_key_cnt]);
  my_desc_key_cnt += decode_one_register(ebx, &my_desc_keys[my_desc_key_cnt]);
  my_desc_key_cnt += decode_one_register(ecx, &my_desc_keys[my_desc_key_cnt]);
  my_desc_key_cnt += decode_one_register(edx, &my_desc_keys[my_desc_key_cnt]);

  /*
   *  Do more sequence
   */
  if (cnt > 1)
    goto more_descs;
}

/*
 *  @Init
 */
static void init_memory_descs()
{
  int i;
  for (i = 0; i < CACHE_MAX_DESC; i++)
    fill_reserve_cache_desc(key);
  for (i = 0; i < TLB_MAX_DESC; i++) 
    fill_reserve_tlb_desc(key);
  /*
   *  All the following content comes from the Intel manual about cpuid
   */
  fill_tlb_desc(0x01, 1, 32, 4, 4096,        0);
  fill_tlb_desc(0x02, 1, 2,  2, 4096 * 1024, 0);
  fill_tlb_desc(0x03, 1, 64, 4, 4096,        1);
  fill_tlb_desc(0x04, 1, 8,  4, 4096 * 1024, 1);
  fill_tlb_desc(0x05, 1, 32, 4, 4096 * 1024, 1);

  fill_cache_desc(0x06, 1, 8  * 1024, 4, 32, 0);
  fill_cache_desc(0x08, 1, 16 * 1024, 4, 32, 0);
  fill_cache_desc(0x09, 1, 32 * 1024, 4, 64, 0);
  fill_cache_desc(0x0A, 1, 8  * 1024, 2, 32, 1);

  fill_tlb_desc(0x0B, 1, 4, 4, 4096 * 1024, 0);

  fill_cache_desc(0x0C, 1, 16 * 1024, 4, 32, 1);
  fill_cache_desc(0x0D, 1, 16 * 1024, 4, 64, 1);
  fill_cache_desc(0x0E, 1, 24 * 1024, 6, 64, 1);

  fill_cache_desc(0x0C, 1, 16 * 1024, 4, 32, 1);
  fill_cache_desc(0x0D, 1, 16 * 1024, 4, 64, 1);
  fill_cache_desc(0x0E, 1, 24 * 1024, 6, 64, 1);

  fill_cache_desc(0x21, 2, 256 * 1024, 8, 64, 1);
  fill_cache_desc(0x22, 3, 512 * 1024, 4, 64, 1);
  fill_cache_desc(0x23, 3, 1024 * 1024, 8, 64, 1);
  fill_cache_desc(0x25, 3, 2048 * 1024, 8, 64, 1);
  fill_cache_desc(0x29, 3, 4096 * 1024, 8, 64, 1);
  fill_cache_desc(0x2C, 1, 32 * 1024, 8, 64, 1);
  fill_cache_desc(0x30, 1, 32 * 1024, 8, 64, 0);


  /* This entry means no 2nd or 3rd level cache...
   * fill_cache_desc(0x40, -1, 32 * 1024, 8, 64, 0); */

  fill_cache_desc(0x41, 2, 128 * 1024, 4, 32, 1);
  fill_cache_desc(0x42, 2, 256 * 1024, 4, 32, 1);
  fill_cache_desc(0x43, 2, 512 * 1024, 4, 32, 1);
  fill_cache_desc(0x44, 2, 1024 * 1024, 4, 32, 1);
  fill_cache_desc(0x45, 2, 2048 * 1024, 4, 32, 1);

  fill_cache_desc(0x46, 3, 4096 * 1024, 4, 64, 1);
  fill_cache_desc(0x47, 3, 8192 * 1024, 8, 64, 1);
  fill_cache_desc(0x48, 2, 3072 * 1024, 12, 64, 1);
  /* This entry can also points to 2nd level cache... Stupid! */
  fill_cache_desc(0x49, 3, 4096 * 1024, 16, 64, 1);

  fill_cache_desc(0x4A, 3, 6144 * 1024, 12, 64, 1);
  fill_cache_desc(0x4B, 3, 8192 * 1024, 16, 64, 1);
  fill_cache_desc(0x4C, 3, 12 * 1024 * 1024, 12, 64, 1);
  fill_cache_desc(0x4D, 3, 16 * 1024 * 1024, 16, 64, 1);

  fill_cache_desc(0x4E, 2, 6 * 1024 * 1024, 24, 64, 1);

  fill_tlb_desc(0x4F, 1, 32, 32, 4096, 0);
  /* Can also be used for 2MB and 4MB page */
  fill_tlb_desc(0x50, 1, 64, 64, 4096, 0);
  fill_tlb_desc(0x51, 1, 128, 128, 4096, 0);
  fill_tlb_desc(0x52, 1, 256, 256, 4096, 0);

  fill_tlb_desc(0x55, 1, 7, 7, 2048 * 1024, 0);
  fill_tlb_desc(0x56, 1, 16, 4, 4096 * 1024, 1);
  fill_tlb_desc(0x57, 1, 16, 4, 4096, 1);

  /*
   *  What does tlb0 means?
   */
  fill_tlb_desc(0x59, 1, 16, 16, 4096, 1);
  /*
   *  5A-5D Can have multiple page size
   */
  fill_tlb_desc(0x5A, 1, 32, 4, 2048 * 1024, 1);
  fill_tlb_desc(0x5B, 1, 64, 64, 4096, 1);
  fill_tlb_desc(0x5C, 1, 128, 128, 4096, 1);
  fill_tlb_desc(0x5D, 1, 256, 256, 4096, 1);

  fill_cache_desc(0x60, 1, 16 * 1024, 8, 64, 1);
  fill_cache_desc(0x66, 1, 8 * 1024, 4, 64, 1);
  fill_cache_desc(0x67, 1, 16 * 1024, 4, 64, 1);
  fill_cache_desc(0x68, 1, 32 * 1024, 4, 64, 1);

  fill_cache_desc(0x70, 1, 12 * 1024, 8, 0, 0);
  fill_cache_desc(0x71, 1, 16 * 1024, 8, 0, 0);
  fill_cache_desc(0x72, 1, 32 * 1024, 8, 0, 0);

  fill_tlb_desc(0x76, 1, 8, 8, 2048 * 1024, 0);

  fill_cache_desc(0x78, 2, 1024 * 1024, 4, 64, 1);
  fill_cache_desc(0x79, 2, 128 * 1024, 8, 64, 1);
  fill_cache_desc(0x7a, 2, 256 * 1024, 8, 64, 1);
  fill_cache_desc(0x7b, 2, 512 * 1024, 8, 64, 1);
  fill_cache_desc(0x7c, 2, 1024 * 1024, 8, 64, 1);
  fill_cache_desc(0x7d, 2, 2048 * 1024, 8, 64, 1);

  fill_cache_desc(0x7f, 2, 512 * 1024, 2, 64, 1);
  fill_cache_desc(0x80, 2, 512 * 1024, 8, 64, 1);

  fill_cache_desc(0x82, 2, 256 * 1024, 8, 32, 1);
  fill_cache_desc(0x83, 2, 512 * 1024, 8, 32, 1);
  fill_cache_desc(0x84, 2, 1024 * 1024, 8, 32, 1);
  fill_cache_desc(0x85, 2, 2048 * 1024, 8, 32, 1);
  fill_cache_desc(0x86, 2, 512 * 1024, 4, 64, 1);
  fill_cache_desc(0x87, 2, 1024 * 1024, 8, 64, 1);

  fill_tlb_desc(0xB0, 1, 128, 4, 4096 , 0);
  fill_tlb_desc(0xB1, 1, 4, 4, 2048 * 1024, 0);
  fill_tlb_desc(0xB2, 1, 64, 4, 4096, 0);
  fill_tlb_desc(0xB3, 1, 128, 4, 4096, 1);
  fill_tlb_desc(0xB4, 1, 256, 4, 4096, 1);

  fill_tlb_desc(0xBA, 1, 64, 4, 4096, 0);

  fill_tlb_desc(0xC0, 1, 8, 4, 4096 * 1024, 0);
  /*
   *  This is a shared TLB. Any comment on that?
   */
  fill_tlb_desc(0xCA, 2, 512, 4, 4096, 1);

  fill_cache_desc(0xD0, 3, 512 * 1024, 4, 64, 1);
  fill_cache_desc(0xD1, 3, 1024 * 1024, 4, 64, 1);
  fill_cache_desc(0xD2, 3, 2048 * 1024, 4, 64, 1);

  fill_cache_desc(0xD6, 3, 1024 * 1024, 8, 64, 1);
  fill_cache_desc(0xD7, 3, 2048 * 1024, 8, 64, 1);
  fill_cache_desc(0xD8, 3, 4096 * 1024, 8, 64, 1);

  fill_cache_desc(0xDC, 3, 3 * 512 * 1024, 12, 64, 1);
  fill_cache_desc(0xDD, 3, 6 * 512 * 1024, 12, 64, 1);
  fill_cache_desc(0xDE, 3, 12 * 512 * 1024, 12, 64, 1);

  fill_cache_desc(0xE2, 3, 2048 * 1024, 16, 64, 1);
  fill_cache_desc(0xE3, 3, 4096 * 1024, 16, 64, 1);
  fill_cache_desc(0xE4, 3, 8192 * 1024, 16, 64, 1);

  fill_cache_desc(0xEA, 3, 12 * 1024 * 1024, 24, 64, 1);
  fill_cache_desc(0xEB, 3, 18 * 1024 * 1024, 24, 64, 1);
  fill_cache_desc(0xEC, 3, 24 * 1024 * 1024, 24, 64, 1);
}

/***********************************************************************
 *          CPU related configurations
 ***********************************************************************/

static double do_test_cpu_freq(int ref_us)
{
  int i;
  double total = 0;
  for (i = 0; i < CPUFREQ_DETECT_LOOP_SIZE; i++) {
    uint64_t sbegin = get_stamp();
    usleep(ref_us);
    uint64_t send = get_stamp();
    total += (send - sbegin) / ref_us * 1E6; /* convert to Hz */
  }
  return total / CPUFREQ_DETECT_LOOP_SIZE;
}

/*
 *  Warning: this function is fully implemented by
 *  magic stuff. Do not rely on this without careful
 *  adjustment. The fact we can not use the marked
 *  frequency on label directly is because modern
 *  CPUs have multiple state, and different scale in
 *  favor of power saving. So we have to measure this
 *  ourselves.
 */
static double test_cpu_freq()
{
  /* compute the frequency, get the average freq */
  double prev = 0, next = 1E10;
  int ref_us = 1; /* start from 1us */

  /*
   *  When difference is still larger than 1M
   */
  while (fabs(next - prev) > 1E6 && ref_us < CPUFREQ_DETECT_TIME_UPPER * 1000) {
    prev = next;
    next = test_cpu_freq(ref_us);
    ref_us *= 2;
  }

  return next;
}

/*
 *  @Init
 */
static void init_cpu_descs() 
{
  cpu_info.freq = test_cpu_freq();
  /*
   *  Lazy initialize
   */
  cpu_info.prefetch_size = -1;
}

/*
 *  Read the CPU frequency
 */
struct CPUDesc* get_cpu_desc(int cpu)
{
  return &cpu_info;
}

/*
 *  TODO
 *    Use CPUID branch 2, encoding is F0/F1
 */
int hardware_prefetch_size()
{
  if (cpu_info.prefetch_size == -1) {
    int i;
    for (i = 0; i < my_desc_key_cnt; i++) 
      if (my_desc_keys[i] == 0xf0) {
        cpu_info.prefetch_size = 64;
        break;
      } else if (my_desc_keys[i] == 0xf1) {
        cpu_info.prefetch_size = 128;
        break;
      }
  }
  return cpu_info.prefetch_size;
}

double cpu_get_freq(int cpu)
{
  return cpu_info.freq;
}

