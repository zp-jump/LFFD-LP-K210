/*
 *  LPbox 模型程序，用于LPbox模型的结果解析。 
 */ 

#include "config.h"

#include <math.h>

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

void delete_bbox(bbox_head_t *bboxes, bbox_t *node)
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
                x1 = (kpu_output->rf_start + w * kpu_output->rf_stride) - (*(kpu_output->bbox_layer + 0 * layer_size + h * kpu_output->w + w)) * kpu_output->rf_size/2.0f;
                y1 = (kpu_output->rf_start + h * kpu_output->rf_stride) - (*(kpu_output->bbox_layer + 1 * layer_size + h * kpu_output->w + w)) * kpu_output->rf_size/2.0f;
                x2 = (kpu_output->rf_start + w * kpu_output->rf_stride) - (*(kpu_output->bbox_layer + 2 * layer_size + h * kpu_output->w + w)) * kpu_output->rf_size/2.0f;
                y2 = (kpu_output->rf_start + h * kpu_output->rf_stride) - (*(kpu_output->bbox_layer + 3 * layer_size + h * kpu_output->w + w)) * kpu_output->rf_size/2.0f;
        
                bbox_t *next = new_bbox(x1, y1, x2, y2, score, NULL);
                push_bbox(bboxes, next);
            }
        }
    }
    return 0;
}


/**
 * NMS 
 */

float IoU(bbox_t *bbox1, bbox_t *bbox2)
{
    float w = fmin(bbox1->x1, bbox2->x1) - fmax(bbox1->x2, bbox2->x2);
    float h = fmin(bbox1->y1, bbox2->y1) - fmax(bbox1->y2, bbox2->y2);
    float inter = (w > 0 || h > 0)? 0 : w * h;
    return inter / ((bbox1->x2 - bbox1->x1)*(bbox1->y2 - bbox1->y1)
                  + (bbox2->x2 - bbox2->x1)*(bbox2->y2 - bbox2->y1)
                  - inter);
} 

void NMS(bbox_head_t *bboxes, float nms_value)
{
    bbox_t *next, *tmp1, *tmp2;
    for (tmp1=bboxes->box; tmp1 != NULL && tmp1->next != NULL; tmp1=tmp1->next) {
        for (tmp2=tmp1->next; tmp2 != NULL; tmp2=next) {
            next = tmp2->next;
            if (IoU(tmp1, tmp2) > nms_value) {
                delete_bbox(bboxes, tmp2);
            }
        }
    }
}

/**
 * 提取 KPU 运算结果
 * 说明：
 *  理论上这段可以用循环替代，但ncc生成的模型的输出顺序不确定，无法用循环代替
 *  请通过输出大小判断输出结果。
 */ 
int get_lpbox_kpu_output(kpu_model_context_t *ctx, lpbox_t *lpbox)
{
    float *score_layer0, *score_layer1, *bbox_layer0, *bbox_layer1;

    size_t score_layer0_size;
    kpu_get_output(ctx, 0, &score_layer0, &score_layer0_size);
#if KPU_DEBUG
    score_layer0_size /= 4;
    LOGD("\nscore_layer0_size: %ld\n", score_layer0_size);
    PRINTF_KPU_OUTPUT((score_layer0), (score_layer0_size));
#endif
    (lpbox->kpu_output)[0].score_layer = score_layer0;

    size_t bbox_layer0_size;
    kpu_get_output(ctx, 1, &bbox_layer0, &bbox_layer0_size);
#if KPU_DEBUG
    bbox_layer0_size /= 4;
    LOGD("bbox_layer0_size: %ld\n", bbox_layer0_size);
    PRINTF_KPU_OUTPUT((bbox_layer0), (bbox_layer0_size));
#endif
    (lpbox->kpu_output)[0].bbox_layer = bbox_layer0;

    return 0;
}

int get_lpbox(
    lpbox_t   *lpbox,
    float      score_threshold, 
    float      nms_value)
{
    free_all_bboxes(lpbox->bboxes);

    // 提取预测框
    for (size_t i=0; i < lpbox->output_branch; i++) {
        get_bbox((lpbox->kpu_output+i), score_threshold, lpbox->bboxes);
    }
    if (nms_value > 0) {
        LOGD("no nms bboxes num: %d", lpbox->bboxes->num);
        NMS(lpbox->bboxes, nms_value);
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