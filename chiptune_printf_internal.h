#ifndef _CHIPTUNE_PRINTF_INTERNAL_H_
#define _CHIPTUNE_PRINTF_INTERNAL_H_

#include <stdbool.h>


#define CHIPTUNE_PRINTF(PRINT_TYPE, FMT, ...)		\
													do { \
														chiptune_printf(PRINT_TYPE, FMT, ##__VA_ARGS__); \
													}while(0)

#if(0)
#define CHIPTUNE_PRINTF(PRINT_TYPE, FMT, ...)				do { \
													(void)0; \
												}while(0)
#endif

#define SET_PROCESSING_CHIPTUNE_PRINTF_ENABLED(IS_ENABLED)		\
													do { \
														set_chiptune_processing_printf_enabled(IS_ENABLED); \
													} while(0)


enum
{
	cDeveloping		= 0,
	cMidiSetup		= 1,
	cNoteOperation	= 2,
	cOscillatorTransition = 3,
};

void chiptune_printf(int const print_type, const char* fmt, ...);
void set_chiptune_processing_printf_enabled(bool is_enabled);


#endif // _CHIPTUNE_PRINTF_INTERNAL_H_
