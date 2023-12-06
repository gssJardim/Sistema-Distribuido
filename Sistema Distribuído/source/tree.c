
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "tree-private.h"
#include "entry.h"

struct tree_t* tree_create()
{
    struct tree_t* p_new_tree = NULL;

    if ( !(p_new_tree = (struct tree_t*)malloc( sizeof( struct tree_t ) )) )
        return NULL;

    p_new_tree->size = 0;
    p_new_tree->p_root = NULL;

    return p_new_tree;
}

void tree_destroy( struct tree_t* p_tree )
{
    if ( !p_tree )
        return;

    if ( p_tree->p_root )
        node_destroy_recursive( p_tree->p_root );


    free( p_tree );
}

struct node_t* node_create( struct entry_t* p_entry )
{
    struct node_t* p_node = NULL;

    if ( !(p_node = (struct node_t*)malloc( sizeof( struct node_t ) )) )
        return NULL;

    p_node->p_entry = p_entry;
    p_node->p_left = NULL;
    p_node->p_right = NULL;

    return p_node;
}

void node_destroy( struct node_t* p_node )
{
    if ( p_node )
    {
        if ( p_node->p_entry )
            entry_destroy( p_node->p_entry );

        free( p_node );
    }
}

void node_destroy_recursive( struct node_t* p_node )
{
    if ( !p_node )
        return;

    if ( !p_node->p_entry )
        entry_destroy( p_node->p_entry );

    node_destroy_recursive( p_node->p_left );
    node_destroy_recursive( p_node->p_right );

    free( p_node );
}

int tree_put( struct tree_t* p_tree, char* p_key, struct data_t* p_value )
{
    if ( !p_tree || !p_key || !p_value )
        return -1;

    char* p_key_dup = strdup( p_key );
    struct data_t* p_value_copy = data_dup( p_value );

    struct node_t** pp_next_node = &p_tree->p_root;
    struct node_t* p_current_node = p_tree->p_root;

    while ( p_current_node )
    {
        int compare_value = strcmp( p_key_dup, p_current_node->p_entry->key );

        // Left branch.
        if ( compare_value < 0 )
        {
            pp_next_node = &p_current_node->p_left;
            p_current_node = p_current_node->p_left;
        }
            // Right branch.
        else if ( compare_value > 0 )
        {
            pp_next_node = &p_current_node->p_right;
            p_current_node = p_current_node->p_right;
        }
            // Node with same key.
        else
        {
            entry_replace( p_current_node->p_entry, p_key_dup, p_value_copy );
            return 0;
        }
    }

    // No node found. Create new one.
    struct node_t* p_new_node = NULL;

    if ( !(p_new_node = node_create( entry_create( p_key_dup, p_value_copy ) )) )
        return -1;

    *pp_next_node = p_new_node;
    p_tree->size++;
    return 0;
}


struct data_t* tree_get( struct tree_t* p_tree, char* p_key )
{
    if ( !p_tree || !p_key )
        return NULL;

    struct node_t* p_current_node = p_tree->p_root;

    while ( p_current_node )
    {
        int compare_value = strcmp( p_key, p_current_node->p_entry->key );

        // Left branch.
        if ( compare_value < 0 )
        {
            p_current_node = p_current_node->p_left;
        }

            // Right branch.
        else if ( compare_value > 0 )
        {
            p_current_node = p_current_node->p_right;
        }
            // Found node.
        else
        {
            return data_dup( p_current_node->p_entry->value );
        }
    }

    // Didn't find node.
    struct data_t* p_returned_data = data_create2( 0, NULL );
    return p_returned_data;
}

int tree_del( struct tree_t* p_tree, char* p_key )
{
    if ( !p_tree || !p_key || !p_tree->p_root )
        return -1;

    size_t tree_size_before = p_tree->size;
    p_tree->p_root = tree_del_node( p_tree, p_tree->p_root, p_key );

    // Tree size didn't change. Node wasn't found/error.
    if ( tree_size_before == p_tree->size )
        return -1;

    return 0;
}

struct node_t* tree_del_node( struct tree_t* p_tree, struct node_t* p_node, char* p_key )
{
    if ( !p_tree || !p_node || !p_key )
        return NULL;

    int compare_value = strcmp( p_key, p_node->p_entry->key );

    // Left branch.
    if ( compare_value < 0 )
    {
        p_node->p_left = tree_del_node( p_tree, p_node->p_left, p_key );
    }
        // Right branch.
    else if ( compare_value > 0 )
    {
        p_node->p_right = tree_del_node( p_tree, p_node->p_right, p_key );
    }
        // Found key.
    else
    {
        p_tree->size--;

        // No children.
        if ( !p_node->p_left && !p_node->p_right )
        {
            node_destroy( p_node );
            return NULL;
        }

        // One child.
        if ( !p_node->p_left || !p_node->p_right )
        {
            struct node_t* p_node_aux;
            p_node_aux = p_node->p_left == NULL ? p_node->p_right : p_node->p_left;

            node_destroy( p_node );
            return p_node_aux;
        }

        // Has two children.
        // Get the node of the lowest value key on the right branch of the current node.
        struct node_t* p_minimum_node = p_node->p_right;

        while ( p_minimum_node && p_minimum_node->p_left != NULL )
            p_minimum_node = p_minimum_node->p_left;

        // Substitute the entry of the current node with the node found on the cicle above.
        char* p_key_copy = strdup( p_minimum_node->p_entry->key );
        struct data_t* p_data_copy = data_dup( p_minimum_node->p_entry->value );
        entry_replace( p_node->p_entry, p_key_copy, p_data_copy );

        p_tree->size++;
        p_node->p_right = tree_del_node( p_tree, p_node->p_right, p_minimum_node->p_entry->key );
    }

    return p_node;
}

int tree_size( struct tree_t* p_tree )
{
    if ( !p_tree )
        return -1;

    return (int)p_tree->size;
}

int tree_height( struct tree_t* p_tree )
{
    if ( !p_tree )
        return 0;

    return tree_height_recursive( p_tree->p_root );
}

int tree_height_recursive( struct node_t* p_node )
{
    if ( !p_node )
        return 0;

    int left_branch = tree_height_recursive( p_node->p_left );
    int right_branch = tree_height_recursive( p_node->p_right );

    return left_branch > right_branch ? left_branch + 1 : right_branch + 1;
}

char** tree_get_keys( struct tree_t* p_tree )
{
    if ( !p_tree )
        return NULL;

    size_t size = p_tree->size;
    char** pp_keys = (char**)calloc( sizeof( char* ), (size + 1) );
    pp_keys[size] = NULL;

    int index = 0;
    int* p_index = &index;

    inorder_append_keys( p_tree->p_root, pp_keys, p_index );

    return pp_keys;
}

void** tree_get_values( struct tree_t* p_tree )
{
    if ( !p_tree )
        return NULL;


    size_t size = p_tree->size;
    void** pp_values = (void**)calloc( sizeof( void* ), (size + 1) );
    pp_values[size] = NULL;

    int index = 0;
    int* p_index = &index;

    inorder_append_values( p_tree->p_root, pp_values, p_index );

    return pp_values;
}

void inorder_append_keys( struct node_t* p_node, char** pp_keys, int* p_index )
{
    if ( !p_node )
        return;

    inorder_append_keys( p_node->p_left, pp_keys, p_index );

    pp_keys[*p_index] = strdup( p_node->p_entry->key );
    (*p_index)++;

    inorder_append_keys( p_node->p_right, pp_keys, p_index );
}

void inorder_append_values( struct node_t* p_node, void** pp_values, int* p_index )
{
    if ( !p_node )
        return;

    inorder_append_values( p_node->p_left, pp_values, p_index );

    pp_values[*p_index] = data_dup( p_node->p_entry->value );

    (*p_index)++;

    inorder_append_values( p_node->p_right, pp_values, p_index );
}

void tree_free_keys( char** pp_keys )
{
    if ( !pp_keys )
        return;

    int i = 0;
    while ( pp_keys[i] )
    {
        free( pp_keys[i] );
        i++;
    }

    free( pp_keys );
}

void tree_free_values( void** pp_values )
{
   if ( !pp_values )
       return;

   int i = 0;
   while ( pp_values[i] )
   {
       data_destroy( pp_values[i] );
       i++;
   }

   free( pp_values );
}