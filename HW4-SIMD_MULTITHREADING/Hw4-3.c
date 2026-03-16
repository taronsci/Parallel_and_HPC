#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
// #include <time.h>
// #include <pthread.h>
#include <immintrin.h>

typedef struct {
    int width;
    int height;
    unsigned char *data;
} Image;

char *file = "rgbShip.pnm";
Image *img;

//utils
int load_ppm(char *ppm);
int save_ppm(char *filename, Image *img);

//scalar
Image *to_grayscale_scalar();

//simd
Image *simd_converter(size_t start, size_t end, Image *img) {
    unsigned char *data = img->data;
    unsigned char *buffer = malloc(img->width * img->height * 3);

    const __m256 wRed = _mm256_set1_ps(0.299f);
    const __m256 wGreen = _mm256_set1_ps(0.587f);
    const __m256 wBlue = _mm256_set1_ps(0.114f);

    const __m256i shufR = _mm256_setr_epi8(
        0, 3, 6, 9, 12, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        2, 5, 8, 11, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    );

    const __m256i shufG = _mm256_setr_epi8(
        1, 4, 7, 10, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 3, 6, 9, 12, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    );

    const __m256i shufB = _mm256_setr_epi8(
        2, 5, 8, 11, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 4, 7, 10, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    );

    size_t i = start;
    size_t limit = end & ~(size_t) 31; // round down to multiple of 32

    for (; i < limit; i += 32) {
        __m256i vector = _mm256_loadu_si256((__m256i *) (data + i));  //load 32

        __m256i r_tmp = _mm256_shuffle_epi8(vector, shufR);
        __m256i g_tmp = _mm256_shuffle_epi8(vector, shufG);
        __m256i b_tmp = _mm256_shuffle_epi8(vector, shufB);

        __m256i r = _mm256_permute2x128_si256(r_tmp, r_tmp, 0x20);      // 11 bytes here
        __m256i g = _mm256_permute2x128_si256(g_tmp, g_tmp, 0x20);      // 11 bytes here
        __m256i b = _mm256_permute2x128_si256(b_tmp, b_tmp, 0x20);      // 10 bytes here

        __m128i r_low = _mm256_castsi256_si128(r);      //grab lower half of 256 bit register
        __m128i g_low = _mm256_castsi256_si128(g);
        __m128i b_low = _mm256_castsi256_si128(b);

        __m256i r_i32 = _mm256_cvtepu8_epi32(r_low);    // converts 8 bytes -> to 8 int 32's
        __m256i g_i32 = _mm256_cvtepu8_epi32(g_low);
        __m256i b_i32 = _mm256_cvtepu8_epi32(b_low);

        __m256 r_f = _mm256_cvtepi32_ps(r_i32);     // converts int32 -> to float = 32 bytes
        __m256 g_f = _mm256_cvtepi32_ps(g_i32);
        __m256 b_f = _mm256_cvtepi32_ps(b_i32);

        r_f = _mm256_mul_ps(wRed, r_f);     //multiply by weight
        g_f = _mm256_mul_ps(wGreen, g_f);
        b_f = _mm256_mul_ps(wBlue, b_f);

        __m256 sum = _mm256_add_ps(_mm256_add_ps(r_f, g_f), b_f);

        __m256i sum_i = _mm256_cvtps_epi32(sum); // convert floats to integers

        __m128i sum_lo = _mm256_castsi256_si128(sum_i);
        __m128i sum_hi = _mm256_extracti128_si256(sum_i, 1); // upper 4 int32

        __m128i sum16 = _mm_packs_epi32(sum_lo, sum_hi);
        __m128i gray_i8 = _mm_packus_epi16(sum16, _mm_setzero_si128());

        unsigned char gray_bytes[8];
        _mm_storel_epi64((__m128i*)gray_bytes, gray_i8); // store lower 8 bytes

        for (int p = 0; p < 8; p++) {
            size_t base = i + p * 3;
            buffer[base] = buffer[base + 1] = buffer[base + 2] = gray_bytes[p];
        }
    }

    for (; i < end; i += 3) {
        float r = data[i];
        float g = data[i+1];
        float b = data[i+2];

        unsigned char gray = (unsigned char) (0.299f*r + 0.587f*g + 0.114f*b);
        buffer[i] = buffer[i+1] = buffer[i+2] = gray;
    }

    Image *grayscale = malloc(sizeof(Image));
    grayscale->data = buffer;
    grayscale->width = img->width;
    grayscale->height = img->height;

    return grayscale;
}


int main(void) {
    if (load_ppm(file) != 0) {
        return 1;
    }
    printf("Loaded image: %dx%d pixels\n", img->width, img->height);

    Image *g = to_grayscale_scalar();
    save_ppm("gray2.ppm", g);


    Image *mine = simd_converter(0, g->width * g->height, img);
    save_ppm("mine.ppm", mine);


    free(mine->data);
    free(mine);

    free(g->data);
    free(g);

    free(img->data);
    free(img);
    return 0;
}

//utils
int load_ppm(char *ppm) {
    //open file
    FILE *fp = fopen(ppm, "rb");
    if (fp == NULL) {
        perror("fopen");
        return 1;
    }

    char format[3];
    int width, height, maxval;

    // read format, make sure it is P6
    if (fscanf(fp, "%2s", format) != 1) {
        fclose(fp);
        return 1;
    }
    if (strcmp(format, "P6") != 0) {
        fclose(fp);
        perror("Not a P6 file");
        return 1;
    }

    //skip comments
    while (1) {
        int ch = fgetc(fp);

        while (isspace(ch)) {
            ch = fgetc(fp);
        }

        if (ch == '#') {
            while (fgetc(fp) != '\n');
        } else {
            ungetc(ch, fp);
            break;
        }
    }

    //read width and height, maxval
    if (fscanf(fp, "%d %d", &width, &height) != 2) {
        fclose(fp);
        perror("unable to read width and height");
        return 1;
    }
    if (fscanf(fp, "%d", &maxval) != 1) {
        fclose(fp);
        perror("unable to read maxval");
    }
    fgetc(fp);

    // allocate and read image data
    unsigned char *data = malloc(width * height * 3);
    if (data == NULL) {
        fclose(fp);
        perror("unable to load data");
        return 1;
    }

    ssize_t dataRead = fread(data, 3, width * height, fp);
    if (dataRead != width * height) {
        fclose(fp);
        perror("unable to read data");
        free(data);
        return 1;
    }

    fclose(fp);

    img = malloc(sizeof(Image));
    img->data = data;
    img->width = width;
    img->height = height;

    return 0;
}

int save_ppm(char *filename, Image *img) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("fopen");
        return 1;
    }

    fprintf(fp, "P6\n%d %d\n255\n", img->width, img->height);

    size_t written = fwrite(img->data, 3, img->width * img->height, fp);
    if (written != img->width * img->height) {
        perror("fwrite failed");
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}

//scalar
Image *to_grayscale_scalar() {
    Image *grayscale = malloc(sizeof(Image));

    int height = img->height;
    int width = img->width;

    unsigned char *data = malloc(width * height * 3);

    int index = 0;

    for (int i = 0; i < height * width; i++) {
        index = i * 3;
        unsigned char r = img->data[index + 0];
        unsigned char g = img->data[index + 1];
        unsigned char b = img->data[index + 2];

        unsigned char gray = 0.299 * r + 0.587 * g + 0.114 * b;
        data[index] = data[index + 1] = data[index + 2] = gray;
    }

    grayscale->data = data;
    grayscale->width = width;
    grayscale->height = height;

    return grayscale;
}