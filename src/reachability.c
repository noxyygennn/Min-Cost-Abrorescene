#include "reachability.h"
#include <stdlib.h>
/* Проверка достижимости root -> все вершины через список смежности */
int dmst_check_reachable(const dmst_graph_t *g, int root){
    int *head = malloc(g->n * sizeof(int));
    int *next = malloc((g->m ? g->m : 1) * sizeof(int));
    char *vis = calloc(g->n, 1); 
    int *q = malloc(g->n * sizeof(int));

    if (!head || !next || !vis || !q){
        free(head);
        free(next);
        free(vis);
        free(q);
        return 0;
    }

    for(int i = 0; i < g->n; i++){ 
        head[i]=-1;
    }
    for (int i = 0; i < g->m; i++){ 
        next[i] = head[g->edges[i].src];
        head[g->edges[i].src] = i; 
    }
    int h = 0, t = 0; vis[root] = 1; q[t++] = root;
    while (h < t){
        int u = q[h++]; 
        for (int e = head[u];e != -1; e = next[e]){
            int v = g->edges[e].dst; 
            if(!vis[v]){
                vis[v] = 1;
                q[t++] = v;
            }
        }
    }
    int ok = 1; 
    for (int i = 0; i < g->n; i++){
        if (!vis[i]) ok = 0;
    }
    free(head); free(next); free(vis); free(q); return ok;
}