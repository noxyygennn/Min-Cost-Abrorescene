#include "dmst.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Консольная программа для поиска ориентированного
 * минимального остовного дерева.
 *
 * Запуск:
 *   dmst [naive|tarjan|gabow]
 *
 * Входной формат:
 *
 *   n m root
 *   u1 v1 w1
 *   u2 v2 w2
 *   ...
 *   um vm wm
 *
 * где:
 *   n    — число вершин,
 *   m    — число рёбер,
 *   root — корневая вершина.
 *
 * Далее идут рёбра графа:
 *   ui -> vi с весом wi.
 *
 * После вычисления программа выводит:
 *   - суммарный вес дерева,
 *   - список выбранных рёбер.
 */
int main(int argc, char **argv)
{
    /* По умолчанию используем алгоритм Габова */
    const char *algo = "gabow";

    if (argc > 1) {
        algo = argv[1];
    }

    int n, m, root;

    if (scanf("%d %d %d", &n, &m, &root) != 3) {
        fprintf(stderr, "Некорректный формат входных данных.\n");
        return 1;
    }

    /* Проверка параметров графа */
    if (n <= 0 || m < 0 || root < 0 || root >= n) {
        fprintf(stderr, "Некорректные параметры графа.\n");
        return 1;
    }

    dmst_graph_t graph;
    graph.n = n;
    graph.m = m;

    graph.edges =
        (dmst_edge_t *)malloc((size_t)m * sizeof(dmst_edge_t));

    if (!graph.edges) {
        fprintf(stderr, "Не удалось выделить память.\n");
        return 1;
    }

    /* Считывание рёбер графа */
    for (int i = 0; i < m; ++i) {
        int u, v;
        long w;

        if (scanf("%d %d %ld", &u, &v, &w) != 3) {
            fprintf(stderr, "Ошибка при чтении ребра.\n");

            free(graph.edges);
            return 1;
        }

        /* Проверка индексов вершин */
        if (u < 0 || u >= n ||
            v < 0 || v >= n)
        {
            fprintf(stderr, "Индекс вершины вне диапазона.\n");

            free(graph.edges);
            return 1;
        }

        graph.edges[i].src = u;
        graph.edges[i].dst = v;
        graph.edges[i].weight = w;
        graph.edges[i].id = i;
    }

    int *parent =
        (int *)malloc((size_t)n * sizeof(int));

    if (!parent) {
        fprintf(stderr, "Не удалось выделить память.\n");

        free(graph.edges);
        return 1;
    }

    long totalWeight = 0;
    int result = -1;

    /* Выбор алгоритма */
    if (strcmp(algo, "naive") == 0) {
        result =
            dmst_naive(
                &graph,
                root,
                parent,
                &totalWeight
            );
    }
    else if (strcmp(algo, "tarjan") == 0) {
        result =
            dmst_tarjan(
                &graph,
                root,
                parent,
                &totalWeight
            );
    }
    else if (strcmp(algo, "gabow") == 0) {
        result =
            dmst_gabow(
                &graph,
                root,
                parent,
                &totalWeight
            );
    }
    else {
        fprintf(stderr, "Неизвестный алгоритм: %s\n", algo);

        free(parent);
        free(graph.edges);

        return 1;
    }

    /* Остовное дерево не существует */
    if (result < 0) {
        fprintf(stderr,
                "Невозможно построить остовное дерево из корня %d.\n",
                root);

        free(parent);
        free(graph.edges);

        return 1;
    }

    /* Вывод суммарного веса */
    printf("%ld\n", totalWeight);

    /*
     * Вывод выбранных рёбер.
     * Для корня parent[root] = -1,
     * поэтому его пропускаем.
     */
    for (int v = 0; v < n; ++v) {

        if (v == root) continue;
        
        printf("%d -> %d\n",
               parent[v],
               v);
    }
    free(parent);
    free(graph.edges);

    return 0;
}
