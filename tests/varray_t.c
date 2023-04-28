#include "scunit.h"
#include "varray.h"


#define MAX( a, b ) ( (( a ) > ( b )) ? ( a ) : ( b ) )

/* calculate array size (must be array, not pointer) */
#define ASIZE( a ) ( sizeof( a ) / sizeof( a[0] ) )

struct test
{
  int a;
  char b;
};


/** Returns the lowest power of 2 bigger than \a x.
 *
 *  \param x Number to fit.
 *  \return Smallest power of 2 bigger than \a x.
 */
static size_t pow2roundup( size_t x )
{
  if( x == 0 )
    return 1;

  x -= 1;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;

  return x + 1;
}


TEST( Basic )
{
  int *a = NULL;
  varray_init( a, 0 );

  ASSERT_EQ( 1, varray_capacity( a ) );
  ASSERT_EQ( 0, varray_len( a ) );

  varray_push( a, 123 );
  ASSERT_EQ( 1, varray_capacity( a ) );
  ASSERT_EQ( 1, varray_len( a ) );
  ASSERT_EQ( 123, a[0] );
  ASSERT_EQ( 123, varray_last( a ) );

  varray_release( a );
  ASSERT_EQ( NULL, a );
}

TEST( int )
{
  const size_t initial_capacity = 1;

  /* creates an array with 2 empty elements */
  int *a = NULL;
  varray_init( a, initial_capacity );

  /* checks the used elements count is zero */
  ASSERT_EQ( 0, varray_len( a ) );
  ASSERT_EQ( initial_capacity, varray_capacity( a ) );

  /* adds a couple elements */
  for( size_t i = 0; i < 100; i++ )
  {
    int elem = i * 2;
    varray_push( a, elem );

    ASSERT_EQ( i + 1, varray_len( a ) );
    ASSERT_EQ( i * 2, a[i] );
    ASSERT_EQ( i * 2, varray_last( a ) );

    ASSERT_EQ( MAX( initial_capacity, pow2roundup( i + 1 ) ), varray_capacity( a ) );
  }

  varray_release( a );
  ASSERT_EQ( NULL, a );
}

TEST( float )
{
  const size_t initial_capacity = 8;

  /* creates an array with 2 empty elements */
  float *a = NULL;
  varray_init( a, initial_capacity );

  /* checks the used elements count is zero */
  ASSERT_EQ( 0, varray_len( a ) );
  ASSERT_EQ( initial_capacity, varray_capacity( a ) );

  /* adds a couple elements */
  for( size_t i = 0; i < 1000; i++ )
  {
    float elem = i / 5;
    varray_push( a, elem );

    ASSERT_EQ( i + 1, varray_len( a ) );
    ASSERT_EQ( MAX( initial_capacity, pow2roundup( i + 1 ) ), varray_capacity( a ) );
  }

  for( size_t i = 0; i < varray_len( a ); i++ )
    ASSERT_EQ( i / 5, a[i] );

  varray_release( a );
  ASSERT_EQ( NULL, a );
}

TEST( struct )
{
  const size_t initial_capacity = 1;

  /* creates an array with 2 empty elements */
  struct test *a = NULL;
  varray_init( a, initial_capacity );

  /* checks the used elements count is zero */
  ASSERT_EQ( 0, varray_len( a ) );
  ASSERT_EQ( initial_capacity, varray_capacity( a ) );

  /* adds a couple elements */
  for( size_t i = 0; i < 150; i++ )
  {
    struct test elem = { .a = i, .b = ( i % 5 ) ? 'a' : 'b' };
    varray_push( a, elem );

    ASSERT_EQ( i + 1, varray_len( a ) );
    ASSERT_EQ( i, a[i].a );

    if( i % 5 )
        ASSERT_EQ( 'a', a[i].b );
    else
        ASSERT_EQ( 'b', a[i].b );

    ASSERT_EQ( MAX( initial_capacity, pow2roundup( i + 1 ) ), varray_capacity( a ) );
  }

  varray_release( a );
  ASSERT_EQ( NULL, a );
}

TEST( LenManipulation )
{
  const size_t initial_capacity = 5;

  /* creates an array with 2 empty elements */
  long *a = NULL;
  varray_init( a, initial_capacity );

  /* adds a couple elements */
  for( size_t i = 0; i < initial_capacity; i++ )
  {
    long elem = i * 3;
    varray_push( a, elem );

    ASSERT_EQ( i + 1, varray_len( a ) );
    ASSERT_EQ( initial_capacity, varray_capacity( a ) );
  }

  /* NOTE: do not do this. This will leave a portion of the buffer uninitialized */
  varray_len( a ) = 15;
  ASSERT_EQ( 15, varray_len( a ) );

  varray_push( a, 123 );
  ASSERT_EQ( 16, varray_len( a ) );
  ASSERT_EQ( 30, varray_capacity( a ) );

  varray_release( a );
  ASSERT_EQ( NULL, a );
}

TEST( PopElements )
{
  const size_t initial_capacity = 5;

  /* creates an array with 2 empty elements */
  long *a = NULL;
  varray_init( a, initial_capacity );

  /* fills the array completely */
  for( size_t i = 0; i < initial_capacity; i++ )
    varray_push( a, ( i + 1 ) * 3 );

  /* pop an element and checks */
  long last_elem = varray_pop( a );
  ASSERT_EQ( initial_capacity - 1, varray_len( a ) );
  ASSERT_EQ( last_elem, initial_capacity * 3 );

  last_elem = varray_pop( a );
  ASSERT_EQ( initial_capacity - 2, varray_len( a ) );
  ASSERT_EQ( last_elem, ( initial_capacity - 1 ) * 3 );

  varray_release( a );
  ASSERT_EQ( NULL, a );
}

TEST( Resize )
{
  /* creates an array with 50 empty elements */
  char *a = NULL;
  varray_init( a, 50 );
  ASSERT_EQ( 50, varray_capacity( a ) );

  varray_push( a, '1' );
  varray_push( a, '2' );
  varray_push( a, '3' );

  ASSERT_EQ( 3, varray_len( a ) );

  varray_resize( a, 1500 );
  ASSERT_EQ( 1500, varray_capacity( a ) );
  ASSERT_EQ( 3, varray_len( a ) );

  ASSERT_EQ( '1', a[0] );
  ASSERT_EQ( '2', a[1] );
  ASSERT_EQ( '3', a[2] );

  varray_release( a );
  ASSERT_EQ( NULL, a );
}

TEST( AppendString )
{
  /* creates an array with 5 empty elements */
  char *a = NULL;
  varray_init( a, 5 );

  /* doesn't need to grow */
  varray_append( a, "hi,", 3 );
  ASSERT_EQ( 3, varray_len( a ) );
  ASSERT_EQ( 5, varray_capacity( a ) );

  /* duplicates 1 time */
  varray_append( a, " i'm", 4 );
  ASSERT_EQ( 7, varray_len( a ) );
  ASSERT_EQ( 10, varray_capacity( a ) );

  /* duplicates 3 times */
  varray_append( a, " trying to test the growth of this array...", 43 );
  ASSERT_EQ( 50, varray_len( a ) );
  ASSERT_EQ( 80, varray_capacity( a ) );

  /* doesn't need to grow */
  varray_append( a, " thanks. Now he capacity is 80", 30 );
  ASSERT_EQ( 80, varray_len( a ) );
  ASSERT_EQ( 80, varray_capacity( a ) );

  /* grow by 1 element */
  varray_append( a, ".", 1 );
  ASSERT_EQ( 81, varray_len( a ) );
  ASSERT_EQ( 160, varray_capacity( a ) );

  varray_push( a, '\0' );

  /* checks the content */
  ASSERT_EQ(
    0,
    strcmp( a,
            "hi, i'm trying to test the growth of this array... thanks. Now he capacity is 80." ) );

  /* append a const array (used to validate that compile time checks accept this) */
  const char *test_const_elems = "const ptr";
  varray_append( a, test_const_elems, 9 );

  varray_release( a );
  ASSERT_EQ( NULL, a );
}

TEST( AppendFloat )
{
  float *a = NULL;
  varray_init( a, 3 );

  float elems[3] = { 1.0, 2.0, 3.0 };
  varray_append( a, elems, ASIZE( elems ) );

  ASSERT_EQ( 3, varray_len( a ) );
  ASSERT_EQ( 3, varray_capacity( a ) );

  /* grows 3 times */
  float elems2[10] = { 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 };
  varray_append( a, elems2, ASIZE( elems2 ) );

  ASSERT_EQ( 13, varray_len( a ) );
  ASSERT_EQ( 24, varray_capacity( a ) );

  for( size_t i = 0; i < varray_len( a ); i++ )
    ASSERT_EQ( ( float )i + 1, a[i] );

  varray_release( a );
  ASSERT_EQ( NULL, a );
}
