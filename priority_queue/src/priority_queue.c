#include "priority_queue.h"
#include <emmintrin.h>
#include <smmintrin.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tmmintrin.h>

#define DEBUG 1

#if DEBUG
void print128_i32(char* label, __m128i var) {
  int32_t val[4];
  memcpy(val, &var, sizeof(val));
  printf("%s: %x %x %x %x \n", label, val[0], val[1], val[2], val[3]);
}
void print256_fCost(char* label, __m256i var) {
  node_t* val[4];
  memcpy(val, &var, sizeof(val));

  printf("%s : ", label);
  for (int i = 0; i < 4; ++i) {
    printf("%d ", val[i]->fCost);
  }
  printf("\n");
}
#endif

priority_queue_t* priority_queue_create(int (*compare_priority)(const void*,
                                                                const void*)) {
  size_t capacity = 1024;
  void** buf = aligned_alloc(16, capacity * sizeof(void*));
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
    void** new_buf = aligned_alloc(16, new_capacity * sizeof(void*));
    if (new_buf == NULL) {
      return 1;
    }
    pq->buf = memcpy(new_buf, pq->buf, pq->size);
    pq->buf = new_buf;
    pq->capacity = new_capacity;
  }

  pq->buf[pq->size++] = data;
  return 0;
}

// ////////////////////////////////////////////////////
// Cette fonction effectue un "round" de la fonction "merge_4x4_32bit" du papier
// "SIMD- and Cache-Friendly Algorithm for Sorting an Array of Structures".
// Plus concrétement, le code compare a128 et b128, puis affecte la valeur
// minimum dans min128 ainsi que l'index correspondant dans min256, mais sur
// 64 bits. Les 64 bits sont repris du registre a256 et b256, mais la
// comparaison vient de a128 et b128.
//
// C'est le pseudo-code suivant :
// ---
// minX = min(a128, b128);
// maxX = max(a128, b128);
// alignrX = shiftX(minX, minX, X bits);
// ---
// ////////////////////////////////////////////////////
void apply_compare(__m128i* min128,
                   __m128i* max128,
                   __m128i* alignr128,
                   __m128i a128,
                   __m128i b128,
                   __m256i* min256,
                   __m256i* max256,
                   __m256i* alignr256,
                   __m256i a256,
                   __m256i b256) {
  // Récupération des masques minimum / maximum
  __m128i mask_min128 = _mm_cmplt_epi32(a128, b128);
  __m256i mask_min256 = _mm256_cvtepi32_epi64(mask_min128);

#if DEBUG
  print128_i32("mask min 128", mask_min128);
#endif
  // Inversion du masque pour obtenir le maximum
  __m128i mask_max128 = _mm_xor_si128(mask_min128, _mm_set1_epi32(-1));
  //__m128i mask_max128 = _mm_cmpgt_epi32(a128, b128);
  __m256i mask_max256 = _mm256_cvtepi32_epi64(mask_max128);
#if DEBUG
  print128_i32("mask max 128", mask_max128);
#endif

  // Affectation selon le masque
  *min128 = _mm_blendv_epi8(a128, b128, mask_min128);
  *min256 = _mm256_blendv_epi8(a256, b256, mask_min256);
#if DEBUG
  print128_i32("min 128", *min128);
#endif

  *max128 = _mm_blendv_epi8(a128, b128, mask_max128);
  *max256 = _mm256_blendv_epi8(a256, b256, mask_max256);
#if DEBUG
  print128_i32("max 128", *max128);
#endif

  // shift circulaire vers la droite (>>>)
  *alignr128 = _mm_alignr_epi8(*min128, *min128, 4);
  *alignr256 = _mm256_alignr_epi8(*min256, *min256, 8);
#if DEBUG
  print128_i32("alignr 128", *alignr128);
#endif
}

void merge_4x4_32bit() {}

void merge_8x8_32bit(__m128i* vA0,
                     __m128i* vA1,
                     // input 1
                     __m128i* vB0,
                     __m128i* vB1,
                     // input 2
                     __m128i* vMin0,
                     __m128i* vMin1,
                     // output
                     __m128i* vMax0,
                     __m128i* vMax1) {  // output
  // 1st step
  merge_4x4_32bit(vA1, vB1, vMin1, vMax1);
  merge_4x4_32bit(vA0, vB0, vMin0, vMax0);
  // 2nd step
  merge_4x4_32bit(vMax0, vMin1, vMin1, vMax0);
}
void load_cost(__m128i* fCosts,
               __m256i* addresses,
               void* buf,
               __m128i indices,
               node_t** tmp_addr) {
  *addresses = _mm256_i32gather_epi64(buf, indices, 8);
  // stockage de ces adresses dans un endroit temporaire afin d'extraire le
  // fCost
  __m128i low = _mm256_extracti128_si256(*addresses, 0);
  __m128i high = _mm256_extracti128_si256(*addresses, 1);
  _mm_store_si128((__m128i*)tmp_addr, high);
  _mm_store_si128((__m128i*)(tmp_addr + 2), low);
  // si AVX-512, peut être remplacé par :
  //_mm256_store_epi64((__m256*)tmp_addr, *addresses);

  // extraction du fCost
  *fCosts = _mm_set_epi32(tmp_addr[1]->fCost, tmp_addr[0]->fCost,
                          tmp_addr[3]->fCost, tmp_addr[2]->fCost);
}

void unload(void** buf, __m256i high, __m256i low) {
  __m128i low0 = _mm256_extracti128_si256(low, 0);
  __m128i low1 = _mm256_extracti128_si256(low, 1);
  __m128i high0 = _mm256_extracti128_si256(high, 0);
  __m128i high1 = _mm256_extracti128_si256(high, 1);

  _mm_store_si128((__m128i*)buf, high0);
  _mm_store_si128((__m128i*)(buf + 2), high1);
  _mm_store_si128((__m128i*)(buf + 4), low0);
  _mm_store_si128((__m128i*)(buf + 6), low1);

  node_t* a = buf[0];
  node_t* b = buf[1];
  node_t* c = buf[2];
  node_t* d = buf[3];
  node_t* e = buf[4];
  node_t* f = buf[5];
  node_t* g = buf[6];
  node_t* h = buf[7];
}

void merge() {}

void prioritize(priority_queue_t* pq) {
  assert(pq->size % 4 == 0);

  size_t N = 8 * (pq->size / 8);

  __m128i indices = _mm_setr_epi32(0, 1, 2, 3);
  __m128i four = _mm_set1_epi32(4);
  // aligné sur 32 dans le doute si utilisation de registre avx512
  node_t** tmp_addr = aligned_alloc(32, 4 * sizeof(node_t*));

  for (size_t i = 0; i < N; i += 8) {
    // if (i != 4 * 8)
    //   continue;
    qsort(pq->buf + i, 4, sizeof(node_t*), pq->cmp);
    qsort(pq->buf + i + 4, 4, sizeof(node_t*), pq->cmp);
    __m256i addrA, addrB;
    __m128i fCostsA, fCostsB;
    // On stocke [fCostA0, fCostA1, fCostA2, fCostA3] (128bits)
    // et chargement des 4 addresses [AddrA0, AddrA1, AddrA2, AddrA3] (256bits)
    load_cost(&fCostsA, &addrA, pq->buf, indices, tmp_addr);
    // Incrémentation de 4 de l'indice afin de charger 4 autres adresses
    indices = _mm_add_epi32(indices, four);

    // On stocke [fCostB0, fCostB1, fCostB2, fCostB3] (128bits)
    // et chargement des 4 addresses [AddrB0, AddrB1, AddrB2, AddrB3] (256bits)
    load_cost(&fCostsB, &addrB, pq->buf, indices, tmp_addr);
    // Incrémentation de 4 de l'indice afin de charger 4 autres adresses lors du
    // prochain tout de boucle
    indices = _mm_add_epi32(indices, four);

    // ////////////////////////////////////////////////////
    // Algorithme selon le papier "SIMD- and Cache-Friendly Algorithm for
    // Sorting an Array of Structures"
    // ////////////////////////////////////////////////////
    __m128i tmp128, min128, max128;
    __m256i tmp256, min256, max256;

#if DEBUG
    print128_i32("fCostsA", fCostsA);
    print256_fCost("AddrA", addrA);
    print128_i32("fCostsB", fCostsB);
    print256_fCost("AddrB", addrB);
    print256_fCost("Avant R1a fCost", addrA);
    print256_fCost("Avant R1b fCost", addrB);
    printf("==================\n");
#endif

    // Récupération du minimum / maximum
    // entre A0 et B0, A1 et B1, A2 et B2, puis A3 et B3.
    // Les registres 256 bits font les mêmes permutations que ceux de 128 bits
    // et contiennent les addresses des structures node_t
    apply_compare(&tmp128, &max128, &tmp128, fCostsA, fCostsB, &tmp256, &max256,
                  &tmp256, addrA, addrB);

#if DEBUG
    print256_fCost("Après R1a fCost", tmp256);
    print256_fCost("Après R1b fCost", max256);
    printf("==================\n");
#endif
    apply_compare(&min128, &max128, &tmp128, tmp128, max128, &min256, &max256,
                  &tmp256, tmp256, max256);
#if DEBUG
    print256_fCost("Après R2a fCost", min256);
    print256_fCost("Après R2b fCost", max256);
    printf("==================\n");
#endif
    apply_compare(&min128, &max128, &tmp128, tmp128, max128, &min256, &max256,
                  &tmp256, tmp256, max256);
#if DEBUG
    print256_fCost("Après R3a fCost", min256);
    print256_fCost("Après R3b fCost", max256);
    printf("==================\n");
#endif

    apply_compare(&min128, &max128, &min128, tmp128, max128, &min256, &max256,
                  &min256, tmp256, max256);
    // min et max sont triés

#if DEBUG
    print256_fCost("Après R4a fCost", min256);
    print256_fCost("Après R4b fCost", max256);
    printf("==================\n");
#endif
    // On stocke les adresses des fCosts, les plus grands au début du tableau
    // comme on souhaite avoir les
    unload(pq->buf + i, min256, max256);

#if DEBUG
    printf("Après : [ ");
    for (int j = 0; j < 8; ++j) {
      node_t* node = pq->buf[i + j];
      printf("%d, ", node->fCost);
    }
    printf("]\n\n");
#endif
  }
  free(tmp_addr);

  // Maintenant notre buffer contient que des éléments triés par paquets de 8

  // sort last 4 elements
  if (N < pq->size) {
    // qsort(pq->buf + N, 4, sizeof(node_t*), pq->cmp);
  }
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
