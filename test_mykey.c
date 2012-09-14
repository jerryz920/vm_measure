/******************************************************************************

 @File Name : {PROJECT_DIR}/test.c

 @Creation Date :

 @Created By: Zhai Yan

 @Purpose :

*******************************************************************************/

#include "misc.h"
#include <stdio.h>

int main()
{
  int mykey_cnt;
  const int* mykeys = get_mem_desc_keys(&mykey_cnt);
  printf("key count %d\n", mykey_cnt);
  while (mykey_cnt-- > 0)
    printf("%d\n", mykeys[mykey_cnt]);
  return 0;
}
