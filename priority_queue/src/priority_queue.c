#include "priority_queue.h"
#include <assert.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <stddef.h>
#include <string.h>
#include <tmmintrin.h>

node_t** merge(node_t** list, size_t l1, size_t r1, size_t l2, size_t r2) {
  node_t** tmp = malloc((r2 - l1 + 1) * sizeof(node_t*));
  size_t i;

  for (i = 0; l1 <= r1 && l2 <= r2; ++i) {
    tmp[i] = list[l1]->fCost > list[l2]->fCost ? list[l1++] : list[l2++];
  }

  while (l1 <= r1) {
    tmp[i++] = list[l1++];
  }

  while (l2 <= r2) {
    tmp[i++] = list[l2++];
  }
  return tmp;
}

// https://www.baeldung.com/cs/non-recursive-merge-sort
void merge_sort(node_t** list, size_t size) {
  for (size_t len = 1; len < size; len *= 2) {
    for (size_t i = 0; i < size; i += 2 * len) {
      size_t l1 = i;
      size_t l2 = i + len;
      size_t r1 = l2 - 1;
      size_t r2 = i + 2 * len - 1;

      if (l2 >= size) {
        break;
      }
      if (r2 >= size) {
        r2 = size - 1;
      }
      node_t** tmp = merge(list, l1, r1, l2, r2);
      for (size_t j = 0; j < (r2 - l1 + 1); ++j) {
        list[i + j] = tmp[j];
      }
      free(tmp);
    }
  }
}

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
  merge_sort((node_t**)pq->buf, pq->size);
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
