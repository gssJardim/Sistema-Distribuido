

#include <stdlib.h>
#include <string.h>
#include "data.h"

struct data_t* data_create( int data_size )
{
    // Data size can't be zero or negative.
    if ( data_size <= 0 )
        return NULL;

    // Allocate memory for new data_t.
    struct data_t* p_data = NULL;
    if ( !(p_data = (struct data_t*)malloc( sizeof( struct data_t ) )) )
        return NULL;

    // Initialize data_t structure items.
    p_data->datasize = data_size;

    // All bits initially to zero with calloc.
    if ( !(p_data->data = calloc( sizeof( void ), data_size )) )
    {
        free( p_data );
        return NULL;
    }

    return p_data;
}

struct data_t* data_create2( int data_size, void* p_data )
{
    if ( data_size <= 0 || !p_data )
        return NULL;

    struct data_t* p_new_data = NULL;
    if ( !(p_new_data = (struct data_t*)malloc( sizeof( struct data_t ) )) )
        return NULL;

    p_new_data->datasize = data_size;
    p_new_data->data = p_data;

    return p_new_data;
}

void data_destroy( struct data_t* p_data )
{
    if ( p_data )
    {
        if ( p_data->data ) free( p_data->data );
        free( p_data );
    }
}

struct data_t* data_dup( struct data_t* p_data )
{
    if ( !p_data || p_data->datasize <= 0 || !p_data->data )
        return NULL;

    struct data_t* p_data_dup = NULL;
    if ( !(p_data_dup = (struct data_t*)malloc( sizeof( struct data_t ) )) )
        return NULL;

    /* Duplicate items. */
    p_data_dup->datasize = p_data->datasize;

    if ( !(p_data_dup->data = malloc( sizeof( void ) * p_data_dup->datasize )) ) {
        free( p_data_dup );
        return NULL;
    }

    memcpy( p_data_dup->data, p_data->data, p_data->datasize );

    return p_data_dup;
}

void data_replace( struct data_t* p_data, int new_data_size, void* p_new_data )
{
    if ( !p_data || new_data_size <= 0 )
        return;

    p_data->datasize = new_data_size;
    if ( p_data->data) free( p_data->data );
    p_data->data = p_new_data;
}