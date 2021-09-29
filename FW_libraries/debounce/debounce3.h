
/*
  Debounce 3 

  HieuNT
  2/5/2019
*/

#ifndef __DEBOUNCE_3_H_
#define __DEBOUNCE_3_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*bounce3_get_state_fn_t)(void);

typedef struct 
{
  uint32_t  previous_millis, interval_millis;
  bool debouncedState;
  bool unstableState;
  bounce3_get_state_fn_t getStateFn;
  bool stateChanged;
} Bounce3_t;

// Create an instance of the bounce library
void Bounce3_Init(Bounce3_t *bounce, uint32_t interval, bounce3_get_state_fn_t fn); 
// Sets the debounce interval
void Bounce3_SetInterval(Bounce3_t *bounce, unsigned long interval_millis); 
// Updates the pin
// Returns true if the state changed
// Returns false if the state did not change
bool Bounce3_Update(Bounce3_t *bounce); 
// Returns the updated pin state
bool Bounce3_Read(Bounce3_t *bounce);

#ifdef __cplusplus
}
#endif

#endif

