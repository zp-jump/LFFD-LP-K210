#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "fpioa.h"
#include "lcd.h"
#include "board_config.h"
#include "plic.h"
#include "sysctl.h"
#include "uarths.h"
#include "nt35310.h"
#include "utils.h"
#include "kpu.h"
#include "w25qxx.h"
#include "iomem.h"
#include "lpbox.h"

#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 400000000UL

extern const unsigned char gImage_image[] __attribute__((aligned(128)));

static uint16_t lcd_gram[320 * 240] __attribute__((aligned(32)));

#define LPBOX_KMODEL_SIZE (558000)
uint8_t* lpbox_model_data;

kpu_model_context_t task;

// uint8_t lpimage[24 * 94 * 3] __attribute__((aligned(32)));

volatile uint8_t g_ai_done_flag;

static int ai_done(void* ctx)
{
    g_ai_done_flag = 1;
    return 0;
}

#if BOARD_LICHEEDAN
static void io_mux_init(void)
{
    /* Init DVP IO map and function settings */
    fpioa_set_function(42, FUNC_CMOS_RST);
    fpioa_set_function(44, FUNC_CMOS_PWDN);
    fpioa_set_function(46, FUNC_CMOS_XCLK);
    fpioa_set_function(43, FUNC_CMOS_VSYNC);
    fpioa_set_function(45, FUNC_CMOS_HREF);
    fpioa_set_function(47, FUNC_CMOS_PCLK);
    fpioa_set_function(41, FUNC_SCCB_SCLK);
    fpioa_set_function(40, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);

    sysctl_set_spi0_dvp_data(1);
}

static void io_set_power(void)
{
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
}

#else
static void io_mux_init(void)
{
    /* Init DVP IO map and function settings */
    fpioa_set_function(11, FUNC_CMOS_RST);
    fpioa_set_function(13, FUNC_CMOS_PWDN);
    fpioa_set_function(14, FUNC_CMOS_XCLK);
    fpioa_set_function(12, FUNC_CMOS_VSYNC);
    fpioa_set_function(17, FUNC_CMOS_HREF);
    fpioa_set_function(15, FUNC_CMOS_PCLK);
    fpioa_set_function(10, FUNC_SCCB_SCLK);
    fpioa_set_function(9, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(6, FUNC_SPI0_SS3);
    fpioa_set_function(7, FUNC_SPI0_SCLK);

    sysctl_set_spi0_dvp_data(1);
}

static void io_set_power(void)
{
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK2, SYSCTL_POWER_V18);
}
#endif

void rgb888_to_lcd(uint8_t* src, uint16_t* dest, size_t width, size_t height)
{
    size_t i, chn_size = width * height;
    for (size_t i = 0; i < width * height; i++) {
        uint8_t r = src[i];
        uint8_t g = src[chn_size + i];
        uint8_t b = src[chn_size * 2 + i];

        uint16_t rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
        size_t d_i = i % 2 ? (i - 1) : (i + 1);
        dest[d_i] = rgb;
    }
}

int main()
{
    // 配置时钟
    long pll0 = sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_OUTPUT_FREQ);
    long pll1 = sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_OUTPUT_FREQ);
    printf("\nPLL0 = %ld PLL1 = %ld\n", pll0, pll1);
    uarths_init();

    io_mux_init();
    io_set_power();
    plic_init();

    printf("flash init\n");
    w25qxx_init(3, 0);
    w25qxx_enable_quad_mode();
    lpbox_model_data = (uint8_t*)iomem_malloc(LPBOX_KMODEL_SIZE);
    w25qxx_read_data(0xA00000, lpbox_model_data, LPBOX_KMODEL_SIZE, W25QXX_QUAD_FAST);

    /* LCD init */
    printf("LCD init\n");
    lcd_init();
#if BOARD_LICHEEDAN
    lcd_set_direction(DIR_YX_RLDU);
#else
    lcd_set_direction(DIR_YX_RLUD);
#endif

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

    // 提取预测框
    lpbox_head_t lpbox;
    printf("\nLPbox run start\n");
    get_lpbox(&task, &lpbox, 0.5, 0.5);
    printf("\nLPbox run OK\n");

    printf("bbox num：%d\n", lpbox.num);

    // 输出运算结果
    // float *output;
    // size_t size;

    // kpu_get_output(&task, 0, &output, &size);
    // size /= 4;
    // printf("\noutput size: %ld\n", size);
    // printf("[\n");
    // for (size_t i=0; i < size; i++) {
    //     printf("%f,\n", *(output + i));
    // }
    // printf("]\n");

    /* display pic*/
    rgb888_to_lcd(gImage_image, lcd_gram, 320, 240);
    lcd_draw_picture(0, 0, 320, 240, lcd_gram);

    printf("\nend\n");

    while (1)
        ;
}