#include <stdbool.h>
#include <stdint.h>

#include<stdarg.h>
#include <stdio.h>

#include "chiptune_common_internal.h"
#include "chiptune_printf_internal.h"

static bool s_enable_processing_print_out = true;

/**********************************************************************************/

void chiptune_printf(int const print_type, const char* fmt, ...)
{
	bool is_print_out = false;

#ifdef _PRINT_MIDI_DEVELOPING
	if(cDeveloping == print_type){
		is_print_out = true;
		//fprintf(stdout, "cDeveloping:: ");
	}
#endif

	if(false == is_print_out){
		if(false == s_enable_processing_print_out){
			return ;
		}
	}

#ifdef _PRINT_MIDI_SETUP
	if(cMidiSetup == print_type){
		is_print_out = true;
		//fprintf(stdout, "cMidiSetup:: ");
	}
#endif

#ifdef _PRINT_MOTE_OPERATION
	if(cNoteOperation == print_type){
		is_print_out = true;
		//fprintf(stdout, "cNoteOperation:: ");
	}
#endif

	if (false == is_print_out){
			return;
	}

	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}

/**********************************************************************************/

void set_chiptune_processing_printf_enabled(bool is_enabled)
{
	s_enable_processing_print_out = is_enabled;
}
