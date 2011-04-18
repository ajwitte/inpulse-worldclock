#ifndef GLOBE_H
#define GLOBE_H

// Draw the globe with:
//  - projection centered on <center_longitude>
//  - daylighted portion centered on <noon_meridian>
//  - daylight according to the table for <month> (january = 0)
// Longitude in degrees with 0 at the prime meridian
void draw_globe(int center_longitude, int noon_meridian, int month);

#endif /* GLOBE_H */
