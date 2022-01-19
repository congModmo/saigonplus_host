#ifndef _BSP_H_
#define _BSP_H_

#include <stdint.h>
#include <stm32l4xx.h>
#include <stdbool.h>
#include <string.h>
#include "log_sys.h"
#define ASSERT_RET(cond, errRet, ...) \
    if (!(cond))                      \
    {                                 \
        error(__VA_ARGS__);   \
        debugx("\n");          \
        return errRet;                \
    }
#endif
