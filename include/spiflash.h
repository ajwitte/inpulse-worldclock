#ifndef SPIFLASH_H
#define SPIFLASH_H

// prototype of private API function for raw SPI flash reads
void spiflash_read(int addr, void *buffer, int size);

// location of first pixel of Earth texture bitmap in flash
#define TEXTURE_BASE_OFFSET (0x40000 + 0x1470 + 0x36)

// Location of daylight table
#define DAYLIGHT_TABLE_OFFSET (0x40000 + 0x20EF0)

// Location/parameters of timezone name table
#define TIMEZONE_TABLE_OFFSET (0x40000 + 0x21110)
#define TIMEZONE_FIELD_WIDTH 16

#endif /* SPIFLASH_H */
