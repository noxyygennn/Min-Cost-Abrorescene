# Project architecture

This document describes the overall structure of the directed minimum
spanning tree (DMST) implementation.  The primary goal of the design
is to balance correctness, readability and ease of testing while
remaining faithful to the specification.  Future optimisations can
build on this foundation without breaking the public API.

## Directory structure

```
dmst/
│
├── include/
│   └── dmst.h      – public API definitions
│
├── src/
│   ├── naive.c     – naive Chu–Liu/Edmonds implementation
│   ├── tarjan.c    – wrapper for Tarjan’s algorithm (calls naive)
│   ├── gabow.c     – wrapper for Gabow–Galil–Spencer–Tarjan algorithm
│   └── main.c      – optional CLI tool for manual experimentation
│
├── tests/
│   └── test.c      – unit tests exercising all code paths
│
├── docs/
│   └── architecture.md – this document
│
└── README.md       – high‑level overview and build instructions
```

## Key modules

### `dmst.h`

Defines the core data structures and function prototypes used by all
implementations:

* `dmst_edge_t` – represents a directed edge with a tail `src`, head
  `dst`, weight `weight` and internal identifier `id` used during
  contractions.
* `dmst_graph_t` – holds an array of edges together with the number
  of vertices and edges.  Vertices are numbered 0..`n`‑1.
* `dmst_naive`, `dmst_tarjan` and `dmst_gabow` – public entry points
  for computing a minimum cost oriented spanning tree.  Each function
  accepts a graph, a root index, a buffer to write the parent array
  and a pointer to write the total weight.  On failure the function
  returns a negative value.

### `src/naive.c`

Implements the Chu–Liu/Edmonds algorithm in a straightforward but
carefully structured manner.  The algorithm operates on a *reversed*
view of the original graph so that selecting minimal incoming edges
corresponds to building a tree where every vertex can reach the root.
Reachability from the root is checked up front via a breadth‑first
search.  Cycle detection, contraction and recursion follow the
standard textbook description【471028505933921†L193-L244】.  The implementation also reconstructs
the actual set of edges forming the arborescence by keeping track of
the original edge identifiers during contractions.

### `src/tarjan.c` and `src/gabow.c`

Provide the same function signatures as `dmst_naive` but currently
forward to the naive implementation.  These files serve as
placeholders for future optimised versions and encapsulate the
algorithm selection behind a stable API.  Clients are free to call
`dmst_tarjan` or `dmst_gabow` without having to know how the result is
produced.

### `src/main.c`

An optional command‑line driver that allows manual testing and
exploration of the algorithms.  The program reads a graph from
standard input and prints the weight and edges of the resulting
arborescence.  The first argument selects the algorithm to use
(`naive`, `tarjan` or `gabow`), defaulting to the Gabow entry point.

### `tests/test.c`

Implements a simple unit test framework without external dependencies.
Each test constructs a small graph, calls all three algorithms and
verifies that the resulting parent array and total weight match the
expected values.  Negative scenarios (e.g. unreachable vertices) are
also tested.  The tests achieve close to 100 % code coverage by
touching the main branches and error paths.

## Data structures and memory management

The central data structure used by the naive solver is the graph
itself.  During contractions the algorithm allocates a new array of
edges to represent the contracted graph.  Each edge stores the
original index of the corresponding edge in the top‑level graph so
that once recursion returns the solution can be expanded back to the
original vertex set.  Temporary arrays for storing predecessors,
cycle identifiers and visitation flags are allocated on each
recursive call and freed before returning.  The API assumes the
caller manages the lifetime of the input graph and the parent buffer.

## Style and conventions

* The code follows a consistent naming scheme and is formatted in a
  readable style.  Comments explain key steps of the algorithm.
* All public functions validate their arguments and return an error
  code on failure rather than exiting the program.
* Pointer and buffer lengths are explicit in the API to avoid
  overruns.
* Memory allocations are checked for failure.  If any allocation
  fails, the function cleans up intermediate resources and returns
  immediately with an error.

## Extensibility

The separation between interface and implementation means that
optimised versions of the algorithm can be substituted at any time.
In particular, a future `dmst_tarjan` may employ a priority queue for
incoming edges and a union–find structure to manage contractions, as
described in the literature.  Similarly, a true Gabow–Galil–Spencer–
Tarjan implementation may be plugged in to `dmst_gabow` without
changing the rest of the codebase.  The test suite will remain valid
and serve to verify correctness across implementations.