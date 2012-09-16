/******************************************************************************

 @File Name : {PROJECT_DIR}/vmaux.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :
   Helper routines that helps measure the virtual memory

*******************************************************************************/

#define _GNU_SOURCE
#include "misc.h"
#include "debug.h"
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***********************************************************************
 *  Scheduler Support
 ***********************************************************************/
#ifdef HAVE_SCHED
#include <sched.h>
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
#undef __GNU_SOURCE
#endif


/***********************************************************************
 *  Timing Support
 ***********************************************************************/
/*
 * @Deprecated
 *  High precision stamp. But hard to use because the TSC counter is
 *  not performing well
 */
#ifdef HAVE_RDTSC
uint64_t get_stamp()
{
  int hi, lo;
  __asm__ __volatile__ (
      "rdtsc\n"
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
  uint64_t d = t1->tv_sec - t2->tv_usec;
  return d * 1000000 - (t2->tv_usec - t1->tv_usec);
}

uint64_t spec_diff(const struct timespec* t1, const struct timespec* t2)
{
  uint64_t d = t1->tv_sec - t2->tv_sec;
  return d * 1000000000 - (t2->tv_nsec - t1->tv_nsec);
}

uint64_t get_usec()
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_usec + t.tv_sec * (uint64_t)1000000;
}

uint64_t get_nsec()
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return t.tv_nsec = t.tv_sec * (uint64_t)1000000;
}

/***********************************************************************
 *                    CPU dependent features
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
static void add_tlb_page_size(int key, int size)
{
  int pos = tlb_descs[key].page_size_cnt++;
  tlb_descs[key].page_size[pos] = size;
}

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
  ur >>= 8;
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
    fill_reserve_cache_desc(i);
  for (i = 0; i < TLB_MAX_DESC; i++) 
    fill_reserve_tlb_desc(i);
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
  add_tlb_page_size(0x50, 2048 * 1024);
  add_tlb_page_size(0x50, 4096 * 1024);
  fill_tlb_desc(0x51, 1, 128, 128, 4096, 0);
  add_tlb_page_size(0x51, 2048 * 1024);
  add_tlb_page_size(0x51, 4096 * 1024);
  fill_tlb_desc(0x52, 1, 256, 256, 4096, 0);
  add_tlb_page_size(0x52, 2048 * 1024);
  add_tlb_page_size(0x52, 4096 * 1024);

  fill_tlb_desc(0x55, 1, 7, 7, 2048 * 1024, 0);
  add_tlb_page_size(0x55, 4096 * 1024);
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
  add_tlb_page_size(0x5A, 4096 * 1024);
  fill_tlb_desc(0x5B, 1, 64, 64, 4096, 1);
  add_tlb_page_size(0x5B, 4096 * 1024);
  fill_tlb_desc(0x5C, 1, 128, 128, 4096, 1);
  add_tlb_page_size(0x5C, 4096 * 1024);
  fill_tlb_desc(0x5D, 1, 256, 256, 4096, 1);
  add_tlb_page_size(0x5D, 4096 * 1024);

  fill_cache_desc(0x60, 1, 16 * 1024, 8, 64, 1);
  fill_cache_desc(0x66, 1, 8 * 1024, 4, 64, 1);
  fill_cache_desc(0x67, 1, 16 * 1024, 4, 64, 1);
  fill_cache_desc(0x68, 1, 32 * 1024, 4, 64, 1);

  fill_cache_desc(0x70, 1, 12 * 1024, 8, 0, 0);
  fill_cache_desc(0x71, 1, 16 * 1024, 8, 0, 0);
  fill_cache_desc(0x72, 1, 32 * 1024, 8, 0, 0);

  fill_tlb_desc(0x76, 1, 8, 8, 2048 * 1024, 0);
  add_tlb_page_size(0x76, 4096 * 1024);

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

  fill_tlb_desc(0xC0, 1, 8, 4, 4096 , 0);
  add_tlb_page_size(0xC0, 4096 * 1024);
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
  fetch_my_memory_descs();
}

const struct CacheDesc* get_cache_desc(int key)
{
  return &cache_descs[key];
}

const struct TLBDesc* get_tlb_desc(int key)
{
  return &tlb_descs[key];
}

const int* get_mem_desc_keys(int* cnt)
{
  *cnt = my_desc_key_cnt;
  return my_desc_keys;
}
/***********************************************************************
 *          CPU related configurations
 ***********************************************************************/

static double do_test_cpu_freq(int ref_us)
{
  int i, cnt = 0;
  double total = 0;
  for (i = 0; i < CPUFREQ_DETECT_LOOP_SIZE; i++) {
    uint64_t sbegin = get_stamp();
    usleep(ref_us);
    uint64_t send = get_stamp();
    double tmp = (send - sbegin) / ref_us * 1E6; /* convert to Hz */
    if (tmp < CPUFREQ_MIN_HINT || tmp > CPUFREQ_MAX_HINT)
      continue;
    total += tmp;
    cnt++;
  }
  if (cnt == 0) return 0;
  return total / cnt;
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

  fprintf(stderr, "testing cpu frequency\n");
  /*
   *  When difference is still larger than 1M
   */
  while (fabs(next - prev) > 1E6 && ref_us < CPUFREQ_DETECT_TIME_UPPER * 1000) {
    double tmp = do_test_cpu_freq(ref_us);
    if (fabs(tmp) > CPUFREQ_MIN_HINT) {
      prev = next;
      next = tmp;
    }
    ref_us *= 2;
  }
  fprintf(stderr, "report cpu frequency to %.2fMHz\n", next / 1E6);

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

double cpu_freq(int cpu)
{
  return cpu_info.freq;
}



/***********************************************************************
 *              Memory Manipulation Interface
 ***********************************************************************/
/*
 *  Allocate pages, aligned to the size boundary
 */
char* alloc_pages(int cnt, int size)
{
  /*
   *  It's not necessary to allocate pages with mmap, if it's large enough
   */
  char* ptr = malloc(cnt * size);
  if (ptr)
    memset(ptr, -1, cnt * size);
  return ptr;
}

/*
 *  Use shmget like routine to allocate huge pages
 */
static int huge_page_index = 0;
struct HugePage* alloc_huge_pages(int cnt, int size)
{
  int shmid = shmget(IPC_PRIVATE, cnt * size, IPC_CREAT | SHM_HUGETLB | SHM_R | SHM_W);
  if (shmid < 0) {
    perror("get shm segment");
    return NULL;
  }

  struct HugePage* new_page = malloc(sizeof(struct HugePage));
  if (!new_page) {
    perror("allocate control block");
    shmctl(shmid, IPC_RMID, NULL);
    return NULL;
  }
  new_page->shmid = shmid;
  new_page->pcnt = cnt;
  new_page->psize = size;
  new_page->addr = shmat(shmid, NULL, 0);
  if (new_page->addr == (void*)-1) {
    perror("attach page");
    free(new_page);
    shmctl(shmid, IPC_RMID, NULL);
  }
  /*
   *  Warm the page
   */
  memset(new_page->addr, 0xcc, new_page->pcnt * new_page->psize);
  return new_page;
}

void free_huge_page(struct HugePage* page)
{
  shmdt(page->addr);
  shmctl(page->shmid, IPC_RMID, NULL);
  free(page);
}



int walk(const char* ptr, int len, int stride)
{
  int i, total_stride = len / stride;
  register int sum = 0;
  /*
   *  Maybe need to rewrite it to assembly to ensure:
   *  1. only one memory reference
   *  2. few register operations
   *  3. jump would not miss-predict
   *
   *  Consider to rewrite in assembly if compiler can
   *  not translate to register-base-scale addressing mode
   */
  for (i = 0; i <= total_stride - 8; i += 8, ptr += 8 * stride)
  {
    sum += *(int*) (ptr);
    sum += *(int*) (ptr + 1 * stride);
    sum += *(int*) (ptr + 2 * stride);
    sum += *(int*) (ptr + 3 * stride);
    sum += *(int*) (ptr + 4 * stride);
    sum += *(int*) (ptr + 5 * stride);
    sum += *(int*) (ptr + 6 * stride);
    sum += *(int*) (ptr + 7 * stride);
  }

  /*
   *  Might ruin the order a little, but does not matter
   */
  switch(total_stride - i) {
    case 7: sum += *(ptr + 7 * stride);
    case 6: sum += *(ptr + 6 * stride);
    case 5: sum += *(ptr + 5 * stride);
    case 4: sum += *(ptr + 4 * stride);
    case 3: sum += *(ptr + 3 * stride);
    case 2: sum += *(ptr + 2 * stride);
    case 1: sum += *(ptr + 1 * stride);
  }
  return sum;
}

typedef int (*SmallWalkFunc) (char* ptr, int stride, int loop);

/*
 *  for each loop, just walk 4 stride
 */
static int small_walk2(char* ptr, int stride, int loop)
{
  int i = 0, sum = 0;
  __asm__ __volatile__(
      "testl %2, %2\n"
      "jz end_loop\n"
      "walk_loop:\n"
      "addl (%3), %1\n"
      "incl %0\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "addl (%3,%4,3), %1\n"
      "addl (%3), %1\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "addl (%3,%4,3), %1\n"
      "cmpl %0, %2\n"
      "jne walk_loop\n"
      "end_loop:\n"
      : "=r"(i), "=r"(sum)
      : "r"(loop), "r"(ptr), "r"(stride)
      );
  return sum;
}

static int small_walk3(char* ptr, int stride, int loop)
{
  int i = 0, sum = 0;
  __asm__ __volatile__(
      "testl %2, %2\n"
      "jz end_loop\n"
      "movl $0, %0\n"
      "walk_loop:\n"
      "addl (%3), %1\n"
      "incl %0\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "addl (%3), %1\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "cmpl %0, %2\n"
      "jne walk_loop\n"
      "end_loop:\n"
      : "=r"(i), "=r"(sum)
      : "r"(loop), "r"(ptr), "r"(stride)
      );
  return sum;
}

static int small_walk4(char* ptr, int stride, int loop)
{
  int i = 0, sum = 0;
  __asm__ __volatile__(
      "testl %2, %2\n"
      "jz end_loop\n"
      "walk_loop:\n"
      "addl (%3), %1\n"
      "incl %0\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "addl (%3,%4,3), %1\n"
      "addl (%3,%4,4), %1\n"
      "addl (%3,%4,5), %1\n"
      "addl (%3,%4,6), %1\n"
      "addl (%3,%4,7), %1\n"
      "cmpl %0, %2\n"
      "jne walk_loop\n"
      "end_loop:\n"
      : "=r"(i), "=r"(sum)
      : "r"(loop), "r"(ptr), "r"(stride)
      );
  return sum;
}

static int small_walk5(char* ptr, int stride, int loop)
{
  int i = 0, sum = 0;
  char* tmp = ptr + 5 * stride;
  __asm__ __volatile__(
      "testl %2, %2\n"
      "jz end_loop\n"
      "walk_loop:\n"
      "addl (%3), %1\n"
      "incl %0\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "addl (%3,%4,3), %1\n"
      "addl (%3,%4,4), %1\n"
      "addl (%5), %1\n"
      "addl (%5,%4,1), %1\n"
      "addl (%5,%4,2), %1\n"
      "addl (%5,%4,3), %1\n"
      "addl (%5,%4,4), %1\n"
      "cmpl %0, %2\n"
      "jne walk_loop\n"
      "end_loop:\n"
      : "=r"(i), "=r"(sum)
      : "r"(loop), "r"(ptr), "r"(stride), "r"(tmp)
      );
  return sum;
}

static int small_walk6(char* ptr, int stride, int loop)
{
  int i = 0, sum = 0;
  char* tmp = ptr + 6 * stride;
  __asm__ __volatile__(
      "testl %2, %2\n"
      "jz end_loop\n"
      "walk_loop:\n"
      "addl (%3), %1\n"
      "incl %0\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "addl (%3,%4,3), %1\n"
      "addl (%3,%4,4), %1\n"
      "addl (%3,%4,5), %1\n"
      "addl (%5), %1\n"
      "addl (%5,%4,1), %1\n"
      "addl (%5,%4,2), %1\n"
      "addl (%5,%4,3), %1\n"
      "addl (%5,%4,4), %1\n"
      "addl (%5,%4,5), %1\n"
      "cmpl %0, %2\n"
      "jne walk_loop\n"
      "end_loop:\n"
      : "=r"(i), "=r"(sum)
      : "r"(loop), "r"(ptr), "r"(stride), "r"(tmp)
      );
  return sum;
}

static int small_walk7(char* ptr, int stride, int loop)
{
  int i = 0, sum = 0;
  char* tmp = ptr + 7 * stride;
  __asm__ __volatile__(
      "testl %2, %2\n"
      "jz end_loop\n"
      "walk_loop:\n"
      "addl (%3), %1\n"
      "incl %0\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "addl (%3,%4,3), %1\n"
      "addl (%3,%4,4), %1\n"
      "addl (%3,%4,5), %1\n"
      "addl (%3,%4,6), %1\n"
      "addl (%5), %1\n"
      "addl (%5,%4,1), %1\n"
      "addl (%5,%4,2), %1\n"
      "addl (%5,%4,3), %1\n"
      "addl (%5,%4,4), %1\n"
      "addl (%5,%4,5), %1\n"
      "addl (%5,%4,6), %1\n"
      "cmpl %0, %2\n"
      "jne walk_loop\n"
      "end_loop:\n"
      : "=r"(i), "=r"(sum)
      : "r"(loop), "r"(ptr), "r"(stride), "r"(tmp)
      );
  return sum;
}

static int small_walk8(char* ptr, int stride, int loop)
{
  int i = 0, sum = 0;
  char* tmp = ptr + 8 * stride;
  __asm__ __volatile__(
      "testl %2, %2\n"
      "jz end_loop\n"
      "walk_loop:\n"
      "addl (%3), %1\n"
      "incl %0\n"
      "addl (%3,%4,1), %1\n"
      "addl (%3,%4,2), %1\n"
      "addl (%3,%4,3), %1\n"
      "addl (%3,%4,4), %1\n"
      "addl (%3,%4,5), %1\n"
      "addl (%3,%4,6), %1\n"
      "addl (%3,%4,7), %1\n"
      "addl (%5), %1\n"
      "addl (%5,%4,1), %1\n"
      "addl (%5,%4,2), %1\n"
      "addl (%5,%4,3), %1\n"
      "addl (%5,%4,4), %1\n"
      "addl (%5,%4,5), %1\n"
      "addl (%5,%4,6), %1\n"
      "addl (%5,%4,7), %1\n"
      "cmpl %0, %2\n"
      "jne walk_loop\n"
      "end_loop:\n"
      : "=r"(i), "=r"(sum)
      : "r"(loop), "r"(ptr), "r"(stride), "r"(tmp)
    );
  return sum;
}

static int large_walk_func(char* ptr, int stride, int loop)
{
  int i = 0, j, sum = 0;
  for (i = 0; i < loop; i++) {
    char* tmp = ptr;
    for(j = 0; j <= stride - 8; j += 8, tmp += 8 * stride) {
      __asm__ __volatile(
          "addl (%1), %0\n"
          "addl (%1,%2,1), %0\n"
          "addl (%1,%2,2), %0\n"
          "addl (%1,%2,3), %0\n"
          "addl (%1,%2,4), %0\n"
          "addl (%1,%2,5), %0\n"
          "addl (%1,%2,6), %0\n"
          "addl (%1,%2,7), %0\n"
          : "=r"(sum)
          : "r"(tmp), "r"(stride)
          );
    }
    switch (stride - j) {
      case 7: sum += *(int*)(tmp + 7 * stride);
      case 6: sum += *(int*)(tmp + 6 * stride);
      case 5: sum += *(int*)(tmp + 5 * stride);
      case 4: sum += *(int*)(tmp + 4 * stride);
      case 3: sum += *(int*)(tmp + 3 * stride);
      case 2: sum += *(int*)(tmp + 2 * stride);
      case 1: sum += *(int*)(tmp + 1 * stride);
    }
  }
  return sum;
}

/*
 *  We just create small walk function for 2, 4, 8, 16, 32 way thrashing
 */
static SmallWalkFunc small_walk_funcs[9] = {
  NULL,
  NULL,
  walk2,
  walk3,
  walk4,
  walk5,
  walk6,
  walk7,
  walk8
};


int small_walk(char* ptr, int stride, int loop, int n)
{
  if (n < 2) return -1;
  if (n > 8) return large_walk_func(ptr, stride, loop);
  return small_walk_funcs[n](ptr, stride, loop);
}


/***********************************************************************
 *              Misc
 ***********************************************************************/
/*
 *  to cancel the optimization for dead defs
 */
void fake_use(void* ptr)
{
  __asm__ __volatile__ ("test %0, %0\n": "=r"(ptr));
}

/*
 *  It's actually undefined for num == 0
 */
uint8_t number_to_power(uint64_t num)
{
  uint8_t cnt = 0;
  if (num) {
    while ((num & 1) == 0) {
      num >>= 1;
      cnt++;
    }
  }
  return cnt;
}

uint64_t power_to_number(uint8_t power)
{
  if (power == -1) 
    return 0;
  else
    return ((uint64_t)1) << power;
}
