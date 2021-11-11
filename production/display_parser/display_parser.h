/*
    Modmo Display parser

    HieuNT
    9/9/2020
*/
#ifndef __MODMO_DISPLAY_PARSER_H_
#define __MODMO_DISPLAY_PARSER_H_

#include "modmo_port.h"
#include <stdlib.h>

#define CONTROLLER_ADDR 0x1A
#define BMS_ADDR 0x16
#define WIRELESS_ADDR 0x12
#define DISP_PROTO_START_BYTE 0x3A
#define DISP_PROTO_TERM_1 0x0D
#define DISP_PROTO_TERM_2 0x0A

typedef struct __packed {
    uint8_t start;
    uint8_t addr;
    uint8_t cmd;
    uint8_t len;
} Display_Proto_Header_t;

typedef struct __packed {
    uint8_t sum1, sum2;
    uint8_t term1, term2;
} Display_Proto_PktEnd_t;

enum {
    DISP_PROTO_CMD_ODO_GEAR_BMS_CHARGE = 0x01,
    DISP_PROTO_CMD_TRIP_SPD = 0x02,
    DISP_PROTO_CMD_VOLT = 0x03,
    DISP_PROTO_CMD_CHRG = 0x04,
    DISP_PROTO_CMD_BATT = 0x05,
    DISP_PROTO_CMD_CAP_BMS_CURRENT = 0x06,
	DISP_PROTO_CMD_LIGHT=0x07
};

#define SWAP_BYTES(_in, _out) \
    _out[0] = _in[3];         \
    _out[1] = _in[2];         \
    _out[2] = _in[1];         \
    _out[3] = _in[0];

#define GET_GEAR(x) (x & 0x0F)
#define GET_BMS_CHARGING(x) ((x & (1 << 4)) ? 1 : 0)
#define GET_BMS_FULL_CHARGED(x) ((x & (1 << 5)) ? 1 : 0)


typedef union __packed {
// 24bit only
	struct __packed{
		uint8_t odo[3];
		uint8_t gearBmsCharge;
	}odoGearBmsCharge;
    struct __packed {
        uint16_t speed;
        uint16_t trip;
    } tripSpeed;
    uint32_t voltage;
    struct __packed {
        uint16_t unchargedTime;
        uint16_t cycles;
    } charge; 
    struct __packed {
        uint8_t remainCapacity;
        uint16_t temperature;
        uint8_t state;
    } battery;
    struct __packed{
		uint16_t resCap;
		int16_t bmsCurrent;
    }resCapBmsCurrent;
    bool light_on;
} Display_Proto_Unified_Arg_t;

typedef void (*display_parser_cb_t)(int header, Display_Proto_Unified_Arg_t *value);
typedef void (*uart_write_handler)(uint8_t *data, uint16_t len);

void display_parser_init(display_parser_cb_t cb);
void display_parse_byte(uint8_t b);

bool display_parser_is_valid_packet(uint8_t *rawPkt, uint16_t rawPktLen);
int display_parser_extract_packet(uint8_t *rawPkt, Display_Proto_Unified_Arg_t *outArg);
void display_parser_send_resp_packet(uart_write_handler writeHandler, uint8_t *cmdPkt, uint8_t headLightState);
int display_parser_test(void);

#endif 
