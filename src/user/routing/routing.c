#include <assert.h>
#include <common/string.h>
#include <gen/track_data.h>
#include <track_node.h>
#include <routing.h>

struct track_node *lookup_track_node(struct track_node *track, char *name) {
  int i;
  for (i = 0; i < TRACK_MAX; i++) {
    if(!streq(track[i].name, name))
      return &track[i];
  }

  return NULL;
}

/* Neighbours must be an array which is large enough to accomodate neighbours + the terminating NULL pointer. */

static void get_neighbours(struct track_node *node, struct track_node *neighbours[3]) {
   switch(node->type) {
     case NODE_SENSOR:
       *neighbours++ = node->edge[DIR_AHEAD].dest;
     	break;
     case NODE_BRANCH:
       *neighbours++ = node->edge[DIR_STRAIGHT].dest;
       *neighbours++ = node->edge[DIR_CURVED].dest;
     	break;
     case NODE_MERGE:
       *neighbours++ = node->edge[DIR_AHEAD].dest;
     	break;
     case NODE_ENTER:
       *neighbours++ = node->edge[DIR_AHEAD].dest;
     	break;
     case NODE_EXIT:
     	break;
     case NODE_NONE:
       assert(0, "WTF? (%x)\n", node);
       break;
   }

   *neighbours = NULL;
 }

/* Returns zero if a path between the nodes exists. The returned path is terminated by a NULL pointer. */
int path_find(struct track_node *track,
    struct track_node *src,
    struct track_node *dst,
    struct track_node **path) {
  int i;
  int p = 0;

  struct track_node *neighbours[3];
  struct track_node **n;

  for (i = 0; i < TRACK_MAX; i++) 
    track[i].seen = 0;

  path[p] = src;
  while(p > -1) {
    if(path[p] == dst) {
      path[++p] = NULL;
      return 0;
    }
    path[p]->seen = 1;

    get_neighbours(path[p], neighbours);

    for(i=0, n = neighbours;*n != NULL;n++)
      if(!((*n)->seen)) {
        i++;
        path[++p] = *n;
        break;
      }

    if(!i)
      path[p--] = NULL;
  }

  return 1;
}

int path_activate(struct track_node **path) {
  struct track_node **c;
  if(!path) return -1;

  for(c = path;*c != NULL;c++) {
    if((*c)->type == NODE_BRANCH) {
      if(*(c+1) && ((*c)->edge[DIR_STRAIGHT].dest == *(c+1))) {
        printf(COM2, "Making switch %d straight\r\n", (*c)->num);
        tr_set_switch((*c)->num, 0);
      } else {
        printf(COM2, "Making switch %d curved\r\n", (*c)->num);
        tr_set_switch((*c)->num, 1);
      }
    }
  }

  return 0;
}

