#include "dmst.h"
#include "reachability.h"
#include "fib_heap.h"

#include <limits.h>
#include <stdlib.h>

/*
 * Основные идеи:
 *   1. Для каждой вершины строится Fibonacci heap входящих рёбер.
 *   2. Минимальное входящее ребро извлекается через extract-min.
 *   3. Выбранные рёбра проверяются на циклы.
 *   4. Циклы сжимаются.
 *   5. Задача рекурсивно решается на сжатом графе.
 *   6. Решение разворачивается обратно в исходные рёбра.
 *
 * Эта версия сохраняет интерфейс проекта:
 *   parent_out[v] = родитель вершины v
 *   parent_out[root] = -1
 */

typedef struct {
    int src;
    int dst;
    long weight;

    /*
     * Индекс ребра в исходном верхнеуровневом графе.
     * Нужен для восстановления ответа после сжатий.
     */
    int orig;

    /*
     * Вершина исходного текущего графа, в которую входило ребро.
     * Нужна для удаления правильного внутреннего ребра цикла
     * при разворачивании.
     */
    int enter;
} tarjan_edge_t;

typedef struct {
    int n;
    int m;
    tarjan_edge_t *edges;
} tarjan_graph_t;

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
    const tarjan_graph_t *g,
    int orig
) {
    for (int i = 0; i < g->m; ++i) {
        if (g->edges[i].orig == orig) {
            return i;
        }
    }

    return -1;
}

static void free_vertex_heaps(
    fib_heap_t *heaps,
    int n
) {
    if (!heaps) {
        return;
    }

    for (int i = 0; i < n; ++i) {
        fib_heap_free(&heaps[i]);
    }

    free(heaps);
}

static fib_heap_t *build_incoming_heaps(
    const tarjan_graph_t *g
) {
    fib_heap_t *heaps =
        (fib_heap_t *)malloc((size_t)g->n * sizeof(fib_heap_t));

    if (!heaps) {
        return NULL;
    }

    for (int i = 0; i < g->n; ++i) {
        fib_heap_init(&heaps[i]);
    }

    for (int i = 0; i < g->m; ++i) {
        int dst = g->edges[i].dst;

        if (g->edges[i].src == g->edges[i].dst) {
            continue;
        }

        if (!fib_heap_insert(
                &heaps[dst],
                g->edges[i].weight,
                i,
                g->edges[i].src,
                g->edges[i].dst
            )) {
            free_vertex_heaps(heaps, g->n);
            return NULL;
        }
    }

    return heaps;
}

static int choose_min_incoming_edges(
    const tarjan_graph_t *g,
    int root,
    long *in,
    int *pre,
    int *pre_orig,
    fib_heap_t *heaps
) {
    for (int v = 0; v < g->n; ++v) {
        in[v] = LONG_MAX;
        pre[v] = -1;
        pre_orig[v] = -1;
    }

    in[root] = 0;

    for (int v = 0; v < g->n; ++v) {
        fib_node_t *node = NULL;

        if (v == root) {
            continue;
        }

        /*
         * Извлекаем минимальное входящее ребро для вершины v.
         *
         * Если попались петли или некорректные рёбра,
         * пропускаем их.
         */
        while ((node = fib_heap_extract_min(&heaps[v])) != NULL) {
            int edge_pos = node->edge_id;

            if (edge_pos >= 0 && edge_pos < g->m) {
                tarjan_edge_t e = g->edges[edge_pos];

                free(node);

                if (e.src != e.dst) {
                    in[v] = e.weight;
                    pre[v] = e.src;
                    pre_orig[v] = e.orig;
                    break;
                }
            } else {
                free(node);
            }
        }

        if (in[v] == LONG_MAX) {
            return -1;
        }
    }

    return 0;
}

static int find_cycles(
    int n,
    int root,
    const int *pre,
    int *id,
    int *vis
) {
    int cycle_count = 0;

    for (int i = 0; i < n; ++i) {
        id[i] = -1;
        vis[i] = -1;
    }

    for (int i = 0; i < n; ++i) {
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

    return cycle_count;
}

static tarjan_edge_t *build_contracted_edges(
    const tarjan_graph_t *g,
    const long *in,
    const int *id,
    int *new_m
) {
    tarjan_edge_t *new_edges =
        (tarjan_edge_t *)malloc((size_t)(g->m > 0 ? g->m : 1) *
                                sizeof(tarjan_edge_t));

    if (!new_edges) {
        return NULL;
    }

    *new_m = 0;

    for (int i = 0; i < g->m; ++i) {
        int u = g->edges[i].src;
        int v = g->edges[i].dst;

        int u_id = id[u];
        int v_id = id[v];

        if (u_id == v_id) {
            continue;
        }

        /*
         * При входе в сжатый цикл вес корректируется:
         *
         * new_weight = old_weight - min_incoming_weight[v]
         *
         * Это стандартный шаг Chu-Liu/Edmonds/Tarjan.
         */
        new_edges[*new_m].src = u_id;
        new_edges[*new_m].dst = v_id;
        new_edges[*new_m].weight = g->edges[i].weight - in[v];
        new_edges[*new_m].orig = g->edges[i].orig;
        new_edges[*new_m].enter = v;

        ++(*new_m);
    }

    return new_edges;
}

static int tarjan_recursive(
    const tarjan_graph_t *g,
    int root,
    int *selected,
    int *selected_count
) {
    long *in = NULL;
    int *pre = NULL;
    int *pre_orig = NULL;
    int *id = NULL;
    int *vis = NULL;

    fib_heap_t *heaps = NULL;

    tarjan_edge_t *new_edges = NULL;
    int *sub_selected = NULL;
    char *removed = NULL;

    int cycle_count;
    int new_n;
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

    heaps = build_incoming_heaps(g);
    if (!heaps) {
        goto cleanup;
    }

    /*
     * 1. Fibonacci heaps дают минимальные входящие рёбра.
     */
    if (choose_min_incoming_edges(
            g,
            root,
            in,
            pre,
            pre_orig,
            heaps
        ) < 0) {
        goto cleanup;
    }

    /*
     * 2. Ищем циклы среди выбранных минимальных входящих рёбер.
     */
    cycle_count = find_cycles(
        g->n,
        root,
        pre,
        id,
        vis
    );

    /*
     * 3. Если циклов нет, выбранные рёбра дают ответ.
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
     * 4. Нумеруем новые вершины.
     *
     * Сначала идут сжатые циклы:
     *   0 ... cycle_count - 1
     *
     * Потом одиночные вершины вне циклов.
     */
    new_n = cycle_count;

    for (int v = 0; v < g->n; ++v) {
        if (id[v] == -1) {
            id[v] = new_n;
            ++new_n;
        }
    }

    /*
     * 5. Строим сжатый граф.
     */
    new_edges = build_contracted_edges(
        g,
        in,
        id,
        &new_m
    );

    if (!new_edges) {
        goto cleanup;
    }

    {
        tarjan_graph_t contracted;

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
         * 6. Рекурсивно решаем задачу на сжатом графе.
         */
        if (tarjan_recursive(
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
         * 7. Разворачиваем рёбра, выбранные в сжатом графе.
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

            /*
             * Если выбранное внешнее ребро входит в сжатый цикл,
             * нужно удалить внутреннее минимальное ребро,
             * которое входило в вершину enter.
             */
            if (pos >= 0 &&
                contracted.edges[pos].dst < cycle_count) {
                removed[contracted.edges[pos].enter] = 1;
            }
        }

        /*
         * 8. Добавляем внутренние рёбра циклов,
         * кроме удалённого ребра входа.
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

    free_vertex_heaps(heaps, g->n);

    free(vis);
    free(id);
    free(pre_orig);
    free(pre);
    free(in);

    return rc;
}

int dmst_tarjan(
    const dmst_graph_t *g,
    int root,
    int *parent_out,
    long *weight_out
) {
    tarjan_graph_t tg;
    tarjan_edge_t *edges = NULL;
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
     * Обязательная проверка из ТЗ:
     * из root должны быть достижимы все вершины.
     */
    if (!dmst_check_reachable(g, root)) {
        return -1;
    }

    edges = (tarjan_edge_t *)malloc(
        (size_t)(g->m > 0 ? g->m : 1) * sizeof(tarjan_edge_t)
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

    tg.n = g->n;
    tg.m = g->m;
    tg.edges = edges;

    if (tarjan_recursive(
            &tg,
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
