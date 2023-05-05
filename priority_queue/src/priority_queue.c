#include "priority_queue.h"

priority_queue_t* priority_queue_create(int (*compare_priority)(const void*,
                                                                const void*)) {
  size_t size = 1024;
  void** buf = (void**)malloc(size * sizeof(void*));

  priority_queue_t* pq = malloc(sizeof(priority_queue_t));

  pq->buf = buf;
  pq->size = 0;
  pq->nb_pushed = 0;
  pq->cmp = compare_priority;
  return pq;
}

void push(priority_queue_t* pq, void* data) {
  /* TODO */
}

void prioritize(priority_queue_t* pq) {
  /* TODO */
}

void* pop(priority_queue_t* pq) {
  /* TODO */
}

void destroy(priority_queue_t* pq) {
  for (size_t i = 0; i < pq->size; ++i) {
    free(pq->buf[i]);
  }
  free(pq->buf);
}
