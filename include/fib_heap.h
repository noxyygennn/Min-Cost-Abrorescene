#ifndef FIB_HEAP_H
#define FIB_HEAP_H

typedef struct fib_node fib_node_t;

struct fib_node {
    long key;
    long raw_key;

    int edge_id;
    int src;
    int dst;

    int degree;
    int mark;

    fib_node_t *parent;
    fib_node_t *child;
    fib_node_t *left;
    fib_node_t *right;
};

typedef struct {
    fib_node_t *min;
    int n;
    long offset;
} fib_heap_t;

void fib_heap_init(fib_heap_t *heap);
void fib_heap_free(fib_heap_t *heap);

fib_node_t *fib_heap_insert(
    fib_heap_t *heap,
    long key,
    int edge_id,
    int src,
    int dst
);

fib_node_t *fib_heap_min(fib_heap_t *heap);

fib_node_t *fib_heap_extract_min(fib_heap_t *heap);

void fib_heap_decrease_key(
    fib_heap_t *heap,
    fib_node_t *x,
    long new_key
);

void fib_heap_meld(
    fib_heap_t *dst,
    fib_heap_t *src
);

void fib_heap_add_offset(
    fib_heap_t *heap,
    long delta
);

#endif
