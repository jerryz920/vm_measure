
/*
 *  Size of the cache line
 */
#define BYTE_SIZE (1 << 8)

/*
 *  Parameters for cache/TLB
 */
#define CACHE_LINE      64
#define CACHE_MAX_DESC (BYTE_SIZE)
#define TLB_MAX_DESC   (BYTE_SIZE)

/*
 *  Check whether it has sched functions
 */
/* #undef HAVE_SCHED */

/*
 *  Check whether it supports CPUID
 */
#define HAVE_CPUID
