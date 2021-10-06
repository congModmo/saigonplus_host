#ifndef __PUBLISH_SCHEDULER_H_
#define __PUBLISH_SCHEDULER_H_
#ifdef __cplusplus
extern "C" {
#endif

#define MIN_PUBLISH_DISTANCE 30 // 30 m

void publish_scheduler_init();
void publish_scheduler_process();
void publish_scheduler_lockpin_update();

#ifdef __cplusplus
}
#endif
#endif
