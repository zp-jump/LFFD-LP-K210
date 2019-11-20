#ifndef _LPBOX_H_
#define _LPBOX_H_

#include <kpu.h>

typedef struct LPBOX 
{
    float         x1;
    float         y1;
    float         x2;
    float         y2;
    float         score;
    struct LPBOX *next;
} lpbox_t;

typedef struct
{
    uint          num; // 预测框数量
    struct LPBOX *box; 
} lpbox_head_t;

lpbox_t* new_lpbox(float x1, float y1, float x2, float y2, float score, lpbox_t* next);
void free_lpbox(lpbox_t* node);
void push_lpbox(lpbox_head_t* lpbox, lpbox_t* next);
void delete_lpbox(lpbox_head_t* lpbox, lpbox_t* node);
void free_all_lpbox(lpbox_head_t* lpbox);

uint get_lpbox(
    float        *score_layer0,
    float        *bbox_layer0, 
    float        *score_layer1,
    float        *bbox_layer1, 
    lpbox_head_t *lpbox, 
    float         score_threshold, 
    float         nms_value);

#endif /*_LPBOX_H_*/