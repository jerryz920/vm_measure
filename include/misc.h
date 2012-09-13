/******************************************************************************

 @File Name : {PROJECT_DIR}/include/test.h

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :

 Just to test project build

*******************************************************************************/
#ifndef ZY_TEST_H
#define ZY_TEST_H

#ifdef HAVE_SCHED
extern void set_cpu(int index);
#else
#define set_cpu(x) ((void*)0)
#endif

#endif
