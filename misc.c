/******************************************************************************

 @File Name : {PROJECT_DIR}/tlb.c

 @Creation Date :
    2012.9.11

 @Created By: Zhai Yan

 @Purpose :

  Create a set of helper in measuring TLB/Cache behaviors.

*******************************************************************************/

/*
 *  We assume cache line is 64 byte effectively.
 *  @TODO
 *    make it configurable
 */
#define CACHE_LINE (64)

struct CacheDesc {
  int level;
  int size;
  int group;
};

struct TLBDesc {
  int level;
  int page_size;
  int cnt;
  int group;
};

/*
 *  The key to measure the cache issue is to make sure that TLB
 *  will hit each time. 
 */
CacheDesc* compute_next_level_cache(CacheDesc* last_level_cache)
{
  return NULL;
}

/*
 *  To measure the TLB size, we need to determine there is a
 *  capacity miss. So the address space we allocate should
 *  be more than the last level TLB. Same time, we want our
 *  TLB wouldn't fail due to set associative. So our first
 *  thing is to check the group size, then check out the
 *  capacity.
 *
 *  @TODO
 *    1. what if the group size of last level tlb is bigger
 *    than this level?
 *    answer: first make sure its capacity is missing. This
 *    is for sure. Then walk through the same group and see
 *    when it begin to thrash.
 *    2. inclusive vs exclusive TLB?
 */
TLBDesc* compute_next_level_tlb(TLBDesc* last_level_tlb)
{
  return NULL;
}

