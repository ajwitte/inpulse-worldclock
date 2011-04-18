#include <pulse_os.h>
#include <pulse_types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "globe.h"
#include "timezone.h"
#include "app_resources.h"

#define TIME_BEFORE_SLEEP 15000

int target_timezone = LOCAL_TIMEZONE;
int longitude = 0;
int32_t sleep_timer_id;

void format_time(char *buffer, struct pulse_time_tm *time);
void update_text_clock();
void schedule_sleep();
void animate_globe(void *cookie);
void handle_wakeup(void *cookie);


//////////////////////////////////////////////////////////////////////////////

void format_time(char *buffer, struct pulse_time_tm *time)
{
  static const char days[7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  char am[] = "am";
  int hour = time->tm_hour;
  if(hour >= 12)
  {
    hour -= 12;
    am[0] = 'p';
  }
  if(hour == 0) hour = 12;
  sprintf(buffer, "%s %d:%02d %s", days[time->tm_wday - 1], hour, time->tm_min, am);
}

void update_text_clock()
{
  static struct PWidgetTextDynamic zone_text;
  static struct PWTextBox zone_text_box = {0, 93, SCREEN_WIDTH, 107};
  static struct PWidgetTextDynamic time_text;
  static struct PWTextBox time_text_box =
    {0, 108, SCREEN_WIDTH, SCREEN_HEIGHT - 1};

  static const color24_t text_color = {227, 107, 9, 255};
  static const color24_t black = {0, 0, 0, 255};
  static const enum PWTextStyle style = PWTS_TRUNCATE | PWTS_CENTER;

  char zone_buffer[20];
  char time_buffer[20];
  struct pulse_time_tm time;
  int i;
  
  // fill in dynamic text
  get_tz_data(target_timezone, &time, zone_buffer);
  pulse_init_dynamic_text_widget(&zone_text, zone_buffer, FONT_MISO_16,
      text_color, style);

  format_time(time_buffer, &time);
  pulse_init_dynamic_text_widget(&time_text, time_buffer, FONT_MISO_16,
      text_color, style);

  // blank text area
  pulse_set_draw_window(zone_text_box.left, zone_text_box.top,
     time_text_box.right, time_text_box.bottom);
  for(i = 0; i < (time_text_box.right - zone_text_box.left + 1) *
                 (time_text_box.bottom - zone_text_box.top + 1); i++)
  {
    pulse_draw_point24(black);
  }

  // paint text
  pulse_render_text(&zone_text_box, &zone_text);
  pulse_render_text(&time_text_box, &time_text);
}

void schedule_sleep()
{
  pulse_cancel_timer(&sleep_timer_id);
  sleep_timer_id = pulse_register_timer(TIME_BEFORE_SLEEP,  
    &pulse_update_power_down_timer, 0);
}

void animate_globe(void *cookie)
{
  int target_longitude = get_center_longitude(target_timezone);
  struct pulse_time_tm now;
  pulse_get_time_date(&now);
  draw_globe(longitude, get_noon_meridian(), now.tm_mon);
  if(longitude != target_longitude && longitude != target_longitude + 1)
  {
    longitude -= 2;
    if(longitude < -180) longitude += 360;
    pulse_register_timer(20, animate_globe, 0);
  }
}

void handle_wakeup(void *cookie)
{
  schedule_sleep();
  update_text_clock();
  animate_globe(0);
}

void main_app_init()
{
  longitude = get_center_longitude(LOCAL_TIMEZONE);
  animate_globe(0);
  update_text_clock();
  pulse_register_callback(ACTION_WOKE_FROM_BUTTON, &handle_wakeup);
}

void main_app_handle_button_down()
{
  schedule_sleep();
  target_timezone--;
  if(target_timezone < -11) target_timezone += 24;
  update_text_clock();
  animate_globe(0);
}

void main_app_handle_button_up()
{
}

void main_app_loop()
{
}

void main_app_handle_doz()
{
}

void main_app_handle_hardware_update(enum PulseHardwareEvent event)
{
}

