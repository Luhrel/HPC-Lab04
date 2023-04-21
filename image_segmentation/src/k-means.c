#include "k-means.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// This function will calculate the euclidean distance between two pixels.
// Instead of using coordinate we use the RGB value for evaluate distance.
float distance(pixel p1, pixel p2){
    float r_diff = p1.r - p2.r;
    float g_diff = p1.g - p2.g;
    float b_diff = p1.b - p2.b;
    return sqrt(r_diff * r_diff + g_diff * g_diff + b_diff * b_diff);
}

void kmeans_pp(pixel *image, int width, int height, int num_clusters, pixel *centers){
    // Randomly select the first center.
    int first_center = rand() % (width * height);
    centers[0] = image[first_center];

    float *distances = (float *)malloc(width * height * sizeof(float));

    // Calculate the euclidean distance between each pixel and the first center randomly selected.
    for (int i = 0; i < width * height; i++) {
        distances[i] = distance(image[i], centers[0]) * distance(image[i], centers[0]);
    }

    // Select the remaining centers using k-means++ algorithm.
    for (int i = 1; i < num_clusters; i++) {
        // Calculate the total weight of all distances.
        float total_weight = 0.0;
        for (int j = 0; j < width * height; j++) {
            total_weight += distances[j];
        }

        // Generate a random number in [0, total_weight) and use it to select a new center.
        float r = ((float)rand() / (float)RAND_MAX) * total_weight;
        int index = 0;
        while (r > distances[index]) {
            r -= distances[index];
            index++;
        }
        
        //Assign new center.
        centers[i] = image[index];

        // Update the distances array with the new center.
        for (int j = 0; j < width * height; j++) {
            float dist = distance(image[j], centers[i]) * distance(image[j], centers[i]);
            if (dist < distances[j]) {
                distances[j] = dist;
            }
        }
    }

    free(distances);
}

// This function performs k-means clustering on an image.
// It takes as input the image, its dimensions (width and height), and the number of clusters to find.
void kmeans(pixel *image, int width, int height, int num_clusters){
    pixel *centers = (pixel *)malloc(num_clusters * sizeof(pixel));

    // Initialize the cluster centers using the k-means++ algorithm.
    kmeans_pp(image, width, height, num_clusters, centers);

    int *assignments = (int *)malloc(width * height * sizeof(int));

    // Assign each pixel in the image to its nearest cluster.
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float min_dist = INFINITY;
            int best_cluster = -1;

            // Compute the distance between the pixel and each cluster center.
            for (int c = 0; c < num_clusters; c++) {
                float dist = distance(image[y * width + x], centers[c]);

                if (dist < min_dist) {
                    min_dist = dist;
                    best_cluster = c;
                }
            }

            // Assign the pixel to the best cluster.
            assignments[y * width + x] = best_cluster;
        }
    }

    ClusterData *cluster_data = (ClusterData *)calloc(num_clusters, sizeof(ClusterData));

     // Compute the sum of the pixel values for each cluster.
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int cluster = assignments[y * width + x];
            cluster_data[cluster].count++;
            cluster_data[cluster].sum_r += image[y * width + x].r;
            cluster_data[cluster].sum_g += image[y * width + x].g;
            cluster_data[cluster].sum_b += image[y * width + x].b;
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
    for (int i = 0; i < width * height; ++i){
        int cluster = assignments[i];
        image[i] = centers[cluster];
    }

    free(assignments);
    free(centers);
}