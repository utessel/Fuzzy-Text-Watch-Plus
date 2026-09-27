#include "num2words.h"
#include "string.h"
#include <stdio.h>

#include "lang-swedish.h"
#include "lang-english.h"
#include "lang-norwegian.h"
#include "lang-dutch.h"
#include "lang-italian.h"
#include "lang-spanish.h"
#include "lang-german-eastern.h"
#include "lang-german-western.h"

static const Language* language = &LANG_GERMAN_W;

void set_language(uint8_t lang) {
  switch (lang) {
    case LANG_EN:
      language = &LANG_ENGLISH;
      break;

    case LANG_SE:
      language = &LANG_SWEDISH;
      break;

    case LANG_NO:
      language = &LANG_NORWEGIAN;
      break;

    case LANG_NL:
      language = &LANG_DUTCH;
      break;

    case LANG_IT:
      language = &LANG_ITALIAN;
      break;

    case LANG_ES:
      language = &LANG_SPANISH;
      break;

    case LANG_GE:
      language = &LANG_GERMAN_E;
      break;

    case LANG_GW:
      language = &LANG_GERMAN_W;
      break;

    default:
      language = &LANG_GERMAN_W;
  }
}

const char* getHourWord(int hour) {
  int pos = (hour + 11) % 12; // + 11 instead of - 1 to avoid negative result
  return language->hours[pos];
}

const char* getFiveMinutePhrase(int fiveMinutePeriod) {
  return language->phrases[fiveMinutePeriod];
}


void check_exceptions(int hours, int pentaminutes, char* words, size_t length) {
  int i;
  for (i = 0; i < language->number_of_exceptions; i++) {
    if (hours == language->exceptions[i].hours) {
      if (pentaminutes == language->exceptions[i].pentaminutes) {
        strncpy(words, language->exceptions[i].phrase, length);
        return;
      }
    }
  }
}

void time_to_words(int hours, int minutes, char* words, size_t length) {

  memset(words, 0, length);

  // Fuzzy time
  int fiveMinutePeriod = (minutes / 5) % 12;

  check_exceptions(hours, fiveMinutePeriod, words, length);

  if (*words == '\0') {
    // No exception
    char phrase[length]; 
    strcpy(phrase, getFiveMinutePhrase(fiveMinutePeriod));
   
    char *variable = NULL;
    const char *hour;

    if ((variable = strstr(phrase, "$1")) != NULL) {
      hour = getHourWord(hours);
    }
    else if ((variable = strstr(phrase, "$2")) != NULL) {
      hour = getHourWord(hours + 1);
    }

    if (variable != NULL) {
      // Substitute '$x' with '%s'
      *variable = '%';
      *(variable + 1) = 's';
      snprintf(words, length, phrase, hour);
    } else {
      strncpy(words, phrase, length);
    }
  }
}

void time_to_greeting(int hour, char* greeting)
{
  int pos;

  if (hour < 6) {
    pos = 3; // night greeting
  } else if (hour < 12) {
    pos = 0; // morning greeting
  } else if (hour < 18) {
    pos = 1; // day greeting
  } else {
    pos = 2; // evening greeting
  }

  strcpy(greeting, language->greetings[pos]);
}

void get_connection_lost_message(char* message)
{
  strcpy(message, language->connection_lost);
}
