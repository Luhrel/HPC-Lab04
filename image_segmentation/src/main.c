#include "k-means.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pixel *load_image(char *filename, int *width, int *height, unsigned char header[54]) {
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Erreur: File %s cannot be opened.\n", filename);
        return NULL;
    }

    int result = fread(header, sizeof(char), 54, f);
    if (!result) {
        printf("Erreur: Header of file %s in not readable.\n", filename);
        return NULL;
    }

    *width = *(int *)&header[18];
    *height = *(int *)&header[22];

    int size = *width * *height;

    pixel *pixels = malloc(size * sizeof(pixel));
    if (!pixels) {
        printf("Erreur: Not enough memory available for loading image.\n");
        return NULL;
    }

    for (int i = 0; i < size; ++i) {
        unsigned char buffer[3];
        if (!fread(buffer, sizeof(unsigned char), 3, f)) {
            printf("Erreur: Pixels are not readable.\n");
            return NULL;
        }

        pixels[i].r = (int)buffer[2];
        pixels[i].g = (int)buffer[1];
        pixels[i].b = (int)buffer[0];
    }

    fclose(f);

    return pixels;
}

int save_image(char *filename, pixel *image, int width, int height, unsigned char header[54]) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Erreur : File %s cannot be opened.\n", filename);
        return 1;
    }

    *(int *)&header[18] = width;
    *(int *)&header[22] = height;
    fwrite(header, sizeof(unsigned char), 54, file);

    unsigned char output[3];

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = y * width + x;
            output[2] = (unsigned char)image[index].r;
            output[1] = (unsigned char)image[index].g;
            output[0] = (unsigned char)image[index].b;
            fwrite(output, sizeof(unsigned char), 3, file);
        }
    }

    fclose(file);
    return 0;
}

int main(int argc, char** argv) {
    unsigned char header[54];
    int width, height;
    int nb_cluster = 0;

    if (argc != 4) {
        fprintf(stderr, "Usage : %s <img_src.bmp> <nb_clusters> <img_dest.bmp>\n", argv[0]);
        return 1;
    }

    char *src_extension = strrchr(argv[1], '.');
    char *dst_extension = strrchr(argv[3], '.');
    if ((src_extension == NULL || strcmp(src_extension, ".bmp") ||
         dst_extension == NULL || strcmp(dst_extension, ".bmp")) != 0) {
        printf("All file must be a \".bmp\"\n");
        return 1;
    }

    nb_cluster = atoi(argv[2]);
    if (nb_cluster <= 0){
        printf("The number of clusters should be greater than 0\n");
        return 1;
    }

    pixel *image = load_image(argv[1], &width, &height, header);

    printf("Image loaded!\n");

    kmeans(image, width, height, nb_cluster);
    
    save_image(argv[3], image, width, height, header);

    printf("Image saved to %s!\n", argv[3]);

    free(image);

    printf("Programm ended successfully\n\n");
    
    return 0;
}
