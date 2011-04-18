#include <pulse_os.h>
#include <pulse_types.h>
#include <stdint.h>
#include <stdbool.h>

#include "globe.h"
#include "sphere_table.h"
#include "spiflash.h"

// Size parameters
#define GLOBE_SIZE 90
#define BMP_ROW_LENGTH (GLOBE_SIZE * 4)
#define START_PIXEL (((SCREEN_WIDTH - GLOBE_SIZE) / 2))

// Number of scans necessary to draw the entire image
#define INTERLACING 3

// Bitmap pixel layout in flash
typedef struct {
  uint8_t blue;
  uint8_t green;
  uint8_t red;
  uint8_t alpha;
} __attribute((packed)) bmp_pixel;

static int daylight_lookup(int month, int row);

static void draw_globe_row(int row, int center_longitude,
                           int noon_meridian, int month);

/////////////////////////////////////////////////////////////////////////////

void draw_globe(int center_longitude, int noon_meridian, int month)
{
  int i, row;
  for(i = 0; i < INTERLACING; i++)
  {
    for(row = i; row < GLOBE_SIZE; row += INTERLACING)
    {
      draw_globe_row(row, center_longitude, noon_meridian, month);
    }
  }
}


static int daylight_lookup(int month, int row)
{
  uint8_t table_entry;
  spiflash_read(DAYLIGHT_TABLE_OFFSET + (month * GLOBE_SIZE / 2) + row,
                &table_entry, 1);
  return table_entry;
}


static void draw_globe_row(int row, int center_longitude,
                           int noon_meridian, int month)
{
  int col;
  int sy;
  int row_offset = TEXTURE_BASE_OFFSET +
                    (BMP_ROW_LENGTH * sizeof(bmp_pixel) * row);
  int center_offset;
  int noon_offset;
  int daylight;
  int sunrise, sunset;
  bool polar_night = false;

  pulse_set_draw_window(START_PIXEL, START_PIXEL + row,
    START_PIXEL + GLOBE_SIZE - 1, START_PIXEL + row); 

  // Convert longitudes to 'offsets' in the range [0,180) with 0 at the dateline
  center_offset = ((center_longitude + 180) * GLOBE_SIZE * 2) / 360;
  noon_offset = ((noon_meridian + 180) * GLOBE_SIZE * 2) / 360;

  // The sphere_table only contains values for the upper right quadrant
  // of the globe image, so determine which quadrant we are in and compute
  // y-index for the sphere_table (sy) accordingly.
  //
  // Likewise the daylight table only contains values for the northern
  // hemisphere so computation of 'sunrise' and 'sunset' (offset of
  // leading [resp. trailing] edge of daylight area) varies per hemisphere.
  //
  // Also in the event 'sunrise' and 'sunset' are at the same longitude,
  // which is possible near the poles, we need auxiliary information
  // in the form of the polar_night flag to distinguish day/night.
  if(row >= GLOBE_SIZE / 2) {
    sy = row - (GLOBE_SIZE / 2);
    daylight = daylight_lookup(month, sy);
    sunrise = noon_offset + GLOBE_SIZE - daylight;
    sunset = noon_offset - GLOBE_SIZE + daylight;
    if(daylight == GLOBE_SIZE) polar_night = true;
  } else {
    sy = (GLOBE_SIZE / 2) - row - 1;
    daylight = daylight_lookup(month, sy);
    sunrise = noon_offset + daylight;
    sunset = noon_offset - daylight;
    if(daylight == 0) polar_night = true;
  }

  // Normalize sunrise/sunset offsets to the range 0..GLOBE_SIZE*2
  // % operator is slow so do it this way
  if(sunrise >= GLOBE_SIZE * 2) sunrise -= GLOBE_SIZE * 2;
  if(sunset < 0) sunset += GLOBE_SIZE * 2;

  for(col = 0; col < GLOBE_SIZE; col++)
  {
    int sx;
    int texture_offset;
    int shade;
    color24_t tmp_color;
    tmp_color.alpha = 0xff;

    // As above, for sphere_table x-index (sx)
    // Now we can determine the longitude for this pixel and thus the
    // offset into the texture bitmap
    if(col >= GLOBE_SIZE / 2) {
      sx = col - (GLOBE_SIZE / 2);
      texture_offset = center_offset + sphere_table[sy][sx].angle;
      if(texture_offset >= GLOBE_SIZE * 2) texture_offset -= GLOBE_SIZE * 2;
    } else {
      sx = (GLOBE_SIZE / 2) - col - 1;
      texture_offset = center_offset - sphere_table[sy][sx].angle;
      if(texture_offset < 0) texture_offset += GLOBE_SIZE * 2;
    }

    // Determine whether this pixel is in the day or night region;
    // if night, adjust texture_offset to point into the nighttime
    // portion of the texture (to the right of the daytime portion)
    if(sunset < sunrise && (texture_offset > sunrise || texture_offset < sunset))
    {
      texture_offset += GLOBE_SIZE * 2;
    }
    else if(sunset > sunrise &&
            (texture_offset > sunrise && texture_offset < sunset))
    {
      texture_offset += GLOBE_SIZE * 2;
    }
    else if(polar_night)
    {
      texture_offset += GLOBE_SIZE * 2;
    }

    // Lookup and draw the pixel
    shade = sphere_table[sy][sx].shade;
    if(shade > 0)
    {
      if((texture_offset != sunset && texture_offset != sunrise)
         || sunrise == sunset)
      {
        // Draw normal pixel with shading according to sphere table
        // (produces crude antialiasing of disk edge)
        bmp_pixel pixel;
        spiflash_read(row_offset + (texture_offset * sizeof(bmp_pixel)),
                      &pixel, sizeof(bmp_pixel));
        tmp_color.red = ((int)pixel.red * (shade + 1)) / 4;
        tmp_color.green = ((int)pixel.green * (shade + 1)) / 4;
        tmp_color.blue = ((int)pixel.blue * (shade + 1)) / 4;
        pulse_draw_point24(tmp_color);
      }
      else
      {
        // This pixel lies exactly on the terminator, so average
        // the day and night pixels to effect very crude antialiasing
        // of the terminator
        bmp_pixel day_pixel, night_pixel;
        spiflash_read(row_offset + (texture_offset * sizeof(bmp_pixel)),
                      &day_pixel, sizeof(bmp_pixel));
        spiflash_read(row_offset + ((GLOBE_SIZE * 2 + texture_offset)
                                    * sizeof(bmp_pixel)),
                      &night_pixel, sizeof(bmp_pixel));
        tmp_color.red = ((int)(day_pixel.red + night_pixel.red)
                         * (shade + 1)) / 8;
        tmp_color.green = ((int)(day_pixel.green + night_pixel.green)
                           * (shade + 1)) / 8;
        tmp_color.blue = ((int)(day_pixel.blue + night_pixel.blue)
                          * (shade + 1)) / 8;
        pulse_draw_point24(tmp_color);
      }
    }
    else
    {
      // pixel lies entirely outside the disk of the globe; draw black
      tmp_color.red = 0;
      tmp_color.green = 0;
      tmp_color.blue = 0;
      pulse_draw_point24(tmp_color);
    }
  }
}

