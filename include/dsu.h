#ifndef DSU_H
#define DSU_H

typedef struct {
    int n;
    int *parent;
    int *rank;
} dsu_t;

int dsu_init(dsu_t *dsu, int n);
void dsu_free(dsu_t *dsu);

int dsu_find(dsu_t *dsu, int x);
int dsu_union(dsu_t *dsu, int a, int b);

#endif
