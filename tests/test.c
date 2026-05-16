#include "dmst.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Простая макрофункция для проверки условия и остановки тестов при ошибке. */
#define CHECK(cond)                                                      \
    do {                                                                \
        if (!(cond)) {                                                  \
            fprintf(stderr, "Test failed: %s at %s:%d\n",            \
                    #cond, __FILE__, __LINE__);                        \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while (0)

static void run_all_algorithms(const dmst_graph_t *g, int root, const int *expected_parent, long expected_weight) {
    int n = g->n;
    int *parent = (int *)malloc((size_t)n * sizeof(int));
    long w;
    int rc;
    /* Наивная версия */
    rc = dmst_naive(g, root, parent, &w);
    CHECK(rc == 0);
    CHECK(w == expected_weight);
    for (int i = 0; i < n; ++i) {
        CHECK(parent[i] == expected_parent[i]);
    }
    /* Версия Tarjan */
    rc = dmst_tarjan(g, root, parent, &w);
    CHECK(rc == 0);
    CHECK(w == expected_weight);
    for (int i = 0; i < n; ++i) {
        CHECK(parent[i] == expected_parent[i]);
    }
    /* Версия Gabow */
    rc = dmst_gabow(g, root, parent, &w);
    CHECK(rc == 0);
    CHECK(w == expected_weight);
    for (int i = 0; i < n; ++i) {
        CHECK(parent[i] == expected_parent[i]);
    }
    free(parent);
}

/* Test a graph with a single vertex and no edges.  The weight should
 * be zero and the parent array should contain only -1. */
static void test_single_vertex() {
    dmst_graph_t g;
    g.n = 1;
    g.m = 0;
    g.edges = NULL;
    int expected_parent[1] = { -1 };
    long expected_weight = 0;
    run_all_algorithms(&g, 0, expected_parent, expected_weight);
}

/* Test a simple chain graph: 0 -> 1 with weight 5.  Root is 0. */
static void test_simple_chain() {
    dmst_graph_t g;
    g.n = 2;
    g.m = 1;
    g.edges = (dmst_edge_t *)malloc(sizeof(dmst_edge_t));
    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 5;
    g.edges[0].id = 0;
    int expected_parent[2] = { -1, 0 };
    long expected_weight = 5;
    run_all_algorithms(&g, 0, expected_parent, expected_weight);
    free(g.edges);
}

/* Test a graph where the root cannot reach all vertices. */
static void test_unreachable() {
    dmst_graph_t g;
    g.n = 3;
    g.m = 1;
    g.edges = (dmst_edge_t *)malloc(sizeof(dmst_edge_t));
    g.edges[0].src = 1;
    g.edges[0].dst = 2;
    g.edges[0].weight = 2;
    g.edges[0].id = 0;
    int parent[3];
    long w;
    /* All algorithms should report failure (return -1). */
    CHECK(dmst_naive(&g, 0, parent, &w) < 0);
    CHECK(dmst_tarjan(&g, 0, parent, &w) < 0);
    CHECK(dmst_gabow(&g, 0, parent, &w) < 0);
    free(g.edges);
}

/* Test a simple cycle graph: 0->1, 1->2, 2->0.  The algorithm
 * should pick edges 0->1 and 1->2 when root is 0. */
static void test_simple_cycle() {
    dmst_graph_t g;
    g.n = 3;
    g.m = 3;
    g.edges = (dmst_edge_t *)malloc(3 * sizeof(dmst_edge_t));
    /* 0 -> 1 (1) */
    g.edges[0].src = 0; g.edges[0].dst = 1; g.edges[0].weight = 1; g.edges[0].id = 0;
    /* 1 -> 2 (1) */
    g.edges[1].src = 1; g.edges[1].dst = 2; g.edges[1].weight = 1; g.edges[1].id = 1;
    /* 2 -> 0 (1) */
    g.edges[2].src = 2; g.edges[2].dst = 0; g.edges[2].weight = 1; g.edges[2].id = 2;
    int expected_parent[3] = { -1, 0, 1 };
    long expected_weight = 2;
    run_all_algorithms(&g, 0, expected_parent, expected_weight);
    free(g.edges);
}

/* Test a more complex graph with multiple edges and a clear optimum. */
static void test_complex_graph() {
    dmst_graph_t g;
    g.n = 4;
    g.m = 6;
    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    /* Edges: 0->1 (10), 0->2 (5), 0->3 (15), 1->3 (40), 2->3 (10), 3->2 (7) */
    g.edges[0].src = 0; g.edges[0].dst = 1; g.edges[0].weight = 10; g.edges[0].id = 0;
    g.edges[1].src = 0; g.edges[1].dst = 2; g.edges[1].weight = 5;  g.edges[1].id = 1;
    g.edges[2].src = 0; g.edges[2].dst = 3; g.edges[2].weight = 15; g.edges[2].id = 2;
    g.edges[3].src = 1; g.edges[3].dst = 3; g.edges[3].weight = 40; g.edges[3].id = 3;
    g.edges[4].src = 2; g.edges[4].dst = 3; g.edges[4].weight = 10; g.edges[4].id = 4;
    g.edges[5].src = 3; g.edges[5].dst = 2; g.edges[5].weight = 7;  g.edges[5].id = 5;
    /* Expected MST: 0->1 (10), 0->2 (5), 2->3 (10) total = 25. */
    int expected_parent[4] = { -1, 0, 0, 2 };
    long expected_weight = 25;
    run_all_algorithms(&g, 0, expected_parent, expected_weight);
    free(g.edges);
}

/* Test graph with negative weights to ensure the algorithm handles
 * negative costs correctly. */
static void test_negative_weights() {
    dmst_graph_t g;
    g.n = 4;
    g.m = 5;
    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    /* Edges: 0->1 (2), 1->2 (-5), 2->3 (4), 0->3 (10), 3->1 (-2) */
    g.edges[0].src = 0; g.edges[0].dst = 1; g.edges[0].weight = 2;  g.edges[0].id = 0;
    g.edges[1].src = 1; g.edges[1].dst = 2; g.edges[1].weight = -5; g.edges[1].id = 1;
    g.edges[2].src = 2; g.edges[2].dst = 3; g.edges[2].weight = 4;  g.edges[2].id = 2;
    g.edges[3].src = 0; g.edges[3].dst = 3; g.edges[3].weight = 10; g.edges[3].id = 3;
    g.edges[4].src = 3; g.edges[4].dst = 1; g.edges[4].weight = -2; g.edges[4].id = 4;
    /* One optimum: 0->1 (2), 1->2 (-5), 1->3 ??? But there is no 1->3.
     * Another possible arborescence picks 0->1 (2), 1->2 (-5), 2->3 (4)
     * with total 1. */
    int expected_parent[4] = { -1, 0, 1, 2 };
    long expected_weight = 1;
    run_all_algorithms(&g, 0, expected_parent, expected_weight);
    free(g.edges);
}

/* Test graph with parallel edges between the same pair of vertices. */
static void test_parallel_edges() {
    dmst_graph_t g;
    g.n = 3;
    g.m = 3;
    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    /* Parallel edges 0->1 (5) and 0->1 (3), and edge 1->2 (2). */
    g.edges[0].src = 0; g.edges[0].dst = 1; g.edges[0].weight = 5; g.edges[0].id = 0;
    g.edges[1].src = 0; g.edges[1].dst = 1; g.edges[1].weight = 3; g.edges[1].id = 1;
    g.edges[2].src = 1; g.edges[2].dst = 2; g.edges[2].weight = 2; g.edges[2].id = 2;
    int expected_parent[3] = { -1, 0, 1 };
    long expected_weight = 5; /* 0->1 weight 3 + 1->2 weight 2 = 5 */
    run_all_algorithms(&g, 0, expected_parent, expected_weight);
    free(g.edges);
}

int main(){
    test_single_vertex();
    test_simple_chain();
    test_unreachable();
    test_simple_cycle();
    test_complex_graph();
    test_negative_weights();
    test_parallel_edges();
    printf("All tests passed.\n");
    return 0;
}