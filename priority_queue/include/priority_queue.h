#include <stdio.h>
#include <stdlib.h>

typedef struct {
  void** buf;
  size_t size;
  size_t capacity;
  int (*cmp)(const void*, const void*);
} priority_queue_t;

typedef struct {
  int x, y;
  int gCost, hCost, fCost;
  int walkable;
  int explored;
} node_t;

priority_queue_t* priority_queue_create(int (*compare_priority)(const void*,
                                                                const void*));

int push(priority_queue_t* pq, void* data);

void prioritize(priority_queue_t* pq);

void* pop(priority_queue_t* pq);

void destroy(priority_queue_t* pq);
