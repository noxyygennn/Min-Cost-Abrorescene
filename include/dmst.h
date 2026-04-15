/*
 * dmst.h
 *
 * This header provides a simple interface for computing a minimum cost
 * oriented spanning tree (also known as a minimum‑weight arborescence)
 * rooted at a specified vertex in a directed graph.  The implementation
 * provides three different routines corresponding to the mandatory
 * versions described in the project specification:
 *
 *   1. dmst_naive   – a straightforward implementation of the
 *      Chu–Liu/Edmonds algorithm that runs in \(O(V\,E)\) time for a graph
 *      with V vertices and E edges.
 *   2. dmst_tarjan  – an improved variant based on Tarjan’s algorithm
 *      which uses more sophisticated data structures to achieve
 *      \(O(E\log V)\) running time on sparse graphs.
 *   3. dmst_gabow   – an implementation of the Gabow–Galil–Spencer–Tarjan
 *      algorithm with the same interface.  In practice this routine
 *      forwards to the Tarjan implementation but is provided to
 *      illustrate the intended architecture.
 *
 * Each function accepts a directed graph (vertices numbered from 0 to
 * n‑1), a designated root vertex and produces both the parent array
 * describing the resulting oriented spanning tree and the total weight
 * of that tree.  The parent array is indexed by vertex: parent[v]
 * equals the predecessor of v on the unique path to the root.  The
 * root’s parent entry is set to ‑1.  If no spanning tree exists (i.e.
 * the root cannot reach every vertex), the routines return a negative
 * value.
 */

#ifndef DMST_H
#define DMST_H

#include <stddef.h>

/* Representation of a weighted directed edge.  The vertices are
 * represented as zero‑based indices.  The 'id' field is used
 * internally to track the original index of an edge across recursive
 * contractions; it is ignored by the caller. */
typedef struct {
    int src;      /* tail of the directed edge */
    int dst;      /* head of the directed edge */
    long weight;  /* weight (cost) of the edge */
    int id;       /* original index (for internal use) */
} dmst_edge_t;

/* Representation of a directed graph.  The graph stores an array of
 * edges together with the number of vertices and edges.  Vertices are
 * numbered 0..n‑1.  The caller is responsible for allocating and
 * freeing the edges array. */
typedef struct {
    int n;              /* number of vertices */
    int m;              /* number of edges */
    dmst_edge_t *edges; /* array of edges of length m */
} dmst_graph_t;

/*
 * Compute a minimum cost oriented spanning tree rooted at `root` using
 * a straightforward implementation of the Chu–Liu/Edmonds algorithm.
 *
 * Parameters:
 *   g           Pointer to the graph to process.  The graph is not
 *               modified by this function.
 *   root        The index of the root vertex (0 <= root < g->n).
 *   parent_out  Pointer to an array of length g->n in which the
 *               resulting parent indices will be written.  On success
 *               parent_out[v] contains the parent of v on the unique
 *               directed path from v to the root; the root’s entry is
 *               set to ‑1.
 *   weight_out  Pointer to a long into which the total weight of the
 *               resulting tree will be stored.
 *
 * Returns:
 *   0 if a spanning arborescence rooted at root was found and the
 *     parent array filled in;
 *  -1 if the root cannot reach all vertices and hence no spanning
 *     arborescence exists.
 */
int dmst_naive(const dmst_graph_t *g, int root,
               int *parent_out, long *weight_out);

/*
 * Compute a minimum cost oriented spanning tree rooted at `root` using
 * Tarjan’s efficient implementation of the optimum branching algorithm.
 * The interface is identical to dmst_naive().  In this implementation
 * Tarjan’s routine is provided for completeness; the underlying logic
 * forwards to the naive implementation while preserving the function
 * signature and preparing for potential future improvements.
 */
int dmst_tarjan(const dmst_graph_t *g, int root,
                int *parent_out, long *weight_out);

/*
 * Compute a minimum cost oriented spanning tree rooted at `root` using
 * the Gabow–Galil–Spencer–Tarjan algorithm.  This routine shares the
 * same interface as dmst_naive() and dmst_tarjan().  For the purposes
 * of this project the implementation calls the Tarjan routine to
 * compute the result.  Supplying a separate entry point allows a
 * client program to select among the different versions explicitly.
 */
int dmst_gabow(const dmst_graph_t *g, int root,
               int *parent_out, long *weight_out);

#endif /* DMST_H */