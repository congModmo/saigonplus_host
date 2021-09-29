

/*
  Debounce 3 

  HieuNT
  2/5/2019
*/
#include "debounce3.h"
#include "../../BSP/target.h"

void Bounce3_Init(Bounce3_t *bounce, uint32_t interval, bounce3_get_state_fn_t fn)
{
	bounce->interval_millis = interval;
	bounce->debouncedState = false;
	bounce->getStateFn = fn;
	bounce->previous_millis = millis();
	bounce->stateChanged = false;
	bounce->debouncedState = bounce->unstableState = fn(); // force init state!
}

void Bounce3_SetInterval(Bounce3_t *bounce, unsigned long interval_millis)
{
  bounce->interval_millis = interval_millis;
}

bool Bounce3_Update(Bounce3_t *bounce)
{
	// Lire l'etat de l'interrupteur dans une variable temporaire.
	bool currentState = bounce->getStateFn();
	bounce->stateChanged = false;

	// Redemarrer le compteur timeStamp tant et aussi longtemps que
	// la lecture ne se stabilise pas.
	if ( currentState != bounce->unstableState ) {
			bounce->previous_millis = millis();
	} else 	if ( millis() - bounce->previous_millis >= bounce->interval_millis ) {
				// Rendu ici, la lecture est stable

				// Est-ce que la lecture est différente de l'etat emmagasine de l'interrupteur?
				if ( currentState != bounce->debouncedState ) {
						bounce->debouncedState = currentState;
						bounce->stateChanged = true;		
				}
	}
	bounce->unstableState = currentState;
	return bounce->stateChanged;
}

bool Bounce3_Read(Bounce3_t *bounce)
{
	return bounce->debouncedState;
}
