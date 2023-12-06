// Grupo 55
// Jose Alves nº 44898
// Gustavo Jardim nº 48483
// Henrique Lopes nº 52840

#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "sdmessage.pb-c.h"

// Wrapper for MessageT.
struct message_t {
    MessageT *p_MessageT;
};

// The message OPCODES.
#define OP_BAD          0
#define OP_SIZE         10
#define OP_HEIGHT       20
#define OP_DEL          30
#define OP_GET          40
#define OP_PUT          50
#define OP_GETKEYS      60
#define OP_GETVALUES    70
#define OP_VERIFY       80
#define OP_ERROR        99

// Response message value type code.
#define CT_BAD          0
#define CT_KEY          10
#define CT_VALUE        20
#define CT_ENTRY        30
#define CT_KEYS         40
#define CT_VALUES       50
#define CT_RESULT       60
#define CT_NONE         70

///**
// * Read an entire string from a network socket.
// *
// * Parameters:
// *      sockfd: socket descriptor.
// *      pp_buffer: pointer to buffer where the string is saved.
// *
// * Returns:
// *      The size of the string received; -1 if an error occurred.
// */
//size_t  read_all( int sockfd, char *p_buffer, size_t size );

/**
 * Read an entire string from a network socket.
 *
 * Parameters:
 *      sockfd: socket descriptor.
 *      pp_buffer: pointer to buffer where the string is saved.
 *
 * Returns:
 *      The size of the string received; -1 if an error occurred.
 */
size_t  read_all( int sockfd, char **p_buffer );

/**
 * Send an entire string through the network socket.
 *
 * Parameters:
 *      sockfd: socket descriptor.
 *      p_buffer: buffer with the string to send.
 *      len: size of the buffer.
 *
 * Returns:
 *      -1 if an error occurred; 0 otherwise.
 */
size_t write_all( int sockfd, char *p_buffer, size_t len );

/**
 * Get the OPCODE constant variable name as string with the value given.
 *
 * Parametros:
 *      opcode_value: value of the opcode string to obtain.
 *
 * Returns:
 *      opcode name as string; NULL if opcode does not exist.
 */
const char *opcode_name( int opcode_value );

/**
 * Get the CTYPE constant variable name as string with the value given.
 *
 * Parametros:
 *      ctype_value: value of the ctype string to obtain.
 *
 * Returns:
 *      ctype name as string; NULL if ctype does not exist.
 */
const char *ctype_name( int ctype_value );

/**
 * Print a message.
 *
 * Parameters:
 *      p_msg: message to print.
 *      is_received_msg: >0 if its a received msg; 0 otherwise.
 */
void print_message( struct message_t *p_msg, unsigned int is_received_msg );

#endif
