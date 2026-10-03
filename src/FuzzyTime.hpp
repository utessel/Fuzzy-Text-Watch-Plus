#pragma once

#include <cstdint>

#define LANG_SE 1
#define LANG_EN 2
#define LANG_NO 3
#define LANG_NL 4
#define LANG_IT 5
#define LANG_ES 6
#define LANG_GE 7
#define LANG_GW 8

struct TimeLine
{
	const char *text;
	bool is_bold;
};

void set_language(uint8_t lang);
uint8_t get_language();

// Returns number of lines (1 to 4) for the given language and time
int get_fuzzy_time(uint8_t lang, int hour, int minute, TimeLine lines[4]);

// Weekdays (0=Sunday .. 6=Saturday) and months (0=Jan .. 11=Dec)
const char *get_fuzzy_weekday(uint8_t lang, int wday);
const char *get_fuzzy_month(uint8_t lang, int mon);

// Bluetooth connection lost message lines (returns number of lines)
int get_connection_lost_lines(uint8_t lang, TimeLine lines[4]);
