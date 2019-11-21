/*
 *  LPbox 模型程序，用于LPbox模型的结果解析。 
 */ 

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "kpu.h"
#include "lpbox.h"


bbox_t *new_bbox(float x1, float y1, float x2, float y2, float score, bbox_t *next)
{
    bbox_t *new;
    new = (bbox_t *)malloc(sizeof(bbox_t));
    new->x1 = x1;
    new->x2 = x2;
    new->y1 = y1;
    new->y2 = y2;
    new->score = score;
    new->next = next;
    return new;
}

void free_bbox(bbox_t *bbox)
{
    free(bbox);
}

void push_bbox(bbox_head_t *bboxes, bbox_t *next)
{
    bbox_t *tmp = bboxes->box;

    bboxes->num += 1;

    if (tmp == NULL || tmp->score <= next->score) {
        next->next = bboxes->box;
        bboxes->box = next;
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

void delete_box(bbox_head_t *bboxes, bbox_t *node)
{
    bbox_t *tmp = bboxes->box;

    if (tmp == node) {
        bboxes->box = tmp->next;
        free_bbox(node);
        bboxes->num -= 1;
        node = NULL;
    } else {
        while (tmp->next != NULL) {
            if (tmp->next == node) {
                tmp->next = node->next;
                free_bbox(node);
                bboxes->num -= 1;
                node = NULL;
                break;
            }
            tmp = tmp->next;
        }
    }
}

void free_all_bboxes(bbox_head_t *bboxes)
{
    for (bbox_t* tmp = bboxes->box; tmp != NULL; tmp = tmp->next)
        free_bbox(tmp);
    bboxes->box = NULL;
    bboxes->num = 0;
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
uint get_bbox(kpu_output_t *kpu_output,
              float         score_threshold,
              bbox_head_t  *bboxes)
{
    float score, x1, x2, y1, y2;
    uint layer_size = kpu_output->w * kpu_output->h;

    for (uint h = 0; h < kpu_output->h; h++) {
        for (uint w=0; w < kpu_output->w; w++) {
            score = *(kpu_output->score_layer + h * kpu_output->w + w);
            if (score > score_threshold) {
                x1 = (kpu_output->rf_start + w * kpu_output->rf_stride) + (*(kpu_output->bbox_layer + 0 * layer_size + h * kpu_output->w + w)) * kpu_output->rf_size;
                y1 = (kpu_output->rf_start + h * kpu_output->rf_stride) + (*(kpu_output->bbox_layer + 1 * layer_size + h * kpu_output->w + w)) * kpu_output->rf_size;
                x2 = (kpu_output->rf_start + w * kpu_output->rf_stride) + (*(kpu_output->bbox_layer + 3 * layer_size + h * kpu_output->w + w)) * kpu_output->rf_size;
                y2 = (kpu_output->rf_start + h * kpu_output->rf_stride) + (*(kpu_output->bbox_layer + 4 * layer_size + h * kpu_output->w + w)) * kpu_output->rf_size;
        
                bbox_t *next = new_bbox(x1, y1, x2, y2, score, NULL);
                push_bbox(bboxes, next);
            }
        }
    }
    return 0;
}


int get_lpbox(
    lpbox_t   *lpbox,
    float      score_threshold, 
    float      nms_value)
{
    free_all_bboxes(lpbox->bboxes);

    // 提取模型推理结果
    float *score_layer0 = (lpbox->kpu_output)[0].score_layer;
    size_t layer0_w     = (lpbox->kpu_output)[0].w;
    size_t layer0_h     = (lpbox->kpu_output)[0].h;
    // printf("addr is %ld\n", (uint64_t)lpbox->kpu_output[0].score_layer);
    printf("[\n");
    for (uint i=0; i < 2; i++) {
        printf("[\n");
        for (uint h=0; h < layer0_h; h++) {
            printf("[ ");
            for (uint w=0; w < layer0_w; w++) {
                printf("%f, ", *(score_layer0 + i * (layer0_h * layer0_w) + h * layer0_w + w));
            }
            printf("],\n");
        }
        printf("],\n");
    }
    printf("]\n");

    // printf("[\n");
    // for (uint i = 0; i < 4; i++) {
    //     printf("[\n");
    //     for (uint h = 0; h < layer0_h; h++) {
    //         printf("[ ");
    //         for (uint w = 0; w < layer0_w; w++) {
    //             printf("%f, ", *(bbox_layer0 + i * (layer0_h + layer0_h) + h * layer0_w + w));
    //         }
    //         printf("],\n");
    //     }
    //     printf("],\n");
    // }
    // printf("]\n");

    // 提取预测框
    for (size_t i=0; i < lpbox->output_branch; i++) {
        get_bbox((lpbox->kpu_output+i), score_threshold, lpbox->bboxes);
    }
    return 0;
}

int lpbox_new(lpbox_t *lpbox, size_t output_branch)
{
    lpbox->output_branch = output_branch;
    lpbox->kpu_output = (kpu_output_t *)malloc(lpbox->output_branch*sizeof(kpu_output_t));
    if (lpbox->kpu_output == NULL) {
        return -1;
    }
    lpbox->bboxes = (bbox_head_t *)malloc(sizeof(bbox_head_t));
    if (lpbox->bboxes == NULL) {
        return -1;
    }
    return 0;
}

void lpbox_free(lpbox_t *lpbox)
{
    lpbox->output_branch = 0;
    free(lpbox->kpu_output);
    free_all_bboxes(lpbox->bboxes);
    free(lpbox->bboxes);
}