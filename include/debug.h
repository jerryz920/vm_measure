/******************************************************************************

 @File Name : {PROJECT_DIR}/include/debug.h

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :

*******************************************************************************/
#ifndef ZY_DEBUG_H
#define ZY_DEBUG_H

#include "config.h"

#ifdef USE_DEBUG
#include <assert.h>
#else
#define assert(x) ((void*)0)
#endif

#endif
