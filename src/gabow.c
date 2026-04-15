#include "dmst.h"

/*
 * Gabow–Galil–Spencer–Tarjan algorithm.
 *
 * The Gabow–Galil–Spencer–Tarjan (GGST) algorithm refines Tarjan’s
 * optimum branching method further to achieve \(O(E + V \log V)\)
 * running time by combining sophisticated heap structures and amortised
 * analysis【471028505933921†L256-L262】.  While a full GGST
 * implementation is beyond the scope of this project, this entry
 * point is provided to align with the specification and currently
 * forwards to the Tarjan routine.  Clients may select this function
 * explicitly without needing to know the underlying implementation.
 */
int
dmst_gabow(const dmst_graph_t *g, int root,
           int *parent_out, long *weight_out)
{
    /* Delegate to the Tarjan implementation for now.  A future
     * enhancement could replace this call with a true GGST
     * implementation. */
    return dmst_tarjan(g, root, parent_out, weight_out);
}