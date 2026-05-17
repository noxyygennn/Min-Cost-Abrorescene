#include "dsu.h"

#include <stdlib.h>

/*
 * Инициализация структуры DSU (Disjoint Set Union).
 * Создаёт n независимых множеств:
 * parent[i] = i.
 *
 * Возвращает:
 * 0  - успех
 * -1 - ошибка выделения памяти или некорректные аргументы
 */
int dsu_init(dsu_t *dsu, int n)
{
    if (!dsu || n <= 0) {
        return -1;
    }

    dsu->n = n;

    /* Массив родителей */
    dsu->parent = (int *)malloc((size_t)n * sizeof(int));

    /* Ранги деревьев, изначально равны 0 */
    dsu->rank = (int *)calloc((size_t)n, sizeof(int));

    if (!dsu->parent || !dsu->rank) {
        free(dsu->parent);
        free(dsu->rank);

        dsu->parent = NULL;
        dsu->rank = NULL;
        dsu->n = 0;

        return -1;
    }

    /* Изначально каждая вершина является своим представителем */
    for (int i = 0; i < n; ++i) {
        dsu->parent[i] = i;
    }

    return 0;
}

/*
 * Освобождение памяти, выделенной под DSU.
 */
void dsu_free(dsu_t *dsu)
{
    if (!dsu) {
        return;
    }

    free(dsu->parent);
    free(dsu->rank);

    dsu->parent = NULL;
    dsu->rank = NULL;
    dsu->n = 0;
}

/*
 * Поиск представителя множества.
 * Используется компрессия пути для ускорения
 * последующих операций поиска.
 */
int dsu_find(dsu_t *dsu, int x)
{
    if (dsu->parent[x] != x) {
        dsu->parent[x] = dsu_find(dsu, dsu->parent[x]);
    }

    return dsu->parent[x];
}

/*
 * Объединение двух множеств.
 * Используется union by rank:
 * дерево меньшей высоты присоединяется
 * к дереву большей высоты.
 *
 * Возвращает представителя нового множества.
 */
int dsu_union(dsu_t *dsu, int a, int b)
{
    int ra = dsu_find(dsu, a);
    int rb = dsu_find(dsu, b);

    if (ra == rb) {
        return ra;
    }

    /* Прикрепляем менее глубокое дерево */
    if (dsu->rank[ra] < dsu->rank[rb]) {
        dsu->parent[ra] = rb;
        return rb;
    }

    if (dsu->rank[ra] > dsu->rank[rb]) {
        dsu->parent[rb] = ra;
        return ra;
    }

    /* Если ранги равны — увеличиваем высоту нового корня */
    dsu->parent[rb] = ra;
    dsu->rank[ra]++;

    return ra;
}
