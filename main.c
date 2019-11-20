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

#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 400000000UL

extern const unsigned char gImage_image[] __attribute__((aligned(128)));

// static uint16_t lcd_gram[320 * 240] __attribute__((aligned(32)));

#define LPBOX_KMODEL_SIZE (556864)
uint8_t* lpbox_model_data;

kpu_model_context_t task;

// uint8_t lpimage[24 * 94 * 3] __attribute__((aligned(32)));

volatile uint8_t g_ai_done_flag;

static int ai_done(void* ctx)
{
    g_ai_done_flag = 1;
    return 0;
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

    // float input[] = {1.0, 1.0, 2.0, 3.0,
    //                  4.0, 5.0, 6.0, 7.0,
    //                  8.0, 9.0, 1.0, 2.0,
    //                  3.0, 4.0, 5.0, 6.0};

    // 运行模型
    g_ai_done_flag = 0;
    kpu_run_kmodel(&task, gImage_image, DMAC_CHANNEL5, ai_done, NULL);
    printf("\nmodel run start\n");
    while (!g_ai_done_flag)
        ;
    printf("\nmodel run OK\n");

    float *output;
    size_t size;

    

    kpu_get_output(&task, 3, &output, &size);
    size /= 4;
    printf("\noutput size: %ld\n", size);
    printf("[\n");
    for (size_t i=0; i < size; i++) {
        printf("%f,\n", *(output + i));
    }
    printf("\n\n");

    // float add_out;
    // for (size_t i=0; i < 16; i++) {
    //     add_out = *(input + i) + *(input + i);
    //     printf("%f -> %f\n", *(input + i), add_out);
    // }


    printf("\nend\n");

    while (1)
        ;
}