#include "dmst.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Internal helper to perform a breadth‑first search on the original
 * graph to verify reachability.  Returns 1 if every vertex is
 * reachable from `root` following the direction of the graph’s edges,
 * otherwise returns 0.  The graph is not modified. */
static int
check_reachable(const dmst_graph_t *g, int root)
{
    int n = g->n;
    int m = g->m;
    /* Boolean array of visited flags.  Initialise all to 0. */
    char *visited = (char *)calloc((size_t)n, sizeof(char));
    if (!visited) {
        return 0;
    }
    /* Simple queue implemented with a dynamic array. */
    int *queue = (int *)malloc((size_t)n * sizeof(int));
    if (!queue) {
        free(visited);
        return 0;
    }
    int head = 0, tail = 0;
    visited[root] = 1;
    queue[tail++] = root;
    while (head < tail) {
        int u = queue[head++];
        /* Traverse all outgoing edges from u. */
        for (int i = 0; i < m; ++i) {
            if (g->edges[i].src == u) {
                int v = g->edges[i].dst;
                if (!visited[v]) {
                    visited[v] = 1;
                    queue[tail++] = v;
                }
            }
        }
    }
    /* Verify that every vertex was visited. */
    int ok = 1;
    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            ok = 0;
            break;
        }
    }
    free(queue);
    free(visited);
    return ok;
}

/* Recursively compute the minimum cost arborescence on a reversed
 * directed graph.  The incoming edges of each vertex are considered
 * and cycles are contracted.  The function writes the selected edge
 * indices (indices into the original graph) into `selected`, and
 * returns the total weight of the arborescence or -1 on failure.  The
 * arguments:
 *   graph         – the current (possibly contracted) graph
 *   root          – index of root in this graph
 *   selected      – array of length >= graph->n-1 into which the
 *                   selected original edge indices will be written
 *   selected_cnt  – pointer to an int which will be updated with the
 *                   number of edges written to `selected`
 */
static long
dmst_recursive(const dmst_graph_t *graph, int root,
               int *selected, int *selected_cnt)
{
    int n = graph->n;
    int m = graph->m;
    const dmst_edge_t *edges = graph->edges;
    /* Allocate temporary arrays. */
    long *in = (long *)malloc((size_t)n * sizeof(long));
    int *pre = (int *)malloc((size_t)n * sizeof(int));
    int *edgeIdx = (int *)malloc((size_t)n * sizeof(int));
    int *id = (int *)malloc((size_t)n * sizeof(int));
    int *vis = (int *)malloc((size_t)n * sizeof(int));
    if (!in || !pre || !edgeIdx || !id || !vis) {
        free(in); free(pre); free(edgeIdx); free(id); free(vis);
        return -1;
    }
    /* Step 1: find the minimum incoming edge for each vertex. */
    for (int i = 0; i < n; ++i) {
        in[i] = LONG_MAX;
        pre[i] = -1;
        edgeIdx[i] = -1;
    }
    /* Traverse all edges to determine the cheapest incoming edge for each
     * vertex other than the root.  Ignore self‑loops explicitly. */
    for (int i = 0; i < m; ++i) {
        int u = edges[i].src;
        int v = edges[i].dst;
        long w = edges[i].weight;
        if (u == v) {
            continue;
        }
        if (w < in[v]) {
            in[v] = w;
            pre[v] = u;
            edgeIdx[v] = edges[i].id;
        }
    }
    /* The root has no incoming edge in the arborescence; set its cost
     * contribution to zero. */
    in[root] = 0;
    /* If there exists any vertex other than the root with no incoming
     * edge, then the graph does not admit a spanning arborescence. */
    for (int v = 0; v < n; ++v) {
        if (v == root) {
            continue;
        }
        if (in[v] == LONG_MAX) {
            /* Clean up and signal failure. */
            free(in); free(pre); free(edgeIdx); free(id); free(vis);
            return -1;
        }
    }
    /* Step 2: detect directed cycles among the selected incoming edges.
     * Accumulate the contributions of the chosen incoming edges. */
    long total = 0;
    for (int i = 0; i < n; ++i) {
        total += in[i] == LONG_MAX ? 0 : in[i];
        id[i] = -1;
        vis[i] = -1;
    }
    int cycleNum = 0;
    for (int i = 0; i < n; ++i) {
        int v = i;
        /* Follow the predecessor chain until we return to a previously
         * visited vertex or reach the root.  vis[] is used to detect
         * repeated visits within this pass. */
        while (vis[v] != i && id[v] == -1 && v != root) {
            vis[v] = i;
            v = pre[v];
        }
        /* If we stopped on a non‑root vertex with no id assigned, we
         * have detected a cycle.  Contract all vertices in the cycle
         * into a single node. */
        if (v != root && id[v] == -1) {
            for (int u = pre[v]; u != v; u = pre[u]) {
                id[u] = cycleNum;
            }
            id[v] = cycleNum;
            cycleNum++;
        }
    }
    /* If no cycles were found, the selected incoming edges form the
     * minimum arborescence.  Record them and return the accumulated
     * weight. */
    if (cycleNum == 0) {
        for (int v = 0; v < n; ++v) {
            if (v == root) {
                continue;
            }
            /* edgeIdx[v] holds the index of the original edge selected
             * for vertex v. */
            selected[(*selected_cnt)++] = edgeIdx[v];
        }
        free(in); free(pre); free(edgeIdx); free(id); free(vis);
        return total;
    }
    /* Step 3: assign identifiers to non‑cycle vertices.  Each cycle
     * already has an id 0..cycleNum-1.  The remaining vertices are
     * numbered consecutively starting from cycleNum. */
    int newCount = cycleNum;
    for (int i = 0; i < n; ++i) {
        if (id[i] == -1) {
            id[i] = newCount++;
        }
    }
    /* Step 4: build the contracted graph.  Each cycle becomes a single
     * vertex.  For edges that cross between different ids we adjust
     * their weights by subtracting the in[] value of the destination
     * (which represents the cost already accounted for in total).
     */
    dmst_edge_t *newEdges = (dmst_edge_t *)malloc((size_t)m * sizeof(dmst_edge_t));
    if (!newEdges) {
        free(in); free(pre); free(edgeIdx); free(id); free(vis);
        return -1;
    }
    int newM = 0;
    for (int i = 0; i < m; ++i) {
        int u = edges[i].src;
        int v = edges[i].dst;
        long w = edges[i].weight;
        int uId = id[u];
        int vId = id[v];
        if (uId != vId) {
            /* Adjust weight for contraction: subtract the cost of the
             * incoming edge selected for v.  Because in[v] may be
             * LONG_MAX for the root, ensure that subtraction is safe. */
            long adjW = w;
            if (in[v] != LONG_MAX) {
                adjW = w - in[v];
            }
            newEdges[newM].src = uId;
            newEdges[newM].dst = vId;
            newEdges[newM].weight = adjW;
            /* Propagate the original edge index stored in edges[i].id. */
            newEdges[newM].id = edges[i].id;
            newM++;
        }
    }
    dmst_graph_t contracted;
    contracted.n = newCount;
    contracted.m = newM;
    contracted.edges = newEdges;
    /* Recursively compute an MST on the contracted graph.  The root of
     * the new graph is id[root].  Allocate a temporary array to hold
     * the selected edge indices from the subproblem. */
    int *subSelected = (int *)malloc((size_t)(contracted.n > 0 ? contracted.n - 1 : 0) * sizeof(int));
    int subCount = 0;
    long subWeight = dmst_recursive(&contracted, id[root], subSelected, &subCount);
    if (subWeight < 0) {
        /* Failure in subproblem implies no arborescence.  Clean up and
         * propagate failure. */
        free(subSelected);
        free(newEdges);
        free(in); free(pre); free(edgeIdx); free(id); free(vis);
        return -1;
    }
    total += subWeight;
    /* Step 5: Uncontract cycles.  Determine which edges inside cycles
     * must be kept and which incoming edge is removed to open the cycle.
     */
    /* Array to mark which vertex’s incoming edge should be removed in
     * each cycle.  Default is 0 (false).  Only vertices that belong to
     * cycles (id < cycleNum) are considered. */
    char *removed = (char *)calloc((size_t)n, sizeof(char));
    if (!removed) {
        free(subSelected);
        free(newEdges);
        free(in); free(pre); free(edgeIdx); free(id); free(vis);
        return -1;
    }
    /* Add edges selected in the subproblem.  For each selected edge in
     * the contracted graph, we look up its corresponding original edge
     * index (stored in newEdges[idx].id) and add that to the final
     * result.  If this edge enters a contracted cycle, we mark which
     * vertex of that cycle loses its selected incoming edge. */
    for (int i = 0; i < subCount; ++i) {
        int idx = subSelected[i];
        if (idx < 0 || idx >= contracted.m) {
            continue;
        }
        dmst_edge_t ne = contracted.edges[idx];
        /* Append the original edge index to the result list. */
        selected[(*selected_cnt)++] = ne.id;
        /* Determine if this edge enters a contracted cycle.  A cycle
         * corresponds to an id less than cycleNum.  When ne.dst is
         * less than cycleNum, we know this edge connects into that
         * cycle; the vertex of the cycle associated with this edge is
         * determined by the original destination of the edge. */
        if (ne.dst < cycleNum) {
            /* The original edge index gives us access to the original
             * destination vertex. */
            int vOrig = graph->edges[ne.id].dst;
            removed[vOrig] = 1;
        }
    }
    /* Include edges that were selected before contraction (one per
     * vertex) except for the vertices whose incoming edge was removed
     * (marked in removed[]). */
    for (int v = 0; v < n; ++v) {
        if (v == root) {
            continue;
        }
        /* v belongs to a cycle if id[v] < cycleNum. */
        if (id[v] < cycleNum) {
            if (!removed[v]) {
                selected[(*selected_cnt)++] = edgeIdx[v];
            }
        } else {
            /* Non‑cycle vertex: include its chosen incoming edge. */
            selected[(*selected_cnt)++] = edgeIdx[v];
        }
    }
    /* Cleanup. */
    free(removed);
    free(subSelected);
    free(newEdges);
    free(in); free(pre); free(edgeIdx); free(id); free(vis);
    return total;
}

/*
 * Public entry point for the naive directed MST computation.  This
 * wrapper performs an initial reachability check, constructs a reversed
 * view of the input graph and invokes the recursive solver.  On
 * success the parent_out array and weight_out value are populated.
 */
int
dmst_naive(const dmst_graph_t *g, int root, int *parent_out, long *weight_out)
{
    if (!g || g->n <= 0 || g->m < 0) {
        return -1;
    }
    if (root < 0 || root >= g->n) {
        return -1;
    }
    /* Check that every vertex is reachable from the root in the original
     * graph.  If not, no oriented spanning tree exists. */
    if (!check_reachable(g, root)) {
        return -1;
    }
    /* Build a reversed version of the graph.  Each edge (u->v) becomes
     * (v->u).  The id field records the original index. */
    dmst_edge_t *revEdges = (dmst_edge_t *)malloc((size_t)g->m * sizeof(dmst_edge_t));
    if (!revEdges) {
        return -1;
    }
    for (int i = 0; i < g->m; ++i) {
        revEdges[i].src = g->edges[i].dst;
        revEdges[i].dst = g->edges[i].src;
        revEdges[i].weight = g->edges[i].weight;
        revEdges[i].id = i;
    }
    dmst_graph_t rev;
    rev.n = g->n;
    rev.m = g->m;
    rev.edges = revEdges;
    /* Allocate space for the selected edge indices.  A spanning tree on
     * n vertices contains exactly n‑1 edges.  The recursive solver
     * writes original edge indices into this array. */
    int *selected = (int *)malloc((size_t)(g->n > 0 ? g->n - 1 : 0) * sizeof(int));
    if (!selected) {
        free(revEdges);
        return -1;
    }
    int selectedCount = 0;
    long total = dmst_recursive(&rev, root, selected, &selectedCount);
    if (total < 0 || selectedCount != g->n - 1) {
        /* Either no arborescence or something went wrong. */
        free(selected);
        free(revEdges);
        return -1;
    }
    /* Initialise parent array to -1. */
    for (int i = 0; i < g->n; ++i) {
        parent_out[i] = -1;
    }
    /* Translate the list of original edge indices into parent pointers.
     * Each selected index corresponds to an edge (u->v) in the original
     * orientation, so the parent of v is u. */
    for (int i = 0; i < selectedCount; ++i) {
        int idx = selected[i];
        if (idx < 0 || idx >= g->m) {
            continue;
        }
        int u = g->edges[idx].src;
        int v = g->edges[idx].dst;
        parent_out[v] = u;
    }
    parent_out[root] = -1;
    *weight_out = total;
    free(selected);
    free(revEdges);
    return 0;
}