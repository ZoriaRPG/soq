/* SO 4103-2621 */
/* Variant 1 - minimal printing */
#include <stdio.h>
#include <stdbool.h>

enum { MAX_ITERATION = 10000000 };

static bool duplicate_digits_1(int value)
{
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%d", value);
    char digits[10] = { 0 };
    char *ptr = buffer;
    char c;
    while ((c = *ptr++) != '\0')
    {
        if (++digits[c - '0'] > 1)
            return true;
    }
    return false;
}

int main(void)
{
    int count = 0;
    for (int i = 0; i < MAX_ITERATION; i++)
    {
        if (!duplicate_digits_1(i))
            count++;
    }
    printf("%d\n", count);
    return 0;
}
