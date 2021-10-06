#define __DEBUG__ 4
#include "bsp.h"
#include "publish_scheduler.h"
#include "app_info.h"
#include "app_mqtt/mqttTask.h"
#include "app_display/app_display.h"
#include "protocol_v3.h"

enum
{
    SCHEDULER_INIT,
    SCHEDULER_PUBLISH_RIDING,
    SCHEDULER_PUBLISH_KEEP_ALIVE,
};

typedef struct
{
    uint8_t state;
    uint32_t tick;
    uint32_t distance_tick;
    float distance;
} publish_scheduler_t;

static publish_scheduler_t scheduler = {0};

static void ps_state_check_process()
{
    if (display_state->display_on)
    {
        if (scheduler.state != SCHEDULER_PUBLISH_RIDING && !publish_setting->report_disabled)
        {
            debug("Start publish riding\n");
            scheduler.tick = millis();
            scheduler.distance_tick = millis();
            scheduler.distance=0;
            scheduler.state = SCHEDULER_PUBLISH_RIDING;
            publish_riding_msg();
        }
    }
    else
    {
        if (scheduler.state != SCHEDULER_PUBLISH_KEEP_ALIVE)
        {
			debug("Start publish keep alive\n");
            scheduler.tick = millis();
            scheduler.state = SCHEDULER_PUBLISH_KEEP_ALIVE;
            publish_battery_msg();
        }
    }
}

static void ps_init_process()
{
    if (mqtt_is_ready())
    {
    	debug("Mqtt ready, start publish\n");
        ps_state_check_process();
        publish_device_info_message();
    }
}

static void ps_publish_riding()
{
    publish_riding_message();
    scheduler.distance = 0;
    scheduler.tick=millis();
}

static void ps_publish_riding_process()
{
    if (millis() - scheduler.distance_tick < 1000)
    {
        goto __exit;
    }
    scheduler.distance_tick = millis();
    if (millis() - scheduler.tick > publish_setting->max_report_interval)
    {
    	debug("Publish due to timeout\n");
        ps_publish_riding();
        goto __exit;
    }
    scheduler.distance += 0.0277778 * display_state->speed;
    if (millis() - scheduler.tick > publish_setting->min_report_interval && scheduler.distance > MIN_PUBLISH_DISTANCE)
    {
    	debug("Publish due to distance %.1f, tick: %d\n", scheduler.distance, millis()-scheduler.tick);
        ps_publish_riding();
    }
__exit:
    ps_state_check_process();
}

static void ps_publish_keep_alive_process()
{
    if(millis()-scheduler.tick > publish_setting->keep_alive_interval)
    {
        scheduler.tick=millis();
        publish_keep_alive_message();
    }
    ps_state_check_process();
}

void publish_scheduler_lockpin_update()
{
	if(display_state->display_on)
	{
		if(!publish_setting->report_disabled){

		}
	}
}

void publish_scheduler_init()
{
    scheduler.state = SCHEDULER_INIT;
    scheduler.tick = millis();
}

void publish_scheduler_process()
{
    switch (scheduler.state)
    {
    case SCHEDULER_INIT:
        ps_init_process();
        break;
    case SCHEDULER_PUBLISH_RIDING:
    	ps_publish_riding_process();
    	break;
    case SCHEDULER_PUBLISH_KEEP_ALIVE:
    	ps_publish_keep_alive_process();
    	break;
    }
}
