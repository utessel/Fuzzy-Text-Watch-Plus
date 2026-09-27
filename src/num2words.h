#pragma once
#include <pebble.h>
#include "string.h"

#define LANG_SE 1
#define LANG_EN 2
#define LANG_NO 3
#define LANG_NL 4
#define LANG_IT 5
#define LANG_ES 6
#define LANG_GE 7
#define LANG_GW 8

#if defined PBL_PLATFORM_CHALK || defined PBL_PLATFORM_EMERY
	#define SCREEN_WIDE 1
#endif 

typedef struct {
	const int hours;
	const int pentaminutes;
	const char* const phrase;
} Exception;

typedef struct {	
	const char* const hours[12];
	const char* const phrases[12];
	const char* const greetings[4];
	const char* const connection_lost;
	const int number_of_exceptions;
	const Exception exceptions[];
} Language;

void time_to_words(int hours, int minutes, char* words, size_t length);

void time_to_greeting(int hour, char* greeting);

void set_language(uint8_t language);

void get_connection_lost_message(char* message);
