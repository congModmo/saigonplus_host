#include "factory_config.h"
#include "stdio.h"

int main(void)
{
    factory_config_t config={.broker.endpoint="a28c4si2pkzbml-ats.iot.eu-west-1.amazonaws.com", .broker.port=8883, .broker.secure=1, .hardwareVersion=0x0201};
    FILE *fp;
    fp=fopen("factoryConfig.bin", "wb");
    fwrite((uint8_t *)&config, sizeof(factory_config_t), 1, fp);
    fclose(fp);
    return 1;
}