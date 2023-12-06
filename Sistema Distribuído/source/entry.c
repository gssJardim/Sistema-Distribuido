

#include <stdlib.h>
#include <string.h>
#include "entry.h"
#include "data.h"

struct entry_t* entry_create( char* p_key, struct data_t* p_data )
{
    struct entry_t* p_entry = NULL;
    if ( !(p_entry = (struct entry_t*)malloc( sizeof( struct entry_t ) )) )
        return NULL;

    // Entry structure can have key and data members NULL.
    p_entry->key = p_key;
    p_entry->value = p_data;

    return p_entry;
}

void entry_destroy( struct entry_t* p_entry )
{
    if ( p_entry )
    {
        if ( p_entry->key ) free( p_entry->key );
        data_destroy( p_entry->value );
        free( p_entry );
    }
}

struct entry_t* entry_dup( struct entry_t* p_entry )
{
    if ( !p_entry )
        return NULL;

    struct entry_t* p_entry_copy;
    if ( !(p_entry_copy = (struct entry_t*)malloc( sizeof( struct entry_t ) )) )
        return NULL;

    char* p_key_copy;
    if ( !(p_key_copy = (char*)malloc( (strlen( p_entry->key ) + 1) * sizeof( char ) )) ) return NULL;

    strcpy( p_key_copy, p_entry->key );

    p_entry_copy->key = p_entry->key ? p_key_copy : NULL;
    p_entry_copy->value = data_dup( p_entry->value );

    return p_entry_copy;
}

void entry_replace( struct entry_t* p_entry, char* p_new_key, struct data_t* p_new_value )
{
    // Entry structure can have key and data members NULL.
    if ( !p_entry )
        return;

    // frees old items
    if ( p_entry->key ) free( p_entry->key );
    data_destroy( p_entry->value );

    // replaces items
    p_entry->key = p_new_key;
    p_entry->value = p_new_value;
}

int entry_compare( struct entry_t* p_entry1, struct entry_t* p_entry2 )
{
    // NULL comparator.
    if ( !p_entry1 && p_entry1 == p_entry2 )
        return 0;

    /*
     * NULL key will be an inferior compare value to a non NULL key.
     * Return value is -1 if p_entry1 has a NULL key and p_entry2 has a non NULL key. Return value is 1 if the contrary.
     */

    // One of the datas is NULL.
    if ( !p_entry1->key && p_entry2->key )
        return -1;

    if ( p_entry1->key && !p_entry2->key )
        return 1;

    // Compare datas.
    // strcmp returns 0 if both parameters are NULL
    int compare_value = strcmp( p_entry1->key, p_entry2->key );
    return compare_value == 0 ? compare_value : (compare_value > 0 ? 1 : -1);
}