// Grupo 55
// Jose Alves nº 44898
// Gustavo Jardim nº 48483
// Henrique Lopes nº 52840

#ifndef _TREE_SKEL_PRIVATE_H
#define _TREE_SKEL_PRIVATE_H

#include <malloc.h>
#include "stddef.h"

#define REQUEST_DEL 0
#define REQUEST_PUT 1

struct op_proc
{
    int max_proc;
    size_t in_progress_size;
    int *p_in_progress;
};

/*
 * Struct that represents a request.
 *
 * Parameters:
 *      op_n: numero da operacao.
 *      op: a operação a executar. op = 0 se for um delete, op = 1 se for um put.
 *      p_key: a chave a remover ou adicionar.
 *      p_data: os dados a adicionar em caso de put, ou NULL em caso de delete.
 *      p_next: a proxima tarefa na fila de tarefas.
 */
struct request_t
{
    int op_n;
    int op;
    char *p_key;
    struct data_t *p_data;
    struct request_t *p_next;
};

/*
 * Termination when a SIGINT is received.
 */
void request_queue_sigint_handler();

/*
 * Cancels/destroys the request queue and thread variables associated.
 *
 * Parameters:
 *    n_threads: number of threads to destroy.
 */
void tree_skel_request_queue_threads_destroy();

/*
 * Initializes the op_proc struct, allocating all the memory necessary.
 *
 * Parameters:
 *   in_progress_size: size of the in_progress array.
 *
 * Returns:
 *    NULL if an error occurred, or a pointer to the op_proc struct.
 */
struct op_proc *op_proc_create(int in_progress_size);

/*
 * Destroys a op_proc struct, freeing all the memory it occupies.
 *
 * Parameters:
 *      p_op_proc: op_proc struct to be destroyed.
 */
void op_proc_destroy(struct op_proc *p_op_proc);

/*
 * Gets the maximum processed request from the op_proc structure.
 *
 * Parameters:
 *     p_op_proc: op_proc struct to get the maximum processed request from.
 *
 * Returns:
 *    The maximum processed request.
 */
int op_proc_get_max_proc(struct op_proc *p_op_proc);

/*
 * Sets the maximum processed request from the op_proc structure.
 *
 * Parameters:
 *      p_op_proc: op_proc struct to set the maximum processed request from.
 *      max_proc: the maximum processed request.
 *
 *  Returns:
 *     The maximum processed request.
 */
void op_proc_set_max_proc(struct op_proc *p_op_proc, int max_proc);

/*
 * Checks if a request is in progress from the op_proc structure.
 *
 * Parameters:
 *     p_op_proc: op_proc struct to check if a request is in progress from.
 *     op_n: the request number to check if it is in progress.
 */
int op_proc_is_in_progress(struct op_proc *p_op_proc, int op_n);

/*
 * Sets a request as in progress from the op_proc structure.
 *
 * Parameters:
 *     p_op_proc: op_proc struct to set a request as in progress from.
 *     index: the index in in_progress array to set new value.
 *     op_n: the request number to set as in progress.
 *
 * Returns:
 *    0 if success, -1 otherwise.
 */
int op_proc_set_in_progress( struct op_proc* p_op_proc, int index, int op_n );

/*
 * Gets the index of the first index in the_progress array, in which there is currently no request being processed.
 * Values with no requests being processed are set to 0.
 * If there is no such index, returns -1.
 *
 * Parameters:
 *    p_op_proc: op_proc struct to get the index from.
 *
 * Returns:
 *   The index of the first index in the_progress array, in which there is currently no request being processed.
 */
int op_proc_get_first_available_in_progress_index(struct op_proc *p_op_proc);

/*
 * Gets a request op_n from the in_progress array.
 *
 * Parameters:
 *    p_op_proc: op_proc struct to get a request op_n from.
 *    index: the index in in_progress array to get the value.
 *
 *  Returns:
 *      The request op_n. Else, -1 if an error occurred.
 */
int op_proc_get_in_progress( struct op_proc* p_op_proc, int index );

/*
 * Initializes the request struct, allocating all the memory necessary.
 *
 * Copies are made of the parameters.
 *
 * Returns:
 *    NULL if an error occurred, or a pointer to the request struct.
 */
struct request_t* request_create( int op_n, int op, char *p_key, struct data_t *p_data );

/*
 * Destroys a structure corresponding to a request freeing all the memory it occupies.
 *
 * Parameters:
 *      p_request: request to be destroyed.
 */
void request_destroy( struct request_t *p_request );

/*
 * Adds a request to the queue of requests.
 *
 * Parameters:
 *      p_request: request to add to the queue of requests.
 */
void queue_add_request( struct request_t *p_request );

/*
 * Gets the next request in the queue of requests.
 *
 * Returns:
 *      head request from the queue of requests.
 */
struct request_t *queue_get_next_request();

struct message_t;

/*
 * It is like the invoke() function, but with the method struct message_t exposed.
 * It was giving conflicting type errors when trying to use the normal invoke() function.
 */
int imvoke( struct message_t *p_msg );

#endif
