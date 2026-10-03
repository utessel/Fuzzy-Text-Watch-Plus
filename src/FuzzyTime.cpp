#include "FuzzyTime.hpp"

static uint8_t s_current_language = LANG_GW;

void set_language(uint8_t lang)
{
	if (lang >= LANG_SE && lang <= LANG_GW)
	{
		s_current_language = lang;
	}
	else
	{
		s_current_language = LANG_GW;
	}
}

uint8_t get_language()
{
	return s_current_language;
}

static void normalize_hours(int hour, int &h, int &next_h)
{
	h = hour % 12;
	if (h == 0)
	{
		h = 12;
	}
	next_h = (h % 12) + 1;
}

// Svenska (LANG_SE = 1)
static int get_fuzzy_time_swedish(int h, int next_h, int p, TimeLine lines[4])
{
	static const char *hours[] = {
		"", "ett", "två", "tre", "fyra", "fem",
		"sex", "sju", "åtta", "nio", "tio", "elva", "tolv"
	};

	switch (p)
	{
		case 0:
			lines[0] = { "klockan är", false };
			lines[1] = { hours[h], true };
			return 2;
		case 1:
			lines[0] = { "fem över", false };
			lines[1] = { hours[h], true };
			return 2;
		case 2:
			lines[0] = { "tio över", false };
			lines[1] = { hours[h], true };
			return 2;
		case 3:
			lines[0] = { "kvart över", false };
			lines[1] = { hours[h], true };
			return 2;
		case 4:
			lines[0] = { "tjugo över", false };
			lines[1] = { hours[h], true };
			return 2;
		case 5:
			lines[0] = { "fem i", false };
			lines[1] = { "halv", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 6:
			lines[0] = { "halv", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 7:
			lines[0] = { "fem över", false };
			lines[1] = { "halv", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 8:
			lines[0] = { "tjugo i", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 9:
			lines[0] = { "kvart i", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 10:
			lines[0] = { "tio i", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 11:
			lines[0] = { "fem i", false };
			lines[1] = { hours[next_h], true };
			return 2;
		default:
			return 0;
	}
}

// English (LANG_EN = 2)
static int get_fuzzy_time_english(int h, int next_h, int p, TimeLine lines[4])
{
	static const char *hours[] = {
		"", "one", "two", "three", "four", "five",
		"six", "seven", "eight", "nine", "ten", "eleven", "twelve"
	};

	switch (p)
	{
		case 0:
			lines[0] = { hours[h], true };
			lines[1] = { "o'clock", false };
			return 2;
		case 1:
			lines[0] = { "five past", false };
			lines[1] = { hours[h], true };
			return 2;
		case 2:
			lines[0] = { "ten past", false };
			lines[1] = { hours[h], true };
			return 2;
		case 3:
			lines[0] = { "quarter past", false };
			lines[1] = { hours[h], true };
			return 2;
		case 4:
			lines[0] = { "twenty past", false };
			lines[1] = { hours[h], true };
			return 2;
		case 5:
			lines[0] = { "twenty five", false };
			lines[1] = { "past", false };
			lines[2] = { hours[h], true };
			return 3;
		case 6:
			lines[0] = { "half past", false };
			lines[1] = { hours[h], true };
			return 2;
		case 7:
			lines[0] = { "twenty five", false };
			lines[1] = { "to", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 8:
			lines[0] = { "twenty to", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 9:
			lines[0] = { "quarter to", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 10:
			lines[0] = { "ten to", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 11:
			lines[0] = { "five to", false };
			lines[1] = { hours[next_h], true };
			return 2;
		default:
			return 0;
	}
}

// Norsk (LANG_NO = 3)
static int get_fuzzy_time_norwegian(int h, int next_h, int p, TimeLine lines[4])
{
	static const char *hours[] = {
		"", "ett", "to", "tre", "fire", "fem",
		"seks", "sju", "åtte", "ni", "ti", "elleve", "tolv"
	};

	switch (p)
	{
		case 0:
			lines[0] = { "klokken er", false };
			lines[1] = { hours[h], true };
			return 2;
		case 1:
			lines[0] = { "fem over", false };
			lines[1] = { hours[h], true };
			return 2;
		case 2:
			lines[0] = { "ti over", false };
			lines[1] = { hours[h], true };
			return 2;
		case 3:
			lines[0] = { "kvart over", false };
			lines[1] = { hours[h], true };
			return 2;
		case 4:
			lines[0] = { "ti på", false };
			lines[1] = { "halv", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 5:
			lines[0] = { "fem på", false };
			lines[1] = { "halv", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 6:
			lines[0] = { "halv", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 7:
			lines[0] = { "fem over", false };
			lines[1] = { "halv", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 8:
			lines[0] = { "ti over", false };
			lines[1] = { "halv", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 9:
			lines[0] = { "kvart på", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 10:
			lines[0] = { "ti på", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 11:
			lines[0] = { "fem på", false };
			lines[1] = { hours[next_h], true };
			return 2;
		default:
			return 0;
	}
}

// Nederlands (LANG_NL = 4)
static int get_fuzzy_time_dutch(int h, int next_h, int p, TimeLine lines[4])
{
	static const char *hours[] = {
		"", "een", "twee", "drie", "vier", "vijf",
		"zes", "zeven", "acht", "negen", "tien", "elf", "twaalf"
	};

	switch (p)
	{
		case 0:
			lines[0] = { hours[h], true };
			lines[1] = { "uur", false };
			return 2;
		case 1:
			lines[0] = { "vijf over", false };
			lines[1] = { hours[h], true };
			return 2;
		case 2:
			lines[0] = { "tien over", false };
			lines[1] = { hours[h], true };
			return 2;
		case 3:
			lines[0] = { "kwart over", false };
			lines[1] = { hours[h], true };
			return 2;
		case 4:
			lines[0] = { "tien voor", false };
			lines[1] = { "half", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 5:
			lines[0] = { "vijf voor", false };
			lines[1] = { "half", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 6:
			lines[0] = { "half", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 7:
			lines[0] = { "vijf over", false };
			lines[1] = { "half", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 8:
			lines[0] = { "tien over", false };
			lines[1] = { "half", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 9:
			lines[0] = { "kwart voor", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 10:
			lines[0] = { "tien voor", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 11:
			lines[0] = { "vijf voor", false };
			lines[1] = { hours[next_h], true };
			return 2;
		default:
			return 0;
	}
}

// Italiano (LANG_IT = 5)
static int get_fuzzy_time_italian(int h, int next_h, int p, TimeLine lines[4])
{
	static const char *hours[] = {
		"", "una", "due", "tre", "quattro", "cinque",
		"sei", "sette", "otto", "nove", "dieci", "undici", "dodici"
	};

	switch (p)
	{
		case 0:
			lines[0] = { hours[h], true };
			lines[1] = { "in punto", false };
			return 2;
		case 1:
			lines[0] = { hours[h], true };
			lines[1] = { "e cinque", false };
			return 2;
		case 2:
			lines[0] = { hours[h], true };
			lines[1] = { "e dieci", false };
			return 2;
		case 3:
			lines[0] = { hours[h], true };
			lines[1] = { "e un quarto", false };
			return 2;
		case 4:
			lines[0] = { hours[h], true };
			lines[1] = { "e venti", false };
			return 2;
		case 5:
			lines[0] = { hours[h], true };
			lines[1] = { "e venticinque", false };
			return 2;
		case 6:
			lines[0] = { hours[h], true };
			lines[1] = { "e mezza", false };
			return 2;
		case 7:
			lines[0] = { hours[h], true };
			lines[1] = { "e trentacinque", false };
			return 2;
		case 8:
			lines[0] = { hours[next_h], true };
			lines[1] = { "meno venti", false };
			return 2;
		case 9:
			lines[0] = { hours[next_h], true };
			lines[1] = { "meno un quarto", false };
			return 2;
		case 10:
			lines[0] = { hours[next_h], true };
			lines[1] = { "meno dieci", false };
			return 2;
		case 11:
			lines[0] = { hours[next_h], true };
			lines[1] = { "meno cinque", false };
			return 2;
		default:
			return 0;
	}
}

// Español (LANG_ES = 6)
static int get_fuzzy_time_spanish(int h, int next_h, int p, TimeLine lines[4])
{
	static const char *hours[] = {
		"", "una", "dos", "tres", "cuatro", "cinco",
		"seis", "siete", "ocho", "nueve", "diez", "once", "doce"
	};

	switch (p)
	{
		case 0:
			lines[0] = { hours[h], true };
			lines[1] = { "en punto", false };
			return 2;
		case 1:
			lines[0] = { hours[h], true };
			lines[1] = { "y cinco", false };
			return 2;
		case 2:
			lines[0] = { hours[h], true };
			lines[1] = { "y diez", false };
			return 2;
		case 3:
			lines[0] = { hours[h], true };
			lines[1] = { "y cuarto", false };
			return 2;
		case 4:
			lines[0] = { hours[h], true };
			lines[1] = { "y veinte", false };
			return 2;
		case 5:
			lines[0] = { hours[h], true };
			lines[1] = { "y veinticinco", false };
			return 2;
		case 6:
			lines[0] = { hours[h], true };
			lines[1] = { "y media", false };
			return 2;
		case 7:
			lines[0] = { hours[next_h], true };
			lines[1] = { "menos veinticinco", false };
			return 2;
		case 8:
			lines[0] = { hours[next_h], true };
			lines[1] = { "menos veinte", false };
			return 2;
		case 9:
			lines[0] = { hours[next_h], true };
			lines[1] = { "menos cuarto", false };
			return 2;
		case 10:
			lines[0] = { hours[next_h], true };
			lines[1] = { "menos diez", false };
			return 2;
		case 11:
			lines[0] = { hours[next_h], true };
			lines[1] = { "menos cinco", false };
			return 2;
		default:
			return 0;
	}
}

// Deutsch (O & W) (LANG_GE = 7, LANG_GW = 8)
static int get_fuzzy_time_german(int h, int next_h, int p, bool is_eastern, TimeLine lines[4])
{
	static const char *hours[] = {
		"", "eins", "zwei", "drei", "vier", "fünf",
		"sechs", "sieben", "acht", "neun", "zehn", "elf", "zwölf"
	};

	switch (p)
	{
		case 0:
			lines[0] = { (h == 1) ? "ein" : hours[h], true };
			lines[1] = { "Uhr", false };
			return 2;
		case 1:
			lines[0] = { "fünf nach", false };
			lines[1] = { hours[h], true };
			return 2;
		case 2:
			lines[0] = { "zehn nach", false };
			lines[1] = { hours[h], true };
			return 2;
		case 3:
			if (is_eastern)
			{
				lines[0] = { "viertel", false };
				lines[1] = { hours[next_h], true };
				return 2;
			}
			else
			{
				lines[0] = { "viertel nach", false };
				lines[1] = { hours[h], true };
				return 2;
			}
		case 4:
			lines[0] = { "zwanzig nach", false };
			lines[1] = { hours[h], true };
			return 2;
		case 5:
			lines[0] = { "fünf vor", false };
			lines[1] = { "halb", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 6:
			lines[0] = { "halb", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 7:
			lines[0] = { "fünf nach", false };
			lines[1] = { "halb", false };
			lines[2] = { hours[next_h], true };
			return 3;
		case 8:
			lines[0] = { "zwanzig vor", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 9:
			if (is_eastern)
			{
				lines[0] = { "dreiviertel", false };
				lines[1] = { hours[next_h], true };
				return 2;
			}
			else
			{
				lines[0] = { "viertel vor", false };
				lines[1] = { hours[next_h], true };
				return 2;
			}
		case 10:
			lines[0] = { "zehn vor", false };
			lines[1] = { hours[next_h], true };
			return 2;
		case 11:
			lines[0] = { "fünf vor", false };
			lines[1] = { hours[next_h], true };
			return 2;
		default:
			return 0;
	}
}

int get_fuzzy_time(uint8_t lang, int hour, int minute, TimeLine lines[4])
{
	int h, next_h;
	normalize_hours(hour, h, next_h);
	int p = (minute / 5) % 12;

	switch (lang)
	{
		case LANG_SE:
			return get_fuzzy_time_swedish(h, next_h, p, lines);
		case LANG_EN:
			return get_fuzzy_time_english(h, next_h, p, lines);
		case LANG_NO:
			return get_fuzzy_time_norwegian(h, next_h, p, lines);
		case LANG_NL:
			return get_fuzzy_time_dutch(h, next_h, p, lines);
		case LANG_IT:
			return get_fuzzy_time_italian(h, next_h, p, lines);
		case LANG_ES:
			return get_fuzzy_time_spanish(h, next_h, p, lines);
		case LANG_GE:
			return get_fuzzy_time_german(h, next_h, p, true, lines);
		case LANG_GW:
		default:
			return get_fuzzy_time_german(h, next_h, p, false, lines);
	}
}

const char *get_fuzzy_weekday(uint8_t lang, int wday)
{
	if (wday < 0 || wday > 6)
	{
		return "";
	}

	static const char *weekdays[][7] = {
		// [0] Dummy for index 0
		{ "", "", "", "", "", "", "" },
		// [1] Svenska
		{ "Söndag", "Måndag", "Tisdag", "Onsdag", "Torsdag", "Fredag", "Lördag" },
		// [2] English
		{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" },
		// [3] Norsk
		{ "Søndag", "Mandag", "Tirsdag", "Onsdag", "Torsdag", "Fredag", "Lørdag" },
		// [4] Nederlands
		{ "Zondag", "Maandag", "Dinsdag", "Woensdag", "Donderdag", "Vrijdag", "Zaterdag" },
		// [5] Italiano
		{ "Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato" },
		// [6] Español
		{ "Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado" },
		// [7] Deutsch (O)
		{ "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag" },
		// [8] Deutsch (w)
		{ "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag" }
	};

	uint8_t l = (lang >= LANG_SE && lang <= LANG_GW) ? lang : LANG_GW;
	return weekdays[l][wday];
}

const char *get_fuzzy_month(uint8_t lang, int mon)
{
	if (mon < 0 || mon > 11)
	{
		return "";
	}

	static const char *months[][12] = {
		// [0] Dummy
		{ "", "", "", "", "", "", "", "", "", "", "", "" },
		// [1] Svenska
		{ "januari", "februari", "mars", "april", "maj", "juni",
		  "juli", "augusti", "september", "oktober", "november", "december" },
		// [2] English
		{ "January", "February", "March", "April", "May", "June",
		  "July", "August", "September", "October", "November", "December" },
		// [3] Norsk
		{ "januar", "februar", "mars", "april", "mai", "juni",
		  "juli", "august", "september", "oktober", "november", "desember" },
		// [4] Nederlands
		{ "januari", "februari", "maart", "april", "mei", "juni",
		  "juli", "augustus", "september", "oktober", "november", "december" },
		// [5] Italiano
		{ "gennaio", "febbraio", "marzo", "aprile", "maggio", "giugno",
		  "luglio", "agosto", "settembre", "ottobre", "novembre", "dicembre" },
		// [6] Español
		{ "enero", "febrero", "marzo", "abril", "mayo", "junio",
		  "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre" },
		// [7] Deutsch (O)
		{ "Januar", "Februar", "März", "April", "Mai", "Juni",
		  "Juli", "August", "September", "Oktober", "November", "Dezember" },
		// [8] Deutsch (w)
		{ "Januar", "Februar", "März", "April", "Mai", "Juni",
		  "Juli", "August", "September", "Oktober", "November", "Dezember" }
	};

	uint8_t l = (lang >= LANG_SE && lang <= LANG_GW) ? lang : LANG_GW;
	return months[l][mon];
}

int get_connection_lost_lines(uint8_t lang, TimeLine lines[4])
{
	switch (lang)
	{
		case LANG_SE:
			lines[0] = { "Var är", false };
			lines[1] = { "din", false };
			lines[2] = { "telefon?", true };
			return 3;
		case LANG_EN:
			lines[0] = { "Where is", false };
			lines[1] = { "your", false };
			lines[2] = { "phone?", true };
			return 3;
		case LANG_NO:
			lines[0] = { "Hvor er", false };
			lines[1] = { "din", false };
			lines[2] = { "telefon?", true };
			return 3;
		case LANG_NL:
			lines[0] = { "Waar is", false };
			lines[1] = { "je", false };
			lines[2] = { "telefoon?", true };
			return 3;
		case LANG_IT:
			lines[0] = { "Dov'è", false };
			lines[1] = { "il tuo", false };
			lines[2] = { "telefono?", true };
			return 3;
		case LANG_ES:
			lines[0] = { "¿Dónde", false };
			lines[1] = { "está tu", false };
			lines[2] = { "teléfono?", true };
			return 3;
		case LANG_GE:
		case LANG_GW:
		default:
			lines[0] = { "Wo ist", false };
			lines[1] = { "dein", false };
			lines[2] = { "Handy?", true };
			return 3;
	}
}
