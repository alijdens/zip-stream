# zip-stream
A small C library to generate streamed ZIP archives. I created this because I needed a lightweight ZIP library that allowed me to stream data on the fly. The available options at the time where either too bloated or had a restrictive license.

## Dependencies

This library uses [zlib](https://github.com/madler/zlib) for the compression algorithm. You can include the project sources in your code base or install them with

```sh
apt install zlib1g-dev
```

## Tests

To run the test suite execute

```sh
make tests
```

you can also include `DEBUG=1` to compile with debug symbols.

**Note**: the tests require the `unzip` command to be available (in order to validate the resulting zip files).

## Usage

The following example illustrates how to create a ZIP archive with 2 files. Error handling is not implemented to make the example simple:

```C
// you need a callback that handles the streamed data. In this example
// the callback will output to a file named "example.zip"
// The callback must implement the zip_out_cb_t interface and will be
// executed everytime the zip_t has to flush the internal buffer of data
static bool _zip_to_file( void *cb_ctx, const uint8_t *data, size_t data_len )
{
    // the context is a pointer to the output file descriptor in this case
    // but could be anything defined by the user and will be passed to every
    // execution of this function
    int *fd = cb_ctx;
    // write to the output file
    return ( write( *fd, data, data_len ) == data_len );
}

int fd = open( ... );

// create and initialize a ZIP type
zip_t z;
if( !zip_init( &z, _zip_to_file, &fd ) )
    exit( 1 );

        /****************/
        /*  file_1.txt  */
        /****************/

// create a new entry (a file) in the archive
if( !zip_entry_add( &z, "file_1.txt", zip_get_datetime() ) )
    exit( 1 );

// add data to the current entry
const char *data1 = "some file data";
if( !zip_entry_update( &z, data1, strlen( data1 ) ) )
    exit( 1 );

// add more data... (you can call this function all the times you need)
const char *data2 = "more data!";
if( !zip_entry_update( &z, data2, strlen( data2 ) ) )
    exit( 1 );

// close the entry (i.e. file_1.txt does not contain any more data)
if( !zip_entry_end( &z ) )
    exit( 1 );

        /****************/
        /*  file_2.txt  */
        /****************/

if( !zip_entry_add( &z, "file_2.txt", zip_get_datetime() ) )
    exit( 1 );

const char *data3 = "data in file 2";
if( !zip_entry_update( &z, data3, strlen( data3 ) ) )
    exit( 1 );

if( !zip_entry_end( &z ) )
    exit( 1 );

// and now finish the ZIP and flush all the remaining data
if( !zip_end( &z ) )
    exit( 1 );

// finally, release resources
zip_release( &z );
```
