/**
 * \file
 * ZIP compression - Implementation.
 */

/* include area */
#include "string.h"
#include "zip.h"
#include "varray.h"
#include <time.h>


/*-----------------------------------------------------------------------------
   Definitions
-----------------------------------------------------------------------------*/

/** Size of the internal buffer of the zip_t structure. */
#define ZIP_INTERNAL_BUFFER_SIZE ( 4 << 10 )


/*-----------------------------------------------------------------------------
   Useful macros
-----------------------------------------------------------------------------*/

/** Writes the little-endian representation of \a n.
 *
 *  \param count Pointer to a variable that will be updated adding the bytes written.
 *  \param cb The output callback.
 *  \param cb_ctx The output callback context.
 *  \param n The number to convert.
 */
#define WRITE_LE( count, cb, cb_ctx, n ) ( _write_le( count, cb, cb_ctx, &( n ), sizeof( n ) ) )

/** Gets the current entry from the ZIP context.
 *
 *  \param z ZIP context.
 */
#define CUR_ENTRY( z ) ( varray_last( ( z )->entries ) )


/*-----------------------------------------------------------------------------
   Internal data types
-----------------------------------------------------------------------------*/

/** The local file header record fields of a ZIP file. */
struct zip_local_file_header
{
  /** Local file header signature (0x04034b50). */
  uint32_t signature;

  /** Version needed to extract. */
  uint16_t extract_version;

  /** General purpose bit flag. */
  uint16_t flags;

  /** Compression method. */
  uint16_t method;

  /** Last modification file time. */
  uint16_t modif_time;

  /** Last modification file date. */
  uint16_t modif_date;

  /** crc-32. */
  uint32_t crc;

  /** Compressed size. */
  uint32_t compressed_size;

  /** Uncompressed size. */
  uint32_t uncompressed_size;

  /** File name length. */
  uint16_t fname_length;

  /** Extra field length. */
  uint16_t extra_field_length;

  /** File name (variable size). */
  /** Excluded in this implementation: Extra field (variable size). */
};


/** Zip's central directory record structure. */
struct zip_central_dir
{
  /** Central file header signature (0x02014b50). */
  uint32_t signature;

  /** Version made by. */
  uint16_t made_by;

  /** Version needed to extract. */
  uint16_t extract_version;

  /** General purpose bit flag. */
  uint16_t flags;

  /** Compression method. */
  uint16_t method;

  /** Last modification file time. */
  uint16_t modif_time;

  /** Last modification file date. */
  uint16_t modif_date;

  /** crc-32. */
  uint32_t crc;

  /** Compressed size. */
  uint32_t compressed_size;

  /** Uncompressed size. */
  uint32_t uncompressed_size;

  /** File name length. */
  uint16_t fname_length;

  /** Extra field length. */
  uint16_t extra_field_length;

  /** File comment length. */
  uint16_t comment_length;

  /** Disk number start. */
  uint16_t disk_num;

  /** Internal file attributes. */
  uint16_t internal_attributes;

  /** External file attributes. */
  uint32_t external_attributes;

  /** Relative offset of local header. */
  uint32_t local_header_offset;

  /** File name (variable size). */
  /** Excluded in this implementation: Extra field (variable size). */
  /** Excluded in this implementation: File comment (variable size). */
};


/** End of the central directory record structure in a ZIP file. */
struct zip_eof_central_dir
{
  /** End of central directory signature (0x06054b50). */
  uint32_t signature;

  /** Number of this disk. */
  uint16_t disk_num;

  /** Number of the disk with the start of the central directory. */
  uint16_t start_disk_num;

  /** Total number of entries in the central directory on this disk. */
  uint16_t num_entries_in_disk;

  /** total number of entries in the central directory. */
  uint16_t num_entries;

  /** Size of the central directory. */
  uint32_t central_dir_size;

  /** Offset of start of central directory with respect to the starting disk number. */
  uint32_t offset;

  /** ZIP file comment length. */
  uint16_t comment_length;

  /** Excluded in this implementation: ZIP file comment (variable size). */
};


/** Writes a 16 bit integer in little-endian into the output.
 *
 *  \param out_cb Output callback.
 *  \param out_cb_ctx Output callback context.
 *  \param input The number to convert.
 *  \return \c false on error.
 *
 *  \note See \a WRITE_LE macro.
 */
static bool _write16_le( zip_out_cb_t out_cb, void *out_cb_ctx, uint16_t input )
{
  uint8_t output[2];
  output[0] = ( uint8_t )( input );
  output[1] = ( uint8_t )( input >> 8 );

  /* writes into the output callback */
  return out_cb( out_cb_ctx, output, sizeof( output ) );
}


/** Writes a 16 bit integer in little-endian into the output.
 *
 *  \param out_cb Output callback.
 *  \param out_cb_ctx Output callback context.
 *  \param input The number to convert.
 *  \return \c false on error.
 *
 *  \note See \a WRITE_LE macro.
 */
static bool _write32_le( zip_out_cb_t out_cb, void *out_cb_ctx, uint32_t input )
{
  uint8_t output[4];
  output[0] = ( uint8_t )( input );
  output[1] = ( uint8_t )( input >> 8 );
  output[2] = ( uint8_t )( input >> 16 );
  output[3] = ( uint8_t )( input >> 24 );

  /* writes into the output callback */
  return out_cb( out_cb_ctx, output, sizeof( output ) );
}


/** Writes a 16 bit integer in little-endian into the output.
 *
 *  \param out_cb Output callback.
 *  \param out_cb_ctx Output callback context.
 *  \param input The number to convert.
 *  \return \c false on error.
 *
 *  \note See \a WRITE_LE macro.
 */
static bool _write64_le( zip_out_cb_t out_cb, void *out_cb_ctx, uint64_t input )
{
  uint8_t output[8];
  output[0] = ( uint8_t )( input );
  output[1] = ( uint8_t )( input >> 8 );
  output[2] = ( uint8_t )( input >> 16 );
  output[3] = ( uint8_t )( input >> 24 );
  output[4] = ( uint8_t )( input >> 32 );
  output[5] = ( uint8_t )( input >> 40 );
  output[6] = ( uint8_t )( input >> 48 );
  output[7] = ( uint8_t )( input >> 56 );

  /* writes into the output callback */
  return out_cb( out_cb_ctx, output, sizeof( output ) );
}


/** Writes a little endian representation of a given number.
 *
 *  \param count Pointer to a counter of bytes written.
 *  \param out_cb Output callback.
 *  \param out_cb_ctx Output callback context.
 *  \param input Pointer to the number to convert.
 *  \param input_size Size of \a input.
 *  \return \c false on error.
 *
 *  \note See \a WRITE_LE macro.
 */
static bool _write_le( size_t *count,
                       zip_out_cb_t out_cb,
                       void *out_cb_ctx,
                       const void *input,
                       size_t input_size )
{
  *count += input_size;

  /* calls the the corresponding function cased on the input size */
  switch( input_size )
  {
    case 2:
      return _write16_le( out_cb, out_cb_ctx, *( ( uint16_t * )input ) );

    case 4:
      return _write32_le( out_cb, out_cb_ctx, *( ( uint32_t * )input ) );

    case 8:
      return _write64_le( out_cb, out_cb_ctx, *( ( uint64_t * )input ) );
  }

  /* shouldn't be reached */
  return false;
}


/** Deflates the input buffer until it's completely consumed. May output some data.
 *
 *  \param z Compression context.
 *  \param flush Flush mode as described in libz.
 *  \param data Buffer of data to compress.
 *  \param data_len Bytes in \a data.
 *  \return \c false on error.
 *
 *  \note This function also updates the ZIP context.
 */
static bool _deflate( zip_t *z, int flush, const void *data, size_t data_len )
{
  CUR_ENTRY( z ).size += data_len;

  z->stream.avail_in = data_len;
  z->stream.next_in = ( Bytef * )data;

  /* run deflate until output buffer not full to make sure all input data was processed */
  do
  {
    /* resets the output buffer to set it as empty */
    z->stream.avail_out = ZIP_INTERNAL_BUFFER_SIZE;
    z->stream.next_out = z->out_buffer;

    /* compresses the data. There are no possible errors when calling this function, so if
     * it fails it's because an invalid memory state */
    if( deflate( &z->stream, flush ) < 0 )
      return false;

    /* outputs compressed data */
    size_t out_size = ZIP_INTERNAL_BUFFER_SIZE - z->stream.avail_out;
    if( !z->out_cb( z->out_cb_ctx, z->out_buffer, out_size ) )
      return false;

    CUR_ENTRY( z ).size_compressed += out_size;
    z->bytes_written += out_size;
  } while( z->stream.avail_out == 0 );

  /* success */
  return true;
}


/** Writes the Central Directory (CD) file header for an entry.
 *
 *  \param z ZIP context.
 *  \param entry_num Index of the entry.
 *  \return \c false on error.
 */
static bool _write_cd_file_header( zip_t *z, size_t entry_num )
{
  size_t entry_name_len = strlen( z->entries[entry_num].name );

  /* writes the central directory record */
  struct zip_central_dir central_data = {
    .signature = 0x02014b50U,
    .made_by = 0U,
    .extract_version = 20U,
    .flags = ( 1U << 3U ), /* bit 3 on to indicate streaming */
    .method = 8U,          /* DEFLATE */
    .modif_time = z->entries[entry_num].time,
    .modif_date = z->entries[entry_num].date,
    .crc = z->entries[entry_num].crc,
    .compressed_size = z->entries[entry_num].size_compressed,
    .uncompressed_size = z->entries[entry_num].size,
    .fname_length = entry_name_len,
    .extra_field_length = 0,
    .comment_length = 0,      /* no comments */
    .disk_num = 0,            /* no fragmentation supported */
    .internal_attributes = 0, /* no attributes */
    .external_attributes = 0, /* no attributes */
    .local_header_offset = z->entries[entry_num].offset,
  };

  size_t cd_bytes_written = 0;
  if( !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.signature ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.made_by ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.extract_version ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.flags ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.method ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.modif_time ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.modif_date ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.crc ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.compressed_size ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.uncompressed_size ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.fname_length ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.extra_field_length ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.comment_length ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.disk_num ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.internal_attributes ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.external_attributes ) ||
      !WRITE_LE( &cd_bytes_written, z->out_cb, z->out_cb_ctx, central_data.local_header_offset ) )
    return false;

  if( !z->out_cb( z->out_cb_ctx, ( uint8_t * ) z->entries[entry_num].name, entry_name_len ) )
    return false;

  z->bytes_written += cd_bytes_written;
  z->bytes_written += entry_name_len;

  /* success */
  return true;
}


/** Writes the End of Central Directory record.
 *
 *  \param z ZIP context.
 *  \return \c false on error.
 */
static bool _write_eocd( zip_t *z )
{
  /* writes the end of central directory record */
  struct zip_eof_central_dir eof_central_dir = {
    .signature = 0x06054b50U,
    .disk_num = 0,                         /* no multiple disks supported */
    .start_disk_num = 0,                   /* always 1 disk */
    .num_entries_in_disk = varray_len( z->entries ), /* only 1 entry supported */
    .num_entries = varray_len( z->entries ),         /* only 1 entry supported */
    .central_dir_size = z->bytes_written - z->central_dir_offset,
    .offset = z->central_dir_offset, /* the offset from the beginning until the central dir */
    .comment_length = 0,
  };

  size_t bytes_written = 0;
  if( !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, eof_central_dir.signature ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, eof_central_dir.disk_num ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, eof_central_dir.start_disk_num ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, eof_central_dir.num_entries_in_disk ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, eof_central_dir.num_entries ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, eof_central_dir.central_dir_size ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, eof_central_dir.offset ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, eof_central_dir.comment_length ) )
    return false;

  /* success */
  return true;
}


/** Converts a given datetime to MS-DOS time format.
 *
 *  \param dt Input datetime.
 *  \return Time in MS-DOS format.
 */
static uint16_t _get_dos_time( struct zip_datetime dt )
{
  /* bit | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
   *     |          hours         |           minutes           |      pair seconds      |
   *
   * note: MS-DOS time represents a 2-second interval within some day.
   */
  uint16_t time = ( ( dt.seconds / 2 ) & 0x1f );
  time |= ( dt.minutes & 0x3f ) << 5;
  time |= ( dt.hours & 0x1f ) << 11;
  return time;
}


/** Converts a given datetime to MS-DOS date format.
 *
 *  \param dt Input datetime.
 *  \return Date in MS-DOS format.
 */
static uint16_t _get_dos_date( struct zip_datetime dt )
{
  /* bit | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
   *     |             year - 1980               |       month       |        day        |
   */
  uint16_t date = ( dt.day & 0x1f );
  date |= ( ( dt.month ) & 0x0f ) << 5;
  date |= ( ( dt.year - 1980 ) & 0x7f ) << 9;
  return date;
}


/** Initializes the ZIP context.
 *
 *  \param z ZIP context to initialize.
 *  \param out_cb Output callback.
 *  \param out_cb_ctx Output callback context.
 *  \return \c false on error.
 */
bool zip_init( zip_t *z, zip_out_cb_t out_cb, void *out_cb_ctx )
{
  if( out_cb == NULL )
    return false;

  z->out_cb = out_cb;
  z->out_cb_ctx = out_cb_ctx;
  z->bytes_written = 0;
  z->central_dir_offset = 0;
  z->entry_opened = false;
  z->out_buffer = malloc( ZIP_INTERNAL_BUFFER_SIZE );

  /* starts with 1 entry in the array */
  varray_init( z->entries, 1 );

  /* initializes the stream */
  z->stream.opaque = Z_NULL;
  z->stream.zalloc = Z_NULL;
  z->stream.zfree = Z_NULL;
  z->stream.next_in = NULL;
  z->stream.data_type = Z_BINARY;

  /* zlib initialization parameters */
  const int memlevel = 8;
  const int strategy = Z_DEFAULT_STRATEGY;
  const int method = Z_DEFLATED;
  const int level = Z_DEFAULT_COMPRESSION;
  const int window_bits = -15;

  if( deflateInit2( &z->stream, level, method, window_bits, memlevel, strategy ) != Z_OK )
  {
    free( z->out_buffer );
    free( z->entries );
    return false;
  }

  return true;
}


/** Releases the resources of an initialized ZIP context.
 *
 *  \param z ZIP context to release.
 *
 *  \note \a zip_end must be called before this function to ensure all data is flushed.
 */
void zip_release( zip_t *z )
{
  z->out_cb = NULL;
  deflateEnd( &z->stream );
  free( z->out_buffer );
  varray_release( z->entries );
}


/** Finishes the ZIP generation and flushes remaining data.
 *
 *  \param z ZIP context.
 *  \return \c false on error.
 */
bool zip_end( zip_t *z )
{
  if( z->entry_opened )
    return false;

  z->central_dir_offset = z->bytes_written;

  /* writes the Central directory file headers */
  for( size_t i = 0; i < varray_len( z->entries ); i++ )
    if( !_write_cd_file_header( z, i ) )
      return false;

  /* writes the end of central directory record */
  return _write_eocd( z );
}


/** Adds a new entry to the ZIP archive. To add content, call \a zip_entry_update repeatedly and
 *  then \a zip_entry_end.
 *
 *  \param z ZIP context.
 *  \param filename Entry name (if larger than \c ZIP_ENTRY_MAX_NAME_LEN it's truncated).
 *  \param datetime Entry date and time.
 *  \return \c false on error.
 */
bool zip_entry_add( zip_t *z, const char *filename, struct zip_datetime datetime )
{
  if( z->entry_opened )
    return false;

  zip_entry_t entry;

  size_t entry_name_len = strlen( filename );
  if( entry_name_len > ZIP_ENTRY_MAX_NAME_LEN )
    entry_name_len = ZIP_ENTRY_MAX_NAME_LEN;

  /* initializes the entry */
  entry.crc = crc32( 0, Z_NULL, 0 );
  entry.size = 0;
  entry.size_compressed = 0;
  memcpy( entry.name, filename, entry_name_len );
  entry.name[entry_name_len] = '\0';
  entry.offset = z->bytes_written;
  entry.date = _get_dos_date( datetime );
  entry.time = _get_dos_time( datetime );

  struct zip_local_file_header lf_header = {
    .signature = 0x04034b50U,
    .extract_version = 20U,
    .flags = ( 1U << 3U ), /* bit 3 on to indicate streaming, bit 1 and 2 for compression options */
    .method = 8U,          /* DEFLATE */
    .modif_time = entry.time,
    .modif_date = entry.date,
    .crc = 0,                       /* set to zero because it's indicated in the data descriptor */
    .compressed_size = 0,           /* set to zero because it's indicated in the data descriptor */
    .uncompressed_size = 0,         /* set to zero because it's indicated in the data descriptor */
    .fname_length = entry_name_len, /* filename length (null character not included) */
    .extra_field_length = 0,        /* no extra field */
  };

  /* writes each field in little-endian */
  size_t bytes_written = 0;
  if( !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.signature ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.extract_version ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.flags ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.method ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.modif_time ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.modif_date ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.crc ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.compressed_size ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.uncompressed_size ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.fname_length ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, lf_header.extra_field_length ) )
    return false;

  /* writes the filename */
  if( !z->out_cb( z->out_cb_ctx, ( uint8_t * ) entry.name, lf_header.fname_length ) )
    return false;

  z->entry_opened = true;
  varray_push( z->entries, entry );

  /* updates the number of bytes written */
  z->bytes_written += bytes_written + lf_header.fname_length;

  /* resets the compression context */
  return ( deflateReset( &z->stream ) == Z_OK );
}


/** Writes data into the current entry (can be called many times for the same entry).
 *
 *  \param z ZIP context.
 *  \param data Data to compress.
 *  \param data_len Bytes in \a data.
 *  \return \c false on error.
 *
 *  \note In order to call this function, \a zip_entry_add must have been called before.
 */
bool zip_entry_update( zip_t *z, const void *data, size_t data_len )
{
  if( !z->entry_opened )
    return false;

  if( data_len == 0 )
    return true;

  /* updates the CRC */
  CUR_ENTRY( z ).crc = crc32( CUR_ENTRY( z ).crc, data, data_len );
  return _deflate( z, Z_NO_FLUSH, data, data_len );
}


/** Closes an entry.
 *
 *  \param z ZIP context.
 *  \return \c false on error.
 */
bool zip_entry_end( zip_t *z )
{
  if( !z->entry_opened )
    return true;

  /* flushes remaining data */
  if( !_deflate( z, Z_FINISH, NULL, 0 ) )
    return false;

  /* writes the data descriptor record */
  const uint32_t data_desc_signature = 0x08074b50U;

  /* writes the data descriptor record */
  size_t bytes_written = 0;
  if( !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, data_desc_signature ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, CUR_ENTRY( z ).crc ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, CUR_ENTRY( z ).size_compressed ) ||
      !WRITE_LE( &bytes_written, z->out_cb, z->out_cb_ctx, CUR_ENTRY( z ).size ) )
    return false;

  /* updates the number of bytes written */
  z->bytes_written += bytes_written;
  z->entry_opened = false;

  /* success */
  return true;
}


/** Returns the number of entries added to the ZIP.
 *
 *  \param z ZIP context.
 *  \return Number of entries.
 */
size_t zip_get_num_entries( zip_t *z )
{
  return varray_len( z->entries );
}


/** Returns the current date time in the format required by ZIP functions.
 *
 *  \return Current datetime.
 */
struct zip_datetime zip_get_datetime( void )
{
  time_t current_time = time( NULL );

#ifdef localtime_r

  struct tm tm = { 0 };
  if( localtime_r( &current_time, &tm ) == NULL )
    return ( struct zip_datetime ){ .year = 2000, .month = 1, .day = 1 };

#else

  /* WARNING: non thread safe localtime being used */
  struct tm *tm_ptr = localtime( &current_time );
  if ( tm_ptr == NULL )
    return ( struct zip_datetime ){ .year = 2000, .month = 1, .day = 1 };

  struct tm tm = *tm_ptr;

#endif

  /* returns the  */
  struct zip_datetime dt = {
    .year = tm.tm_year + 1900,
    .month = tm.tm_mon + 1,
    .day = tm.tm_mday,
    .hours = tm.tm_hour,
    .minutes = tm.tm_min,
    .seconds = tm.tm_sec,
  };

  return dt;
}
