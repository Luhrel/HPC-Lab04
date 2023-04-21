typedef struct {
    int r;
    int g;
    int b;
} pixel;

typedef struct {
    int count;
    int sum_r;
    int sum_g;
    int sum_b;
} ClusterData;


float distance(pixel p1, pixel p2);

void kmeans_pp(pixel *image, int width, int height, int num_clusters, pixel *centers);

void kmeans(pixel *image, int width, int height, int num_clusters);