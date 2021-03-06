#include <stdio.h>

#define DIM(x)  (sizeof(x) / sizeof(*(x)))

static
void bsort_2(int *A, int N)
{
    int opos[N];
    int odds = 0;
    for (int i = 0; i < N; i++)
    {
        if ((A[i] & 1) == 1)
            opos[odds++] = i;
    }
    if (odds < 2)
        return;
    for (int i = 0; i < odds - 1; i++)
    {
        for (int j = 0; j < odds - 1 - i; j++)
        {
            if (A[opos[j + 1]] < A[opos[j]])
            {
                int t = A[opos[j + 1]];
                A[opos[j + 1]] = A[opos[j]];
                A[opos[j]] = t;
            }
        }
    }
}

static void dump_array(const char *tag, const int *a, int n)
{
    printf("%-8s", tag);
    for (int i = 0; i < n; i++)
        printf(" %3d", a[i]);
    putchar('\n');
}

int main(void)
{
    int array[] =
    {
        13,   5,  -3,   4,   2,  -7,  23, -33,   1,   6,
        -8,   7,  10, -51,   9,  -5,  12,  92,  69,  55,
    };

    dump_array("Before:", array, DIM(array));
    bsort_2(array, DIM(array));
    dump_array("After:", array, DIM(array));

    return 0;
}
