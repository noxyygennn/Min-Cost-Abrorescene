#include "dmst.h"
#include "reachability.h"
#include "fib_heap.h"
#include "dsu.h"

#include <limits.h>
#include <stdlib.h>



typedef struct {
    int src;
    int dst;
    long weight;

    /*
     * Индекс ребра в исходном графе пользователя.
     */
    int orig;

    /*
     * Вершина текущего графа, в которую входило ребро.
     * Нужно для разворачивания сжатого цикла.
     */
    int enter;
} gabow_edge_t;

typedef struct {
    int n;
    int m;
    gabow_edge_t *edges;
} gabow_graph_t;

typedef struct {
    int *data;
    int size;
    int cap;
} int_vec_t;

static int vec_init(int_vec_t *v)
{
    v->size = 0;
    v->cap = 8;
    v->data = (int *)malloc((size_t)v->cap * sizeof(int));

    return v->data ? 0 : -1;
}

static void vec_free(int_vec_t *v)
{
    if (!v) {
        return;
    }

    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}

static int vec_push(int_vec_t *v, int x)
{
    int *new_data;

    if (v->size == v->cap) {
        int new_cap = v->cap * 2;

        new_data = (int *)realloc(
            v->data,
            (size_t)new_cap * sizeof(int)
        );

        if (!new_data) {
            return -1;
        }

        v->data = new_data;
        v->cap = new_cap;
    }

    v->data[v->size++] = x;
    return 0;
}

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
    const gabow_graph_t *g,
    int orig
) {
    for (int i = 0; i < g->m; ++i) {
        if (g->edges[i].orig == orig) {
            return i;
        }
    }

    return -1;
}

static void free_heaps(
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

/*
 * Для каждой вершины строим Fibonacci heap входящих рёбер.
 *
 * В heap вершины v кладём все рёбра u -> v.
 */
static fib_heap_t *build_incoming_heaps(
    const gabow_graph_t *g
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
            free_heaps(heaps, g->n);
            return NULL;
        }
    }

    return heaps;
}

/*
 * Выбор минимального входящего ребра для каждой вершины.
 *
 * В версии Gabow-style мы дополнительно используем DSU:
 * если ребро ведёт внутри уже известной компоненты, его можно пропустить.
 */
static int choose_min_edges_with_dsu(
    const gabow_graph_t *g,
    int root,
    long *in,
    int *pre,
    int *pre_orig,
    fib_heap_t *heaps,
    dsu_t *dsu
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

        while ((node = fib_heap_extract_min(&heaps[v])) != NULL) {
            int edge_pos = node->edge_id;

            free(node);

            if (edge_pos < 0 || edge_pos >= g->m) {
                continue;
            }

            {
                gabow_edge_t e = g->edges[edge_pos];

                if (e.src == e.dst) {
                    continue;
                }

                /*
                 * DSU-фильтр:
                 * если концы ребра уже в одной компоненте,
                 * такое ребро не может быть внешним входящим.
                 */
                if (dsu_find(dsu, e.src) == dsu_find(dsu, e.dst)) {
                    continue;
                }

                in[v] = e.weight;
                pre[v] = e.src;
                pre_orig[v] = e.orig;
                break;
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

/*
 * Собираем список вершин каждого найденного цикла.
 */
static int build_cycle_members(
    int n,
    int cycle_count,
    const int *id,
    int_vec_t *cycles
) {
    for (int c = 0; c < cycle_count; ++c) {
        if (vec_init(&cycles[c]) < 0) {
            for (int j = 0; j < c; ++j) {
                vec_free(&cycles[j]);
            }

            return -1;
        }
    }

    for (int v = 0; v < n; ++v) {
        if (id[v] >= 0 && id[v] < cycle_count) {
            if (vec_push(&cycles[id[v]], v) < 0) {
                for (int c = 0; c < cycle_count; ++c) {
                    vec_free(&cycles[c]);
                }

                return -1;
            }
        }
    }

    return 0;
}

/*
 * Gabow-style merge step.
 *
 * Здесь мы явно объединяем компоненты цикла через DSU
 * и сливаем их Fibonacci heaps.
 *
 * Важно:
 *   fib_heap_add_offset используется как lazy-механизм.
 *   Он позволяет не пересчитывать сразу все ключи.
 */
static int merge_cycle_components(
    int cycle_count,
    int_vec_t *cycles,
    fib_heap_t *heaps,
    dsu_t *dsu,
    const long *in
) {
    for (int c = 0; c < cycle_count; ++c) {
        if (cycles[c].size == 0) {
            continue;
        }

        {
            int base = cycles[c].data[0];

            /*
             * Для каждой вершины цикла применяем lazy offset:
             * вычитаем стоимость выбранного минимального входящего ребра.
             *
             * Это соответствует пересчёту:
             *   w'(u, v) = w(u, v) - in[v]
             */
            for (int i = 0; i < cycles[c].size; ++i) {
                int v = cycles[c].data[i];

                if (in[v] != LONG_MAX && in[v] != 0) {
                    fib_heap_add_offset(&heaps[v], -in[v]);
                }
            }

            /*
             * Сливаем DSU-компоненты и кучи вершин цикла.
             */
            for (int i = 1; i < cycles[c].size; ++i) {
                int v = cycles[c].data[i];

                dsu_union(dsu, base, v);
                fib_heap_meld(&heaps[base], &heaps[v]);
            }
        }
    }

    return 0;
}

static gabow_edge_t *build_contracted_edges(
    const gabow_graph_t *g,
    const long *in,
    const int *id,
    int *new_m
) {
    gabow_edge_t *new_edges =
        (gabow_edge_t *)malloc((size_t)(g->m > 0 ? g->m : 1) *
                               sizeof(gabow_edge_t));

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
         * Стандартная корректировка веса при сжатии:
         *   new_weight = old_weight - in[v]
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

static int gabow_recursive(
    const gabow_graph_t *g,
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
    dsu_t dsu;
    int dsu_ok = 0;

    int_vec_t *cycles = NULL;

    gabow_edge_t *new_edges = NULL;
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

    if (dsu_init(&dsu, g->n) < 0) {
        goto cleanup;
    }

    dsu_ok = 1;

    heaps = build_incoming_heaps(g);
    if (!heaps) {
        goto cleanup;
    }

    /*
     * 1. Выбираем минимальные входящие рёбра через Fibonacci heaps.
     */
    if (choose_min_edges_with_dsu(
            g,
            root,
            in,
            pre,
            pre_orig,
            heaps,
            &dsu
        ) < 0) {
        goto cleanup;
    }

    /*
     * 2. Находим циклы среди выбранных рёбер.
     */
    cycle_count = find_cycles(
        g->n,
        root,
        pre,
        id,
        vis
    );

    /*
     * 3. Если циклов нет, выбранные рёбра уже дают ответ.
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
     * 4. Собираем вершины каждого цикла.
     */
    cycles = (int_vec_t *)malloc(
        (size_t)cycle_count * sizeof(int_vec_t)
    );

    if (!cycles) {
        goto cleanup;
    }

    if (build_cycle_members(
            g->n,
            cycle_count,
            id,
            cycles
        ) < 0) {
        goto cleanup;
    }

    /*
     * 5. Gabow-style часть:
     * объединяем компоненты циклов через DSU,
     * сливаем Fibonacci heaps и применяем lazy offsets.
     */
    if (merge_cycle_components(
            cycle_count,
            cycles,
            heaps,
            &dsu,
            in
        ) < 0) {
        goto cleanup;
    }

    /*
     * 6. Нумеруем вершины сжатого графа.
     */
    new_n = cycle_count;

    for (int v = 0; v < g->n; ++v) {
        if (id[v] == -1) {
            id[v] = new_n;
            ++new_n;
        }
    }

    /*
     * 7. Строим сжатый граф.
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
        gabow_graph_t contracted;

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
         * 8. Рекурсивно решаем задачу на сжатом графе.
         */
        if (gabow_recursive(
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
         * 9. Разворачиваем выбранные рёбра из сжатого графа.
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
             * Если выбранное ребро входит в сжатый цикл,
             * удаляем внутреннее ребро, входящее в вершину enter.
             */
            if (pos >= 0 &&
                contracted.edges[pos].dst < cycle_count) {
                removed[contracted.edges[pos].enter] = 1;
            }
        }

        /*
         * 10. Добавляем внутренние рёбра циклов,
         * кроме удалённых.
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
    if (cycles) {
        for (int c = 0; c < cycle_count; ++c) {
            vec_free(&cycles[c]);
        }

        free(cycles);
    }

    free(removed);
    free(sub_selected);
    free(new_edges);

    free_heaps(heaps, g->n);

    if (dsu_ok) {
        dsu_free(&dsu);
    }

    free(vis);
    free(id);
    free(pre_orig);
    free(pre);
    free(in);

    return rc;
}

int dmst_gabow(
    const dmst_graph_t *g,
    int root,
    int *parent_out,
    long *weight_out
) {
    gabow_graph_t gg;
    gabow_edge_t *edges = NULL;
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
     * Обязательная проверка по ТЗ:
     * все вершины должны быть достижимы из root.
     */
    if (!dmst_check_reachable(g, root)) {
        return -1;
    }

    edges = (gabow_edge_t *)malloc(
        (size_t)(g->m > 0 ? g->m : 1) * sizeof(gabow_edge_t)
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

    gg.n = g->n;
    gg.m = g->m;
    gg.edges = edges;

    if (gabow_recursive(
            &gg,
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
