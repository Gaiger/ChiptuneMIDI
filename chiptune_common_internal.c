#include "chiptune_common_internal.h"

#ifndef _FIXED_MAX_OSCILLATOR_AND_EVENT_NUMBER
#include <stdlib.h>

/**********************************************************************************/

void* chiptune_malloc(size_t size){ return malloc(size);}

/**********************************************************************************/

void chiptune_free(void* ptr){ free(ptr);}

#endif