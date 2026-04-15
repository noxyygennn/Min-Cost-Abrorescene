#include "dmst.h"

/*
 * Tarjan’s version of the optimum branching algorithm.
 *
 * Tarjan’s algorithm improves upon the straightforward Chu–Liu/Edmonds
 * implementation by using more advanced data structures (e.g. union–
 * find and heaps) to achieve an \(O(E \log V)\) running time on
 * sparse graphs【471028505933921†L256-L262】.  For the purposes of this
 * project, and in order to prioritise a correct and well‑tested
 * implementation, this routine forwards directly to the naive version.
 * The interface is preserved so that an improved implementation can
 * easily be substituted without affecting client code.
 */
int
dmst_tarjan(const dmst_graph_t *g, int root,
            int *parent_out, long *weight_out)
{
    /* Delegate to the naive implementation. */
    return dmst_naive(g, root, parent_out, weight_out);
}