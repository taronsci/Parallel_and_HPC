#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
// #include <time.h>
// #include <pthread.h>
#include <immintrin.h>

#define DEBUG 0   // set 0 to disable all debug printing

#if DEBUG
#define DEBUG_PRINT_M128I(label, vec) \
do { \
printf("%s\n", label); \
print_m128i(vec); \
} while(0)

#define DEBUG_PRINT_M256(label, vec) \
do { \
printf("%s\n", label); \
print_m256(vec); \
} while(0)

#define DEBUG_PRINT_M256I(label, vec) \
do { \
printf("%s\n", label); \
print_m256i(vec); \
} while(0)
#else
#define DEBUG_PRINT_M128I(label, vec) do {} while(0)
#define DEBUG_PRINT_M256(label, vec) do {} while(0)
#define DEBUG_PRINT_M256I(label, vec) do {} while(0)
#endif


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

//SIMD
__m128i pack_bytes(__m256i v, __m128i mask_lo, __m128i mask_hi, int m);

Image *simd_converter(size_t start, size_t end);    // loads 32, separates channels, etc.
Image *simd_converter_v2(size_t start, size_t end); // loads 16, multiplies by index


int main(void) {
    if (load_ppm(file) != 0) {
        printf("error loading image");
        return 1;
    }

    int end = img->width * img->height * 3;

    Image *scalar = to_grayscale_scalar();
    save_ppm("scalar.ppm", scalar);
    free(scalar->data);
    free(scalar);

    Image *simd1 = simd_converter(0, end);
    save_ppm("simd1.ppm", simd1);
    free(simd1->data);
    free(simd1);

    Image *simd2 = simd_converter_v2(0, end);
    save_ppm("simd2.ppm", simd2);
    free(simd2->data);
    free(simd2);



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

    size_t dataRead = fread(data, 3, width * height, fp);
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

        unsigned char gray = (unsigned char) (0.299f * r + 0.587f * g + 0.114f * b);
        data[index] = data[index + 1] = data[index + 2] = gray;
    }

    printf("scalar\n");
    for (int k = 2400; k < 2424; k++) {
        printf("%02X ", data[k]);
    }
    printf("    |   ");
    for (int k = 2424; k < 2448; k++) {
        printf("%02X ", data[k]);
    }
    printf("    |   ");
    for (int k = 2448; k < 2472; k++) {
        printf("%02X ", data[k]);
    }
    printf("scalar end\n");

    grayscale->data = data;
    grayscale->width = width;
    grayscale->height = height;

    return grayscale;
}


//simd
Image *simd_converter(size_t start, size_t end) {
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

    const __m256i shuf3 = _mm256_setr_epi8(
        0, 4, 8, 1, 5, 9, 2, 6, 10, 3, 7, 11, -1, -1, -1, -1,
        0, 4, 8, 1, 5, 9, 2, 6, 10, 3, 7, 11, -1, -1, -1, -1
    );

    __m128i mask_lo_red = _mm_setr_epi8(0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    __m128i mask_hi_red = _mm_setr_epi8(0, 1, 2, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

    __m128i mask_lo_green = _mm_setr_epi8(0, 1, 2, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    __m128i mask_hi_green = _mm_setr_epi8(0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

    __m128i mask_lo_blue = _mm_setr_epi8(0, 1, 2, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    __m128i mask_hi_blue = _mm_setr_epi8(0, 1, 2, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

    size_t i = start;
    size_t limit = end & ~(size_t) 23; // round down to multiple of 24

    for (; i < limit; i += 24) {
        __m256i vector = _mm256_loadu_si256((__m256i *) (data + i)); // load 32
        DEBUG_PRINT_M256I("vector", vector);

        __m256i r_tmp = _mm256_shuffle_epi8(vector, shufR);
        DEBUG_PRINT_M256I("r_tmp", r_tmp);
        __m256i g_tmp = _mm256_shuffle_epi8(vector, shufG);
        DEBUG_PRINT_M256I("g_tmp", g_tmp);
        __m256i b_tmp = _mm256_shuffle_epi8(vector, shufB);
        DEBUG_PRINT_M256I("b_tmp", b_tmp);

        __m128i r_low = pack_bytes(r_tmp, mask_lo_red, mask_hi_red, 6);
        DEBUG_PRINT_M256I("r_low", r_low);
        __m128i g_low = pack_bytes(g_tmp, mask_lo_green, mask_hi_green, 5);
        DEBUG_PRINT_M256I("g_low", g_low);
        __m128i b_low = pack_bytes(b_tmp, mask_lo_blue, mask_hi_blue, 5);
        DEBUG_PRINT_M256I("b_low", b_low);

        __m256i r_i32 = _mm256_cvtepu8_epi32(r_low); // converts 8 bytes -> to 8 int 32's
        DEBUG_PRINT_M256I("r_i32", r_i32);
        __m256i g_i32 = _mm256_cvtepu8_epi32(g_low);
        DEBUG_PRINT_M256I("g_i32", g_i32);
        __m256i b_i32 = _mm256_cvtepu8_epi32(b_low);
        DEBUG_PRINT_M256I("b_i32", b_i32);

        __m256 r_f = _mm256_cvtepi32_ps(r_i32); // converts int32 -> to float = 32 bytes
        DEBUG_PRINT_M256("r_f", r_f);
        __m256 g_f = _mm256_cvtepi32_ps(g_i32);
        DEBUG_PRINT_M256("g_f", g_f);
        __m256 b_f = _mm256_cvtepi32_ps(b_i32);
        DEBUG_PRINT_M256("b_f", b_f);

        r_f = _mm256_mul_ps(r_f, wRed); //multiply by weight
        DEBUG_PRINT_M256("r_f mult", r_f);
        g_f = _mm256_mul_ps(g_f, wGreen);
        DEBUG_PRINT_M256("g_f mult", g_f);
        b_f = _mm256_mul_ps(b_f, wBlue);
        DEBUG_PRINT_M256("b_f mult", b_f);

        __m256 sum = _mm256_add_ps(_mm256_add_ps(r_f, g_f), b_f);
        DEBUG_PRINT_M256("sum", sum);

        __m256i sum_i = _mm256_cvtps_epi32(sum); // convert floats to integers i32 // 8 int 32s
        DEBUG_PRINT_M256I("sum_i", sum_i);

        __m256i i16 = _mm256_packs_epi32(sum_i, sum_i); // 16 int16s
        DEBUG_PRINT_M256I("i16", i16);

        __m256i i8 = _mm256_packs_epi16(i16, i16); // 32 int 8s
        DEBUG_PRINT_M256I("i8", i8);

        __m256i out = _mm256_shuffle_epi8(i8, shuf3);
        DEBUG_PRINT_M256I("out", out);

        __m128i lo = _mm256_castsi256_si128(out); // b1..b4 expanded
        DEBUG_PRINT_M128I("lo", lo);
        __m128i hi = _mm256_extracti128_si256(out, 1); // b5..b8 expanded
        DEBUG_PRINT_M128I("hi", hi);

        // store first 8 bytes
        _mm_storel_epi64((__m128i*)(buffer + i), lo); // bytes 0–7
        // store next 4 bytes
        _mm_storeu_si32(buffer + i + 8, _mm_srli_si128(lo, 8));

        // store next 12 bytes
        _mm_storel_epi64((__m128i *)(buffer + i + 12), hi); // bytes 12–19
        _mm_storeu_si32(buffer + i + 20, _mm_srli_si128(hi, 8));
    }

    for (; i < end; i += 3) {
        unsigned char r = data[i];
        unsigned char g = data[i + 1];
        unsigned char b = data[i + 2];

        unsigned char gray = (unsigned char) (0.299f * r + 0.587f * g + 0.114f * b);
        buffer[i] = buffer[i + 1] = buffer[i + 2] = gray;
    }

    Image *grayscale = malloc(sizeof(Image));
    grayscale->data = buffer;
    grayscale->width = img->width;
    grayscale->height = img->height;

    return grayscale;
}

__m128i pack_bytes(__m256i v, __m128i mask_lo, __m128i mask_hi, int m) {
    __m128i lo = _mm256_castsi256_si128(v); // low 128-bit
    __m128i hi = _mm256_extracti128_si256(v, 1); // high 128-bit

    // Shuffle each lane to pack meaningful bytes
    lo = _mm_shuffle_epi8(lo, mask_lo);
    hi = _mm_shuffle_epi8(hi, mask_hi);

    // Merge hi bytes into lo lane
    switch (m) {
        case 6:
            return _mm_or_si128(lo, _mm_slli_si128(hi, 6)); // first 128-bit lane contains all meaningful bytes
        case 5:
            return _mm_or_si128(lo, _mm_slli_si128(hi, 5)); // first 128-bit lane contains all meaningful bytes
        default:
            return lo;
    }
}


Image *simd_converter_v2(size_t start, size_t end) {
    unsigned char *data = img->data;
    unsigned char *buffer = malloc(img->width * img->height * 3);

    const __m128i shuf3 = _mm_setr_epi8(
        0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, -1
    );

    size_t i = start;
    size_t limit = end & ~(size_t) 15; // round down to multiple of 15

    for (; i < limit; i += 15) {
        //next iteration should take from r6
        __m128i vector = _mm_loadu_si128((__m128i *) (data + i)); // load 16 = 5 * 3
        // r1 g1 b1 	r2 g2 b2 	r3 g3 b3	r4 g4 b4    r5 g5 b5    r6
        DEBUG_PRINT_M128I("Vector", vector);

        __m128i high8 = _mm_srli_si128(vector, 8);  // bytes 8–15
        DEBUG_PRINT_M128I("high8", high8);
        // b3	r4 g4 b4    r5 g5 b5    r6

        __m256i lo = _mm256_cvtepu8_epi32(vector); //we have ints here
        // r1 g1 b1 	r2 g2 b2 	r3 g3
        DEBUG_PRINT_M256I("lo", lo);

        __m256i hi = _mm256_cvtepu8_epi32(high8);
        DEBUG_PRINT_M256I("hi", hi);

        //convert to float
        __m256 lo_f = _mm256_cvtepi32_ps(lo);
        DEBUG_PRINT_M256("lo_f", lo_f);

        __m256 hi_f = _mm256_cvtepi32_ps(hi);
        DEBUG_PRINT_M256("hi_f", hi_f);

        //multiply by weight
        __m256 factors_lo = _mm256_setr_ps(
        0.299f,
        0.587f,
        0.114f,

        0.299f,
        0.587f,
        0.114f,

        0.299f,
        0.587f
        );
        lo_f = _mm256_mul_ps(lo_f, factors_lo);
        // r1 g1 b1 	r2 g2 b2 	r3 g3
        DEBUG_PRINT_M256("lo_f mult", lo_f);

        __m256 factors_hi = _mm256_setr_ps(
        0.114f,

        0.299f,
        0.587f,
        0.114f,

        0.299f,
        0.587f,
        0.114f,

        0.299f
        );
        hi_f = _mm256_mul_ps(hi_f, factors_hi);
        // b3	r4 g4 b4    r5 g5 b5    r6
        DEBUG_PRINT_M256("hi_f mult", hi_f);

        __m256 sh1_lo = _mm256_permutevar8x32_ps(lo_f,
            _mm256_setr_epi32(1,2,3,4,5,6,7,0));
        __m256 sh2_lo = _mm256_permutevar8x32_ps(lo_f,
            _mm256_setr_epi32(2,3,4,5,6,7,0,0));

        __m256 sum_lo = _mm256_add_ps(lo_f, _mm256_add_ps(sh1_lo, sh2_lo));
        // r1 g1 b1 r2 g2 b2 r3 g3
        // g1 b1 r2 g2 b2 r3 g3 r1
        // b1 r2 g2 b2 r3 g3 r1 g1
        // __ xx xx __ r3 xx xx xx
        DEBUG_PRINT_M256("sum lo", sum_lo);

        __m256 sh1_hi = _mm256_permutevar8x32_ps(hi_f,
            _mm256_setr_epi32(1,2,3,4,5,6,7,0));
        __m256 sh2_hi = _mm256_permutevar8x32_ps(hi_f,
            _mm256_setr_epi32(2,3,4,5,6,7,0,0));

        __m256 sum_hi = _mm256_add_ps(hi_f, _mm256_add_ps(sh1_hi, sh2_hi));
        // b3 r4 g4 b4 r5 g5 b5 r6
        // r4 g4 b4 r5 g5 b5 r6 b3
        // g4 b4 r5 g5 b5 r6 b3	r4
        // xx __ xx xx __ xx xx xx
        DEBUG_PRINT_M256("sum hi", sum_hi);

        __m256 sum_mid = _mm256_add_ps(lo_f, _mm256_add_ps(sh1_lo, sh2_hi));
        // r1 g1 b1 r2 g2 b2 r3 g3
        // g1 b1 r2 g2 b2 r3 g3 r1 //shifted 1
        // g4 b4 r5 g5 b5 r6 b3 r4 //hi shifted 2 times
        // xx xx xx xx xx xx __ xx
        DEBUG_PRINT_M256("sum mid", sum_mid);

        float tmp[5];
        tmp[0] = ((float*)&sum_lo)[0];
        tmp[1] = ((float*)&sum_lo)[3];
        tmp[2] = ((float*)&sum_hi)[1];
        tmp[3] = ((float*)&sum_hi)[4];
        tmp[4] = ((float*)&sum_mid)[6];

        __m256 result = _mm256_setr_ps(
        tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], 0.0f, 0.0f, 0.0f
        );
        DEBUG_PRINT_M256("result", result);

        __m256i sum_i = _mm256_cvtps_epi32(result); // convert floats to integers i32
        // g1, g2, g3, g4, g5, 0, 0, 0  // 8 int32
        DEBUG_PRINT_M256I("sum_i", sum_i);

        __m128i low128 = _mm256_castsi256_si128(sum_i);  // take lower 128 bits
        DEBUG_PRINT_M128I("low128", low128);

        __m128i i16_low    = _mm_packs_epi32(low128, _mm_setzero_si128()); // packs g0..g3 + 0s
        DEBUG_PRINT_M128I("i16 low", i16_low);

        __m128i g4_16 = _mm_cvtsi32_si128(((int32_t*)&sum_i)[4]); // g4 as 32-bit int
        DEBUG_PRINT_M128I("g4_16 low", g4_16);

        g4_16 = _mm_packs_epi32(g4_16, _mm_setzero_si128());       // g4 → int16 in lower 16 bits
        DEBUG_PRINT_M128I("g4_16 2", g4_16);

        __m128i result_bytes = _mm_or_si128(i16_low, _mm_slli_si128(g4_16, 8));
        // g1, g2, g3, g4, g5, 0, 0, 0 // 8 int16
        DEBUG_PRINT_M128I("result bytes", result_bytes);

        __m128i i8 = _mm_packs_epi16(result_bytes, result_bytes); // 16 int 8s
        // g1, g2, g3, g4, g5, 0, 0, 0 | g1, g2, g3, g4, g5, 0, 0, 0
        DEBUG_PRINT_M128I("i8", i8);

        __m128i out = _mm_shuffle_epi8(i8, shuf3);
        DEBUG_PRINT_M128I("out", out);

        // store 16 bytes
        _mm_storeu_si128((__m128i*)(buffer + i), out);
    }

    for (; i < end; i += 3) {
        unsigned char r = data[i];
        unsigned char g = data[i + 1];
        unsigned char b = data[i + 2];

        unsigned char gray = (unsigned char) (0.299f * r + 0.587f * g + 0.114f * b);
        buffer[i] = buffer[i + 1] = buffer[i + 2] = gray;
    }

    Image *grayscale = malloc(sizeof(Image));
    grayscale->data = buffer;
    grayscale->width = img->width;
    grayscale->height = img->height;

    return grayscale;
}