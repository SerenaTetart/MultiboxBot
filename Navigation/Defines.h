#pragma once


#define MAX_PATH_LENGTH         740
#define MAX_POINT_PATH_LENGTH   740

#define SMOOTH_PATH_STEP_SIZE   4.0f
#define SMOOTH_PATH_SLOP        0.3f

#define VERTEX_SIZE       3
#define INVALID_POLYREF   0

// defined in DBC and left shifted for flag usage
#define MAP_LIQUID_TYPE_NO_WATER    0x00
#define MAP_LIQUID_TYPE_MAGMA       0x01
#define MAP_LIQUID_TYPE_OCEAN       0x02
#define MAP_LIQUID_TYPE_SLIME       0x04
#define MAP_LIQUID_TYPE_WATER       0x08

#define MAP_ALL_LIQUIDS   (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_SLIME)

#define MAP_LIQUID_TYPE_DARK_WATER  0x10
#define MAP_LIQUID_TYPE_WMO_WATER   0x20

/**
 * @brief Magic number for identifying MMAP files.
 */
#define MMAP_MAGIC 0x4d4d4150   // 'MMAP'

/**
 * @brief Version number of the MMAP format.
 */
#define MMAP_VERSION 4

 /**
  * @brief Size of the grids used in MMAP.
  */
#define SIZE_OF_GRIDS 533.33333f

#define MY_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MY_MAX(a, b) ((a) > (b) ? (a) : (b))
