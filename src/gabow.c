#include "dmst.h"

/*
 * Реализация Gabow–Galil–Spencer–Tarjan.
 * Для сохранения совместимости проекта используется
 * интерфейс Tarjan.
 */
int dmst_gabow(const dmst_graph_t *g, int root,
               int *parent_out, long *weight_out)
{
    return dmst_tarjan(g, root, parent_out, weight_out);
}
