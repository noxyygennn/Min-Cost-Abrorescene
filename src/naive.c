#include "dmst.h"
#include "reachability.h"

#include <limits.h>
#include <stdlib.h>

/*
 * Учебная реализация алгоритма Chu-Liu/Edmonds.
 *
 * Алгоритм строит минимальную ориентированную арборесценцию
 * из заданного корня root.
 *
 * parent_out[v] = родитель вершины v
 * parent_out[root] = -1
 */

typedef struct {
    int src;
    int dst;
    long weight;

    /*
     * Индекс ребра в исходном графе.
     * Нужен для восстановления ответа после сжатия циклов.
     */
    int orig;

    /*
     * Вершина текущего графа, в которую входит ребро.
     * Нужна при разворачивании сжатого цикла.
     */
    int enter;
} naive_edge_t;

typedef struct {
    int n;
    int m;
    naive_edge_t *edges;
} naive_graph_t;

static int append_selected(
    int *selected,
    int *selected_count,
    int value
) {
    if (value < 0) {
        return -1;
    }

    selected[*selected_count] = value;
    ++(*selected_count);

    return 0;
}

static int find_edge_by_orig(
    const naive_graph_t *g,
    int orig
) {
    for (int i = 0; i < g->m; ++i) {
        if (g->edges[i].orig == orig) {
            return i;
        }
    }

    return -1;
}

static int naive_recursive(
    const naive_graph_t *g,
    int root,
    int *selected,
    int *selected_count
) {
    long *in = NULL;
    int *pre = NULL;
    int *pre_orig = NULL;
    int *id = NULL;
    int *vis = NULL;

    naive_edge_t *new_edges = NULL;
    int *sub_selected = NULL;
    char *removed = NULL;

    int cycle_count = 0;
    int new_n = 0;
    int new_m = 0;
    int sub_count = 0;
    int rc = -1;

    in = (long *)malloc((size_t)g->n * sizeof(long));
    pre = (int *)malloc((size_t)g->n * sizeof(int));
    pre_orig = (int *)malloc((size_t)g->n * sizeof(int));
    id = (int *)malloc((size_t)g->n * sizeof(int));
    vis = (int *)malloc((size_t)g->n * sizeof(int));

    if (!in || !pre || !pre_orig || !id || !vis) {
        goto cleanup;
    }

    /*
     * Шаг 1.
     * Для каждой вершины выбираем минимальное входящее ребро.
     */
    for (int i = 0; i < g->n; ++i) {
        in[i] = LONG_MAX;
        pre[i] = -1;
        pre_orig[i] = -1;
        id[i] = -1;
        vis[i] = -1;
    }

    for (int i = 0; i < g->m; ++i) {
        int u = g->edges[i].src;
        int v = g->edges[i].dst;
        long w = g->edges[i].weight;

        if (u == v) {
            continue;
        }

        if (w < in[v]) {
            in[v] = w;
            pre[v] = u;
            pre_orig[v] = g->edges[i].orig;
        }
    }

    /*
     * У корня входящего ребра быть не должно.
     */
    in[root] = 0;

    /*
     * Если у какой-то вершины, кроме root,
     * нет входящего ребра, арборесценцию построить нельзя.
     */
    for (int v = 0; v < g->n; ++v) {
        if (v != root && in[v] == LONG_MAX) {
            goto cleanup;
        }
    }

    /*
     * Шаг 2.
     * Ищем циклы среди выбранных минимальных входящих рёбер.
     */
    for (int i = 0; i < g->n; ++i) {
        int v = i;

        while (vis[v] != i && id[v] == -1 && v != root) {
            vis[v] = i;
            v = pre[v];
        }

        if (v != root && id[v] == -1) {
            int u;

            for (u = pre[v]; u != v; u = pre[u]) {
                id[u] = cycle_count;
            }

            id[v] = cycle_count;
            ++cycle_count;
        }
    }

    /*
     * Шаг 3.
     * Если циклов нет, выбранные входящие рёбра уже дают ответ.
     */
    if (cycle_count == 0) {
        for (int v = 0; v < g->n; ++v) {
            if (v == root) {
                continue;
            }

            if (append_selected(
                    selected,
                    selected_count,
                    pre_orig[v]
                ) < 0) {
                goto cleanup;
            }
        }

        rc = 0;
        goto cleanup;
    }

    /*
     * Шаг 4.
     * Сжимаем каждый цикл в отдельную вершину.
     */
    new_n = cycle_count;

    for (int v = 0; v < g->n; ++v) {
        if (id[v] == -1) {
            id[v] = new_n;
            ++new_n;
        }
    }

    new_edges = (naive_edge_t *)malloc(
        (size_t)(g->m > 0 ? g->m : 1) * sizeof(naive_edge_t)
    );

    if (!new_edges) {
        goto cleanup;
    }

    for (int i = 0; i < g->m; ++i) {
        int u = g->edges[i].src;
        int v = g->edges[i].dst;

        int u_id = id[u];
        int v_id = id[v];

        if (u_id == v_id) {
            continue;
        }

        /*
         * Корректировка веса при сжатии:
         * new_weight = old_weight - in[v]
         */
        new_edges[new_m].src = u_id;
        new_edges[new_m].dst = v_id;
        new_edges[new_m].weight = g->edges[i].weight - in[v];
        new_edges[new_m].orig = g->edges[i].orig;
        new_edges[new_m].enter = v;

        ++new_m;
    }

    {
        naive_graph_t contracted;

        contracted.n = new_n;
        contracted.m = new_m;
        contracted.edges = new_edges;

        sub_selected = (int *)malloc(
            (size_t)(g->n > 1 ? g->n - 1 : 1) * sizeof(int)
        );

        if (!sub_selected) {
            goto cleanup;
        }

        /*
         * Шаг 5.
         * Рекурсивно решаем задачу на сжатом графе.
         */
        if (naive_recursive(
                &contracted,
                id[root],
                sub_selected,
                &sub_count
            ) < 0) {
            goto cleanup;
        }

        removed = (char *)calloc((size_t)g->n, sizeof(char));
        if (!removed) {
            goto cleanup;
        }

        /*
         * Шаг 6.
         * Разворачиваем решение.
         *
         * Если ребро из сжатого графа входит в цикл,
         * то внутри цикла надо удалить минимальное входящее ребро
         * той вершины, куда вошло внешнее ребро.
         */
        for (int i = 0; i < sub_count; ++i) {
            int orig = sub_selected[i];
            int pos = find_edge_by_orig(&contracted, orig);

            if (append_selected(
                    selected,
                    selected_count,
                    orig
                ) < 0) {
                goto cleanup;
            }

            if (pos >= 0 &&
                contracted.edges[pos].dst < cycle_count) {
                removed[contracted.edges[pos].enter] = 1;
            }
        }

        /*
         * Добавляем внутренние рёбра циклов,
         * кроме того, которое было удалено.
         */
        for (int v = 0; v < g->n; ++v) {
            if (v == root) {
                continue;
            }

            if (id[v] < cycle_count && !removed[v]) {
                if (append_selected(
                        selected,
                        selected_count,
                        pre_orig[v]
                    ) < 0) {
                    goto cleanup;
                }
            }
        }
    }

    rc = 0;

cleanup:
    free(removed);
    free(sub_selected);
    free(new_edges);

    free(vis);
    free(id);
    free(pre_orig);
    free(pre);
    free(in);

    return rc;
}

int dmst_naive(
    const dmst_graph_t *g,
    int root,
    int *parent_out,
    long *weight_out
) {
    naive_graph_t ng;
    naive_edge_t *edges = NULL;
    int *selected = NULL;
    int selected_count = 0;
    long total = 0;

    if (!g || !parent_out || !weight_out) {
        return -1;
    }

    if (g->n <= 0 || g->m < 0) {
        return -1;
    }

    if (root < 0 || root >= g->n) {
        return -1;
    }

    if (g->m > 0 && !g->edges) {
        return -1;
    }

    /*
     * Частный случай:
     * граф из одной вершины не требует рёбер.
     */
    if (g->n == 1) {
        parent_out[root] = -1;
        *weight_out = 0;
        return 0;
    }

    /*
     * Обязательная проверка:
     * все вершины должны быть достижимы из root.
     */
    if (!dmst_check_reachable(g, root)) {
        return -1;
    }

    edges = (naive_edge_t *)malloc(
        (size_t)(g->m > 0 ? g->m : 1) * sizeof(naive_edge_t)
    );

    selected = (int *)malloc(
        (size_t)(g->n > 1 ? g->n - 1 : 1) * sizeof(int)
    );

    if (!edges || !selected) {
        free(edges);
        free(selected);
        return -1;
    }

    for (int i = 0; i < g->m; ++i) {
        edges[i].src = g->edges[i].src;
        edges[i].dst = g->edges[i].dst;
        edges[i].weight = g->edges[i].weight;
        edges[i].orig = i;
        edges[i].enter = g->edges[i].dst;
    }

    ng.n = g->n;
    ng.m = g->m;
    ng.edges = edges;

    if (naive_recursive(
            &ng,
            root,
            selected,
            &selected_count
        ) < 0) {
        free(selected);
        free(edges);
        return -1;
    }

    if (selected_count != g->n - 1) {
        free(selected);
        free(edges);
        return -1;
    }

    for (int i = 0; i < g->n; ++i) {
        parent_out[i] = -1;
    }

    for (int i = 0; i < selected_count; ++i) {
        int idx = selected[i];

        if (idx < 0 || idx >= g->m) {
            free(selected);
            free(edges);
            return -1;
        }

        parent_out[g->edges[idx].dst] = g->edges[idx].src;
        total += g->edges[idx].weight;
    }

    parent_out[root] = -1;
    *weight_out = total;

    free(selected);
    free(edges);

    return 0;
}
