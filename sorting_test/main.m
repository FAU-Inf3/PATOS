//#include <stdio.h>

#include "header.h"

//int main(int argc, char **argv)
//{
//    int items1[10] = {13, 3, 12, 11, 1991, 7, 28, 19, 24, 21};
//    sort<int, Comparator<int> >((int *)items1, 10);
//
//    double items2[10] = {13., 3., 12., 11., 1991., 7., 28., 19., 24., 21.};
//    sort<double, Comparator<double> >((double *)items2, 10);
//
//    for (unsigned i = 0; i < 10; ++i)
//    {
//        printf("%d ", items1[i]);
//    }
//    printf("\n");
//
//    for (unsigned i = 0; i < 10; ++i)
//    {
//        printf("%.1f ", items2[i]);
//    }
//    printf("\n");
//
//    return 0;
//}

template<typename DATA_T0, typename COMP>
__kernel void mykernel(DATA_T0 *items, int count)
{
    sort<DATA_T0, COMP>(items, count);
}

__kernel void test()
{

}
