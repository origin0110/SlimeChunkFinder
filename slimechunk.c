#include "slimechunk.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

uint8_t slime_lut[LUT_SIZ];

////////////////////////////////////////////////////////////////

// 每个block中有BLOCK_SIZ个seed。需要是2^n并且不小于8，在5900hx上64最快
// 总共有BLOCK_NUM个block。block内使用simd并行，block间使用多线程并行
#define BLOCK_SIZ 64
#define BLOCK_NUM (LUT_SIZ / BLOCK_SIZ * 8)

void gen_lut_block(uint32_t block_idx) {
    const uint32_t seed_0 = block_idx * BLOCK_SIZ;
    const uint32_t lut_idx0 = seed_0 / 8;

    // 缓存中间值，把判定过程改写成矢量化写法
    uint32_t s[BLOCK_SIZ];
    uint32_t m[BLOCK_SIZ];

#pragma omp simd
    for (uint32_t j = 0; j < BLOCK_SIZ; j++) {
        s[j] = seed_0 | j;
        m[j] = 0x6c078965 * (s[j] ^ s[j] >> 30) + 1;
        s[j] = (s[j] & 0x80000000) | (m[j] & 0x7fffffff);
        s[j] = (s[j] >> 1) ^ ((-((int32_t)(s[j] & 1))) & 0x9908b0df);
    }

    for (int k = 2; k < 398; k++)
#pragma omp simd
        for (uint32_t j = 0; j < BLOCK_SIZ; j++)
            m[j] = 0x6c078965 * (m[j] ^ m[j] >> 30) + k;

#pragma omp simd
    for (uint32_t j = 0; j < BLOCK_SIZ; j++) {
        m[j] ^= s[j];
        m[j] ^= m[j] >> 11;
        m[j] ^= m[j] << 07 & 0x9d2c5680;
        m[j] ^= m[j] << 15 & 0xefc60000;
        m[j] ^= m[j] >> 18;
        m[j] = (!(m[j] % 10));
    }

    // 缓存中间值，把压位生成lut的过程改写成矢量化写法
    uint8_t tmp0[BLOCK_SIZ];
    uint8_t tmp1[BLOCK_SIZ / 2];
    uint8_t tmp2[BLOCK_SIZ / 4];

#pragma omp simd
    for (uint32_t k = 0; k < BLOCK_SIZ; k++)
        tmp0[k] = (uint8_t)m[k] << (7 - (uint8_t)k % 8);
#pragma omp simd
    for (int i = 0; i < BLOCK_SIZ / 2; i++)
        tmp1[i] = tmp0[2 * i] + tmp0[2 * i + 1];
#pragma omp simd
    for (int i = 0; i < BLOCK_SIZ / 4; i++)
        tmp2[i] = tmp1[2 * i] + tmp1[2 * i + 1];
#pragma omp simd
    for (int i = 0; i < BLOCK_SIZ / 8; i++)
        slime_lut[lut_idx0 + i] = tmp2[2 * i] + tmp2[2 * i + 1];
}

void gen_lut(void) {
#pragma omp parallel for
    for (uint32_t block_idx = 0; block_idx < BLOCK_NUM; block_idx++) {
        gen_lut_block(block_idx);
    }
}

void slime_init(void) {
    FILE* fp = fopen(LUT_FILENAME, "rb");
    if (fp == NULL) {
        fprintf(stderr, "找不到重要文件%s，正在尝试重新生成，请耐心等待约一分钟\n", LUT_FILENAME);
        double time0 = omp_get_wtime();
        gen_lut();
        double time1 = omp_get_wtime();
        fp = fopen(LUT_FILENAME, "wb");
        fwrite(slime_lut, 1, LUT_SIZ, fp);
        fclose(fp);
        fprintf(stderr, "文件%s已经重新生成，初始化成功，用时%lfs\n", LUT_FILENAME, time1 - time0);
    } else {
        fread(slime_lut, 1, LUT_SIZ, fp);
        fclose(fp);
        fprintf(stderr, "找到文件%s，已加载文件，初始化成功\n", LUT_FILENAME);
    }
}

////////////////////////////////////////////////////////////////

int32_t to_chunkpos(int32_t pos) {
    int32_t tmp = pos % 16;
    if (tmp != 0 && pos < 0)
        return pos / 16 - 1;
    else
        return pos / 16;
}

uint32_t to_seed(int x, int z) {
    return ((uint32_t)x * 0x1f1f1f1f) ^ (uint32_t)z;
}

int is_slime_chunk_seed(uint32_t s) {
    return (slime_lut[s / 8] & (0b10000000 >> (s % 8))) != 0;
}

int is_slime_chunk(int x, int z) {
    int32_t X = to_chunkpos(x);
    int32_t Z = to_chunkpos(z);
    return is_slime_chunk_seed(to_seed(X, Z));
}

////////////////////////////////////////////////////////////////

void slime_map(int x1, int z1, int x2, int z2) {
    const int XMIN = to_chunkpos(min(x1, x2));
    const int XMAX = to_chunkpos(max(x1, x2)) + 1;
    const int ZMIN = to_chunkpos(min(z1, z2));
    const int ZMAX = to_chunkpos(max(z1, z2)) + 1;
    const int DX = XMAX - XMIN;
    const int DZ = ZMAX - ZMIN;

    char* str = (char*)calloc(2 * (size_t)DX * (size_t)DZ, sizeof(char) + 1);
    char* write = str;

    for (int z = ZMIN; z <= ZMAX; ++z) {
        for (int x = XMIN; x <= XMAX; ++x) {
            *write++ = is_slime_chunk_seed(to_seed(x, z)) ? '#' : ' ';
            *write++ = x == XMAX ? '\n' : ' ';
        }
    }

    puts(str);
    free(str);
}

////////////////////////////////////////////////////////////////

void slime_finder(int XMIN, int ZMIN, int XMAX, int ZMAX, int N, int THR) {
    const int DZ = ZMAX - ZMIN;

    uint8_t* f2 = (uint8_t*)calloc(N * DZ, 1);
    uint8_t* f1 = (uint8_t*)calloc(DZ, 1);

    for (int32_t x = XMIN; x < XMAX; x++) {
        size_t xmodN = (x - XMIN) % N;
        size_t f2_base_index = xmodN * DZ;

#pragma omp simd
        for (size_t zi = 0; zi < DZ; zi++) {
            f1[zi] -= f2[f2_base_index + zi];
        }

        for (size_t zi = 0; zi < DZ; zi++) {  // 没法向量化
            f2[f2_base_index + zi] = slime_lut[to_seed(x, zi + ZMIN) / 8];
        }

#pragma omp simd
        for (size_t zi = 0; zi < DZ; zi++) {
            uint8_t tmp = 0b10000000 >> (to_seed(x, zi + ZMIN) % 8);
            f2[f2_base_index + zi] = (f2[f2_base_index + zi] & tmp) != 0;
            f1[zi] += f2[f2_base_index + zi];
        }

        int32_t count = 0;
        for (size_t zi = 0; zi < N; zi++) {
            count += f1[zi];
        }
        for (size_t zi = N; zi < DZ; zi++) {
            count += f1[zi] - f1[zi - N];
            if (count >= THR) {
                int32_t z = zi + ZMIN;
                printf("x=%d,z=%d,n=%d\n", (x + 1) * 16 - (N * 8), (z + 1) * 16 - (N * 8), count);
            }
        }
    }

    free(f2);
    free(f1);
}

int slime_finder_parallel(int x1, int z1, int x2, int z2, int N, int THR) {
    double time0 = omp_get_wtime();

    const int XMIN = to_chunkpos(min(x1, x2));
    const int XMAX = to_chunkpos(max(x1, x2)) + 1;
    const int ZMIN = to_chunkpos(min(z1, z2));
    const int ZMAX = to_chunkpos(max(z1, z2)) + 1;
    const int DX = XMAX - XMIN;
    const int DZ = ZMAX - ZMIN;

    int thread_num;

#pragma omp parallel
    {
        thread_num = omp_get_num_threads();
    }

    int z_size = DZ / thread_num;

    if (N >= 128) {
        printf("N太大了，有溢出风险，别搜\n");
        return 5;
    }
    if (N < 1) {
        printf("搜索到一份炒饭\n");
        return 5;
    }
    if (THR < 1) {
        printf("搜索到一杯啤酒\n");
        return 6;
    }
    if (DX < N || DZ < N) {
        printf("搜索到一个笨比\n");
        return 1;
    }
    if (N * N < THR) {
        printf("搜索到一颗桃子\n");
        return -1;
    }

    if (DX <= N * 2 || z_size <= N * 2) {
        slime_finder(XMIN, ZMIN, XMAX, ZMAX, N, THR);
    } else {
        fprintf(stderr, "搜索范围较大，将使用%d线程\n", thread_num);
#pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
            int zmin = ZMIN + thread_id * z_size;
            int zmax;
            if (thread_id != thread_num - 1)
                zmax = ZMIN + (thread_id + 1) * z_size + N - 1;
            else
                zmax = ZMAX;
            slime_finder(XMIN, zmin, XMAX, zmax, N, THR);
        }
    }

    double time1 = omp_get_wtime();
    double time = time1 - time0;
    fprintf(stderr, "搜索结束，用时%lfs，%lfchunks/s\n", time, (uint64_t)DZ * (uint64_t)DX / time);

    return 0;
}

////////////////////////////////////////////////////////////////
