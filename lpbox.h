/**
 * 19/11/20 重新设计 API
 * TODO:
 *  [] 重构 API 添加 lpbox_t 类。
 *  [] 测试程序
 * 注意：
 *  由于未知原因，模型输出指针无法正常传入函数，请参考 Dome 重构代码。
 */

#ifndef _LPBOX_H_
#define _LPBOX_H_

#include <kpu.h>

typedef struct BBOX {
    float         x1;
    float         y1;
    float         x2;
    float         y2;
    float         score;
    struct BBOX *next;
} bbox_t;

typedef struct {
    uint          num; // 预测框数量
    struct BBOX *box; 
} bbox_head_t;



bbox_t* new_bbox(float x1, float y1, float x2, float y2, float score, bbox_t* next);
void free_bbox(bbox_t *bbox);
void push_bbox(bbox_head_t *bboxes, bbox_t* next);
void delete_bbox(bbox_head_t *bboxes, bbox_t* node);
void free_all_bboxes(bbox_head_t *bboxes);

uint get_lpbox(
    float        *score_layer0,
    float        *bbox_layer0, 
    float        *score_layer1,
    float        *bbox_layer1, 
    bbox_head_t *lpbox, 
    float         score_threshold, 
    float         nms_value);

#endif /*_LPBOX_H_*/