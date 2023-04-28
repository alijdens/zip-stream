/**
 *  \file
 *  Array that grows dynamically - Interface.
 *
 *  The var array gives dynamic buffer functionality to normal C pointer but
 *  maintaining the native array access interface.
 *
 *  To use it just declare the desired array type and call the public macros
 *  on it:
 *
 *    int *va = NULL;
 *    varray_init( va, 1 );
 *
 *    // pushing elements resizes the array under the hood
 *    varray_push( va, 123 );
 *    varray_push( va, 1235 );
 *
 *    // using pop reduces the length
 *    printf( "%d\n", varray_pop( a ) );  // 1235
 *
 *    // elements can be accessed using native C syntax
 *    printf( "%d\n", a[0] );  // 123
 *
 *    printf( "%zu\n", varray_len( a ) );       // 1
 *    printf( "%zu\n", varray_capacity( a ) );  // 2
 *
 *    varray_release( a );
 *
 *  **Note:** when calls to `varray_push` have to increase the capacity (`realloc` under the
 *            hood) the pointer `va` is changed (although not explicit). The user should be
 *            **very careful** when passing a `varray` as a function parameter, because the
 *            pointer might be changed inside the function without the caller knowing it (and
 *            thus be left with a dangling pointer):
 *
 *
 *    int *add_elems( int *va ) {
 *      varray_push( va, 1 );
 *      varray_push( va, 1 );
 *      return va;
 *    }
 *
 *    int *va;
 *    varray_init( va, 1 );
 *
 *    // DON'T!!!
 *    add_elems( va );  // add_elems will likely invalidate the va pointer
 *
 *    // DO
 *    va = add_elems( va );
 *
 */

#ifndef varray_H
#define varray_H

/* include area */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


/*-----------------------------------------------------------------------------
   Library data types
-----------------------------------------------------------------------------*/

/** Var array type. */
typedef struct
{
  /** Size of the internal buffer (in element units). */
  size_t capacity;

  /** Number of elements allocated. */
  size_t len;

  /** Actual buffer of data. */
  uint8_t data[];

} varray_t;


/*-----------------------------------------------------------------------------
   Internal macros
-----------------------------------------------------------------------------*/

/** Compile time check that \a ptr is a pointer type.
 *
 *  \param ptr Pointer.
 */
#define CHECK_PTR( ptr ) ( ( void )( 0 ? &( *( ptr ) ) : NULL ) )

/** Compile time check to test that the elements of the array \c elems are compatible with
 *  \c ptr.
 *
 *  \param ptr Pointer to var array data.
 *  \param elems Array to append to the var array.
 *
 *  \note The test is done by using the ternary operator checks. Both branches of the ternary
 *        operator must return compatible types, or else the compiler throws a warning, so both
 *        \c elems and \c ptr must be pointers to the same type of variable. Then, the result of
 *        the ternary must be assigned to some variable, so it casts a 1 to a pointer to pointer
 *        (it's \c const in case \c elems is too). To avoid this assignment from actually happening,
 *        wraps the whole expression in another ternary so it never executes.
 */
#define CHECK_ELEMS_TYPE( ptr, elems ) \
  ( 0 ? ( ( *( const void ** )1 ) = ( 1 ? ( ptr ) : ( elems ) ) ) : 0 )

/** Compile time check for a pointer to not be of type void.
 *
 *  \param p Pointer to check
 */
#define CHECK_NOT_VOID_PTR( p ) ( ( void )( ( p )[0] ) )

/** Gets the hidden header of a var array. Users will get a pointer to the array data
 *  so we need to subtract the size of the header to get the actual pointer to the
 *  \a varray_t structure.
 *
 *  \param ptr Var array pointer.
 *  \return Pointer to \a varray_t.
 */
#define _va_header( ptr ) \
  ( CHECK_PTR( ptr ), ( varray_t * )( ( uint8_t * )( ptr )-offsetof( varray_t, data ) ) )

/** Returns the array length (number of elements stored).
 *
 *  \param ptr Var array pointer.
 *  \return Length.
 */
#define _va_len( ptr ) ( _va_header( ptr )->len )

/** Returns the array capacity (total number of elements reserved).
 *
 *  \param ptr Var array pointer.
 *  \return Capacity.
 */
#define _va_cap( ptr ) ( _va_header( ptr )->capacity )

/** Sets the capacity to the double of the size.
 *
 *  \param ptr Var array pointer.
 *  \return Var array pointer.
 */
#define _va_double_size( ptr ) \
  ( _varray_resize( _va_header( ptr ), _va_len( ptr ) * 2, sizeof( *ptr ) ) )

/** Doubles the size of the array if it's full.
 *
 *  \param ptr Var array pointer.
 *  \return Pointer to \a varray_t.
 *
 *  \note This function may redefine \a ptr.
 */
#define _resize_if_req( ptr ) \
  ( ( ptr ) = ( _va_cap( ptr ) <= _va_len( ptr ) ) ? _va_double_size( ptr ) : ( ptr ) )


/*-----------------------------------------------------------------------------
   Library interface
-----------------------------------------------------------------------------*/

/** Initializes a var array with reserved space for the desired number of elements.
 *
 *  \param ptr Normal pointer to the type of the var array.
 *  \param n Number of elements to reserve.
 */
#define varray_init( ptr, n ) ( ( ptr ) = _varray_init( sizeof( *( ptr ) ), n ) )

/** Releases a var array initialized by \a varray_init.
 *
 *  \param ptr Pointer used as var array.
 */
#define varray_release( ptr ) ( free( _va_header( ptr ) ), ( ptr ) = NULL )

/** Returns the length of a var array (actual number of elements stored).
 *
 *  \param ptr Pointer used as var array.
 *  \return Var array length.
 *
 *  \note This value may be modified but it shouldn't be increased further than the capacity.
 */
#define varray_len( ptr ) ( _va_len( ptr ) )

/** Returns the capacity of a var array (number of elements that fit in the buffer).
 *
 *  \param ptr Pointer used as var array.
 *  \return Var array capacity.
 *
 *  \note This value must not be modified (use \a varray_resize).
 */
#define varray_capacity( ptr ) ( _va_cap( ptr ) )

/** Sets the capacity of the array.
 *
 *  \param ptr Pointer used as var array.
 *  \param n Number of elements to reserve.
 *
 *  \note \a ptr may be modified by this operation.
 */
#define varray_resize( ptr, n ) \
  ( ( ptr ) = _varray_resize( _va_header( ptr ), n, sizeof( *( ptr ) ) ) )

/** Stores an element in the first unused position increasing the length and the capacity
 *  (if required).
 *
 *  \param ptr Pointer used as var array.
 *  \param elem Element to push.
 *
 *  \note \a ptr may be modified by this operation.
 */
#define varray_push( ptr, elem ) \
  ( _resize_if_req( ptr ), ( ptr )[varray_len( ptr )++] = elem )

/** Stores several elements in the first unused position increasing the length and the capacity
 *  (if required).
 *
 *  \param ptr Pointer used as var array.
 *  \param elems Array of elements to append.
 *  \param n Number of elements in the array \c elems.
 *
 *  \note \a ptr may be modified by this operation.
 */
#define varray_append( ptr, elems, n ) \
  ( CHECK_ELEMS_TYPE( ptr, elems ),       \
    CHECK_NOT_VOID_PTR( elems ),          \
    ( ptr ) = _varray_append( _va_header( ptr ), elems, n, sizeof( *( ptr ) ) ) )

/** Returns the last element in the array and decreases the length.
 *
 *  \param ptr Pointer used as var array.
 *  \return Last element in the array.
 */
#define varray_pop( ptr ) ( ( ptr )[--varray_len( ptr )] )

/** Returns the last element.
 *
 *  \param ptr Pointer used as var array.
 *  \return Last element in the array.
 */
#define varray_last( ptr ) ( ( ptr )[varray_len( ptr ) - 1] )


/*-----------------------------------------------------------------------------
   Internal use
-----------------------------------------------------------------------------*/

void *_varray_init( size_t elem_size, size_t num_elems );
void *_varray_resize( varray_t *va, size_t n, size_t elem_size );
void *_varray_append( varray_t *va, const void *elems, size_t n, size_t elem_size );


#endif
