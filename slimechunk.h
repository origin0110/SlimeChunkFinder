#pragma once

#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LUT_FILENAME "slimechunk.bin"
#define LUT_SIZ 0x20000000

#define HELP_DOC \
    "代码开源 https://github.com/origin0110/SlimeChunkFinder\n\
输入命令进行查询，命令列表:\n\
f x1 z1 x2 z2 n k\t在(x1,z1)到(x2,z2)的坐标范围内，检查所有n*n正方形范围，当史莱姆区块数>=k时输出\n\
t x z\t\t\t检查坐标(x,z)是否在史莱姆区块内\n\
m x1 z1 x2 z2\t\t显示从(x1,z1)到(x2,z2)的史莱姆区块地图\n\
q\t\t\t退出程序，直接点关闭也可以的\n\
h\t\t\t显示帮助文档，就是就你正在看的这个\n\
示例: f -10000 -10000 10000 10000 6 14\n"
#define WRONG_COMMAND_DOC "格式错误，请输入正确的命令\n"
#define WELCOM_DOC "输入命令进行查询，输入h并回车获取帮助\n"

void gen_lut(void);
void slime_init(void);

int is_slime_chunk(int x, int z);
void slime_map(int x1, int z1, int x2, int z2);
int slime_finder_parallel(int x1, int z1, int x2, int z2, int N, int THR);