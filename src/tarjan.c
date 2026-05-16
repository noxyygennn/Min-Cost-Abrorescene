#include "dmst.h"

/*
 * Реализация Tarjan пока использует проверенную реализацию
 * Chu–Liu/Edmonds для обеспечения совместимости API проекта.
 * Интерфейс полностью сохранён, поэтому позже можно заменить
 * тело функции на оптимизированный алгоритм без изменения
 * клиентского кода.
 */
int dmst_tarjan(const dmst_graph_t *g, int root,
                int *parent_out, long *weight_out)
{
    return dmst_naive(g, root, parent_out, weight_out);
}
