
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>

#include "tree.h"
#include "tree_skel.h"
#include "tree_skel-private.h"
#include "sdmessage.pb-c.h"
#include "message-private.h"

// Server tree.
struct tree_t *gp_TREE;

// Threads
int g_n_threads;
pthread_t *gp_threads_ids;

int g_are_threads_running = 1;

// Mutexes.
pthread_mutex_t g_tree_lock, g_queue_lock, g_op_proc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_queue_not_empty_cond = PTHREAD_COND_INITIALIZER;

// Writing operation counters.
int g_last_assignment = 1;

// Queue head.
struct request_t *gp_queue_head = NULL;

// op_proc
struct op_proc *gp_op_proc = NULL;

void request_queue_sigint_handler()
{
    g_are_threads_running = 0;

    // We will lock and clean the queue.
    pthread_mutex_lock( &g_queue_lock );

    while( gp_queue_head != NULL )
    {
        struct request_t *p_request = gp_queue_head;
        gp_queue_head = gp_queue_head->p_next;
        request_destroy( p_request );
    }


}

void *process_request( void *p_params )
{
    int thread_id = *(int *)p_params;

    while ( g_are_threads_running )
    {
        struct request_t *p_request = queue_get_next_request();

        // This is only supposed to happen when the server and, consequently, the thread is shutting down. Since in
        // normal occasions, the thread waits untils there is a request in the queue.
        if ( p_request == NULL )
        {
            break;
        }

        // Modify op_proc with the op number of the request being processed by this thread.
        op_proc_set_in_progress( gp_op_proc, thread_id, p_request->op_n );

        int result = -1;

        // Process request.
        if ( p_request->op == REQUEST_PUT )
        {
            struct data_t *p_data = data_create2( p_request->p_data->datasize, p_request->p_data->data );

            pthread_mutex_lock( &g_tree_lock );
            result = tree_put( gp_TREE, p_request->p_key, p_request->p_data );
            pthread_mutex_unlock( &g_tree_lock );

            free( p_data );
        } else
        {
            pthread_mutex_lock( &g_tree_lock );
            result = tree_del( gp_TREE, p_request->p_key );
            pthread_mutex_unlock( &g_tree_lock );
        }

        // Marks this request as finished in the op_proc struct.
        op_proc_set_in_progress( gp_op_proc, thread_id, 0 );

        if ( p_request  != NULL )
            printf( "\nThread %d has finished op_n %d", thread_id, p_request->op_n );

        // If the request op was bigger that op_proc->max_proc, we need to update the max_proc.
        if ( p_request->op_n > op_proc_get_max_proc( gp_op_proc ) )
        {
            printf( " and changed the max_proc variable." );
            op_proc_set_max_proc( gp_op_proc, p_request->op_n );
        }

        printf("\n\n");

        request_destroy( p_request );
    }

    return NULL;
}

int tree_skel_init( int n_threads )
{
    // For the sigint handler.
    g_n_threads = n_threads;

    if ( !(gp_TREE = tree_create()))
    {
        fprintf( stderr, "%s : error creating the tree.\n", strerror(errno));
        return -1;
    }

    int i;

    if ( !(gp_threads_ids = (pthread_t *) malloc( n_threads * sizeof( pthread_t ))))
    {
        fprintf( stderr, "%s : error allocating memory for the threads.\n", strerror(errno));
        tree_skel_destroy();
        return -1;
    }

    if ( !(gp_op_proc = op_proc_create( n_threads )))
    {
        fprintf( stderr, "Error creating the op_proc struct.\n" );
        tree_skel_destroy();
        tree_skel_request_queue_threads_destroy();
        return -1;
    }

    for ( i = 0; i < n_threads; i++ )
    {
        if ( pthread_create( &gp_threads_ids[i], NULL, process_request, (void *) &i ))
        {
            fprintf( stderr, "%s : error creating thread.\n", strerror(errno));
            free( gp_threads_ids );
            return -1;
        }

        pthread_detach( gp_threads_ids[i] );
    }

    return 0;
}

void tree_skel_request_queue_threads_destroy( )
{
    pthread_mutex_unlock( &g_queue_lock );

    // Stop threads from running.
    g_are_threads_running = 0;

    // Unlock stuck threads waiting for non empty queue.
    pthread_cond_broadcast( &g_queue_not_empty_cond );
}

void tree_skel_destroy()
{

    tree_destroy( gp_TREE );
    op_proc_destroy( gp_op_proc );
}


int imvoke( struct message_t *p_msg )
{
    if ( !p_msg )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : argument p_msg cannot be NULL.\n", strerror(errno));
        return -1;
    }

    // Tree not initialized.
    if ( !gp_TREE )
    {
        errno = ENODATA;
        fprintf( stderr, "%s : tree was not initialized.\n", strerror(errno));
        return -1;
    }

    // Default response values.
    p_msg->p_MessageT->c_type = CT_NONE;

    // Flag for if the operation was executed with success.
    int has_succeeded = 0;

    // Chooses operation and executes it.
    switch ( p_msg->p_MessageT->opcode )
    {
        case OP_SIZE:
        {

            p_msg->p_MessageT->c_type = CT_RESULT;

            pthread_mutex_lock( &g_tree_lock );
            p_msg->p_MessageT->result = tree_size( gp_TREE );
            pthread_mutex_unlock( &g_tree_lock );

            has_succeeded = 1;
            break;
        }
        case OP_HEIGHT:
        {
            p_msg->p_MessageT->c_type = CT_RESULT;

            pthread_mutex_lock( &g_tree_lock );
            int height = tree_height( gp_TREE );
            pthread_mutex_unlock( &g_tree_lock );

            p_msg->p_MessageT->result = height;

            has_succeeded = 1;
            break;
        }
        case OP_DEL:
        {
            struct request_t *p_request = request_create(
                    g_last_assignment,
                    REQUEST_DEL,
                    p_msg->p_MessageT->key,
                    NULL );

            queue_add_request( p_request );

            p_msg->p_MessageT->c_type = CT_RESULT;
            p_msg->p_MessageT->result = g_last_assignment;
            g_last_assignment++;

            has_succeeded = 1;
            break;
        }

        case OP_GET:
        {
            p_msg->p_MessageT->c_type = CT_VALUE;

            pthread_mutex_lock( &g_tree_lock );
            // Get the value (if NULL, it's not an error).
            struct data_t *p_data_from_tree = tree_get( gp_TREE, p_msg->p_MessageT->key );
            pthread_mutex_unlock( &g_tree_lock );

            ProtobufCBinaryData data_temp;

            if ( p_data_from_tree )
            {
                data_temp.len = p_data_from_tree->datasize;

                // Copy data to message data.
                if ( !(data_temp.data = (void *) malloc( sizeof( void ) * data_temp.len )))
                {
                    data_destroy( p_data_from_tree );
                    break;
                }

                memcpy( data_temp.data, p_data_from_tree->data, data_temp.len );
                data_destroy( p_data_from_tree );
            }
                // Key was not found.
            else
            {
                data_temp.len = 0;
                data_temp.data = NULL;
            }

            p_msg->p_MessageT->data = data_temp;
            has_succeeded = 1;
            break;
        }
        case OP_PUT:
        {
            struct data_t* p_data = data_create( (int)p_msg->p_MessageT->entry->data.len );
            memcpy( p_data->data, p_msg->p_MessageT->entry->data.data, p_data->datasize );

            struct request_t *p_request = request_create(
                    g_last_assignment,
                    REQUEST_PUT,
                    p_msg->p_MessageT->entry->key,
                    p_data );

            data_destroy( p_data );
            queue_add_request( p_request );

            p_msg->p_MessageT->c_type = CT_RESULT;
            p_msg->p_MessageT->result = g_last_assignment;
            g_last_assignment++;

            has_succeeded = 1;
            break;
        }
        case OP_GETKEYS:
        {
            p_msg->p_MessageT->c_type = CT_KEYS;

            pthread_mutex_lock( &g_tree_lock );
            int num_keys = tree_size( gp_TREE );
            char **pp_keys_temp = tree_get_keys( gp_TREE );
            pthread_mutex_unlock( &g_tree_lock );

            p_msg->p_MessageT->n_keys = num_keys;
            p_msg->p_MessageT->keys = malloc( sizeof( char * ) * num_keys );

            for ( int i = 0; i < num_keys; i++ )
            {
                p_msg->p_MessageT->keys[i] = pp_keys_temp[i];
            }

            free( pp_keys_temp );
            has_succeeded = 1;
            break;
        }
        case OP_GETVALUES:
        {
            p_msg->p_MessageT->c_type = CT_VALUES;

            pthread_mutex_lock( &g_tree_lock );
            int num_values = tree_size( gp_TREE );
            void **pp_values_temp = tree_get_values( gp_TREE );
            pthread_mutex_unlock( &g_tree_lock );

            p_msg->p_MessageT->n_datas = num_values;
            p_msg->p_MessageT->datas = malloc( sizeof( ProtobufCBinaryData ) * num_values );

            for ( int i = 0; i < num_values; i++ )
            {
                ProtobufCBinaryData data_temp;
                struct data_t *p_data_temp = ((struct data_t *) pp_values_temp[i]);
                data_temp.len = p_data_temp->datasize;
                data_temp.data = malloc( data_temp.len * sizeof( u_int8_t ));

                memcpy((&data_temp)->data, p_data_temp->data, p_data_temp->datasize );

                p_msg->p_MessageT->datas[i] = data_temp;
            }

            tree_free_values( pp_values_temp);
            has_succeeded = 1;
            break;
        }
        case OP_VERIFY:
        {
            int op_n = (int)p_msg->p_MessageT->result;

            // Operation number was not assigned.
            if ( op_n < 1 || op_n >= g_last_assignment )
            {
                break;
            }


            p_msg->p_MessageT->c_type = CT_RESULT;
            p_msg->p_MessageT->result = verify( op_n );
            g_last_assignment++;

            has_succeeded = 1;
            break;
        }
        default:
            break;
    }

    // Change opcode according to the success flag
    p_msg->p_MessageT->opcode = has_succeeded ? ++(p_msg->p_MessageT->opcode) : OP_ERROR;

    return 0;
}

int verify(int op_n) {
    if ( op_n >= g_last_assignment )
        return -1;

    int result = -1;
    if ( op_n <= op_proc_get_max_proc( gp_op_proc ) )
    {
        // Check if operation is still in progress.
        pthread_mutex_lock( &g_op_proc_lock );
        for ( int i = 0; i < gp_op_proc->in_progress_size; i++ )
        {
            if ( gp_op_proc->p_in_progress[i] == op_n )
            {
                break;
            }
        }
        pthread_mutex_unlock( &g_op_proc_lock );

        result = 0;
    }

    return result;
}

void queue_add_request( struct request_t *p_request )
{
    pthread_mutex_lock( &g_queue_lock );

    // Head is new request if queue is empty.
    if ( !gp_queue_head )
    {
        gp_queue_head = p_request;
        p_request->p_next = NULL;
    }
        // Add request to the end of the queue.
    else
    {
        struct request_t *p_request_itr = gp_queue_head;
        while ( p_request_itr->p_next )
        {
            p_request_itr = p_request_itr->p_next;
        }

        p_request_itr->p_next = p_request;
        p_request->p_next = NULL;
    }

    pthread_cond_signal( &g_queue_not_empty_cond );
    pthread_mutex_unlock( &g_queue_lock );
}

struct request_t *queue_get_next_request()
{
    pthread_mutex_lock( &g_queue_lock );

    // Wait until queue is not empty.
    while ( !gp_queue_head )
    {
        pthread_cond_wait( &g_queue_not_empty_cond, &g_queue_lock );
    }

    struct request_t *p_request = gp_queue_head;
    gp_queue_head = gp_queue_head->p_next;

    pthread_mutex_unlock( &g_queue_lock );

    return p_request;
}

struct request_t *request_create( int op_n, int op, char *p_key, struct data_t *p_data )
{
    struct request_t *p_request = (struct request_t *) malloc( sizeof( struct request_t ));

    p_request->op_n = op_n;
    p_request->op = op;
    p_request->p_key = strdup( p_key );
    p_request->p_data = data_dup( p_data );
    p_request->p_next = NULL;

    return p_request;
}

void request_destroy( struct request_t *p_request )
{
    free( p_request->p_key );
    data_destroy( p_request->p_data );
    free( p_request );
}

struct op_proc *op_proc_create( int in_progress_size )
{
    struct op_proc *p_op_proc = (struct op_proc *) malloc( sizeof( struct op_proc ));

    p_op_proc->max_proc = 0;
    p_op_proc->in_progress_size = in_progress_size;
    p_op_proc->p_in_progress = (int *) calloc( sizeof( int ),  in_progress_size );

    return p_op_proc;
}

void op_proc_destroy( struct op_proc *p_op_proc )
{
    free( p_op_proc->p_in_progress );
    free( p_op_proc );
}

int op_proc_get_max_proc( struct op_proc *p_op_proc )
{
    int result;
    pthread_mutex_lock( &g_op_proc_lock );
    result = p_op_proc->max_proc;
    pthread_mutex_unlock( &g_op_proc_lock );
    return result;
}

void op_proc_set_max_proc( struct op_proc *p_op_proc, int max_proc )
{
    pthread_mutex_lock( &g_op_proc_lock );
    p_op_proc->max_proc = max_proc;
    pthread_mutex_unlock( &g_op_proc_lock );
}

int op_proc_set_in_progress( struct op_proc *p_op_proc, int index, int op_n )
{
    if ( p_op_proc == NULL || p_op_proc->p_in_progress == NULL || index < 0 || index >= p_op_proc->in_progress_size )
    {
        return -1;
    }

    pthread_mutex_lock( &g_op_proc_lock );
    p_op_proc->p_in_progress[index] = op_n;
    pthread_mutex_unlock( &g_op_proc_lock );

    return 0;
}