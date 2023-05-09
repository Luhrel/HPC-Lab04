#include "priority_queue.h"

priority_queue_t* priority_queue_create(int (*compare_priority)(const void*,
                                                                const void*)) {
  size_t capacity = 1024;
  void** buf = malloc(capacity * sizeof(void*));
  if (buf == NULL) {
    return NULL;
  }

  priority_queue_t* pq = malloc(sizeof(priority_queue_t));
  if (pq == NULL) {
    free(buf);
    return NULL;
  }

  pq->buf = buf;
  pq->size = 0;
  pq->capacity = capacity;
  pq->cmp = compare_priority;
  return pq;
}

int push(priority_queue_t* pq, void* data) {
  if (pq->size == pq->capacity) {
    size_t new_capacity = 1.5 * pq->capacity;
    void** new_buf = realloc(pq->buf, new_capacity * sizeof(void*));
    if (new_buf == NULL) {
      return 1;
    }
    pq->buf = new_buf;
    pq->capacity = new_capacity;
  }

  pq->buf[pq->size++] = data;
  return 0;
}

void prioritize(priority_queue_t* pq) {
  qsort(pq->buf, pq->size, sizeof(void*), pq->cmp);
}

void* pop(priority_queue_t* pq) {
  if (pq->size == 0) {
    return NULL;
  }

  return pq->buf[--pq->size];
}

void destroy(priority_queue_t* pq) {
  free(pq->buf);
  free(pq);
}
