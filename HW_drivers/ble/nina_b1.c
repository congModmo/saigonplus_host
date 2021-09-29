

#define __DEBUG__ 4
#include "bsp.h"
#include "nina_b1.h"
#include "slip/slip_v2.h"
#include "ringbuf/ringbuf.h"
#include "app_fota/serial_transport.h"
#include "app_ble/host_comm/host_comm.h"

typedef struct
{
	uint8_t type;
	uint8_t expected;
	bool completed;
	bool result;
	uint8_t **resp;
	uint32_t *resp_len;
} nina_b1_ask_t;

static nina_b1_ask_t ask;
static frame_handler transport_handler = NULL;
static RINGBUF bleRingbuf;
static uint8_t bleSlipBuff[512];
static slip_t slip;

void nina_b1_init(void)
{
	nina_b1_bsp_init(&bleRingbuf);
	slip_init(&slip, bleSlipBuff, 512, nina_b1_bsp_send_byte);
}

void nina_b1_polling(frame_handler handler)
{
	static uint8_t c;
	static uint32_t frame_len;
	while (RINGBUF_Available(&bleRingbuf))
	{
		RINGBUF_Get(&bleRingbuf, &c);
		frame_len = slip_parse(&slip, c);
		if (frame_len > 0 && handler != NULL)
		{
			handler(slip.buff, frame_len);
		}
	}
}

void nina_b1_send0(uint8_t *data, size_t len)
{

	if (data != NULL && len > 0)
	{
		slip_send(&slip, data, len, SLIP_FRAME_COMPLETE);
	}
}

void nina_b1_send1(uint8_t type, uint8_t *data, size_t len)
{

	if (data != NULL && len > 0)
	{
		slip_send(&slip, &type, 1, SLIP_FRAME_BEGIN);
		slip_send(&slip, data, len, SLIP_FRAME_END);
	}
	else
	{
		slip_send(&slip, &type, 1, SLIP_FRAME_COMPLETE);
	}
}

void nina_b1_send2(uint8_t type1, uint8_t type2, uint8_t *data, size_t len)
{

	if (data != NULL && len > 0)
	{
		slip_send(&slip, &type1, 1, SLIP_FRAME_BEGIN);
		slip_send(&slip, &type2, 1, SLIP_FRAME_MIDDLE);
		slip_send(&slip, data, len, SLIP_FRAME_END);
	}
	else
	{
		slip_send(&slip, &type1, 1, SLIP_FRAME_BEGIN);
		slip_send(&slip, &type2, 1, SLIP_FRAME_END);
	}
}

static void ask_cb(uint8_t *frame, size_t len)
{
	if (frame[0] == ask.type)
	{
		ask.completed = true;
		if (frame[1] == ask.expected)
		{
			ask.result = true;
		}
		else
		{
			ask.result = false;
		}
	}
}

static void ask_get_response_cb(uint8_t *frame, size_t len)
{
	if (ask.resp != NULL && ask.resp_len != NULL)
	{
		*ask.resp = slip.buff;
		*ask.resp_len = len;
		ask.completed = true;
	}
}

bool nina_b1_ask(uint8_t type, uint8_t *data, size_t len, uint8_t expected, int timeout, int retry)
{

	ask.completed = false;
	ask.type = type;
	ask.expected = expected;
	ask.result = false;
	RINGBUF_Flush(&bleRingbuf);
	while (retry-- && !ask.result)
	{
		uint32_t tick = millis();
		ask.completed = false;
		if (data != NULL && len > 0)
		{
			slip_send(&slip, &type, 1, SLIP_FRAME_BEGIN);
			slip_send(&slip, data, len, SLIP_FRAME_END);
		}
		else
		{
			slip_send(&slip, &type, 1, SLIP_FRAME_COMPLETE);
		}
		while (millis() - tick < timeout && !ask.completed)
		{
			nina_b1_polling(ask_cb);
			delay(5);
		}
	}
	return ask.result;
}

bool nina_b1_ask_get_response(uint8_t *cmd, size_t cmd_len, uint8_t **resp, uint32_t *resp_len, size_t timeout, int retry)
{
	static uint32_t tick;
	ask.resp = resp;
	ask.resp_len = resp_len;
	ask.completed = false;
	RINGBUF_Flush(&bleRingbuf);
	while (retry-- && !ask.completed)
	{
		tick = millis();
		slip_send(&slip, cmd, cmd_len, SLIP_FRAME_COMPLETE);

		while (millis() - tick < timeout && !ask.completed)
		{
			nina_b1_polling(ask_get_response_cb);
			delay(5);
		}
	}
	return ask.completed;
}

static void ble_frame_cb(uint8_t *frame, size_t len)
{
	if (frame[0] == HOST_COMM_UI_MSG)
	{
		if (transport_handler != NULL)
		{
			transport_handler(frame + 1, len - 1);
		}
	}
}

static void transport_polling(frame_handler handler)
{
	transport_handler = handler;
	nina_b1_polling(ble_frame_cb);
}

static void transport_send(uint8_t *data, size_t len, bool _continue)
{
	static bool start_frame = true;
	static uint8_t cmd = HOST_COMM_UI_MSG;
	if (!_continue)
	{
		if (start_frame)
		{
			slip_send(&slip, &cmd, 1, SLIP_FRAME_BEGIN);
			slip_send(&slip, data, len, SLIP_FRAME_END);
		}
		else
		{
			start_frame = true;
			slip_send(&slip, data, len, SLIP_FRAME_END);
		}
	}
	else
	{
		if (start_frame)
		{
			start_frame = false;
			slip_send(&slip, &cmd, 1, SLIP_FRAME_BEGIN);
			slip_send(&slip, data, len, SLIP_FRAME_MIDDLE);
		}
		else
		{
			slip_send(&slip, data, len, SLIP_FRAME_MIDDLE);
		}
	}
}

static void transport_init()
{
	uint8_t cmd[2]={HOST_COMM_BLE_MSG, HOST_CMD_FAST_MODE};
	nina_b1_send0(cmd, 2);
	delay(200); //delay to let ble update connection params
}

const serial_interface_t ble_serial = {.mtu = 200, .init=transport_init, .polling = transport_polling, .send = transport_send};
