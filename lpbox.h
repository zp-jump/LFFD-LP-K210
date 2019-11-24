/**
 * 19/11/20 重新设计 API
 * TODO:
 *  [x] 重构 API 添加 lpbox_t 类。
 *  [x] 测试程序
 * 注意：
 *  由于未知原因，模型输出指针无法正常传入函数，请参考 Dome 重构代码。
 */

#ifndef _LPBOX_H_
#define _LPBOX_H_

typedef struct BBOX {
    float         x1;
    float         y1;
    float         x2;
    float         y2;
    float         score;
    struct BBOX  *next;
} bbox_t; // 预测框

typedef struct {
    uint         num; // 预测框数量
    struct BBOX *box; // 预测框
} bbox_head_t;        // 预测框头部

typedef struct {
    size_t   w;
    size_t   h;
    size_t   rf_stride;
    size_t   rf_start;
    size_t   rf_size;
    float   *score_layer;
    float   *bbox_layer;
} kpu_output_t;

typedef struct {
    size_t         output_branch;
    kpu_output_t  *kpu_output;
    bbox_head_t   *bboxes;
} lpbox_t;

bbox_t* new_bbox(float x1, float y1, float x2, float y2, float score, bbox_t* next);
void free_bbox(bbox_t *bbox);
void push_bbox(bbox_head_t *bboxes, bbox_t* next);
void delete_bbox(bbox_head_t *bboxes, bbox_t* node);
void free_all_bboxes(bbox_head_t *bboxes);

int get_lpbox(
    lpbox_t  *lpbox,
    float     score_threshold,
    float     nms_value);

int lpbox_new(lpbox_t* lpbox, size_t output_branch);
void lpbox_free(lpbox_t* lpbox);

#endif /*_LPBOX_H_*/