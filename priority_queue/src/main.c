#include <limits.h>
#include <stdlib.h>
#include "priority_queue.h"

#include <stdio.h>
#include <stdlib.h>

#define map_size_rows 10
#define map_size_cols 10

char map[map_size_rows][map_size_cols] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 1, 0, 0, 0, 1, 0, 1}, {1, 0, 0, 1, 0, 0, 0, 1, 0, 1},
    {1, 0, 0, 1, 1, 1, 1, 1, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

node_t* get_nodes_from_map(int start_x, int start_y, int end_x, int end_y);

int compare_node(const void* a, const void* b) {
  node_t* node_a = (node_t*)*((node_t**)a);
  node_t* node_b = (node_t*)*((node_t**)b);
  if (node_a->gCost == INT_MAX || node_b->gCost == INT_MAX) {
    return node_b->gCost - node_a->gCost;
  }
  return node_b->fCost - node_a->fCost;
}

int main() {
  priority_queue_t* priority_queue = priority_queue_create(&compare_node);

  node_t* nodes = get_nodes_from_map(1, 1, 9, 9);

  for (size_t i = 0; i < map_size_rows * map_size_cols; i++) {
    if (push(priority_queue, &nodes[i]) != 0) {
      fprintf(stderr, "Unable to push element (%d, %d) gCost=%d\n", nodes[i].x,
              nodes[i].y, nodes[i].gCost);
    }
  }

  prioritize(priority_queue);

  for (size_t i = 0; i < map_size_rows * map_size_cols; i++) {
    node_t* node = (node_t*)pop(priority_queue);
    printf("(%d, %d) gCost=%d hCost=%d fCost=%d walkable=%d\n", node->x,
           node->y, node->gCost, node->hCost, node->fCost, node->walkable);
  }

  destroy(priority_queue);
  free(nodes);

  return 0;
}

node_t* get_nodes_from_map(int start_x, int start_y, int end_x, int end_y) {
  node_t* nodes =
      (node_t*)malloc(sizeof(node_t) * map_size_rows * map_size_cols);
  int i, j, idx = 0;
  for (i = 0; i < map_size_rows; i++) {
    for (j = 0; j < map_size_cols; j++) {
      node_t node = {.x = j, .y = i, .walkable = !map[i][j]};
      int dx = abs(j - end_x);
      int dy = abs(i - end_y);
      node.hCost = dx + dy;
      node.gCost = INT_MAX;
      node.fCost = INT_MAX;
      nodes[idx] = node;
      idx++;
    }
  }

  int start_idx = start_y * map_size_cols + start_x;
  nodes[start_idx].gCost = 0;

  int current_idx = start_idx;
  while (current_idx != -1) {
    node_t current_node = nodes[current_idx];
    int current_x = current_node.x;
    int current_y = current_node.y;

    int neighbor_indices[] = {
        current_idx - map_size_cols,  // top neighbor
        current_idx + map_size_cols,  // bottom neighbor
        current_idx - 1,              // left neighbor
        current_idx + 1               // right neighbor
    };
    int num_neighbors = 4;
    if (current_x == 0) {        // current node is on left edge
      neighbor_indices[2] = -1;  // left neighbor does not exist
      num_neighbors--;
    }
    if (current_x == map_size_cols - 1) {  // current node is on right edge
      neighbor_indices[3] = -1;            // right neighbor does not exist
      num_neighbors--;
    }
    if (current_y == 0) {        // current node is on top edge
      neighbor_indices[0] = -1;  // top neighbor does not exist
      num_neighbors--;
    }
    if (current_y == map_size_rows - 1) {  // current node is on bottom edge
      neighbor_indices[1] = -1;            // bottom neighbor does not exist
      num_neighbors--;
    }
    for (i = 0; i < num_neighbors; i++) {
      int neighbor_idx = neighbor_indices[i];
      if (neighbor_idx == -1) {
        continue;
      }
      node_t neighbor_node = nodes[neighbor_idx];
      if (!neighbor_node.walkable) {
        continue;
      }
      int tentative_gCost =
          current_node.gCost + 1;  // Assuming cost of moving to a neighbor is 1
      if (tentative_gCost < neighbor_node.gCost) {
        neighbor_node.gCost = tentative_gCost;
        neighbor_node.fCost = neighbor_node.gCost + neighbor_node.hCost;
        nodes[neighbor_idx] = neighbor_node;
      }
    }

    int min_fCost = INT_MAX;
    int min_fCost_idx = -1;

    for (i = 0; i < map_size_rows * map_size_cols; i++) {
      node_t node = nodes[i];
      if (node.gCost == INT_MAX || node.explored) {
        continue;
      }
      if (node.fCost < min_fCost) {
        min_fCost = node.fCost;
        min_fCost_idx = i;
      }
    }
    if (min_fCost_idx == -1) {
      break;
    }
    nodes[min_fCost_idx].explored = 1;
    current_idx = min_fCost_idx;
  }
  return nodes;
}
