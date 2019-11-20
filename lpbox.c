/*
 *  LPbox 模型程序，用于LPbox模型的结果解析。 
 */ 

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "kpu.h"
#include "lpbox.h"

const size_t layer0_w = 19, layer0_h = 14;
const size_t layer1_w = 9 , layer1_h = 6;

const size_t rf_stride0 = 16 , rf_stride1 = 32;
const size_t rf_start0  = 15 , rf_start1  = 31;
const size_t rf_size0   = 128, rf_size1   = 256;

lpbox_t *new_lpbox(float x1, float y1, float x2, float y2, float score, lpbox_t *next)
{
    lpbox_t *new;
    new = (lpbox_t *)malloc(sizeof(lpbox_t));
    new->x1 = x1;
    new->x2 = x2;
    new->y1 = y1;
    new->y2 = y2;
    new->score = score;
    new->next = next;
    return new;
}

void free_lpbox(lpbox_t *node)
{
    free(node);
}

void push_lpbox(lpbox_head_t *lpbox, lpbox_t *next)
{
    lpbox_t *tmp = lpbox->box;

    lpbox->num += 1;

    if (tmp == NULL || tmp->score <= next->score) {
        next->next = lpbox->box;
        lpbox->box = next;
        return;
    }

    while (tmp->next != NULL) {
        if (tmp->next->score <= next->score)
            break;
        tmp = tmp->next;
    }

    next->next = tmp->next;
    tmp->next = next;
}

void delete_lpbox(lpbox_head_t *lpbox, lpbox_t *node)
{
    lpbox_t *tmp = lpbox->box;

    if (tmp == node) {
        lpbox->box = tmp->next;
        free_lpbox(node);
        lpbox->num -= 1;
        node = NULL;
    } else {
        while (tmp->next != NULL) {
            if (tmp->next == node) {
                tmp->next = node->next;
                free_lpbox(node);
                lpbox->num -= 1;
                node = NULL;
                break;
            }
            tmp = tmp->next;
        }
    }
}

void free_all_lpbox(lpbox_head_t *lpbox)
{
    for (lpbox_t* tmp = lpbox->box; tmp != NULL; tmp = tmp->next)
        free_lpbox(tmp);
    lpbox->box = NULL;
    lpbox->num = 0;
}

/*
 * 提取预测框
 * 参数：
 *     score_layer
 *     bbox_layer
 *     layer_w 
 *     layer_h 
 *     rf_stride
 *     rf_start
 *     rf_size
 *     score_threshold
 *     lp_bbox
*/
uint get_bbox(float        *score_layer,
              float        *bbox_layer,
              const size_t  layer_w,
              const size_t  layer_h,
              const size_t  rf_stride,
              const size_t  rf_start,
              const size_t  rf_size,
              float         score_threshold,
              lpbox_head_t *lpbox)
{
    float score, x1, x2, y1, y2;
    uint layer_size = layer_w * layer_h;

    for (uint h=0; h < layer_h; h++) {
        for (uint w=0; w < layer_w; w++) {
            score = *(score_layer + h * layer_w + w);
            if (score > score_threshold) {
                x1 = (rf_start + w * rf_stride) + (*(bbox_layer + 0 * layer_size + h * layer_w + w)) * rf_size;
                y1 = (rf_start + h * rf_stride) + (*(bbox_layer + 1 * layer_size + h * layer_w + w)) * rf_size;
                x2 = (rf_start + w * rf_stride) + (*(bbox_layer + 3 * layer_size + h * layer_w + w)) * rf_size;
                y2 = (rf_start + h * rf_stride) + (*(bbox_layer + 4 * layer_size + h * layer_w + w)) * rf_size;
        
                lpbox_t *next = new_lpbox(x1, y1, x2, y2, score, NULL);
                push_lpbox(lpbox, next);
            }
        }
    }
    return 0;
}


uint get_lpbox(
    float        *score_layer0,
    float        *bbox_layer0, 
    float        *score_layer1,
    float        *bbox_layer1, 
    lpbox_head_t *lpbox, 
    float         score_threshold, 
    float         nms_value)
{
    // float *score_layer0, *score_layer1, *bbox_layer0, *bbox_layer1;
    free_all_lpbox(lpbox);

    // 提取模型推理结果
    // size_t score_layer0_size;
    // kpu_get_output(ctx, 0, &score_layer0, &score_layer0_size);
    // printf("\nscore_layer0_size: %ld\n", score_layer0_size/4);
    printf("[\n");
    for (uint i=0; i < 2; i++) {
        printf("[\n");
        for (uint h=0; h < layer0_h; h++) {
            printf("[ ");
            for (uint w=0; w < layer0_w; w++) {
                printf("%f, ", *(score_layer0 + i * (layer0_h + layer0_h) + h * layer0_w + w));
            }
            printf("],\n");
        }
        printf("],\n");
    }
    printf("]\n");
    // size_t bbox_layer0_size;
    // kpu_get_output(ctx, 1, &bbox_layer0, &bbox_layer0_size);
    // printf("bbox_layer0_size: %ld\n", bbox_layer0_size/4);    
    printf("[\n");
    for (uint i = 0; i < 4; i++) {
        printf("[\n");
        for (uint h = 0; h < layer0_h; h++) {
            printf("[ ");
            for (uint w = 0; w < layer0_w; w++) {
                printf("%f, ", *(bbox_layer0 + i * (layer0_h + layer0_h) + h * layer0_w + w));
            }
            printf("],\n");
        }
        printf("],\n");
    }
    printf("]\n");
    // size_t bbox_layer1_size;
    // kpu_get_output(ctx, 2, &bbox_layer1, &bbox_layer1_size);
    // printf("bbox_layer1_size: %ld\n", bbox_layer1_size/4);
    // size_t score_layer1_size;
    // kpu_get_output(ctx, 3, &score_layer1, &score_layer1_size);
    // printf("score_layer1_size: %ld\n", score_layer1_size/4);

    // 提取预测框
    get_bbox(score_layer0, bbox_layer0, layer0_w, layer0_h,
        rf_stride0, rf_start0, rf_size0, score_threshold, lpbox);
    get_bbox(score_layer1, bbox_layer1, layer1_w, layer1_h,
        rf_stride1, rf_start1, rf_size1, score_threshold, lpbox);

    return 0;
}