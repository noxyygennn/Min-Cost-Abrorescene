#include "fib_heap.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>

static void list_insert_after(fib_node_t *a, fib_node_t *x)
{
    x->left = a;
    x->right = a->right;
    a->right->left = x;
    a->right = x;
}

static void list_remove(fib_node_t *x)
{
    x->left->right = x->right;
    x->right->left = x->left;
    x->left = x;
    x->right = x;
}

static void root_insert(fib_heap_t *heap, fib_node_t *x)
{
    x->parent = NULL;

    if (!heap->min) {
        x->left = x;
        x->right = x;
        heap->min = x;
    } else {
        list_insert_after(heap->min, x);

        if (x->key < heap->min->key) {
            heap->min = x;
        }
    }
}

void fib_heap_init(fib_heap_t *heap)
{
    heap->min = NULL;
    heap->n = 0;
    heap->offset = 0;
}

static void free_list(fib_node_t *node)
{
    if (!node) {
        return;
    }

    fib_node_t *start = node;
    fib_node_t *cur = node;

    do {
        fib_node_t *next = cur->right;

        if (cur->child) {
            free_list(cur->child);
        }

        free(cur);
        cur = next;
    } while (cur != start);
}

void fib_heap_free(fib_heap_t *heap)
{
    if (!heap) {
        return;
    }

    free_list(heap->min);

    heap->min = NULL;
    heap->n = 0;
    heap->offset = 0;
}

fib_node_t *fib_heap_insert(
    fib_heap_t *heap,
    long key,
    int edge_id,
    int src,
    int dst
) {
    fib_node_t *x = (fib_node_t *)malloc(sizeof(fib_node_t));

    if (!x) {
        return NULL;
    }

    x->key = key - heap->offset;
    x->raw_key = key;
    x->edge_id = edge_id;
    x->src = src;
    x->dst = dst;

    x->degree = 0;
    x->mark = 0;

    x->parent = NULL;
    x->child = NULL;
    x->left = x;
    x->right = x;

    root_insert(heap, x);
    heap->n++;

    return x;
}

fib_node_t *fib_heap_min(fib_heap_t *heap)
{
    return heap->min;
}

static void fib_link(fib_heap_t *heap, fib_node_t *y, fib_node_t *x)
{
    (void)heap;

    list_remove(y);

    y->parent = x;
    y->mark = 0;

    if (!x->child) {
        y->left = y;
        y->right = y;
        x->child = y;
    } else {
        list_insert_after(x->child, y);
    }

    x->degree++;
}

static int degree_bound(int n)
{
    int d = 0;

    while (n > 0) {
        n >>= 1;
        d++;
    }

    return d * 3 + 10;
}

static void consolidate(fib_heap_t *heap)
{
    if (!heap->min) {
        return;
    }

    int max_degree = degree_bound(heap->n);
    fib_node_t **a =
        (fib_node_t **)calloc((size_t)max_degree, sizeof(fib_node_t *));

    if (!a) {
        return;
    }

    int root_count = 0;
    fib_node_t *w = heap->min;

    do {
        root_count++;
        w = w->right;
    } while (w != heap->min);

    fib_node_t **roots =
        (fib_node_t **)malloc((size_t)root_count * sizeof(fib_node_t *));

    if (!roots) {
        free(a);
        return;
    }

    w = heap->min;

    for (int i = 0; i < root_count; ++i) {
        roots[i] = w;
        w = w->right;
    }

    for (int i = 0; i < root_count; ++i) {
        fib_node_t *x = roots[i];
        int d = x->degree;

        while (d < max_degree && a[d]) {
            fib_node_t *y = a[d];

            if (y->key < x->key) {
                fib_node_t *tmp = x;
                x = y;
                y = tmp;
            }

            fib_link(heap, y, x);
            a[d] = NULL;
            d++;
        }

        if (d < max_degree) {
            a[d] = x;
        }
    }

    heap->min = NULL;

    for (int i = 0; i < max_degree; ++i) {
        if (a[i]) {
            a[i]->left = a[i];
            a[i]->right = a[i];

            root_insert(heap, a[i]);
        }
    }

    free(roots);
    free(a);
}

fib_node_t *fib_heap_extract_min(fib_heap_t *heap)
{
    fib_node_t *z = heap->min;

    if (!z) {
        return NULL;
    }

    if (z->child) {
        fib_node_t *child = z->child;
        fib_node_t *start = child;
        fib_node_t *cur = child;

        do {
            fib_node_t *next = cur->right;

            cur->parent = NULL;
            cur->left = cur;
            cur->right = cur;
            root_insert(heap, cur);

            cur = next;
        } while (cur != start);

        z->child = NULL;
    }

    if (z->right == z) {
        heap->min = NULL;
    } else {
        fib_node_t *next = z->right;
        list_remove(z);
        heap->min = next;
        consolidate(heap);
    }

    heap->n--;

    z->left = z;
    z->right = z;
    z->parent = NULL;
    z->child = NULL;
    z->degree = 0;
    z->mark = 0;

    z->key += heap->offset;

    return z;
}

static void cut(fib_heap_t *heap, fib_node_t *x, fib_node_t *y)
{
    if (x->right == x) {
        y->child = NULL;
    } else {
        if (y->child == x) {
            y->child = x->right;
        }

        list_remove(x);
    }

    y->degree--;

    x->parent = NULL;
    x->mark = 0;
    x->left = x;
    x->right = x;

    root_insert(heap, x);
}

static void cascading_cut(fib_heap_t *heap, fib_node_t *y)
{
    fib_node_t *z = y->parent;

    if (z) {
        if (!y->mark) {
            y->mark = 1;
        } else {
            cut(heap, y, z);
            cascading_cut(heap, z);
        }
    }
}

void fib_heap_decrease_key(
    fib_heap_t *heap,
    fib_node_t *x,
    long new_key
) {
    long adjusted = new_key - heap->offset;

    if (adjusted > x->key) {
        return;
    }

    x->key = adjusted;

    fib_node_t *y = x->parent;

    if (y && x->key < y->key) {
        cut(heap, x, y);
        cascading_cut(heap, y);
    }

    if (x->key < heap->min->key) {
        heap->min = x;
    }
}

void fib_heap_meld(
    fib_heap_t *dst,
    fib_heap_t *src
) {
    if (!src->min) {
        return;
    }

    /*
     * Перед слиянием переносим offset src в ключи его корней.
     * Так heap остаётся корректной даже при lazy offsets.
     */
    if (src->offset != dst->offset) {
        fib_node_t *start = src->min;
        fib_node_t *cur = start;

        do {
            cur->key += src->offset - dst->offset;
            cur = cur->right;
        } while (cur != start);
    }

    if (!dst->min) {
        dst->min = src->min;
        dst->n = src->n;
        dst->offset = dst->offset;

        src->min = NULL;
        src->n = 0;
        src->offset = 0;

        return;
    }

    fib_node_t *a = dst->min;
    fib_node_t *b = src->min;

    fib_node_t *a_right = a->right;
    fib_node_t *b_left = b->left;

    a->right = b;
    b->left = a;

    a_right->left = b_left;
    b_left->right = a_right;

    if (b->key < a->key) {
        dst->min = b;
    }

    dst->n += src->n;

    src->min = NULL;
    src->n = 0;
    src->offset = 0;
}

void fib_heap_add_offset(
    fib_heap_t *heap,
    long delta
) {
    heap->offset += delta;
}
