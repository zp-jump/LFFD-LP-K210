#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "fpioa.h"
#include "plic.h"
#include "sysctl.h"
#include "uarths.h"
#include "utils.h"
#include "kpu.h"
#include "w25qxx.h"
#include "iomem.h"
#include "lpbox.h"

#define KPU_DEBUG 0

#if KPU_DEBUG
#define PRINTF_KPU_OUTPUT(output, size)                          \
    {                                                            \
        printf("%s addr is %ld\n", #output, (uint64_t)(output)); \
        printf("[");                                             \
        for (size_t i=0; i < (size); i++) {                      \
            if (i%5 < 0) {                                       \
                printf("\n");                                    \
            }                                                    \
            printf("%f, ", *((output)+i));                       \
        }                                                        \
        printf("\n]");                                           \
    }
#else  // KPU_DEBUG
#define PRINTF_KPU_OUTPUT(output, size)
#endif // KPU_DEBUG

#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 400000000UL

extern const unsigned char gImage_image[] __attribute__((aligned(128)));

// static uint16_t lcd_gram[320 * 240] __attribute__((aligned(32)));

#define LPBOX_KMODEL_SIZE (556864)
uint8_t* lpbox_model_data;

kpu_model_context_t task;

// uint8_t lpimage[24 * 94 * 3] __attribute__((aligned(32)));

volatile uint8_t g_ai_done_flag;

static void ai_done(void* arg)
{
    g_ai_done_flag = 1;
}

int main()
{
    // 配置时钟
    long pll0 = sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_OUTPUT_FREQ);
    long pll1 = sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_OUTPUT_FREQ);
    printf("\nPLL0 = %ld PLL1 = %ld\n", pll0, pll1);
    uarths_init();

    plic_init();

    printf("flash init\n");
    w25qxx_init(3, 0);
    w25qxx_enable_quad_mode();
    lpbox_model_data = (uint8_t*)malloc(LPBOX_KMODEL_SIZE);
    w25qxx_read_data(0xA00000, lpbox_model_data, LPBOX_KMODEL_SIZE, W25QXX_QUAD_FAST);

    sysctl_enable_irq();

    // 加载模型
    printf("\nmodel init start\n");
    if (kpu_load_kmodel(&task, lpbox_model_data) != 0) {
        printf("model init error\n");
        while (1)
            ;
    }
    printf("\nmodel init OK\n");

    // 运行模型
    g_ai_done_flag = 0;
    kpu_run_kmodel(&task, gImage_image, DMAC_CHANNEL5, ai_done, NULL);
    printf("\nmodel run start\n");
    while (!g_ai_done_flag)
        ;
    printf("\nmodel run OK\n");

    float *score_layer0, *score_layer1, *bbox_layer0, *bbox_layer1;

    // // 提取模型推理结果
    size_t score_layer0_size;
    kpu_get_output(&task, 0, &score_layer0, &score_layer0_size);
    score_layer0_size /= 4;
    printf("\nscore_layer0_size: %ld\n", score_layer0_size);
    PRINTF_KPU_OUTPUT((score_layer0), (score_layer0_size));

    size_t bbox_layer0_size;
    kpu_get_output(&task, 1, &bbox_layer0, &bbox_layer0_size);
    bbox_layer0_size /= 4;
    printf("bbox_layer0_size: %ld\n", bbox_layer0_size);
    PRINTF_KPU_OUTPUT((bbox_layer0), (bbox_layer0_size));

    size_t bbox_layer1_size;
    kpu_get_output(&task, 2, &bbox_layer1, &bbox_layer1_size);
    bbox_layer1_size /= 4;
    printf("bbox_layer1_size: %ld\n", bbox_layer1_size);
    PRINTF_KPU_OUTPUT((bbox_layer1), (bbox_layer1_size));

    size_t score_layer1_size;
    kpu_get_output(&task, 3, &score_layer1, &score_layer1_size);
    score_layer1_size /= 4;
    printf("score_layer1_size: %ld\n", score_layer1_size);
    PRINTF_KPU_OUTPUT((score_layer1), (score_layer1_size));

    // 提取预测框
    bbox_head_t lpbox;
    printf("\nLPbox run start\n");

    // get_lpbox(score_layer0, bbox_layer0, score_layer1, bbox_layer1, &lpbox, 0.9, 0.5);
    
    printf("\nLPbox run OK\n");

    // printf("bbox num：%d\n", lpbox.num);

    printf("\nend\n");

    while (1)
        ;
}