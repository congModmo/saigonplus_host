/*
    Modmo Display parser

    HieuNT
    9/9/2020
*/
#define __debug__ 4
#ifndef __MODULE__
  #define __MODULE__ "display_parser"
#endif
#include "log_sys.h"

#include <string.h>
#include "display_parser.h"
// const char *TAG = "disp_parser";

/*
    Check valid for both packet format and own slave address
*/



#define DISPLAY_PARSER_BUFF_SIZE 30
typedef struct{
	uint8_t idx;
	uint8_t pos;
	display_parser_cb_t cb;
	uint8_t data_count;
	uint8_t buff[DISPLAY_PARSER_BUFF_SIZE];
}display_parser_t;

static display_parser_t parser;

enum{
	DISPLAY_FRAME_START,
	DISPLAY_FRAME_ADDR,
	DISPLAY_FRAME_CMD,
	DISPLAY_FRAME_LEN,
	DISPLAY_FRAME_DATA,
	DISPLAY_FRAME_SUM_1,
	DISPLAY_FRAME_SUM_2,
	DISPLAY_FRAME_TERM_1,
	DISPLAY_FRAME_TERM_2,
};

static void display_parser_reset(){
	parser.pos=DISPLAY_FRAME_START;
	parser.idx=0;
	parser.data_count=0;
}

void display_parser_init(display_parser_cb_t cb){
	parser.cb=cb;
	display_parser_reset();
}

bool display_frame_checksum(){
	uint16_t crc=0;
	for(uint8_t i=1; i<parser.idx-4; i++){
		crc += parser.buff[i];
	}
	if((crc&0x0FF)!=parser.buff[parser.idx-4])
		return false;
	if(((crc>>8)&0x0FF) !=parser.buff[parser.idx-3]){
		return false;
	}
	return true;
}

void display_parse_byte(uint8_t b){
	switch(parser.pos){
	case DISPLAY_FRAME_START:
		if(b!=DISP_PROTO_START_BYTE){
			return;
		}
		parser.buff[parser.idx++]=b;
		parser.pos=DISPLAY_FRAME_ADDR;
		break;
	case DISPLAY_FRAME_ADDR:
		if(b!=WIRELESS_ADDR && b!=CONTROLLER_ADDR){
			goto __error;
		}
		parser.buff[parser.idx++]=b;
		parser.pos=DISPLAY_FRAME_CMD;
		break;
	case DISPLAY_FRAME_CMD:
		parser.buff[parser.idx++]=b;
		parser.pos=DISPLAY_FRAME_LEN;
		break;
	case DISPLAY_FRAME_LEN:
		if(b>20){
			error("len\n");
			goto __error;
		}
		parser.buff[parser.idx++]=b;
		if(b>0){
			parser.pos=DISPLAY_FRAME_DATA;
			parser.data_count=0;
		}
		else{
			parser.pos=DISPLAY_FRAME_SUM_1;
		}
		break;
	case DISPLAY_FRAME_DATA:
		parser.buff[parser.idx++]=b;
		parser.data_count++;
		if(parser.data_count>=parser.buff[3]){
			parser.pos=DISPLAY_FRAME_SUM_1;
		}
		break;
	case DISPLAY_FRAME_SUM_1:
		parser.buff[parser.idx++]=b;
		parser.pos=DISPLAY_FRAME_SUM_2;
		break;
	case DISPLAY_FRAME_SUM_2:
		parser.buff[parser.idx++]=b;
		parser.pos=DISPLAY_FRAME_TERM_1;
		break;
	case DISPLAY_FRAME_TERM_1:
		if(b!=DISP_PROTO_TERM_1){
			error("term 1\n");
			goto __error;
		}
		parser.buff[parser.idx++]=b;
		parser.pos=DISPLAY_FRAME_TERM_2;
		break;
	case DISPLAY_FRAME_TERM_2:
//		if(b!=DISP_PROTO_TERM_2){
//			error("term 2\n");
//			goto __error;
//		}
		parser.buff[parser.idx++]=b;
		if(!display_frame_checksum()){
			error("crc\n");
			goto __error;
		}
		static Display_Proto_Unified_Arg_t outArg={0};
		if(parser.buff[1]==CONTROLLER_ADDR){
			if(parser.buff[5]&0x02){
				outArg.light_on=true;
			}
			else{
				outArg.light_on=false;
			}
			parser.cb(DISP_PROTO_CMD_LIGHT, &outArg);
		}
		else{
			int header=display_parser_extract_packet(parser.buff, &outArg);
			parser.cb(header, &outArg);
		}
		display_parser_reset();
		break;
	default:
		goto __error;
		break;
	}

	return;
	__error:
	display_parser_reset();
}

bool display_parser_is_valid_packet(uint8_t *rawPkt, uint16_t rawPktLen)
{
    uint16_t len;
    uint16_t crc;
    uint16_t i;
    Display_Proto_Header_t *hdr = (Display_Proto_Header_t *)rawPkt;
    Display_Proto_PktEnd_t *pktEnd;
    uint8_t *data;

    // min len
    if (rawPktLen < sizeof(Display_Proto_Header_t)){
        // WARN("Min len");
        return false;
    }

    data = rawPkt;
 //  debug("Pkt Len %u: \n", rawPktLen);
//    for (i = 0; i < rawPktLen; i++){
//        debugx("%02X", *data++);
//    }
//    debugx("\n");

    // packet dest address == wireless addr
    if (hdr->addr != WIRELESS_ADDR){
    //	WARNING("Not own addr\n");
        return false;
    }
    // actual len
    len = sizeof(Display_Proto_Header_t) + hdr->len + sizeof(Display_Proto_PktEnd_t);
    if(len!=rawPktLen){
    	return false;
    }
    // ASSERT_RET(len == rawPktLen, "Actual len\n", false);
    // crc
    data = rawPkt + sizeof(Display_Proto_Header_t);
    crc = hdr->addr + hdr->cmd + hdr->len;
    for (i = 0; i < hdr->len; i++){
        crc += *data++;
    }
    pktEnd = (Display_Proto_PktEnd_t *)data;
    if(!(pktEnd->sum1 == (crc & 0xFF))) return false;
    if(!(pktEnd->sum2 == ((crc >> 8) & 0xFF))) return false;
    if(!(pktEnd->term1 == DISP_PROTO_TERM_1)) return false;
    if(!(pktEnd->term2 == DISP_PROTO_TERM_2)) return false;

    // data = rawPkt + sizeof(Display_Proto_Header_t);
    //debug("start: %02X, addr %02X, cmd %02X, len %02X", hdr->start, hdr->addr, hdr->cmd, hdr->len);
    // for (i = 0; i < hdr->len; i++){
    //    debug("%02X ", *data++);
    // }

    // data = rawPkt;
    //debug("Packet: ");
    // for (i = 0; i < sizeof(Display_Proto_Header_t) + hdr->len + sizeof(Display_Proto_PktEnd_t); i++){
    //    debug("%02X ", *data++);
    // }
    // ESP_LOG_BUFFER_HEX(TAG, rawPkt, rawPktLen);

    return true;
}

/*
    Return -1: no command found
*/
int display_parser_extract_packet(uint8_t *rawPkt, Display_Proto_Unified_Arg_t *outArg)
{
    Display_Proto_Header_t *hdr = (Display_Proto_Header_t *)rawPkt;
    uint8_t *data = rawPkt + sizeof(Display_Proto_Header_t);
    uint8_t swap[4];
    Display_Proto_Unified_Arg_t *arg = (Display_Proto_Unified_Arg_t *)&swap;
    
    SWAP_BYTES(data, swap);
    if (outArg){
        memcpy(outArg, arg, sizeof(Display_Proto_Unified_Arg_t));
    }

    return (int)hdr->cmd;
}

/*
    The MCU needs to reply as follows after receiving the data with the address of 0x12
    (regardless of the command of the frame data sent by the display. And the command
    of the data returned by the MCU is the command of the frame data, and the address
    is the MCU's own device address 0x12)
*/
void display_parser_send_resp_packet(uart_write_handler writeHandler, uint8_t *cmdPkt, uint8_t headLightState)
{
    Display_Proto_Header_t *cmdHdr = (Display_Proto_Header_t *)cmdPkt;
    uint8_t pkt[sizeof(Display_Proto_Header_t) + 1 + sizeof(Display_Proto_PktEnd_t)];
    Display_Proto_Header_t *hdr = (Display_Proto_Header_t *)pkt;
    uint8_t *data = pkt + sizeof(Display_Proto_Header_t);
    Display_Proto_PktEnd_t *pktEnd = (Display_Proto_PktEnd_t *)(data + 1);
    uint16_t crc;

    hdr->start = DISP_PROTO_START_BYTE;
    hdr->addr = WIRELESS_ADDR;
    hdr->cmd = cmdHdr->cmd;
    hdr->len = 1;
    *data = headLightState;
    crc = hdr->addr + hdr->cmd + hdr->len + *data;
    pktEnd->sum1 = (crc & 0xFF);
    pktEnd->sum2 = ((crc >> 8) & 0xFF);
    pktEnd->term1 = DISP_PROTO_TERM_1;
    pktEnd->term2 = DISP_PROTO_TERM_2;

    writeHandler(pkt, sizeof(pkt));
}

static int cmd;
static Display_Proto_Unified_Arg_t outArg;
static int display_test_cb(int header, Display_Proto_Unified_Arg_t value){
	cmd=header;
	outArg=value;
	return 1;
}

//bool display_parser_test(void)
//{
////    uint8_t testOdo[] = {0x3A, 0x12, 0x01, 0x04, 0x03, 0x00, 0x00, 0x0D, 0x27, 0x00, 0x0D, 0x0A};
////    uint8_t testTripSpeed[] = {0x3A, 0x12, 0x02, 0x04, 0x00, 0x86, 0x00, 0x99, 0x37, 0x01, 0x0D, 0x0A};
////    uint8_t testVoltage[] = {0x3A, 0x12, 0x03, 0x04, 0x00, 0x00, 0x8A, 0x48, 0xEB, 0x00, 0x0D, 0x0A};
////    uint8_t testCharge[] = {0x3A, 0x12, 0x04, 0x04, 0x00, 0x0C, 0x00, 0x48, 0x6E, 0x00, 0x0D, 0x0A};
////    uint8_t testBatt[] = {0x3A, 0x12, 0x05, 0x04, 0x00, 0x0B, 0xD3, 0x3C, 0x35, 0x01, 0x0D, 0x0A};
////    uint8_t testBattResidualCap[] = {0x3A, 0x12, 0x06, 0x04, 0x00, 0x00, 0x4E, 0x20, 0x8A, 0x00, 0x0D, 0x0A};
////    display_parser_init(display_test_cb);
////    uint8_t i;
////
////    // Test ODO
////    for(i=0; i<sizeof(testOdo); i++){
////		display_parse_byte(testOdo[i]);
////	}
////    ASSERT_RET(display_parser_is_valid_packet(testOdo, sizeof(testOdo)), "TestODO: invalid packet", -1);
////    cmd = display_parser_extract_packet(testOdo, &outArg);
////    ASSERT_RET(cmd == DISP_PROTO_CMD_ODO, false, "TestODO: invalid cmd");
////    ASSERT_RET(outArg.odo == 13,false, "TestODO: invalid value");
////   debug("TestODO: PASSED");
////
////    // Test trip, speed
////	for(i=0; i<sizeof(testTripSpeed); i++){
////		display_parse_byte(testTripSpeed[i]);
////	}
////    ASSERT_RET(display_parser_is_valid_packet(testTripSpeed, sizeof(testTripSpeed)),false, "TestTripSpeed: invalid packet");
////    cmd = display_parser_extract_packet(testTripSpeed, &outArg);
////    ASSERT_RET(cmd == DISP_PROTO_CMD_TRIP_SPD, false, "TestTripSpeed: invalid cmd");
////    ASSERT_RET(outArg.tripSpeed.speed == 153,false, "TestTripSpeed: invalid speed value");
////    ASSERT_RET(outArg.tripSpeed.trip == 134, false, "TestTripSpeed: invalid trip value");
////   debug("TestTripSpeed: PASSED");
////
////    // Test voltage
////	for(i=0; i<sizeof(testVoltage); i++){
////	display_parse_byte(testVoltage[i]);
////	}
////    ASSERT_RET(display_parser_is_valid_packet(testVoltage, sizeof(testVoltage)), "TestVoltage: invalid packet");
////    cmd = display_parser_extract_packet(testVoltage, &outArg);
////    ASSERT_RET(cmd == DISP_PROTO_CMD_VOLT, "TestVoltage: invalid cmd", -1);
////    ASSERT_RET(outArg.voltage == 35400, "TestVoltage: invalid value", -1);
////   debug("TestVoltage: PASSED");
////
////    // Test charge
////	for(i=0; i<sizeof(testCharge); i++){
////		display_parse_byte(testCharge[i]);
////	}
////    ASSERT_RET(display_parser_is_valid_packet(testCharge, sizeof(testCharge)), "TestCharge: invalid packet", -1);
////    cmd = display_parser_extract_packet(testCharge, &outArg);
////    ASSERT_RET(cmd == DISP_PROTO_CMD_CHRG, "TestCharge: invalid cmd", -1);
////    ASSERT_RET(outArg.charge.unchargedTime == 72, "TestCharge: invalid uncharged time value", -1);
////    ASSERT_RET(outArg.charge.cycles == 12, "TestCharge: invalid cycles value", -1);
////   debug("TestCharge: PASSED");
////
////    // Test battery
////	for(i=0; i<sizeof(testBatt); i++){
////	   display_parse_byte(testBatt[i]);
////	}
////    ASSERT_RET(display_parser_is_valid_packet(testBatt, sizeof(testBatt)), "TestBattery: invalid packet", -1);
////    cmd = display_parser_extract_packet(testBatt, &outArg);
////    ASSERT_RET(cmd == DISP_PROTO_CMD_BATT, "TestBattery: invalid cmd", -1);
////    ASSERT_RET(outArg.battery.remainCapacity == 60, "TestBattery: invalid remain capacity value", -1);
////    ASSERT_RET((outArg.battery.temperature - 2731) == 296, "TestBattery: invalid temperature value", -1);
////    ASSERT_RET(outArg.battery.state == 0, "TestBattery: invalid state value", -1);
////   debug("TestBattery: PASSED");
////
////    // Test batt. residual cap
////	for(i=0; i<sizeof(testBattResidualCap); i++){
////		display_parse_byte(testBattResidualCap[i]);
////	}
////    ASSERT_RET(display_parser_is_valid_packet(testBattResidualCap, sizeof(testBattResidualCap)), "TestBattResidualCapacity: invalid packet", -1);
////    cmd = display_parser_extract_packet(testBattResidualCap, &outArg);
////    ASSERT_RET(cmd == DISP_PROTO_CMD_CAP, "TestBattResidualCapacity: invalid cmd", -1);
////    ASSERT_RET(outArg.battResidualCapacity == 20000, "TestBattResidualCapacity: invalid value", -1);
////   debug("TestBattResidualCapacity: PASSED");
////
////   debug("ALL TEST PASSED");
//
//    return 0;
//}
