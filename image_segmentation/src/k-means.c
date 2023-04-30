#include "k-means.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>


// This function will calculate the euclidean distance between two pixels.
// Instead of using coordinate we use the RGB value for evaluate distance.
float distance(pixel p1, pixel p2) {
    float r_diff = p1.r - p2.r;
    float g_diff = p1.g - p2.g;
    float b_diff = p1.b - p2.b;
    return r_diff * r_diff + g_diff * g_diff + b_diff * b_diff;
}

// Vectorization of the following code :
// ---
// float distance(pixel p1, pixel p2) {
//     float r_diff = p1.r - p2.r;
//     float g_diff = p1.g - p2.g;
//     float b_diff = p1.b - p2.b;
//     return r_diff * r_diff + g_diff * g_diff + b_diff * b_diff;
// }
// ---
__m128 compute_distance(__m128 p1_r, __m128 p1_g, __m128 p1_b, const pixel* p2, int it) {

    // chargement des r, g et b en tant que float
    __m128 c_r = _mm_cvtepi32_ps(_mm_set_epi32(p2[it].r, p2[it + 1].r, p2[it + 2].r, p2[it + 3].r));
    __m128 c_g = _mm_cvtepi32_ps(_mm_set_epi32(p2[it].g, p2[it + 1].g, p2[it + 2].g, p2[it + 3].g));
    __m128 c_b = _mm_cvtepi32_ps(_mm_set_epi32(p2[it].b, p2[it + 1].b, p2[it + 2].b, p2[it + 3].b));

    // calcul de la différence
    __m128 r_diff = _mm_sub_ps(p1_r, c_r);
    __m128 g_diff = _mm_sub_ps(p1_g, c_g);
    __m128 b_diff = _mm_sub_ps(p1_b, c_b);

    // multiplication
    c_r = _mm_mul_ps(r_diff, r_diff);
    c_g = _mm_mul_ps(g_diff, g_diff);
    c_b = _mm_mul_ps(b_diff, b_diff);

    // Somme finale
    __m128 sum = _mm_add_ps(c_r, c_g);
    return _mm_add_ps(sum, c_b);
}

int find_best_cluster(const pixel image, const pixel* centers, int num_clusters) {
    float min = INFINITY;
    int idx = 0;

    int N = 4 * (num_clusters / 4); // pour avoir un multiple de 4

    // Inspiré de : http://0x80.pl/notesen/2018-10-03-simd-index-of-min.html
    const __m128i increment = _mm_set1_epi32(4);
    __m128i indices = _mm_setr_epi32(0, 1, 2, 3);
    __m128i minindices = indices;
    __m128 minvalues = _mm_set1_ps(min);

    const __m128 image_r = _mm_cvtepi32_ps(_mm_set1_epi32(image.r));
    const __m128 image_g = _mm_cvtepi32_ps(_mm_set1_epi32(image.g));
    const __m128 image_b = _mm_cvtepi32_ps(_mm_set1_epi32(image.b));

    for (int i = 0; i < N; i += 4) {
        // calcul de la distance
        const __m128 values = compute_distance(image_r, image_g, image_b, centers, i);

        // recherche du minimum de chaque colonne
        // on fait une comparaison en colonne float par float
        const __m128i lt = (__m128i) _mm_cmplt_ps(values, minvalues);

        // récupération des minimums et des indices des minimums
        minindices = _mm_blendv_epi8(minindices, indices, lt);
        minvalues = _mm_min_ps(values, minvalues);

        // on incrémente les indices
        indices = _mm_add_epi32(indices, increment);
    }

    // find min index in vector result (in an extremely naive way)
    float values_array[4];
    int indices_array[4];

    _mm_store_ps(values_array, minvalues);
    _mm_storeu_si128((__m128i*) indices_array, minindices);

    idx = indices_array[0];
    min = values_array[0];
    for (int i = 1; i < 4; i++) {
        if (values_array[i] < min) {
            min = values_array[i];
            idx = indices_array[i];
        }
    }

    // Standard way
    for (int i = N; i < num_clusters; ++i) {
        float d = distance(image, centers[i]);
        if (d < min) {
            min = d;
            idx = i;
        }
    }
    return idx;
}
// Vectorization of the following code :
// ---
// for (int i = 0; i < size; i++) {
//     distances[i] = distance(image[i], centers[0]);
// }
// ---
void compute_euclidean(float* distances, const pixel* image, int size, pixel center) {
    int N = 4 * (size / 4); // pour avoir un multiple de 4

    const __m128 center_r = _mm_cvtepi32_ps(_mm_set1_epi32(center.r));
    const __m128 center_g = _mm_cvtepi32_ps(_mm_set1_epi32(center.g));
    const __m128 center_b = _mm_cvtepi32_ps(_mm_set1_epi32(center.b));

    for (int i = 0; i < N; i += 4) {
        const __m128 values = compute_distance(center_r, center_g, center_b, image, i);
        _mm_store_ps(&distances[i], values);
    }

    // Standard way
    for (int i = N; i < size; ++i) {
        distances[i] = distance(image[i], center);
    }
}

void kmeans_pp(pixel* image, int width, int height, int num_clusters, pixel* centers) {
    int size = width * height;
    // Randomly select the first center.
    int first_center = rand() % size;
    centers[0] = image[first_center];

    float* distances = (float*) malloc(size * sizeof(float));
    if (distances == NULL) {
        return;
    }

    // Calculate the euclidean distance between each pixel and the first center randomly selected.
    compute_euclidean(distances, image, size, centers[0]);


    // Select the remaining centers using k-means++ algorithm.
    for (int i = 1; i < num_clusters; i++) {
        // Calculate the total weight of all distances.
        float total_weight = 0.0;
        for (int j = 0; j < size; j++) {
            total_weight += distances[j];
        }

        // Generate a random number in [0, total_weight) and use it to select a new center.
        float r = ((float) rand() / (float) RAND_MAX) * total_weight;
        int index = 0;
        while (r > distances[index]) {
            r -= distances[index];
            index++;
        }

        //Assign new center.
        centers[i] = image[index];

        // Update the distances array with the new center.
        for (int j = 0; j < size; j++) {
            float dist = distance(image[j], centers[i]);
            if (dist < distances[j]) {
                distances[j] = dist;
            }
        }
    }

    free(distances);
}

// This function performs k-means clustering on an image.
// It takes as input the image, its dimensions (width and height), and the number of clusters to find.
void kmeans(pixel* image, int width, int height, int num_clusters) {
    pixel* centers = (pixel*) malloc(num_clusters * sizeof(pixel));
    int size = width * height;

    // Initialize the cluster centers using the k-means++ algorithm.
    kmeans_pp(image, width, height, num_clusters, centers);

    int* assignments = (int*) malloc(size * sizeof(int));

    // Assign each pixel in the image to its nearest cluster.
    for (int y = 0; y < height; y++) {
        int yw = y * width;
        for (int x = 0; x < width; x++) {
            int index = yw+x;

            // Compute the distance between the pixel and each cluster center.
            int best_cluster = find_best_cluster(image[index], centers, num_clusters);

            // Assign the pixel to the best cluster.
            assignments[index] = best_cluster;
        }
    }

    ClusterData* cluster_data = (ClusterData*) calloc(num_clusters, sizeof(ClusterData));

    // Compute the sum of the pixel values for each cluster.
    for (int y = 0; y < height; y++) {
        int yw = y * width;
        for (int x = 0; x < width; x++) {
            int index = yw+x;
            int cluster = assignments[index];
            cluster_data[cluster].count++;
            cluster_data[cluster].sum_r += image[index].r;
            cluster_data[cluster].sum_g += image[index].g;
            cluster_data[cluster].sum_b += image[index].b;
        }
    }

    // Compute the new cluster centers.
    for (int c = 0; c < num_clusters; c++) {
        if (cluster_data[c].count > 0) {
            centers[c].r = cluster_data[c].sum_r / cluster_data[c].count;
            centers[c].g = cluster_data[c].sum_g / cluster_data[c].count;
            centers[c].b = cluster_data[c].sum_b / cluster_data[c].count;
        }
    }

    free(cluster_data);

    // Color all the pixels with the colors of their assigned cluster.
    for (int i = 0; i < size; ++i) {
        int cluster = assignments[i];
        image[i] = centers[cluster];
    }

    free(assignments);
    free(centers);
}
