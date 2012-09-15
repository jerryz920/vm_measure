/******************************************************************************

 @File Name : {PROJECT_DIR}/include/debug.h

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :

*******************************************************************************/
#ifndef ZY_DEBUG_H
#define ZY_DEBUG_H

#include "config.h"
#include <stdio.h>


#ifdef USE_DEBUG
#include <assert.h>
#define DEBUG(fmt, ...) fprintf(stderr, "debug %s-%s-%d:\n"fmt, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define ERR(fmt, ...)   fprintf(stderr, "error %s-%s-%d:\n"fmt, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define assert(x) ((void*)0)
#define DEBUG(x,...) ((void*)0)
#define ERR(fmt, ...)   fprintf(stderr, "error:\n"fmt, ##__VA_ARGS__)
#endif

#endif
