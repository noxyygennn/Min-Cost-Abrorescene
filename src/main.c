#include "dmst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Simple command‑line interface for computing a minimum cost oriented
 * spanning tree.  Usage:
 *   dmst [naive|tarjan|gabow]
 *
 * The program reads a directed graph from standard input in the
 * following format:
 *
 *   n m root
 *   u1 v1 w1
 *   u2 v2 w2
 *   ...
 *   um vm wm
 *
 * where n is the number of vertices, m the number of edges and root
 * the root vertex.  Vertices are numbered 0..n‑1.  Each subsequent
 * line describes an edge directed from ui to vi with weight wi.  Upon
 * success the program prints the total weight of the oriented
 * spanning tree followed by the list of edges (parent pointers).
 */
int
main(int argc, char **argv)
{
    /* Select the algorithm based on the first argument.  Default to
     * gabow if none is specified. */
    const char *algo = "gabow";
    if (argc > 1) {
        algo = argv[1];
    }
    int n, m, root;
    if (scanf("%d %d %d", &n, &m, &root) != 3) {
        fprintf(stderr, "Invalid input format.\n");
        return 1;
    }
    if (n <= 0 || m < 0 || root < 0 || root >= n) {
        fprintf(stderr, "Invalid graph parameters.\n");
        return 1;
    }
    dmst_graph_t graph;
    graph.n = n;
    graph.m = m;
    graph.edges = (dmst_edge_t *)malloc((size_t)m * sizeof(dmst_edge_t));
    if (!graph.edges) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }
    for (int i = 0; i < m; ++i) {
        int u, v;
        long w;
        if (scanf("%d %d %ld", &u, &v, &w) != 3) {
            fprintf(stderr, "Invalid edge input.\n");
            free(graph.edges);
            return 1;
        }
        if (u < 0 || u >= n || v < 0 || v >= n) {
            fprintf(stderr, "Vertex index out of range.\n");
            free(graph.edges);
            return 1;
        }
        graph.edges[i].src = u;
        graph.edges[i].dst = v;
        graph.edges[i].weight = w;
        graph.edges[i].id = i;
    }
    int *parent = (int *)malloc((size_t)n * sizeof(int));
    long totalWeight = 0;
    int result = -1;
    if (strcmp(algo, "naive") == 0) {
        result = dmst_naive(&graph, root, parent, &totalWeight);
    } else if (strcmp(algo, "tarjan") == 0) {
        result = dmst_tarjan(&graph, root, parent, &totalWeight);
    } else {
        /* Default to Gabow. */
        result = dmst_gabow(&graph, root, parent, &totalWeight);
    }
    if (result < 0) {
        fprintf(stderr, "No spanning arborescence exists from root %d.\n", root);
        free(parent);
        free(graph.edges);
        return 1;
    }
    /* Print total weight and the edges of the arborescence. */
    printf("%ld\n", totalWeight);
    /* Parent of root is indicated as -1.  For each non‑root vertex v
     * print (parent[v], v, weight) on a separate line to show the
     * chosen edge. */
    for (int v = 0; v < n; ++v) {
        if (v == root) {
            continue;
        }
        int p = parent[v];
        long w = 0;
        /* Look up the weight of the chosen edge. */
        for (int i = 0; i < m; ++i) {
            if (graph.edges[i].src == p && graph.edges[i].dst == v) {
                w = graph.edges[i].weight;
                break;
            }
        }
        printf("%d -> %d (w=%ld)\n", p, v, w);
    }
    free(parent);
    free(graph.edges);
    return 0;
}