#pragma once

#include <pebble.h>

// Make watch switch time every 5 seconds
#define DEBUG 0

// Data keys
#define KEY_INVERSE 0
#define KEY_BACKGROUND 1
#define KEY_REGULAR_TEXT 2
#define KEY_BOLD_TEXT 3
#define KEY_LANGUAGE 4
#define KEY_OFFSET 5
#define KEY_MESSAGE_TIME 6
#define KEY_GESTURE 7
#define KEY_BT_NOTIFICATION 8
#define KEY_ANIMATION_DURATION 9

// Max number of lines
#define NUM_LINES 4
// Size of text buffer for lines
#define BUFFER_SIZE 32

// How long to show messages, in seconds
#define BT_LOST_DISPLAY_TIME 7

// How long to wait in seconds between connection lost notification and displaying message
#define CONNECTION_LOST_MARGIN 2

// Gestures
#define GESTURE_OFF  0
#define GESTURE_X    1
#define GESTURE_Y    2
#define GESTURE_Z    3
#define GESTURE_ANY  4

#define BT_NOTIFY_OFF      0
#define BT_NOTIFY_NO_VIBE  1
#define BT_NOTIFY_ON       2

// Functions
int configureLayersForText(char text[NUM_LINES][BUFFER_SIZE], char format[], float scales[NUM_LINES]);
void display_date_time();
void display_time(struct tm *t, bool force);
void check_connection(time_t *now);
void handle_tick(struct tm *tick_time, TimeUnits units_changed);
struct tm *get_localtime();
void refresh_time();
void set_offset(int offset);
void set_animation_duration(uint32_t duration_ms);
void inbox_received_handler(DictionaryIterator *iter, void *context);
void notify_bt_lost();
void bt_handler(bool connected);
void readPersistedState();
void handle_init();
void handle_deinit();
