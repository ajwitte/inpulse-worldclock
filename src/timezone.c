#include <pulse_os.h>
#include <pulse_types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "timezone.h"
#include "spiflash.h"

// Add (or subtract if negative) up to 24 hours to the time and manage the date
static void time_add_hours(struct pulse_time_tm *time, int hours);

static bool is_leap_year(int year);
static int days_in_month(int month, int year);

//////////////////////////////////////////////////////////////////////////////

void get_utc(struct pulse_time_tm *utc)
{
  pulse_get_time_date(utc);
  time_add_hours(utc, -LOCAL_TIMEZONE);
}

int get_noon_meridian(void)
{
  int meridian;
  struct pulse_time_tm utc;
  get_utc(&utc);
  meridian = (12 - utc.tm_hour) * 15;
  meridian -= utc.tm_min / 4;
  return meridian;
}

int get_center_longitude(int timezone)
{
  int longitude = timezone * 15;
  if(longitude == 180) longitude = -180;
  return longitude;
}

void get_tz_data(int timezone, struct pulse_time_tm *time, char *city_buffer)
{
  char city_name[TIMEZONE_FIELD_WIDTH];
  pulse_get_time_date(time);
  time_add_hours(time, timezone - LOCAL_TIMEZONE);
  spiflash_read(TIMEZONE_TABLE_OFFSET + ((timezone + 11) * TIMEZONE_FIELD_WIDTH),
                city_name, TIMEZONE_FIELD_WIDTH);
  sprintf(city_buffer, "%+d %s", timezone, city_name);
}

static void time_add_hours(struct pulse_time_tm *time, int hours)
{
  int month;
  int day_accum = 0;

  time->tm_hour += hours;

  // fixup wday, yday, year
  // note contrary to documentation, wday and yday are numbered from 1
  if(time->tm_hour >= 24)
  {
    time->tm_hour -= 24;
    time->tm_wday++;
    if(time->tm_wday > 7) time->tm_wday -= 7;
    time->tm_yday++;
    if(is_leap_year(time->tm_year))
    {
      if(time->tm_yday > 366)
      {
        time->tm_year++;
        time->tm_yday = 1;
      }
    }
    else
    {
      if(time->tm_yday > 365)
      {
        time->tm_year++;
        time->tm_yday = 1;
      }
    }
  }
  else if(time->tm_hour < 0)
  {
    time->tm_hour += 24;
    time->tm_wday--;
    if(time->tm_wday < 1) time->tm_wday += 7;
    time->tm_yday--;
    if(time->tm_yday < 1)
    {
      time->tm_year--;
      if(is_leap_year(time->tm_year))
      {
        time->tm_yday = 366;
      }
      else
      {
        time->tm_yday = 365;
      }
    }
  }

  // rebuild mon, mday from year, yday
  for(month = 0; month < 12; month++)
  {
    int days_this_month = days_in_month(month, time->tm_year);
    if(day_accum + days_this_month > time->tm_yday)
    {
      time->tm_mon = month;
      time->tm_mday = time->tm_yday - day_accum;
      break;
    }
    else
    {
      day_accum += days_this_month;
    }
  }
}

static bool is_leap_year(int year)
{
  year += 1900;
  if(year % 400 == 0) return true;
  if(year % 100 == 0) return false;
  if(year % 4 == 0) return true;
  return false;
}

static int days_in_month(int month, int year)
{
  static const uint8_t months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if(month == 1)
  {
    return is_leap_year(year) ? 29 : 28;
  }
  else
  {
    return months[month];
  }
}
