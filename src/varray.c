/**
 *  \file
 *  Variable array - Implementation.
 */

/* include area */
#include "varray.h"
#include <string.h>


/** Returns an initialized var array. The trick is that this function will
 *  return a pointer to the array data and not to the var array structure,
 *  but the array structre can still be accessed by subtracting the size of
 *  the structure from the pointer.
 *
 *  \param elem_size Size of the elements stored in the var array.
 *  \param num_elems Number of initial elements
 *  \return Pointer to the user data.
 */
void *_varray_init( size_t elem_size, size_t num_elems )
{
  if( num_elems == 0 )
    num_elems = 1;

  varray_t *va = malloc( sizeof( varray_t ) + elem_size * num_elems );
  va->capacity = num_elems;
  va->len = 0;

  /* returns a pointer to the user data, not the array structure */
  return va->data;
}


/** Sets the capacity of the var array.
 *
 *  \param va Var array to resize.
 *  \param n New capacity (number of elements to reserve in the buffer).
 *  \param elem_size Size of the elements.
 *  \return Pointer to the user data.
 */
void *_varray_resize( varray_t *va, size_t n, size_t elem_size )
{
  va = realloc( va, sizeof( varray_t ) + n * elem_size );
  va->capacity = n;
  return va->data;
}


/** Appends an array of items at the end of the var array.
 *
 *  \param va Var array.
 *  \param elems Array of elements to append.
 *  \param n Number of elements in the input array.
 *  \param elem_size Size of each element.
 *  \return Pointer to the user data.
 */
void *_varray_append( varray_t *va, const void *elems, size_t n, size_t elem_size )
{
  /* calculates the new required capacity */
  size_t required_cap = va->capacity;
  while( required_cap < va->len + n )
    required_cap *= 2;

  /* grows if necessary */
  if( required_cap > va->capacity )
  {
    va = realloc( va, sizeof( varray_t ) + required_cap * elem_size );
    va->capacity = required_cap;
  }

  /* appends the elements */
  memcpy( va->data + elem_size * va->len, elems, elem_size * n );
  va->len += n;

  return va->data;
}
