#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LUT_NAME "slimechunk.bin"
#define LUT_SIZE 0x20000000

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

static uint8_t lut[LUT_SIZE];

void generate_lut(void) {
#pragma omp parallel for
    for (uint32_t i = 0; i < LUT_SIZE; i++) {
        uint32_t seed_0 = i * 8;
        uint32_t s[8];
        uint32_t m[8];

#pragma omp simd
        for (uint32_t j = 0; j < 8; j++) {
            s[j] = seed_0 | j;
            m[j] = 0x6c078965 * (s[j] ^ s[j] >> 30) + 1;
            s[j] = (s[j] & 0x80000000) | (m[j] & 0x7fffffff);
        }

        for (int i = 2; i < 398; i++)
#pragma omp simd
            for (uint32_t j = 0; j < 8; j++)
                m[j] = 0x6c078965 * (m[j] ^ m[j] >> 30) + i;

#pragma omp simd
        for (uint32_t j = 0; j < 8; j++) {
            m[j] ^= (s[j] >> 1) ^ ((-((int32_t)(s[j] & 1))) & 0x9908b0df);
            m[j] ^= m[j] >> 11;
            m[j] ^= m[j] << 07 & 0x9d2c5680;
            m[j] ^= m[j] << 15 & 0xefc60000;
            m[j] ^= m[j] >> 18;
            s[j] = (!(m[j] % 10));
        }

        uint8_t tmp = 0;
        for (uint32_t j = 0; j < 8; j++) {
            tmp <<= 1;
            tmp |= (uint8_t)s[j];
        }

        lut[i] = tmp;
    }
}

void slime_finder(int XMIN, int ZMIN, int XMAX, int ZMAX, int N, int THR) {
    const int DZ = ZMAX - ZMIN;

    uint8_t* f2 = calloc(N * DZ, 8);
    uint8_t* f1 = calloc(DZ, 8);

    uint32_t* tmp_u32 = calloc(DZ, sizeof(uint32_t));
    int8_t* tmp_i8 = calloc(DZ, sizeof(int8_t));

    for (int32_t x = XMIN; x < XMAX; x++) {
        size_t xmodN = (x - XMIN) % N;
        size_t f2_base_index = xmodN * DZ;

#pragma omp simd
        for (size_t zi = 0; zi < DZ; zi++) {
            tmp_u32[zi] = ((uint32_t)x * 0x1f1f1f1f) ^ (uint32_t)(zi + ZMIN);
            tmp_i8[zi] = (1 << (8 - 1)) >> (tmp_u32[zi] % 8);
            tmp_u32[zi] = tmp_u32[zi] / 8;
            f1[zi] -= f2[f2_base_index + zi];
        }

        for (size_t zi = 0; zi < DZ; zi++)  // 没法向量化
            f2[f2_base_index + zi] = lut[tmp_u32[zi]];

#pragma omp simd
        for (size_t zi = 0; zi < DZ; zi++) {
            f2[f2_base_index + zi] = (f2[f2_base_index + zi] & tmp_i8[zi]) != 0;
            f1[zi] += f2[f2_base_index + zi];
        }

        for (size_t zi = 0; zi < N; zi++)
            tmp_i8[zi] = f1[zi];
#pragma omp simd
        for (size_t zi = N; zi < DZ; zi++)
            tmp_i8[zi] = f1[zi] - f1[zi - N];

        int32_t count = 0;
        for (size_t zi = 0; zi < DZ; zi++) {
            count += tmp_i8[zi];
            if (count >= THR) {
                int32_t z = zi + ZMIN;
                printf("x=%d,z=%d,n=%d\n", (x + 1) * 16 - (N * 8), (z + 1) * 16 - (N * 8), count);
            }
        }
    }

    free(f2);
    free(f1);
    free(tmp_u32);
    free(tmp_i8);
}

int func_f(int x1, int z1, int x2, int z2, int N, int THR) {
    double time0 = omp_get_wtime();

    const int XMIN = (int)floor(min(x1, x2) / 16.0);
    const int XMAX = (int)floor(max(x1, x2) / 16.0) + 1;
    const int ZMIN = (int)floor(min(z1, z2) / 16.0);
    const int ZMAX = (int)floor(max(z1, z2) / 16.0) + 1;
    const int DX = XMAX - XMIN;
    const int DZ = ZMAX - ZMIN;

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
    if (DX * DZ < THR) {
        printf("搜索到一颗桃子\n");
        return -1;
    }

    if (DX <= 256 || DZ <= 256) {
        slime_finder(XMIN, ZMIN, XMAX, ZMAX, N, THR);
    } else
#pragma omp parallel
    {
        const int thread_num = omp_get_num_threads();

        int z_size = DZ / thread_num;

        int thread_id = omp_get_thread_num();
        int zmin = ZMIN + thread_id * z_size;
        int zmax;
        if (thread_id != thread_num - 1)
            zmax = ZMIN + (thread_id + 1) * z_size + N - 1;
        else
            zmax = ZMAX;
        slime_finder(XMIN, zmin, XMAX, zmax, N, THR);
    }

    double time1 = omp_get_wtime();
    double time = time1 - time0;
    fprintf(stderr, "搜索结束，用时%lfs，%lfchunks/s\n", time, (uint64_t)DZ * (uint64_t)DX / time);

    return 0;
}

void slime_initialization(void) {
    FILE* fp = fopen(LUT_NAME, "rb");
    if (fp == NULL) {
        fprintf(stderr, "找不到重要文件%s，正在尝试重新生成，请耐心等待约一分钟\n", LUT_NAME);
        generate_lut();
        fp = fopen(LUT_NAME, "wb");
        fwrite(lut, 1, LUT_SIZE, fp);
        fclose(fp);
        fprintf(stderr, "文件%s已经重新生成，初始化成功\n", LUT_NAME);
    } else {
        fread(lut, 1, LUT_SIZE, fp);
        fclose(fp);
        fprintf(stderr, "找到文件%s，已加载文件，初始化成功\n", LUT_NAME);
    }
}

#define HELP_DOC \
    "输入命令进行查询，命令列表:\n\
f x1 z1 x2 z2 n k\t在(x1,z1)到(x2,z2)的坐标范围内，检查所有n*n正方形范围，当史莱姆区块数>=k时输出\n\
q\t\t\t退出程序，直接点关闭也可以的\n\
h\t\t\t显示帮助文档，就是就你正在看的这个\n\
示例: f -10000 -10000 10000 10000 6 14\n"

#define WRONG_COMMAND_DOC "格式错误，请输入正确的命令\n"

int main(void) {
    slime_initialization();

    fprintf(stderr, "输入命令进行查询，输入h并回车获取帮助\n");

    int x1, x2, z1, z2, n, thr, tmp;

    while (1) {
        switch (getchar()) {
            case 'f':
                tmp = scanf("%d %d %d %d %d %d", &x1, &x2, &z1, &z2, &n, &thr);
                if (tmp != 6) {
                    fprintf(stderr, WRONG_COMMAND_DOC);
                    break;
                } else {
                    func_f(x1, x2, z1, z2, n, thr);
                }
                break;
            case 'h':
                fprintf(stderr, HELP_DOC);
                break;
            case 'q':
                return 0;
        }
    }
}