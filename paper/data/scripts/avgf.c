/******************************************************************************

 @File Name : {PROJECT_DIR}/avgf.c

 @Creation Date : 20-09-2012

 @Created By: Zhai Yan

 @Purpose :

*******************************************************************************/

#include <stdio.h>

int main()
{
  double a,b ;
  double sum1 = 0, sum2 = 0;
  int cnt = 0;
  while ((scanf("%lf%lf", &a, &b)) == 2) {
    sum1 += a; sum2 += b;
    cnt++;
  }
  printf("%.4f %.4f\n", sum1 / cnt, sum2 / cnt);
  return 0;
}
