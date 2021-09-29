#ifndef __UTILS_H_
#define __UTILS_H_

#include "../../BSP/target.h"

#ifdef __cplusplus
extern "C" {
#endif

int str_token(char *s, char *set, char **token, int maxSz);

#ifdef __cplusplus
}
#endif

#endif
