#include "dmst.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Простая макрофункция для проверки условия.
 * При ошибке выполнение тестов завершается.
 */
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            fprintf(stderr,                                              \
                    "Тест не пройден: %s (%s:%d)\n",                     \
                    #cond,                                               \
                    __FILE__,                                            \
                    __LINE__);                                           \
            exit(EXIT_FAILURE);                                          \
        }                                                                \
    } while (0)

/*
 * Запуск всех реализаций алгоритма
 * и проверка результата по parent_out и total weight.
 */
static void run_all_algorithms(
    const dmst_graph_t *g,
    int root,
    const int *expected_parent,
    long expected_weight
) {
    int n = g->n;

    int *parent =
        (int *)malloc((size_t)n * sizeof(int));

    long w;
    int rc;

    CHECK(parent != NULL);

    /*
     * Проверяем наивную реализацию.
     */
    rc = dmst_naive(g, root, parent, &w);

    CHECK(rc == 0);
    CHECK(w == expected_weight);

    for (int i = 0; i < n; ++i) {
        CHECK(parent[i] == expected_parent[i]);
    }

    /*
     * Проверяем реализацию Тарьяна.
     */
    rc = dmst_tarjan(g, root, parent, &w);

    CHECK(rc == 0);
    CHECK(w == expected_weight);

    for (int i = 0; i < n; ++i) {
        CHECK(parent[i] == expected_parent[i]);
    }

    /*
     * Проверяем реализацию Габова.
     */
    rc = dmst_gabow(g, root, parent, &w);

    CHECK(rc == 0);
    CHECK(w == expected_weight);

    for (int i = 0; i < n; ++i) {
        CHECK(parent[i] == expected_parent[i]);
    }

    free(parent);
}

/*
 * Запуск всех реализаций алгоритма
 * и проверка только по итоговому весу.
 *
 * Это нужно для сложных графов, где может существовать
 * несколько разных минимальных арборесценций с одинаковым весом.
 */
static void run_all_algorithms_weight_only(
    const dmst_graph_t *g,
    int root,
    long expected_weight
) {
    int n = g->n;

    int *parent =
        (int *)malloc((size_t)n * sizeof(int));

    long w;
    int rc;

    CHECK(parent != NULL);

    rc = dmst_naive(g, root, parent, &w);
    CHECK(rc == 0);
    CHECK(w == expected_weight);

    rc = dmst_tarjan(g, root, parent, &w);
    CHECK(rc == 0);
    CHECK(w == expected_weight);

    rc = dmst_gabow(g, root, parent, &w);
    CHECK(rc == 0);
    CHECK(w == expected_weight);

    free(parent);
}

/*
 * Граф из одной вершины без рёбер.
 *
 * Вес дерева должен быть 0,
 * parent[root] = -1.
 */
static void test_single_vertex(void) {
    dmst_graph_t g;

    g.n = 1;
    g.m = 0;
    g.edges = NULL;

    {
        int expected_parent[1] = {
            -1
        };

        run_all_algorithms(
            &g,
            0,
            expected_parent,
            0
        );
    }
}

/*
 * Простая цепочка:
 *
 * 0 -> 1
 *
 * Вес ребра = 5.
 */
static void test_simple_chain(void) {
    dmst_graph_t g;

    g.n = 2;
    g.m = 1;

    g.edges = (dmst_edge_t *)malloc(sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 5;
    g.edges[0].id = 0;

    {
        int expected_parent[2] = {
            -1,
            0
        };

        run_all_algorithms(
            &g,
            0,
            expected_parent,
            5
        );
    }

    free(g.edges);
}

/*
 * Граф, где корень не достигает всех вершин.
 *
 * Все алгоритмы должны вернуть ошибку.
 */
static void test_unreachable(void) {
    dmst_graph_t g;
    int parent[3];
    long w;

    g.n = 3;
    g.m = 1;

    g.edges = (dmst_edge_t *)malloc(sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 1;
    g.edges[0].dst = 2;
    g.edges[0].weight = 2;
    g.edges[0].id = 0;

    CHECK(dmst_naive(&g, 0, parent, &w) < 0);
    CHECK(dmst_tarjan(&g, 0, parent, &w) < 0);
    CHECK(dmst_gabow(&g, 0, parent, &w) < 0);

    free(g.edges);
}

/*
 * Простой цикл:
 *
 * 0 -> 1
 * 1 -> 2
 * 2 -> 0
 *
 * При root = 0 ожидаем:
 * 0 -> 1
 * 1 -> 2
 *
 * Вес = 2.
 */
static void test_simple_cycle(void) {
    dmst_graph_t g;

    g.n = 3;
    g.m = 3;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 1;
    g.edges[0].id = 0;

    g.edges[1].src = 1;
    g.edges[1].dst = 2;
    g.edges[1].weight = 1;
    g.edges[1].id = 1;

    g.edges[2].src = 2;
    g.edges[2].dst = 0;
    g.edges[2].weight = 1;
    g.edges[2].id = 2;

    {
        int expected_parent[3] = {
            -1,
            0,
            1
        };

        run_all_algorithms(
            &g,
            0,
            expected_parent,
            2
        );
    }

    free(g.edges);
}

/*
 * Более сложный граф с несколькими вариантами рёбер.
 *
 * Ожидаемое дерево:
 * 0 -> 1
 * 0 -> 2
 * 2 -> 3
 *
 * Вес = 25.
 */
static void test_complex_graph(void) {
    dmst_graph_t g;

    g.n = 4;
    g.m = 6;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 10;
    g.edges[0].id = 0;

    g.edges[1].src = 0;
    g.edges[1].dst = 2;
    g.edges[1].weight = 5;
    g.edges[1].id = 1;

    g.edges[2].src = 0;
    g.edges[2].dst = 3;
    g.edges[2].weight = 15;
    g.edges[2].id = 2;

    g.edges[3].src = 1;
    g.edges[3].dst = 3;
    g.edges[3].weight = 40;
    g.edges[3].id = 3;

    g.edges[4].src = 2;
    g.edges[4].dst = 3;
    g.edges[4].weight = 10;
    g.edges[4].id = 4;

    g.edges[5].src = 3;
    g.edges[5].dst = 2;
    g.edges[5].weight = 7;
    g.edges[5].id = 5;

    {
        int expected_parent[4] = {
            -1,
            0,
            0,
            2
        };

        run_all_algorithms(
            &g,
            0,
            expected_parent,
            25
        );
    }

    free(g.edges);
}

/*
 * Граф с отрицательными весами.
 *
 * Ожидаемое дерево:
 * 0 -> 1
 * 1 -> 2
 * 2 -> 3
 *
 * Вес = 1.
 */
static void test_negative_weights(void) {
    dmst_graph_t g;

    g.n = 4;
    g.m = 5;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 2;
    g.edges[0].id = 0;

    g.edges[1].src = 1;
    g.edges[1].dst = 2;
    g.edges[1].weight = -5;
    g.edges[1].id = 1;

    g.edges[2].src = 2;
    g.edges[2].dst = 3;
    g.edges[2].weight = 4;
    g.edges[2].id = 2;

    g.edges[3].src = 0;
    g.edges[3].dst = 3;
    g.edges[3].weight = 10;
    g.edges[3].id = 3;

    g.edges[4].src = 3;
    g.edges[4].dst = 1;
    g.edges[4].weight = -2;
    g.edges[4].id = 4;

    {
        int expected_parent[4] = {
            -1,
            0,
            1,
            2
        };

        run_all_algorithms(
            &g,
            0,
            expected_parent,
            1
        );
    }

    free(g.edges);
}

/*
 * Параллельные рёбра между одинаковыми вершинами.
 *
 * Алгоритм должен выбрать более дешёвое ребро 0 -> 1.
 *
 * Вес = 3 + 2 = 5.
 */
static void test_parallel_edges(void) {
    dmst_graph_t g;

    g.n = 3;
    g.m = 3;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 5;
    g.edges[0].id = 0;

    g.edges[1].src = 0;
    g.edges[1].dst = 1;
    g.edges[1].weight = 3;
    g.edges[1].id = 1;

    g.edges[2].src = 1;
    g.edges[2].dst = 2;
    g.edges[2].weight = 2;
    g.edges[2].id = 2;

    {
        int expected_parent[3] = {
            -1,
            0,
            1
        };

        run_all_algorithms(
            &g,
            0,
            expected_parent,
            5
        );
    }

    free(g.edges);
}

/*
 * Сложный граф с несколькими конкурирующими циклами.
 *
 * Корень: 0
 *
 * Один оптимальный результат:
 *   0 -> 1 вес 4
 *   1 -> 2 вес 1
 *   2 -> 3 вес 1
 *   3 -> 4 вес 1
 *
 * Общий вес = 7.
 */
static void test_multiple_cycles_weight(void) {
    dmst_graph_t g;

    g.n = 5;
    g.m = 9;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 4;
    g.edges[0].id = 0;

    g.edges[1].src = 1;
    g.edges[1].dst = 2;
    g.edges[1].weight = 1;
    g.edges[1].id = 1;

    g.edges[2].src = 2;
    g.edges[2].dst = 1;
    g.edges[2].weight = 1;
    g.edges[2].id = 2;

    g.edges[3].src = 2;
    g.edges[3].dst = 3;
    g.edges[3].weight = 1;
    g.edges[3].id = 3;

    g.edges[4].src = 3;
    g.edges[4].dst = 4;
    g.edges[4].weight = 1;
    g.edges[4].id = 4;

    g.edges[5].src = 4;
    g.edges[5].dst = 3;
    g.edges[5].weight = 1;
    g.edges[5].id = 5;

    g.edges[6].src = 0;
    g.edges[6].dst = 3;
    g.edges[6].weight = 10;
    g.edges[6].id = 6;

    g.edges[7].src = 0;
    g.edges[7].dst = 4;
    g.edges[7].weight = 10;
    g.edges[7].id = 7;

    g.edges[8].src = 1;
    g.edges[8].dst = 4;
    g.edges[8].weight = 8;
    g.edges[8].id = 8;

    run_all_algorithms_weight_only(
        &g,
        0,
        7
    );

    free(g.edges);
}

/*
 * Граф с циклом, который требует сжатия и разворачивания.
 *
 * Корень: 0
 *
 * Один оптимальный результат:
 *   0 -> 1 вес 5
 *   1 -> 2 вес 1
 *   2 -> 3 вес 1
 *   3 -> 4 вес 1
 *
 * Общий вес = 8.
 */
static void test_nested_cycle_weight(void) {
    dmst_graph_t g;

    g.n = 5;
    g.m = 8;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 5;
    g.edges[0].id = 0;

    g.edges[1].src = 1;
    g.edges[1].dst = 2;
    g.edges[1].weight = 1;
    g.edges[1].id = 1;

    g.edges[2].src = 2;
    g.edges[2].dst = 3;
    g.edges[2].weight = 1;
    g.edges[2].id = 2;

    g.edges[3].src = 3;
    g.edges[3].dst = 1;
    g.edges[3].weight = 1;
    g.edges[3].id = 3;

    g.edges[4].src = 3;
    g.edges[4].dst = 4;
    g.edges[4].weight = 1;
    g.edges[4].id = 4;

    g.edges[5].src = 0;
    g.edges[5].dst = 2;
    g.edges[5].weight = 9;
    g.edges[5].id = 5;

    g.edges[6].src = 0;
    g.edges[6].dst = 3;
    g.edges[6].weight = 9;
    g.edges[6].id = 6;

    g.edges[7].src = 4;
    g.edges[7].dst = 2;
    g.edges[7].weight = 3;
    g.edges[7].id = 7;

    run_all_algorithms_weight_only(
        &g,
        0,
        8
    );

    free(g.edges);
}

/*
 * Граф с отрицательными весами и циклом.
 *
 * Оптимальное дерево:
 *   0 -> 1 вес 2
 *   1 -> 2 вес -4
 *   2 -> 3 вес -3
 *
 * Общий вес = -5.
 */
static void test_negative_cycle_weight(void) {
    dmst_graph_t g;

    g.n = 4;
    g.m = 6;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 2;
    g.edges[0].id = 0;

    g.edges[1].src = 1;
    g.edges[1].dst = 2;
    g.edges[1].weight = -4;
    g.edges[1].id = 1;

    g.edges[2].src = 2;
    g.edges[2].dst = 3;
    g.edges[2].weight = -3;
    g.edges[2].id = 2;

    g.edges[3].src = 3;
    g.edges[3].dst = 1;
    g.edges[3].weight = -2;
    g.edges[3].id = 3;

    g.edges[4].src = 0;
    g.edges[4].dst = 2;
    g.edges[4].weight = 10;
    g.edges[4].id = 4;

    g.edges[5].src = 0;
    g.edges[5].dst = 3;
    g.edges[5].weight = 10;
    g.edges[5].id = 5;

    run_all_algorithms_weight_only(
        &g,
        0,
        -5
    );

    free(g.edges);
}

/*
 * Плотный граф.
 *
 * Оптимальное дерево:
 *   0 -> 1 вес 1
 *   1 -> 2 вес 1
 *   2 -> 3 вес 1
 *   3 -> 4 вес 1
 *
 * Общий вес = 4.
 */
static void test_dense_graph_weight(void) {
    dmst_graph_t g;

    g.n = 5;
    g.m = 12;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 1;
    g.edges[0].id = 0;

    g.edges[1].src = 1;
    g.edges[1].dst = 2;
    g.edges[1].weight = 1;
    g.edges[1].id = 1;

    g.edges[2].src = 2;
    g.edges[2].dst = 3;
    g.edges[2].weight = 1;
    g.edges[2].id = 2;

    g.edges[3].src = 3;
    g.edges[3].dst = 4;
    g.edges[3].weight = 1;
    g.edges[3].id = 3;

    g.edges[4].src = 0;
    g.edges[4].dst = 2;
    g.edges[4].weight = 8;
    g.edges[4].id = 4;

    g.edges[5].src = 0;
    g.edges[5].dst = 3;
    g.edges[5].weight = 8;
    g.edges[5].id = 5;

    g.edges[6].src = 0;
    g.edges[6].dst = 4;
    g.edges[6].weight = 8;
    g.edges[6].id = 6;

    g.edges[7].src = 1;
    g.edges[7].dst = 3;
    g.edges[7].weight = 6;
    g.edges[7].id = 7;

    g.edges[8].src = 1;
    g.edges[8].dst = 4;
    g.edges[8].weight = 6;
    g.edges[8].id = 8;

    g.edges[9].src = 2;
    g.edges[9].dst = 4;
    g.edges[9].weight = 5;
    g.edges[9].id = 9;

    g.edges[10].src = 4;
    g.edges[10].dst = 1;
    g.edges[10].weight = 20;
    g.edges[10].id = 10;

    g.edges[11].src = 3;
    g.edges[11].dst = 1;
    g.edges[11].weight = 20;
    g.edges[11].id = 11;

    run_all_algorithms_weight_only(
        &g,
        0,
        4
    );

    free(g.edges);
}

/*
 * Сравнение трёх реализаций между собой.
 *
 * Здесь expected_weight заранее не задаём.
 * Берём результат naive как эталон и проверяем,
 * что tarjan и gabow дают такой же вес.
 */
static void test_cross_check_algorithms(void) {
    dmst_graph_t g;

    g.n = 6;
    g.m = 13;

    g.edges = (dmst_edge_t *)malloc((size_t)g.m * sizeof(dmst_edge_t));
    CHECK(g.edges != NULL);

    g.edges[0].src = 0;
    g.edges[0].dst = 1;
    g.edges[0].weight = 7;
    g.edges[0].id = 0;

    g.edges[1].src = 0;
    g.edges[1].dst = 2;
    g.edges[1].weight = 9;
    g.edges[1].id = 1;

    g.edges[2].src = 1;
    g.edges[2].dst = 2;
    g.edges[2].weight = 2;
    g.edges[2].id = 2;

    g.edges[3].src = 2;
    g.edges[3].dst = 1;
    g.edges[3].weight = 1;
    g.edges[3].id = 3;

    g.edges[4].src = 1;
    g.edges[4].dst = 3;
    g.edges[4].weight = 3;
    g.edges[4].id = 4;

    g.edges[5].src = 2;
    g.edges[5].dst = 3;
    g.edges[5].weight = 4;
    g.edges[5].id = 5;

    g.edges[6].src = 3;
    g.edges[6].dst = 4;
    g.edges[6].weight = 2;
    g.edges[6].id = 6;

    g.edges[7].src = 4;
    g.edges[7].dst = 5;
    g.edges[7].weight = 2;
    g.edges[7].id = 7;

    g.edges[8].src = 5;
    g.edges[8].dst = 3;
    g.edges[8].weight = 1;
    g.edges[8].id = 8;

    g.edges[9].src = 0;
    g.edges[9].dst = 5;
    g.edges[9].weight = 20;
    g.edges[9].id = 9;

    g.edges[10].src = 2;
    g.edges[10].dst = 5;
    g.edges[10].weight = 6;
    g.edges[10].id = 10;

    g.edges[11].src = 4;
    g.edges[11].dst = 1;
    g.edges[11].weight = 8;
    g.edges[11].id = 11;

    g.edges[12].src = 5;
    g.edges[12].dst = 2;
    g.edges[12].weight = 5;
    g.edges[12].id = 12;

    {
        int *parent = (int *)malloc((size_t)g.n * sizeof(int));

        long w_naive;
        long w_tarjan;
        long w_gabow;

        CHECK(parent != NULL);

        CHECK(dmst_naive(&g, 0, parent, &w_naive) == 0);
        CHECK(dmst_tarjan(&g, 0, parent, &w_tarjan) == 0);
        CHECK(dmst_gabow(&g, 0, parent, &w_gabow) == 0);

        CHECK(w_tarjan == w_naive);
        CHECK(w_gabow == w_naive);

        free(parent);
    }

    free(g.edges);
}

int main(void) {
    test_single_vertex();
    test_simple_chain();
    test_unreachable();
    test_simple_cycle();
    test_complex_graph();
    test_negative_weights();
    test_parallel_edges();

    test_multiple_cycles_weight();
    test_nested_cycle_weight();
    test_negative_cycle_weight();
    test_dense_graph_weight();
    test_cross_check_algorithms();

    printf("Все тесты успешно пройдены.\n");

    return 0;
}
