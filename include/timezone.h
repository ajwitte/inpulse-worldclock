#ifndef TIMEZONE_H
#define TIMEZONE_H

#include <pulse_types.h>

// Timezone of the time for which the watch RTC is currently set
// This should probably match the timezone of your PC or smartphone
// Or use 0 if you set the watch to UTC
// Remember this app doesn't know anything about daylight savings
#define LOCAL_TIMEZONE (-7)

// fill in time struct with UTC
void get_utc(struct pulse_time_tm *utc);

// get the longitude (in degrees) at which it is currently noon
int get_noon_meridian(void);

// get the longitude at the center of the specified timezone
int get_center_longitude(int timezone);

// get the local time and a printable name in the specified timezone
void get_tz_data(int timezone, struct pulse_time_tm *time, char *city_buffer);

#endif /* TIMEZONE_H */
