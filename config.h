#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define KPU_DEBUG 0

#define PRINTF_KPU_OUTPUT(output, size)                          \
    {                                                            \
        printf("%s addr is %ld\n", #output, (uint64_t)(output)); \
        printf("[");                                             \
        for (size_t i=0; i < (size); i++) {                      \
            if (i%5 == 0) {                                      \
                printf("\n");                                    \
            }                                                    \
            printf("%f, ", *((output)+i));                       \
        }                                                        \
        printf("]\n");                                           \                                           
    }

/*********************************************************************************/
#define LOG_ERROR_FLAGE       (1 << 1)
#define LOG_WRAN_FLAGE        (1 << 2)
#define LOG_INFO_FLAGE        (1 << 3)
#define LOG_DEBUG_FLAGE       (1 << 4)

#define LOG_FLAGE             (LOG_ERROR_FLAGE|LOG_WRAN_FLAGE|LOG_INFO_FLAGE|LOG_DEBUG_FLAGE)

#define LOG_PRINT(class, fmt, ...)\
    printf("%s"fmt"[line:%d] [%s]\n", class, ##__VA_ARGS__, __LINE__, __FILE__);

#if (LOG_ERROR_FLAGE & LOG_FLAGE)
#define LOGE(fmt, ...) LOG_PRINT("[ERROR]", fmt, ##__VA_ARGS__)
#else
#define LOGE(fmt, ...) 
#endif // LOG_ERROR_FLAGE

#if (LOG_WRAN_FLAGE & LOG_FLAGE)
#define LOGW(fmt, ...) LOG_PRINT("[WRAR] ", fmt, ##__VA_ARGS__)
#else
#define LOGW(fmt, ...)
#endif // LOG_WRAN_FLAGE

#if (LOG_INFO_FLAGE & LOG_FLAGE)
#define LOGI(fmt, ...) LOG_PRINT("[INFO] ", fmt, ##__VA_ARGS__)
#else
#define LOGI(fmt, ...)
#endif // LOG_INFO_FLAGE

#if (LOG_DEBUG_FLAGE & LOG_FLAGE)
#define LOGD(fmt, ...) LOG_PRINT("[DEBUG]", fmt, ##__VA_ARGS__)
#else
#define LOGD(fmt, ...)
#endif // LOG_DEBUG_FLAGE
/**********************************************************************************/

#endif //_CONFIG_H_