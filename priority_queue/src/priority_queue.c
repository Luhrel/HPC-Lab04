#include "priority_queue.h"
#include <assert.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <tmmintrin.h>

#define DEBUG 0

#if DEBUG

void print128(char* label, __m128i var) {
  int32_t val[4];
  memcpy(val, &var, sizeof(val));
  printf("%s:\t %x %x %x %x \n", label, val[0], val[1], val[2], val[3]);
}

void print256(char* label, __m256i var) {
  node_t* val[4];
  memcpy(val, &var, sizeof(val));

  printf("%s:\t ", label);
  for (int i = 0; i < 4; ++i) {
    printf("%x ", val[i]->fCost);
  }
  printf("\n");
}

#endif

void shr_n_cmp(__m128i* fCosts, __m256i* addr) {
  // >>> 2
  __m256i addr_shr = _mm256_permute4x64_epi64(*addr, 0x4e);
  __m128i fCosts_shr = _mm_alignr_epi8(*fCosts, *fCosts, 8);

  // On récupère le fCost minimum et les adresses correspondantes
  __m128i mask_min128 = _mm_cmpgt_epi32(*fCosts, fCosts_shr);
  __m256i mask_min256 = _mm256_cvtepi32_epi64(mask_min128);

  // On récupère le fCost maximum et les adresses correspondantes
  __m128i mask_max128 = _mm_xor_si128(mask_min128, _mm_set1_epi32(-1));
  // extension du masque pour 256 bits
  __m256i mask_max256 = _mm256_cvtepi32_epi64(mask_max128);

  // récupération des fCosts et adresses minimum
  __m128i min_fCosts = _mm_blendv_epi8(*fCosts, fCosts_shr, mask_min128);
  __m256i min_addr = _mm256_blendv_epi8(*addr, addr_shr, mask_min256);

  // récupération des fCosts et adresses maximum
  __m128i max_fCosts = _mm_blendv_epi8(*fCosts, fCosts_shr, mask_max128);
  __m256i max_addr = _mm256_blendv_epi8(*addr, addr_shr, mask_max256);

  // return [min0, max0, min1, max1]
  *fCosts = _mm_unpackhi_epi32(max_fCosts, min_fCosts);
  // Pas le même comportement comme c'est des adresses.
  (*addr)[3] = _mm256_extract_epi64(min_addr, 3);
  (*addr)[2] = _mm256_extract_epi64(max_addr, 1);
  (*addr)[1] = _mm256_extract_epi64(min_addr, 2);
  (*addr)[0] = _mm256_extract_epi64(max_addr, 0);
}

void end_cmp(__m128i* fCosts, __m256i* addr) {
  // On récupère les deux fCosts du milieu du vecteur [3, 2, 1, 0]
  int fCosts2 = _mm_extract_epi32(*fCosts, 2);
  int fCosts1 = _mm_extract_epi32(*fCosts, 1);
  if (fCosts2 > fCosts1) {
    *addr = _mm256_permute4x64_epi64(*addr, 0xea);
    *fCosts = (__m128i)_mm_permute_ps((__m128)*fCosts, 0xea);
  }
}

void sse_merge_sort4x4(node_t** list, size_t size) {
  for (size_t i = 0; i < size; i += 4) {
    node_t** to_sort = list + i;

    // Chargement des adresses et des fCosts correspondants
    __m256i addr = _mm256_loadu_si256((__m256i*)to_sort);
    __m128i fCosts = _mm_setr_epi32(to_sort[0]->fCost, to_sort[1]->fCost,
                                    to_sort[2]->fCost, to_sort[3]->fCost);

#if DEBUG
    print128("start cost", fCosts);
    print256("start addr", addr);
#endif
    shr_n_cmp(&fCosts, &addr);

#if DEBUG
    print128("shr_n_cmp cost", fCosts);
    print256("shr_n_cmp addr", addr);
#endif
    shr_n_cmp(&fCosts, &addr);

#if DEBUG
    print128("shr_n_cmp2 cost", fCosts);
    print256("shr_n_cmp2 addr", addr);
#endif
    end_cmp(&fCosts, &addr);

    _mm256_storeu_si256((__m256i*)to_sort, addr);
#if DEBUG
    print128("end cost", fCosts);
    print256("end addr", addr);
    printf("======== \n\n");
#endif
  }
}

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
  assert(size % 4 == 0 && size >= 4);

  sse_merge_sort4x4(list, size);

  for (size_t len = 4; len < size; len *= 2) {
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
