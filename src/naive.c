#include "dmst.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "reachability.h"

/* Рекурсивный поиск ориентированного минимального
 * остовного дерева на обращённом графе.
 *
 * Для каждой вершины выбирается минимальное
 * входящее ребро, после чего найденные циклы
 * сворачиваются.
 *
 * selected хранит индексы выбранных рёбер
 * исходного графа.
 */
static long
dmst_recursive(const dmst_graph_t *graph, int root,
               int *selected, int *selected_cnt)
{
    int n = graph->n;
    int m = graph->m;
    const dmst_edge_t *edges = graph->edges;

    /* Временные массивы */
    long *in = (long *)malloc((size_t)n * sizeof(long));
    int *pre = (int *)malloc((size_t)n * sizeof(int));
    int *edgeIdx = (int *)malloc((size_t)n * sizeof(int));
    int *id = (int *)malloc((size_t)n * sizeof(int));
    int *vis = (int *)malloc((size_t)n * sizeof(int));

    if (!in || !pre || !edgeIdx || !id || !vis) {
        free(in);
        free(pre);
        free(edgeIdx);
        free(id);
        free(vis);
        return -1;
    }

    /* Ищем минимальное входящее ребро для каждой вершины */
    for (int i = 0; i < n; ++i) {
        in[i] = LONG_MAX;
        pre[i] = -1;
        edgeIdx[i] = -1;
    }

    for (int i = 0; i < m; ++i) {
        int u = edges[i].src;
        int v = edges[i].dst;
        long w = edges[i].weight;

        /* Петли не учитываем */
        if (u == v) {
            continue;
        }

        if (w < in[v] ||
            (w == in[v] && u < pre[v]))
        {
            in[v] = w;
            pre[v] = u;
            edgeIdx[v] = edges[i].id;
        }
    }

    /* У корня нет входящего ребра */
    in[root] = 0;

    /* Если у вершины нет входящего ребра,
     * дерево построить нельзя */
    for (int v = 0; v < n; ++v) {
        if (v == root) {
            continue;
        }

        if (in[v] == LONG_MAX) {
            free(in);
            free(pre);
            free(edgeIdx);
            free(id);
            free(vis);
            return -1;
        }
    }

    /* Ищем циклы и считаем вес */
    long total = 0;

    for (int i = 0; i < n; ++i) {
        total += (in[i] == LONG_MAX ? 0 : in[i]);
        id[i] = -1;
        vis[i] = -1;
    }

    int cycleNum = 0;

    for (int i = 0; i < n; ++i) {
        int v = i;

        /* Идём по цепочке предков */
        while (vis[v] != i &&
               id[v] == -1 &&
               v != root)
        {
            vis[v] = i;
            v = pre[v];
        }

        /* Найден цикл */
        if (v != root && id[v] == -1) {
            for (int u = pre[v];
                 u != v;
                 u = pre[u])
            {
                id[u] = cycleNum;
            }

            id[v] = cycleNum;
            cycleNum++;
        }
    }

    /* Если циклов нет — решение найдено */
    if (cycleNum == 0) {
        for (int v = 0; v < n; ++v) {
            if (v == root) {
                continue;
            }

            selected[(*selected_cnt)++] =
                edgeIdx[v];
        }

        free(in);
        free(pre);
        free(edgeIdx);
        free(id);
        free(vis);

        return total;
    }

    /* Нумеруем вершины вне циклов */
    int newCount = cycleNum;

    for (int i = 0; i < n; ++i) {
        if (id[i] == -1) {
            id[i] = newCount++;
        }
    }

    /* Строим граф после свёртки циклов */
    dmst_edge_t *newEdges =
        (dmst_edge_t *)malloc(
            (size_t)m *
            sizeof(dmst_edge_t)
        );

    if (!newEdges) {
        free(in);
        free(pre);
        free(edgeIdx);
        free(id);
        free(vis);
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
            long adjW = w;

            if (in[v] != LONG_MAX) {
                adjW = w - in[v];
            }

            newEdges[newM].src = uId;
            newEdges[newM].dst = vId;
            newEdges[newM].weight = adjW;
            newEdges[newM].id = edges[i].id;

            newM++;
        }
    }

    dmst_graph_t contracted;
    contracted.n = newCount;
    contracted.m = newM;
    contracted.edges = newEdges;

    int *subSelected =
        (int *)malloc(
            (size_t)(
                contracted.n > 0
                ? contracted.n - 1
                : 0
            ) * sizeof(int)
        );

    int subCount = 0;

    long subWeight =
        dmst_recursive(
            &contracted,
            id[root],
            subSelected,
            &subCount
        );

    if (subWeight < 0) {
        free(subSelected);
        free(newEdges);
        free(in);
        free(pre);
        free(edgeIdx);
        free(id);
        free(vis);

        return -1;
    }

    total += subWeight;

    /* Помечаем вершины,
     * у которых нужно убрать ребро */
    char *removed =
        (char *)calloc(
            (size_t)n,
            sizeof(char)
        );

    if (!removed) {
        free(subSelected);
        free(newEdges);
        free(in);
        free(pre);
        free(edgeIdx);
        free(id);
        free(vis);

        return -1;
    }

    /* Добавляем рёбра
     * из рекурсивной задачи */
    for (int i = 0; i < subCount; ++i) {
        int idx = subSelected[i];

        if (idx < 0 ||
            idx >= contracted.m)
        {
            continue;
        }

        dmst_edge_t ne =
            contracted.edges[idx];

        selected[(*selected_cnt)++] =
            ne.id;

        if (ne.dst < cycleNum) {
            int vOrig =
                graph->edges[ne.id].dst;

            removed[vOrig] = 1;
        }
    }

    /* Добавляем рёбра до свёртки */
    for (int v = 0; v < n; ++v) {
        if (v == root) {
            continue;
        }

        if (id[v] < cycleNum) {
            if (!removed[v]) {
                selected[(*selected_cnt)++] =
                    edgeIdx[v];
            }
        } else {
            selected[(*selected_cnt)++] =
                edgeIdx[v];
        }
    }

    free(removed);
    free(subSelected);
    free(newEdges);
    free(in);
    free(pre);
    free(edgeIdx);
    free(id);
    free(vis);

    return total;
}

/* Основная функция алгоритма */
int
dmst_naive(const dmst_graph_t *g,
           int root,
           int *parent_out,
           long *weight_out)
{
    if (!g ||
        g->n <= 0 ||
        g->m < 0)
    {
        return -1;
    }

    if (root < 0 ||
        root >= g->n)
    {
        return -1;
    }

    /* Проверяем достижимость */
    if (!dmst_check_reachable(g, root)) {
        return -1;
    }

    /* Строим обращённый граф */
    dmst_edge_t *revEdges =
        (dmst_edge_t *)malloc(
            (size_t)g->m *
            sizeof(dmst_edge_t)
        );

    if (!revEdges) {
        return -1;
    }

    for (int i = 0; i < g->m; ++i) {
        revEdges[i].src =
            g->edges[i].dst;

        revEdges[i].dst =
            g->edges[i].src;

        revEdges[i].weight =
            g->edges[i].weight;

        revEdges[i].id = i;
    }

    dmst_graph_t rev;
    rev.n = g->n;
    rev.m = g->m;
    rev.edges = revEdges;

    int *selected =
        (int *)malloc(
            (size_t)(
                g->n > 0
                ? g->n - 1
                : 0
            ) * sizeof(int)
        );

    if (!selected) {
        free(revEdges);
        return -1;
    }

    int selectedCount = 0;

    long total =
        dmst_recursive(
            &rev,
            root,
            selected,
            &selectedCount
        );

    if (total < 0 ||
        selectedCount != g->n - 1)
    {
        free(selected);
        free(revEdges);
        return -1;
    }

    /* Инициализируем parent */
    for (int i = 0; i < g->n; ++i) {
        parent_out[i] = -1;
    }

    /* Заполняем массив родителей */
    for (int i = 0;
         i < selectedCount;
         ++i)
    {
        int idx = selected[i];

        if (idx < 0 ||
            idx >= g->m)
        {
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
