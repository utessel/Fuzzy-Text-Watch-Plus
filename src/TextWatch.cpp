extern "C" {
#include <pebble.h>
#ifdef PBL_COLOR
  #include "gcolor_definitions.h" // Allows the use of color
#endif
#include "TextWatch.h"
}

#include "Matrix2D.hpp"
#include "TextLayout.hpp"
#include "SdfRenderer.hpp"
#include "sdf_font.h"
#include "FuzzyTime.hpp"
#include <algorithm>
#include <cmath>

Window *window;

int currentNLines;

// Current colors
GColor8 backgroundColor;
GColor8 regularTextColor;
GColor8 boldTextColor;

// Time in seconds to add to the current time before calculating which
// 5-minute period we should display
int timeOffset;

// Variable to keep track of the last minute that we updated the time
// Used to optimise so we only need to run time logic once per minute.
int lastMinute = -1;

// Time in seconds since epoch when a displayed message should be removed.
// Only set to non zero when a message is displaying.
time_t resetMessageTime = 0;

// Time in seconds since epoch when connection lost message will be displayed,
// if connection is still lost... (attempt to reduce false notifications)
time_t connectionLostTime = 0;

// Which gesture to activate date screen
// 0 = off
// 1 = Boxing move (X-axis)
// 2 = flick wrist (Y-axis)
// 3 = Shake up/down (Z-axis)
// 4 = Any shake
int dateGesture = GESTURE_ANY;

// Notify when BT connection is lost?
// 0 = off
// 1 = text and light only
// 2 = on
int bt_lost_notification = BT_NOTIFY_ON;

// Screen resolution. Set in the init function.
int16_t xres;
int16_t yres;



// Animation duration in ms: 700ms provides crisp, smooth transitions (configurable)
#define DEFAULT_ANIMATION_DURATION_MS 700
static uint32_t s_animation_duration_ms = DEFAULT_ANIMATION_DURATION_MS;

void set_animation_duration(uint32_t duration_ms)
{
	s_animation_duration_ms = duration_ms;
}

struct LineState
{
	char text[BUFFER_SIZE];
	bool is_bold;
	float scale;
	float target_x;
	float target_y;
	bool active;
};

enum AnimType
{
	ANIM_SLIDE_AND_SCALE,
	ANIM_ZOOM_OUT,
	ANIM_ZOOM_IN
};

struct TransitionItem
{
	char text[BUFFER_SIZE];
	bool is_bold;
	AnimType type;
	float scale_from;
	float scale_to;
	float x_from;
	float x_to;
	float y_from;
	float y_to;
};

static LineState s_current_lines[NUM_LINES];
static LineState s_target_lines[NUM_LINES];
static TransitionItem s_transitions[NUM_LINES * 2];
static int s_num_transitions = 0;
static bool s_has_initial_display = false;
static bool s_is_animating = false;
static float s_anim_progress = 1.0f;
static Animation *s_transition_anim = nullptr;
static Layer *s_canvas_layer = nullptr;

static void render_text_item(uint8_t *fb_data, int row_bytes, const char *text, bool is_bold, float cx, float cy, float scale, float rot_deg = 0.0f)
{
	if (!text || text[0] == '\0' || scale < 0.05f)
	{
		return;
	}

	TextLayout layout;
	layout.Layout(text, &sdf_font);
	if (layout.glyph_count <= 0)
	{
		return;
	}

	float weight_bias = is_bold ? 0.70f : 0.25f;
	GColor8 text_color = is_bold ? boldTextColor : regularTextColor;

	Matrix2D M = Matrix2D::Translation(cx, cy)
	           * Matrix2D::RotationDeg(rot_deg)
	           * Matrix2D::UniformScaling(scale)
	           * Matrix2D::Translation(-layout.center_x, -layout.center_y);

	SdfRenderer::DrawToFramebuffer8Bit(
		fb_data,
		xres,
		yres,
		row_bytes,
		layout,
		M,
		1.0f,
		weight_bias,
		text_color.argb,
		backgroundColor.argb
	);
}

static void canvas_layer_update_proc(Layer *layer, GContext *ctx)
{
	GBitmap *fb = graphics_capture_frame_buffer(ctx);
	if (!fb)
	{
		return;
	}

	uint8_t *fb_data = gbitmap_get_data(fb);
	int row_bytes = gbitmap_get_bytes_per_row(fb);

	// Clear full screen with background color
	memset(fb_data, backgroundColor.argb, row_bytes * yres);

	float t = s_anim_progress;

	if (!s_is_animating || t >= 1.0f)
	{
		for (int i = 0; i < NUM_LINES; i++)
		{
			if (s_current_lines[i].active && s_current_lines[i].text[0] != '\0')
			{
				render_text_item(fb_data, row_bytes, s_current_lines[i].text, s_current_lines[i].is_bold,
				                 s_current_lines[i].target_x, s_current_lines[i].target_y, s_current_lines[i].scale, 0.0f);
			}
		}
	}
	else
	{
		for (int i = 0; i < s_num_transitions; i++)
		{
			const TransitionItem &item = s_transitions[i];
			float cur_scale, cur_x, cur_y;

			if (item.type == ANIM_SLIDE_AND_SCALE)
			{
				cur_scale = item.scale_from + (item.scale_to - item.scale_from) * t;
				cur_x = item.x_from + (item.x_to - item.x_from) * t;
				cur_y = item.y_from + (item.y_to - item.y_from) * t;
				render_text_item(fb_data, row_bytes, item.text, item.is_bold, cur_x, cur_y, cur_scale, 0.0f);
			}
			else if (item.type == ANIM_ZOOM_OUT)
			{
				if (t < 0.6f)
				{
					float p = t / 0.6f;
					cur_scale = item.scale_from + (item.scale_to - item.scale_from) * p;
					cur_x = item.x_from + (item.x_to - item.x_from) * p;
					cur_y = item.y_from;
					float rot = -90.0f * p;
					render_text_item(fb_data, row_bytes, item.text, item.is_bold, cur_x, cur_y, cur_scale, rot);
				}
			}
			else if (item.type == ANIM_ZOOM_IN)
			{
				if (t > 0.4f)
				{
					float p = (t - 0.4f) / 0.6f;
					cur_scale = item.scale_from + (item.scale_to - item.scale_from) * p;
					cur_x = item.x_from + (item.x_to - item.x_from) * p;
					cur_y = item.y_to;
					float rot = 90.0f * (1.0f - p);
					render_text_item(fb_data, row_bytes, item.text, item.is_bold, cur_x, cur_y, cur_scale, rot);
				}
			}
		}
	}

	graphics_release_frame_buffer(ctx, fb);
}

static void transition_anim_update(Animation *anim, const AnimationProgress progress)
{
	s_anim_progress = static_cast<float>(progress) / static_cast<float>(ANIMATION_NORMALIZED_MAX);
	if (s_canvas_layer)
	{
		layer_mark_dirty(s_canvas_layer);
	}
}

static void transition_anim_teardown(Animation *anim)
{
	s_is_animating = false;
	s_anim_progress = 1.0f;
	for (int i = 0; i < NUM_LINES; i++)
	{
		s_current_lines[i] = s_target_lines[i];
	}
	s_transition_anim = nullptr;
	if (s_canvas_layer)
	{
		layer_mark_dirty(s_canvas_layer);
	}
}

static void start_transition()
{
	s_num_transitions = 0;
	bool current_matched[NUM_LINES] = {false};

	for (int j = 0; j < NUM_LINES; j++)
	{
		if (!s_target_lines[j].active)
		{
			continue;
		}

		int match_idx = -1;
		for (int k = 0; k < NUM_LINES; k++)
		{
			if (s_current_lines[k].active && !current_matched[k])
			{
				if (strcmp(s_current_lines[k].text, s_target_lines[j].text) == 0)
				{
					match_idx = k;
					break;
				}
			}
		}

		if (match_idx >= 0)
		{
			current_matched[match_idx] = true;
			TransitionItem &it = s_transitions[s_num_transitions++];
			strncpy(it.text, s_target_lines[j].text, sizeof(it.text) - 1);
			it.text[sizeof(it.text) - 1] = '\0';
			it.is_bold = s_target_lines[j].is_bold;
			it.type = ANIM_SLIDE_AND_SCALE;
			it.scale_from = s_current_lines[match_idx].scale;
			it.scale_to = s_target_lines[j].scale;
			it.x_from = s_current_lines[match_idx].target_x;
			it.x_to = s_target_lines[j].target_x;
			it.y_from = s_current_lines[match_idx].target_y;
			it.y_to = s_target_lines[j].target_y;
		}
		else
		{
			TransitionItem &it = s_transitions[s_num_transitions++];
			strncpy(it.text, s_target_lines[j].text, sizeof(it.text) - 1);
			it.text[sizeof(it.text) - 1] = '\0';
			it.is_bold = s_target_lines[j].is_bold;
			it.type = ANIM_ZOOM_IN;
			it.scale_from = s_target_lines[j].scale * 0.10f;
			it.scale_to = s_target_lines[j].scale;
			it.x_from = s_target_lines[j].target_x + static_cast<float>(xres) * 0.70f;
			it.x_to = s_target_lines[j].target_x;
			it.y_from = s_target_lines[j].target_y;
			it.y_to = s_target_lines[j].target_y;
		}
	}

	for (int k = 0; k < NUM_LINES; k++)
	{
		if (s_current_lines[k].active && !current_matched[k])
		{
			TransitionItem &it = s_transitions[s_num_transitions++];
			strncpy(it.text, s_current_lines[k].text, sizeof(it.text) - 1);
			it.text[sizeof(it.text) - 1] = '\0';
			it.is_bold = s_current_lines[k].is_bold;
			it.type = ANIM_ZOOM_OUT;
			it.scale_from = s_current_lines[k].scale;
			it.scale_to = s_current_lines[k].scale * 0.10f;
			it.x_from = s_current_lines[k].target_x;
			it.x_to = s_current_lines[k].target_x - static_cast<float>(xres) * 0.70f;
			it.y_from = s_current_lines[k].target_y;
			it.y_to = s_current_lines[k].target_y;
		}
	}

	if (s_transition_anim)
	{
		animation_unschedule(s_transition_anim);
		animation_destroy(s_transition_anim);
		s_transition_anim = nullptr;
	}

	static const AnimationImplementation anim_impl = {
		.setup = NULL,
		.update = transition_anim_update,
		.teardown = transition_anim_teardown
	};

	s_transition_anim = animation_create();
	animation_set_implementation(s_transition_anim, &anim_impl);
	animation_set_duration(s_transition_anim, s_animation_duration_ms);
	animation_set_curve(s_transition_anim, AnimationCurveEaseInOut);

	s_anim_progress = 0.0f;
	s_is_animating = true;
	animation_schedule(s_transition_anim);
}

static void apply_target_lines(bool force)
{
	if (!s_has_initial_display)
	{
		for (int i = 0; i < NUM_LINES; i++)
		{
			s_current_lines[i] = s_target_lines[i];
		}
		s_has_initial_display = true;
		s_is_animating = false;
		s_anim_progress = 1.0f;
		if (s_canvas_layer)
		{
			layer_mark_dirty(s_canvas_layer);
		}
		return;
	}

	bool changed = false;
	for (int i = 0; i < NUM_LINES; i++)
	{
		if (s_current_lines[i].active != s_target_lines[i].active ||
		    strcmp(s_current_lines[i].text, s_target_lines[i].text) != 0 ||
		    std::abs(s_current_lines[i].scale - s_target_lines[i].scale) > 0.05f ||
		    std::abs(s_current_lines[i].target_y - s_target_lines[i].target_y) > 2.0f)
		{
			changed = true;
			break;
		}
	}

	if (changed || force)
	{
		start_transition();
	}
}

// Configure the layers for the given text with dynamic per-line scaling
int configureLayersForText(char text[NUM_LINES][BUFFER_SIZE], char format[], float scales[NUM_LINES])
{
	int numLines = 0;
	for (int i = 0; i < NUM_LINES; i++)
	{
		if (text[i][0] == '\0')
		{
			break;
		}
		numLines++;
	}

	if (numLines == 0)
	{
		return 0;
	}

	float widths[NUM_LINES];
	bool is_bolds[NUM_LINES];
	float max_reg_w = 0.0f;

	for (int i = 0; i < numLines; i++)
	{
		TextLayout::TextExtent ext = TextLayout::GetExtent(text[i], &sdf_font);
		widths[i] = ext.width;
		is_bolds[i] = (format[i] == 'B' || format[i] == 'b');
		if (!is_bolds[i] && widths[i] > max_reg_w)
		{
			max_reg_w = widths[i];
		}
	}

	float max_w = static_cast<float>(xres) - ((xres > 180) ? 20.0f : 14.0f);
	float max_h = static_cast<float>(yres) - ((yres > 200) ? 16.0f : 12.0f);

	// Determine bold scale (maximally fills width)
	float bold_scale = 3.0f;
	for (int i = 0; i < numLines; i++)
	{
		if (is_bolds[i] && widths[i] > 0.0f)
		{
			bold_scale = max_w / widths[i];
		}
	}

	// Regular text scales to widest regular line, but is capped at 70% of bold (approx. 30% smaller)
	float scale_reg = (max_reg_w > 0.0f) ? (max_w / max_reg_w) : (bold_scale * 0.70f);
	if (scale_reg > bold_scale * 0.70f)
	{
		scale_reg = bold_scale * 0.70f;
	}

	for (int i = 0; i < numLines; i++)
	{
		if (is_bolds[i])
		{
			scales[i] = bold_scale;
		}
		else
		{
			scales[i] = scale_reg;
		}
	}

	// Calculate vertical positioning based on line scales (16px base font)
	float font_em = static_cast<float>(sdf_font.em_height); // 16.0f
	int offsets[NUM_LINES];
	int16_t row_heights[NUM_LINES];
	float total_height = 0.0f;

	for (int i = 0; i < numLines; i++)
	{
		float step = font_em * scales[i] * 0.95f + ((numLines >= 4) ? 2.0f : 4.0f);
		float h = font_em * scales[i] * 1.15f;
		total_height += (i < numLines - 1) ? step : h;
	}

	// If total height exceeds max_h, scale down proportionally
	if (total_height > max_h && total_height > 0.0f)
	{
		float shrink = max_h / total_height;
		total_height = 0.0f;
		for (int i = 0; i < numLines; i++)
		{
			scales[i] *= shrink;
			float step = font_em * scales[i] * 0.95f + ((numLines >= 4) ? 2.0f : 4.0f);
			float h = font_em * scales[i] * 1.15f;
			total_height += (i < numLines - 1) ? step : h;
		}
	}

	for (int i = 0; i < numLines; i++)
	{
		row_heights[i] = static_cast<int16_t>(font_em * scales[i] * 1.15f);
		if (row_heights[i] < 28)
		{
			row_heights[i] = 28;
		}
		offsets[i] = static_cast<int>(font_em * scales[i] * 0.95f + ((numLines >= 4) ? 2.0f : 4.0f));
	}

	int16_t ypos = (yres - static_cast<int>(total_height)) / 2;
	if (ypos < 2)
	{
		ypos = 2;
	}

	for (int i = 0; i < NUM_LINES; i++)
	{
		if (i < numLines)
		{
			s_target_lines[i].active = true;
			strncpy(s_target_lines[i].text, text[i], sizeof(s_target_lines[i].text) - 1);
			s_target_lines[i].text[sizeof(s_target_lines[i].text) - 1] = '\0';
			s_target_lines[i].is_bold = (format[i] == 'B' || format[i] == 'b');
			s_target_lines[i].scale = scales[i];
			s_target_lines[i].target_x = static_cast<float>(xres) * 0.5f;
			s_target_lines[i].target_y = static_cast<float>(ypos) + static_cast<float>(row_heights[i]) * 0.5f;
			ypos += offsets[i];
		}
		else
		{
			s_target_lines[i].active = false;
			s_target_lines[i].text[0] = '\0';
		}
	}

	return numLines;
}


// Update screen based on new time
void display_time(struct tm *t, bool force)
{
	if (resetMessageTime != 0) { // Don't update time if a message is showing
		return;
	}

	time_t timestamp = mktime(t);
	timestamp += timeOffset; // Add offset time
	t = localtime(&timestamp);

#if DEBUG == 0
	if (lastMinute == t->tm_min && !force) { // No change in time
		return;
	}
#endif

	// Mark this minute as checked;
	lastMinute = t->tm_min;

	// The current time text will be stored in the following strings
	char textLine[NUM_LINES][BUFFER_SIZE];
	char format[NUM_LINES];

	TimeLine tl[NUM_LINES];
#if DEBUG == 1
	int nLines = get_fuzzy_time(get_language(), t->tm_hour, t->tm_sec, tl);
#else
	int nLines = get_fuzzy_time(get_language(), t->tm_hour, t->tm_min, tl);
#endif
	for (int i = 0; i < NUM_LINES; i++)
	{
		if (i < nLines)
		{
			strncpy(textLine[i], tl[i].text, sizeof(textLine[i]) - 1);
			textLine[i][sizeof(textLine[i]) - 1] = '\0';
			format[i] = tl[i].is_bold ? 'B' : ' ';
		}
		else
		{
			textLine[i][0] = '\0';
			format[i] = ' ';
		}
	}
	
	float scales[NUM_LINES];
	currentNLines = configureLayersForText(textLine, format, scales);
	apply_target_lines(force);
}

#define DEMO_MODE_10S 0

#if DEMO_MODE_10S
static int s_sim_hour = 4;
static int s_sim_min = 20;
#endif

void display_date_time()
{
	time_t now = time(nullptr);
	static time_t s_last_show_time = 0;

	if (resetMessageTime != 0)
	{
		// Debounce: do not dismiss within 2 seconds of showing
		if (now - s_last_show_time < 2)
		{
			return;
		}

		// Toggle back to fuzzy time immediately with smooth animation
		resetMessageTime = 0;
		lastMinute = -1;
#if DEMO_MODE_10S
		struct tm sim_tm = *localtime(&now);
		sim_tm.tm_hour = s_sim_hour;
		sim_tm.tm_min = s_sim_min;
		display_time(&sim_tm, false);
#else
		display_time(localtime(&now), false);
#endif
		return;
	}

	s_last_show_time = now;

	struct tm *t = localtime(&now);

	char textLine[NUM_LINES][BUFFER_SIZE];
	char format[NUM_LINES];

#if DEMO_MODE_10S
	int hour = s_sim_hour;
	int min = s_sim_min;
#else
	int hour = t->tm_hour;
	int min = t->tm_min;
#endif

	// Line 0: Exact Time "04:20" (Bold)
	snprintf(textLine[0], sizeof(textLine[0]), "%02d:%02d", hour, min);
	format[0] = 'B';

	uint8_t lang = get_language();

	// Line 1: Weekday (Regular)
	const char *wday = get_fuzzy_weekday(lang, t->tm_wday);
	snprintf(textLine[1], sizeof(textLine[1]), "%s", wday);
	format[1] = ' ';

	// Line 2: Date (Regular)
	const char *mon = get_fuzzy_month(lang, t->tm_mon);
	if (lang == LANG_EN)
	{
		snprintf(textLine[2], sizeof(textLine[2]), "%s %d", mon, t->tm_mday);
	}
	else if (lang == LANG_ES)
	{
		snprintf(textLine[2], sizeof(textLine[2]), "%d de %s", t->tm_mday, mon);
	}
	else
	{
		snprintf(textLine[2], sizeof(textLine[2]), "%d. %s", t->tm_mday, mon);
	}
	format[2] = ' ';

	for (int i = 3; i < NUM_LINES; i++)
	{
		textLine[i][0] = '\0';
		format[i] = ' ';
	}

	float scales[NUM_LINES];
	currentNLines = configureLayersForText(textLine, format, scales);
	apply_target_lines(false);

	lastMinute = -1;
	resetMessageTime = now + 7;
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction)
{
	(void)axis;
	(void)direction;
	display_date_time();
}

void check_connection(time_t *now)
{
	if (connectionLostTime > 0 && connectionLostTime <= *now)
	{
		if (!connection_service_peek_pebble_app_connection())
		{
			notify_bt_lost();
		}
		connectionLostTime = 0;
	}
}

// Time handler called every second by the system
void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
	time_t now;
	time(&now);
	if (resetMessageTime != 0)
	{
		if (now >= resetMessageTime)
		{
			resetMessageTime = 0;
			lastMinute = -1;
#if DEMO_MODE_10S
			s_sim_min += 5;
			if (s_sim_min >= 60)
			{
				s_sim_min = 0;
				s_sim_hour = (s_sim_hour % 12) + 1;
			}
			struct tm sim_tm = *tick_time;
			sim_tm.tm_hour = s_sim_hour;
			sim_tm.tm_min = s_sim_min;
			display_time(&sim_tm, false);
#else
			display_time(tick_time, false);
#endif
		}
		return;
	}

	check_connection(&now);

#if DEMO_MODE_10S
	static int s_demo_sec = 0;

	s_demo_sec++;
	if (s_demo_sec >= 6)
	{
		s_demo_sec = 0;
		display_date_time();
	}
#else
	display_time(tick_time, false);
#endif
}


struct tm *get_localtime()
{
	time_t raw_time;
	time(&raw_time);
	return localtime(&raw_time);
}

void refresh_time() {
	display_time(get_localtime(), true);
}

void set_offset(int offset) {
	timeOffset = offset;
}

void set_gesture(int gesture) {
	dateGesture = gesture;
	accel_tap_service_unsubscribe();
	if (gesture != GESTURE_OFF) {
		accel_tap_service_subscribe(accel_tap_handler);
	}
}

void set_bt_lost_notification(int bt_notification) {
	bt_lost_notification = bt_notification;
}

void inbox_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received inbox message");

  // Language
  Tuple *language_t = dict_find(iter, KEY_LANGUAGE);
  if (language_t) {
  	set_language(language_t->value->uint8);
  	persist_write_int(KEY_LANGUAGE, language_t->value->uint8);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Language is %d", language_t->value->uint8);
  	lastMinute = -1;
  }

  // Time offset
  Tuple *offset_t = dict_find(iter, KEY_OFFSET);
  if (offset_t) {
  	set_offset(offset_t->value->uint16);
  	persist_write_int(KEY_OFFSET, offset_t->value->uint16);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Offset is %d", offset_t->value->uint16);
  	lastMinute = -1;
  }

  // Gesture
  Tuple *gesture_t = dict_find(iter, KEY_GESTURE);
  if (gesture_t) {
  	set_gesture(gesture_t->value->uint8);
  	persist_write_int(KEY_GESTURE, gesture_t->value->uint8);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Gesture is %d", gesture_t->value->uint8);
  }

  // BT lost notification
  Tuple *bt_notification_t = dict_find(iter, KEY_BT_NOTIFICATION);
  if (bt_notification_t) {
  	set_bt_lost_notification(bt_notification_t->value->uint8);
  	persist_write_int(KEY_BT_NOTIFICATION, bt_notification_t->value->uint8);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "BT notification is %d", bt_notification_t->value->uint8);
  }

  // Animation duration
  Tuple *anim_duration_t = dict_find(iter, KEY_ANIMATION_DURATION);
  if (anim_duration_t) {
  	set_animation_duration(anim_duration_t->value->uint16);
  	persist_write_int(KEY_ANIMATION_DURATION, anim_duration_t->value->uint16);
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Animation duration is %d ms", (int)anim_duration_t->value->uint16);
  }

#ifdef PBL_COLOR
  // Background color
  Tuple *background_color_t = dict_find(iter, KEY_BACKGROUND);
  if(background_color_t) {
  	backgroundColor.argb = background_color_t->value->uint8;
  	window_set_background_color(window, backgroundColor);
  	persist_write_int(KEY_BACKGROUND, backgroundColor.argb);	
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Background color is 0x%02X", backgroundColor.argb);
  }

  // Regular text color
  Tuple *regular_text_t = dict_find(iter, KEY_REGULAR_TEXT);
  if(regular_text_t) {
  	regularTextColor.argb = regular_text_t->value->uint8;
  	persist_write_int(KEY_REGULAR_TEXT, regularTextColor.argb);
  }

  // Bold text color
  Tuple *bold_text_t = dict_find(iter, KEY_BOLD_TEXT);
  if(bold_text_t) {
  	boldTextColor.argb = bold_text_t->value->uint8;
  	persist_write_int(KEY_BOLD_TEXT, boldTextColor.argb);
  }

#else
  // Inverse colors
  Tuple *color_inverse_t = dict_find(iter, KEY_INVERSE);
  if(color_inverse_t) {
  	APP_LOG(APP_LOG_LEVEL_DEBUG, "Inverse colors is %d", color_inverse_t->value->int8);
  	if (color_inverse_t->value->int8 > 0) {  // Read boolean as an integer
	    // Set inverse colors
	    window_set_background_color(window, GColorWhite);
	    regularTextColor.argb = GColorBlack.argb;
	    boldTextColor.argb = GColorBlack.argb;
	    // Persist value
	    persist_write_bool(KEY_INVERSE, true);
	} else {
	    // Set normal colors
	    window_set_background_color(window, GColorBlack);
	    regularTextColor.argb = GColorWhite.argb;
	    boldTextColor.argb = GColorWhite.argb;
	    // Persist value
	    persist_write_bool(KEY_INVERSE, false);
	}
  }

#endif

  refresh_time();
}

void notify_bt_lost()
{
	if (bt_lost_notification != BT_NOTIFY_OFF)
	{
		if (bt_lost_notification == BT_NOTIFY_ON && !quiet_time_is_active())
		{
			vibes_long_pulse();
		}
		light_enable_interaction();

		TimeLine tl[NUM_LINES];
		int nLines = get_connection_lost_lines(get_language(), tl);
		char textLine[NUM_LINES][BUFFER_SIZE];
		char format[NUM_LINES];
		for (int i = 0; i < NUM_LINES; i++)
		{
			if (i < nLines)
			{
				strncpy(textLine[i], tl[i].text, sizeof(textLine[i]) - 1);
				textLine[i][sizeof(textLine[i]) - 1] = '\0';
				format[i] = tl[i].is_bold ? 'B' : ' ';
			}
			else
			{
				textLine[i][0] = '\0';
				format[i] = ' ';
			}
		}

		float scales[NUM_LINES];
		currentNLines = configureLayersForText(textLine, format, scales);
		apply_target_lines(true);

		time_t now = time(nullptr);
		resetMessageTime = now + BT_LOST_DISPLAY_TIME;
	}
}

void bt_handler(bool connected)
{
	if (connected)
	{
		connectionLostTime = 0;
	}
	else
	{
		time_t now = time(nullptr);
		connectionLostTime = now + CONNECTION_LOST_MARGIN;
	}
}

void readPersistedState()
{
	if (persist_exists(KEY_LANGUAGE))
	{
		set_language(persist_read_int(KEY_LANGUAGE));
	}

	if (persist_exists(KEY_OFFSET))
	{
		set_offset(persist_read_int(KEY_OFFSET));
	}

	if (persist_exists(KEY_GESTURE))
	{
		set_gesture(persist_read_int(KEY_GESTURE));
	}

	if (persist_exists(KEY_BT_NOTIFICATION))
	{
		set_bt_lost_notification(persist_read_int(KEY_BT_NOTIFICATION));
	}

	if (persist_exists(KEY_ANIMATION_DURATION))
	{
		set_animation_duration(persist_read_int(KEY_ANIMATION_DURATION));
	}

	// Set default colors
	backgroundColor.argb = GColorBlack.argb;
	regularTextColor.argb = GColorWhite.argb;
	boldTextColor.argb = GColorWhite.argb;

#ifdef PBL_COLOR
	if (persist_exists(KEY_BACKGROUND))
	{
		backgroundColor.argb = persist_read_int(KEY_BACKGROUND);
	}
	if (persist_exists(KEY_REGULAR_TEXT))
	{
		regularTextColor.argb = persist_read_int(KEY_REGULAR_TEXT);
	}
	if (persist_exists(KEY_BOLD_TEXT))
	{
		boldTextColor.argb = persist_read_int(KEY_BOLD_TEXT);
	}
#else
	if (persist_read_bool(KEY_INVERSE))
	{
		backgroundColor.argb = GColorWhite.argb;
		regularTextColor.argb = GColorBlack.argb;
		boldTextColor.argb = GColorBlack.argb;
	}
#endif

	// Set background color
	window_set_background_color(window, backgroundColor);
}

void handle_init()
{
	window = window_create();
	window_stack_push(window, true);

	Layer *window_layer = window_get_root_layer(window);
	GRect window_bounds = layer_get_bounds(window_layer);
	xres = window_bounds.size.w;
	yres = window_bounds.size.h;

	// Subscribe to taps
	accel_tap_service_subscribe(accel_tap_handler);

	readPersistedState();

	// Canvas layer for SDF rendering & transitions
	s_canvas_layer = layer_create(window_bounds);
	layer_set_update_proc(s_canvas_layer, canvas_layer_update_proc);
	layer_add_child(window_layer, s_canvas_layer);

	refresh_time();

	// Subscribe to ticks
	tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

	// Subscribe to bluetooth events
	connection_service_subscribe((ConnectionHandlers) {
	  .pebble_app_connection_handler = bt_handler,
	  .pebblekit_connection_handler = NULL
	});

	// Set up listener for configuration changes
	app_message_register_inbox_received(inbox_received_handler);
  	AppMessageResult result = app_message_open(512, 512);
  	if (result != APP_MSG_OK)
	{
  		APP_LOG(APP_LOG_LEVEL_WARNING, "app_message_open() failed with error %d", result);
  	}
}

void handle_deinit()
{
	accel_tap_service_unsubscribe();

	if (s_transition_anim)
	{
		animation_unschedule(s_transition_anim);
		animation_destroy(s_transition_anim);
		s_transition_anim = nullptr;
	}

	if (s_canvas_layer)
	{
		layer_destroy(s_canvas_layer);
		s_canvas_layer = nullptr;
	}

	// Free window
	window_destroy(window);
}

int main(void)
{
	handle_init();
	app_event_loop();
	handle_deinit();
}

