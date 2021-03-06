/* SO 2016-4204 */
#define DEBUG

#include "posixver.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define WRAPPED_HEADER "range.h"
#include "wraphead.h"
#define WRAPPED_HEADER "stderr.h"
#include "wraphead.h"
#define WRAPPED_HEADER "debug.h"
#include "wraphead.h"

typedef enum { WHITE, BLACK, RED } Colour;

static char colour_code(Colour c);
static void trace_colours(FILE *fp, char const *tag, Colour *data, unsigned num, size_t w, size_t r, size_t c);

static inline void swap(Colour *p1, Colour *p2)
{
    Colour tmp;
    tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
}

static void partition3(size_t n, Colour *arr)
{
    if (n <= 1)
        return;

    size_t w = 0;
    size_t r = n;

    DB_TRACE(1, "\nw0 = %zu; r0 = %zu: ", w, r);
    while (w < r && arr[w] == WHITE)
        w++;
    while (r > w && arr[r-1] == RED)
        r--;
    DB_TRACE(1, "w1 = %zu; r1 = %zu\n", w, r);

    for (size_t i = w; i < r; i++)
    {
        DB_TRACE(1, "i = %2zu [1] w = %2zu, r = %2zu, c = %c", i, w, r, colour_code(arr[i]));
        DB_CALL(1, trace_colours(stderr, "", arr, n, w, r, i));
        if (arr[i] == RED)
        {
            swap(&arr[i], &arr[--r]);
            DB_TRACE(1, "i = %2zu [2] w = %2zu, r = %2zu, c = %c", i, w, r, colour_code(arr[i]));
            DB_CALL(1, trace_colours(stderr, "", arr, n, w, r, i));
        }
        if (arr[i] == WHITE)
        {
            swap(&arr[i], &arr[w++]);
            DB_TRACE(1, "i = %2zu [3] w = %2zu, r = %2zu, c = %c", i, w, r, colour_code(arr[i]));
            DB_CALL(1, trace_colours(stderr, "", arr, n, w, r, i));
        }
        while (r > w && arr[r-1] == RED)
            r--;
        while (w < r && arr[w] == WHITE)
            w++;
        if (i < w && w > 0)
            i = w - 1;
    }
}

/* DEBUGGING and TEST infrastructure */

static char const *colour_name(Colour c)
{
    return(c == WHITE ? "WHITE" : c == RED ? "RED" : "BLACK");
}

static char colour_code(Colour c)
{
    return(c == WHITE ? 'W' : c == RED ? 'R' : 'B');
}

static void print_colours(FILE *fp, char const *tag, Colour *data, size_t num)
{
    fprintf(fp, "%s: (%zu)", tag, num);
    for (size_t i = 0; i < num; i++)
        fprintf(fp, " %c", colour_code(data[i]));
    putc('\n', fp);
}

static void dump_colours(char const *tag, Colour *data, size_t num)
{
    print_colours(stdout, tag, data, num);
}

static void trace_colours(FILE *fp, char const *tag, Colour *data, unsigned num, size_t w, size_t r, size_t c)
{
    assert(w <= r);
    assert(r <= num);
    fprintf(fp, "%s: ", tag);
    for (unsigned i = 0; i < num; i++)
    {
        char pad = ' ';
        if (i == w || i == r)
            pad = '|';
        if (i == c)
            pad = '[';
        if (i == c+1)
            pad = ']';
        fprintf(fp, "%c%c", pad, colour_code(data[i]));
    }
    if (c == num - 1)
        putc(']', fp);
    else if (r == num || w == num)
        putc('|', fp);
    putc('\n', fp);
}

static Colour *dup_sequence(size_t n, Colour const *a)
{
    Colour *d = (Colour *)malloc(n * sizeof(*d));   /*=C++=*/
    if (d == 0)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    for (size_t i = 0; i < n; i++)
        d[i] = a[i];
    return d;
}

static bool is_invalid_sequence(size_t n, Colour *a, bool report)
{
    bool rc = false;
    size_t w;
    for (w = 0; w < n; w++)
    {
        if (a[w] != WHITE)
            break;
    }

    size_t b;
    for (b = w; b < n; b++)
    {
        if (a[b] == WHITE)
        {
            if (report)
            {
                fprintf(stderr, "Error: %c out of position (%zu)", colour_code(a[b]), b);
                print_colours(stderr, "", a, n);
            }
            rc = true;
        }
        if (a[b] != BLACK)
            break;
    }

    size_t r;
    for (r = b; r < n; r++)
    {
        if (a[r] != RED)
        {
            if (report)
            {
                fprintf(stderr, "Error: %c out of position (%zu)", colour_code(a[r]), r);
                print_colours(stderr, "", a, n);
            }
            rc = true;
        }
    }

    return rc;
}

static size_t seqno = 0;
static bool wflag = false;
static bool verbose = false;

typedef struct Test
{
    Colour *data;
    size_t size;
} Test;

static void write_sequence(size_t seq, size_t n, Colour *a)
{
    size_t i;
    printf("Colour seq_%03zu[] =\n{\n", seq);
    for (i = 0; i < n; i++)
    {
        printf(" %s,", colour_name(a[i]));
        if (i % 10 == 9)
            putchar('\n');
    }
    if (i %10 != 0)
        putchar('\n');
    printf("};\n");
}

static bool test_sequence(Test t)
{
    bool rc = true;
    size_t n = t.size;
    Colour *a = t.data;
    Colour *d = dup_sequence(n, a);
    if (verbose)
        dump_colours("Before", a, n);
    partition3(n, d);
    if (verbose)
        dump_colours("After ", d, n);
    if (is_invalid_sequence(n, d, false))
    {
        if (!verbose)
            dump_colours("Before", a, n);
        is_invalid_sequence(n, d, true);
        if (!verbose)
            dump_colours("After ", d, n);
        if (wflag)
            write_sequence(++seqno, n, a);
        rc = false;
    }
    free(d);
    return rc;
}

static size_t fixed_tests(char const *range, size_t *counter)
{
    size_t fail = 0;

    Colour seq_001[] = { WHITE, BLACK, RED };
    Colour seq_002[] = { WHITE, WHITE, WHITE };
    Colour seq_003[] = { RED, RED, RED };
    Colour seq_004[] = { BLACK, BLACK, BLACK };
    Colour seq_005[] = { RED, BLACK, WHITE };
    Colour seq_006[] = { WHITE, WHITE, RED, RED, BLACK, BLACK, WHITE };
    Colour seq_007[] =
    {
        BLACK, BLACK, WHITE, WHITE, RED, RED, BLACK, BLACK, WHITE,
        BLACK, BLACK,
    };
    Colour seq_008[] = { WHITE, BLACK };
    Colour seq_009[] = { BLACK, BLACK, RED, RED, WHITE, WHITE, RED };
    Colour seq_010[] = { BLACK, BLACK, RED, WHITE, RED };
    Colour seq_011[] =
    {
        RED, BLACK, RED, WHITE, RED, RED, BLACK, WHITE, RED, BLACK, RED,
        BLACK, BLACK, RED, BLACK, WHITE, BLACK, WHITE, WHITE, WHITE,
        WHITE, RED, RED, RED, RED, BLACK, WHITE
    };
    Colour seq_012[] =
    {
        WHITE, WHITE, RED, WHITE, RED, BLACK, RED, BLACK, WHITE, BLACK,
        RED, RED, RED, WHITE, RED, RED, BLACK, BLACK, BLACK, RED, RED,
        BLACK, BLACK, WHITE, WHITE, RED, WHITE, BLACK, RED, BLACK,
        WHITE, RED, WHITE, WHITE, RED, WHITE, BLACK, RED, RED, RED,
        WHITE,
    };
    Colour seq_013[] = { RED, WHITE, RED, };
    Colour seq_014[] = { RED, WHITE, };
    Colour seq_015[] = { RED, BLACK, };
    Colour seq_016[] = { RED, RED, };
    Colour seq_017[] = { BLACK, WHITE, };
    Colour seq_018[] = { BLACK, BLACK, };
    Colour seq_019[] = { BLACK, RED, };
    Colour seq_020[] = { WHITE, WHITE, };
    Colour seq_021[] = { WHITE, RED, };
    Colour seq_022[] = { RED, WHITE, RED, WHITE, };
    Colour seq_023[] =
    {
        RED, WHITE, RED, WHITE, RED, RED, WHITE, WHITE, WHITE,
    };
    Test tests[] =
    {
        { seq_001, sizeof(seq_001)/sizeof(seq_001[0]) },
        { seq_002, sizeof(seq_002)/sizeof(seq_002[0]) },
        { seq_003, sizeof(seq_003)/sizeof(seq_003[0]) },
        { seq_004, sizeof(seq_004)/sizeof(seq_004[0]) },
        { seq_005, sizeof(seq_005)/sizeof(seq_005[0]) },
        { seq_006, sizeof(seq_006)/sizeof(seq_006[0]) },
        { seq_007, sizeof(seq_007)/sizeof(seq_007[0]) },
        { seq_008, sizeof(seq_008)/sizeof(seq_008[0]) },
        { seq_009, sizeof(seq_009)/sizeof(seq_009[0]) },
        { seq_010, sizeof(seq_010)/sizeof(seq_010[0]) },
        { seq_011, sizeof(seq_011)/sizeof(seq_011[0]) },
        { seq_012, sizeof(seq_012)/sizeof(seq_012[0]) },
        { seq_013, sizeof(seq_013)/sizeof(seq_013[0]) },
        { seq_014, sizeof(seq_014)/sizeof(seq_014[0]) },
        { seq_015, sizeof(seq_015)/sizeof(seq_015[0]) },
        { seq_016, sizeof(seq_016)/sizeof(seq_016[0]) },
        { seq_017, sizeof(seq_017)/sizeof(seq_017[0]) },
        { seq_018, sizeof(seq_018)/sizeof(seq_018[0]) },
        { seq_019, sizeof(seq_019)/sizeof(seq_019[0]) },
        { seq_020, sizeof(seq_020)/sizeof(seq_020[0]) },
        { seq_021, sizeof(seq_021)/sizeof(seq_021[0]) },
        { seq_022, sizeof(seq_022)/sizeof(seq_022[0]) },
        { seq_023, sizeof(seq_023)/sizeof(seq_023[0]) },
    };
    enum { NUM_TESTS = sizeof(tests) / sizeof(tests[0]) };

    *counter = 0;
    if (range != 0)
    {
        const char *ptr = range;
        const char *nxt;
        long lo;
        long hi;
        while ((nxt = parse_range(ptr, &lo, &hi)) != 0)
        {
            if (nxt == ptr)
                err_error("invalid range string (%s)\n", range);
            if (hi == 0)
                hi = NUM_TESTS-1;
            for (long i = lo; i <= hi; i++)
            {
                (*counter)++;
                if (test_sequence(tests[i]) == false)
                {
                    printf("Test %ld: Failed\n", i);
                    fail++;
                }
            }
            ptr = nxt;
        }
    }
    else
    {
        for (size_t i = 0; i < NUM_TESTS; i++)
        {
            (*counter)++;
            if (test_sequence(tests[i]) == false)
            {
                printf("Test %ld: Failed\n", i);
                fail++;
            }
        }
    }

    return fail;
}

static size_t random_tests(size_t seed, size_t number, size_t maxsize)
{
    size_t fail = 0;
    srand(seed);
    printf("Seed: %zu\n", seed);

    for (size_t i = 0; i < number; i++)
    {
        Test t;
        t.size = rand() % maxsize;
        t.data = (Colour *)malloc(t.size * sizeof(*t.data));    /*=C++=*/
        if (t.data == 0)
        {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
        if (verbose)
            printf("Test: %zu (%zu)\n", i, t.size);
        for (size_t j = 0; j < t.size; j++)
            t.data[j] = (Colour)(rand() % 3);
        if (test_sequence(t) == false)
        {
            fail++;
            break;
        }
    }
    return fail;
}

int main(int argc, char **argv)
{
    static char const optstr[] = "dfm:n:o:rs:t:vw";
    static char const usestr[] = "[-dfrvw][-m maxsize][-n number][-s seed][-t tests]";
    char const *range = 0;
    unsigned seed = time(0);
    size_t number = 1000;
    size_t maxsize = 100;
    bool fixed = true;
    bool random = true;
    int opt;

    err_setarg0(argv[0]);

    while ((opt = getopt(argc, argv, optstr)) != -1)
    {
        switch (opt)
        {
        case 'd':
            db_setdebug(1);
            verbose = 1;
            break;
        case 'f':
            fixed = false;
            break;
        case 'm':
            maxsize = strtoul(optarg, 0, 0);
            break;
        case 'n':
            number = strtoul(optarg, 0, 0);
            break;
        case 'r':
            random = false;
            break;
        case 's':
            seed = atoi(optarg);
            break;
        case 't':
            range = optarg;
            break;
        case 'v':
            verbose = true;
            break;
        case 'w':
            wflag = true;
            break;
        default:
            err_usage(usestr);
            break;
        }
    }
    if (optind != argc)
        err_usage(usestr);

    size_t fail = 0;

    if (fixed)
    {
        size_t counter;
        fail = fixed_tests(range, &counter);
        printf("Failures: %zu in %zu fixed tests\n", fail, counter);
    }
    if (fail == 0 && random)
    {
        fail = random_tests(seed, number, maxsize);
        printf("Failures: %zu in %zu random tests\n", fail, number);
    }

    return 0;
}

