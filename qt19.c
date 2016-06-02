/* SO 37577522 */
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static const double CTR_X =    0.0;
static const double CTR_Y =    0.0;
static const double WIDTH = +100.0;

typedef struct Particle Particle;
typedef struct Node Node;

struct Particle
{
    double mass;
    double x_pos;
    double y_pos;
    double x_vel;
    double y_vel;
};

struct Node
{
    Particle *p;
    double    center_x;
    double    center_y;
    double    width;
    Node     *sw;
    Node     *nw;
    Node     *se;
    Node     *ne;
};

static inline bool in_box(const Particle *p, double center_x, double center_y, double width)
{
    return((p->x_pos >= center_x - width && p->x_pos < center_x + width) &&
           (p->y_pos >= center_y - width && p->y_pos < center_y + width));
}

static void check_in_box(const Particle *p, double center_x, double center_y, double width)
{
        if (!in_box(p, center_x, center_y, width))
        {
            fprintf(stderr, "Point (%6.2f,%6.2f) is outside rectangle (%6.2f,%6.2f) W %6.2f\n",
                    p->x_pos, p->y_pos, center_x, center_y, width);
            exit(EXIT_FAILURE);
        }
}

static Node *make_node(double center_x, double center_y, double width)
{
    Node *n = malloc(sizeof(*n));
    if (n == 0)
    {
        fprintf(stderr, "Failed to allocate %zu bytes\n", sizeof(*n));
        exit(EXIT_FAILURE);
    }
    n->center_x = center_x;
    n->center_y = center_y;
    n->width = width;
    n->p = 0;
    n->sw = 0;
    n->se = 0;
    n->ne = 0;
    n->nw = 0;
    return n;
}

/* Node can be leaf node with no particle */
/* Node can be leaf node with a particle */
/* Node can be non-leaf node with no particle */
/* Leaf nodes have all sector pointers null */
/* Non-leaf nodes have all sector pointers not null */
static inline void check_node(Node *n)
{
    if (n != 0)
        assert((n->p == 0 && (n->nw != 0 && n->sw != 0 && n->se != 0 && n->ne != 0)) ||
               (n->p == 0 && (n->nw == 0 && n->sw == 0 && n->se == 0 && n->ne == 0)) ||
               (n->p != 0 && (n->nw == 0 && n->sw == 0 && n->se == 0 && n->ne == 0)));
}

static void split_node(Node *n)
{
    assert(n != 0);
    printf("= Split node: (%6.2f,%6.2f) W %6.2f\n", n->center_x, n->center_y, n->width);
    assert(n != 0 && n->sw == 0 && n->nw == 0 && n->ne == 0 && n->se == 0 && n->p != 0);
    double new_width = n->width / 2.0;
    n->sw = make_node(n->center_x - new_width, n->center_y - new_width, new_width);
    n->nw = make_node(n->center_x - new_width, n->center_y + new_width, new_width);
    n->se = make_node(n->center_x + new_width, n->center_y - new_width, new_width);
    n->ne = make_node(n->center_x + new_width, n->center_y + new_width, new_width);
    Particle *p = n->p;
    if (in_box(p, n->sw->center_x, n->sw->center_y, n->sw->width))
        n->sw->p = p;
    else if (in_box(p, n->nw->center_x, n->nw->center_y, n->nw->width))
        n->nw->p = p;
    else if (in_box(p, n->se->center_x, n->se->center_y, n->se->width))
        n->se->p = p;
    else if (in_box(p, n->ne->center_x, n->ne->center_y, n->ne->width))
        n->ne->p = p;
    else
        assert(0);  /* Can't happen! */
    n->p = 0;
    check_node(n);
}

static Node *quadtree_insert(Node *n, struct Particle *p, double center_x, double center_y, double width)
{
    printf("Point (%6.2f,%6.2f), Centre (%6.2f,%6.2f), W = %6.2f\n",
            p->x_pos, p->y_pos, center_x, center_y, width);
    check_node(n);
    /* Check that point falls in bounding box */
    check_in_box(p, center_x, center_y, width);
    if (n == NULL)
    {
        n = make_node(center_x, center_y, width);
        n->p = p;
        check_node(n);
    }
    else if (n->nw == 0 && n->p == 0)
    {
        n->p = p;
    }
    else
    {
        if (n->nw == 0)  // Could check any segment for null-ness
        {
            /* Need to split node into four and insert particle into correct node */
            assert(n->p != 0);
            split_node(n);
        }

        double new_width = width * 0.5;
        if (p->x_pos < center_x && p->y_pos < center_y)
        {
            printf("Recurse SW 1: ");
            Node *sw = quadtree_insert(n->sw, p, center_x - new_width, center_y - new_width, new_width);
            assert(sw == n->sw);
            check_node(sw);
        }
        else if (p->x_pos < center_x && p->y_pos >= center_y)
        {
            printf("Recurse NW 1: ");
            Node *nw = quadtree_insert(n->nw, p, center_x - new_width, center_y + new_width, new_width);
            assert(nw == n->nw);
            check_node(nw);
        }
        else if (p->x_pos >= center_x && p->y_pos < center_y)
        {
            printf("Recurse SE 1: ");
            Node *se = quadtree_insert(n->se, p, center_x + new_width, center_y - new_width, new_width);
            assert(se == n->se);
            check_node(se);
        }
        else
        {
            printf("Recurse NE 1: ");
            Node *ne = quadtree_insert(n->ne, p, center_x + new_width, center_y + new_width, new_width);
            assert(ne == n->ne);
            check_node(ne);
        }
    }
    check_node(n);
    return n;
}

static void print_quadtree_details(const char *tag, Node *n, int d)
{
    static const char indent[] =
        "                                                            ";
    assert(d < 15);
    const char *prefix = indent + sizeof(indent) - d * 3;
    printf("%s%s Centre (%6.2f,%6.2f) W %6.2f P %p\n",
           prefix, tag, n->center_x, n->center_y, n->width, (void *)n);
    if (n->p != 0)
        printf("%s%s Point  (%6.2f,%6.2f)\n",
              prefix, tag, n->p->x_pos, n->p->y_pos);
    if (n->nw != 0)
        print_quadtree_details("NW:", n->nw, d+1);
    if (n->sw != 0)
        print_quadtree_details("SW:", n->sw, d+1);
    if (n->se != 0)
        print_quadtree_details("SE:", n->se, d+1);
    if (n->ne != 0)
        print_quadtree_details("NE:", n->ne, d+1);
}

static void print_quadtree(Node *n)
{
    assert(n != 0);
    printf("QT Node %p\n", (void *)n);
    print_quadtree_details("===", n, 0);
}

/*
   random -n 10 -T '    { %6:2[1:20]f, %6:2[-100:100]f, %6:2[-100:100]f, %6:2[-100:100]f, %6:2[-100:100]f },'
 */

static struct Particle particles[] =
{
    {  19.99,  96.07,  62.79, -99.46,  19.70 },
    {  12.94,   1.43, -33.45,  31.80, -66.08 },
    {   6.49,  16.99, -20.83,  92.51,  35.98 },
    {  17.01, -28.85, -94.10,  42.82,  -1.30 },
    {  14.27,  85.07,  88.21,  11.22,  16.85 },
    {  15.73, -56.37,  46.85,  27.40, -15.15 },
    {   1.53, -49.44, -64.27, -29.45, -38.25 },
    {   8.03,  92.11, -47.50,  63.77, -29.99 },
    {   8.67, -99.81,  73.19,  18.75,  88.66 },
    {  16.36,  66.33,  14.23,  87.65,  40.01 },
};

enum { nParticles = sizeof(particles) / sizeof(particles[0]) };

int main(void)
{
    Node *root = NULL;
    printf("# Particle 0: ");
    root = quadtree_insert(root, &particles[0], CTR_X, CTR_Y, WIDTH);
    print_quadtree(root);

    for (int i = 1; i < nParticles; i++)
    {
        printf("# Particle %d: ", i);
        Node *tree = quadtree_insert(root, &particles[i], CTR_X, CTR_Y, WIDTH);
        assert(tree == root);
        print_quadtree(root);
    }
    return 0;
}
