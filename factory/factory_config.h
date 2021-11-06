#ifndef __FACTORY_CONFIG_H_
#define __FACTORY_CONFIG_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

typedef struct
{
    char endpoint[128];
    int port;
    int secure;
} broker_t;

typedef struct
{
    uint16_t hardwareVersion;
    broker_t broker;
} factory_config_t;

#ifdef __cplusplus
}
#endif
#endif
