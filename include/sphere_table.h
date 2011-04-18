#ifndef SPHERE_TABLE_H
#define SPHERE_TABLE_H

#include <stdint.h>

const struct sphere_table_entry {
  unsigned int shade : 2;
  unsigned int angle : 6;
} __attribute((packed)) sphere_table[45][45];

#endif /* SPHERE_TABLE_H */
