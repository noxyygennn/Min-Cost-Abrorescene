#include "dsu.h"

#include <stdlib.h>

int dsu_init(dsu_t *dsu, int n)
{
    if (!dsu || n <= 0) {
        return -1;
    }

    dsu->n = n;
    dsu->parent = (int *)malloc((size_t)n * sizeof(int));
    dsu->rank = (int *)calloc((size_t)n, sizeof(int));

    if (!dsu->parent || !dsu->rank) {
        free(dsu->parent);
        free(dsu->rank);

        dsu->parent = NULL;
        dsu->rank = NULL;
        dsu->n = 0;

        return -1;
    }

    for (int i = 0; i < n; ++i) {
        dsu->parent[i] = i;
    }

    return 0;
}

void dsu_free(dsu_t *dsu)
{
    if (!dsu) {
        return;
    }

    free(dsu->parent);
    free(dsu->rank);

    dsu->parent = NULL;
    dsu->rank = NULL;
    dsu->n = 0;
}

int dsu_find(dsu_t *dsu, int x)
{
    if (dsu->parent[x] != x) {
        dsu->parent[x] = dsu_find(dsu, dsu->parent[x]);
    }

    return dsu->parent[x];
}

int dsu_union(dsu_t *dsu, int a, int b)
{
    int ra = dsu_find(dsu, a);
    int rb = dsu_find(dsu, b);

    if (ra == rb) {
        return ra;
    }

    if (dsu->rank[ra] < dsu->rank[rb]) {
        dsu->parent[ra] = rb;
        return rb;
    }

    if (dsu->rank[ra] > dsu->rank[rb]) {
        dsu->parent[rb] = ra;
        return ra;
    }

    dsu->parent[rb] = ra;
    dsu->rank[ra]++;

    return ra;
}
