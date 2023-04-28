/**
 * \file
 * ZIP compression - Interface.
 */

#ifndef ZIP
#define ZIP

/* include area */
#include "zlib.h"
#include <stdint.h>
#include <stdbool.h>


/*-----------------------------------------------------------------------------
   Library definitions
-----------------------------------------------------------------------------*/

/** Maximum length of an entry name (NOT including the terminating null character). */
#ifndef ZIP_ENTRY_MAX_NAME_LEN
#  define ZIP_ENTRY_MAX_NAME_LEN 127
#endif


/*-----------------------------------------------------------------------------
   Library data types
-----------------------------------------------------------------------------*/

/** The datetime for ZIP entries. */
struct zip_datetime
{
  unsigned year;
  unsigned month;
  unsigned day;
  unsigned hours;
  unsigned minutes;
  unsigned seconds;
};


/** Callback for the user to handle the compressed data. */
typedef bool ( *zip_out_cb_t )( void *cb_ctx, const uint8_t *data, size_t data_len );

/** Structure representing an entry in the ZIP archive. */
typedef struct
{
  /** The entry's offset in the file. */
  uint32_t offset;

  /** The CRC-32 of the uncompressed entry data. */
  uint32_t crc;

  /** Entry size. */
  uint32_t size;

  /** Compressed entry size. */
  uint32_t size_compressed;

  /** Entry name as a C string. */
  char name[ZIP_ENTRY_MAX_NAME_LEN + 1];

  /** Entry's time in MS-DOS format. */
  uint16_t time;

  /** Entry's date in MS-DOS format. */
  uint16_t date;

} zip_entry_t;

/** ZIP context type. */
typedef struct
{
  /** Zlib stream */
  z_stream stream;

  /** Callback that handles compressed data output. */
  zip_out_cb_t out_cb;

  /** User defined context for \a out_cb */
  void *out_cb_ctx;

  /** \a varray of entries in the ZIP archive. */
  zip_entry_t *entries;

  /** Internal buffer to hold compressed data. */
  uint8_t *out_buffer;

  /** The number of bytes written to the output file. */
  size_t bytes_written;

  /** The offset until the central directory. */
  size_t central_dir_offset;

  /** Whether an entry is in process. */
  bool entry_opened;

} zip_t;


/*-----------------------------------------------------------------------------
   Function prototypes
-----------------------------------------------------------------------------*/

/** Init/uninit */
bool zip_init( zip_t *z, zip_out_cb_t out_cb, void *out_cb_ctx );
void zip_release( zip_t *z );

bool zip_end( zip_t *z );

/** Entry handling */
bool zip_entry_add( zip_t *z, const char *filename, struct zip_datetime datetime );
bool zip_entry_update( zip_t *z, const void *data, size_t data_len );
bool zip_entry_end( zip_t *z );
size_t zip_get_num_entries( zip_t *z );

/** Miscellaneous */
struct zip_datetime zip_get_datetime( void );


#endif
