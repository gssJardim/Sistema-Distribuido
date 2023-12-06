

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>

#include "message-private.h"

size_t read_all( int sockfd, char **pp_buffer )
{
    if ( sockfd == -1 || !pp_buffer )
    {
        errno = EINVAL;
        fprintf(stderr, "%s : at least one of the read_all arguments is not valid.\n", strerror(errno));
        return -1;
    }

    ssize_t num_bytes = 0;
    // Wait for connection to have data to send.
    while ( num_bytes == 0 )
    {
        if ( ioctl( sockfd, FIONREAD, &num_bytes ) < 0 )
        {
            fprintf(stderr, "%s : ioctl() failed.\n", strerror(errno));
            return -1;
        }
    }
    {
        // How much data is available to be received.
        ioctl(sockfd, FIONREAD, &num_bytes );
    }

    if ( !(*pp_buffer = (char *)malloc( sizeof( char ) * (num_bytes + 1) )) )
        return -1;

    (*pp_buffer)[num_bytes] = '\0';

    // Multiple reads until buffer is complete.
    ssize_t num_bytes_read = 0;

    while( num_bytes > 0 )
    {
        ssize_t read_result = read(sockfd, *pp_buffer + num_bytes_read, num_bytes );

        if ( read_result == 0 )
            break;

        if ( read_result < 0 )
        {
            fprintf(stderr, "%s : error reading from server socket.\n", strerror(errno));
            return -1;
        }

        num_bytes_read += read_result;
        num_bytes -= read_result;
    }

    return num_bytes_read;
}

size_t write_all(int sockfd, char *p_buffer, size_t len)
{
    if (sockfd == -1 || !p_buffer)
    {
        errno = EINVAL;
        fprintf(stderr, "%s : at least one of the write_all arguments is not valid.\n", strerror(errno));
        return -1;
    }

    size_t buffer_size = len;

    while (len > 0)
    {
        ssize_t num_bytes = write(sockfd, p_buffer, len);

        if (num_bytes < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "%s : error writing to server socket.\n", strerror(errno));
            return num_bytes;
        }

        p_buffer += num_bytes;
        len -= num_bytes;
    }

    return buffer_size;
}

const char *opcode_name( int opcode_value )
{
#define NAME(OPCODE) case OPCODE: return #OPCODE;
    switch (opcode_value) {
        NAME(OP_BAD)
        NAME(OP_SIZE)
        NAME(OP_HEIGHT)
        NAME(OP_DEL)
        NAME(OP_GET)
        NAME(OP_PUT)
        NAME(OP_GETKEYS)
        NAME(OP_GETVALUES)
        NAME(OP_VERIFY)
        NAME(OP_ERROR)
        default:
            return "UNKNOWN_OPCODE";
    }
#undef NAME
}

const char *ctype_name( int ctype_value )
{
#define NAME(CTYPE) case CTYPE: return #CTYPE;
    switch (ctype_value) {
        NAME(CT_BAD)
        NAME(CT_KEY)
        NAME(CT_VALUE)
        NAME(CT_ENTRY)
        NAME(CT_KEYS)
        NAME(CT_VALUES)
        NAME(CT_RESULT)
        NAME(CT_NONE)
        default:
            return "UNKNOWN_CTYPE";
    }
#undef NAME
}

void print_message( struct message_t *p_msg, unsigned int is_received_msg )
{
    if (!p_msg) return;

    // Print opcode.
    if (is_received_msg)
    {
        printf("\nMessage received: ");
        printf("%s ", opcode_name(p_msg->p_MessageT->opcode));
    } else {
        int opcode = p_msg->p_MessageT->opcode;
        printf("Message sent: ");
        if (opcode == OP_ERROR)
            printf("%s ", opcode_name(opcode));
        else
            printf("%s+1 ", opcode_name(opcode - 1));
    }

    // Print c_type.
    printf("%s ", ctype_name(p_msg->p_MessageT->c_type) );

    int c_type = p_msg->p_MessageT->c_type;

    switch ( c_type )
    {
        case CT_KEY:
            printf("%s", p_msg->p_MessageT->key);
            break;
        case CT_RESULT:
            printf("%d", p_msg->p_MessageT->result);
            break;
        case CT_VALUE:
        {
            ProtobufCBinaryData data_temp = p_msg->p_MessageT->data;
            char *p_str = malloc( sizeof( char ) * (data_temp.len + 1 ) );
            p_str[data_temp.len] = '\0';
            memcpy(p_str,data_temp.data, data_temp.len);
            printf("{datasize: %zu; data: %s}\n", data_temp.len, p_str );
            free(p_str);
            break;
        }
        case CT_ENTRY:
            printf("[key: %s ", p_msg->p_MessageT->entry->key);

            ProtobufCBinaryData data_temp = p_msg->p_MessageT->entry->data;
            char *p_str = malloc( sizeof( char ) * (data_temp.len + 1 ) );
            p_str[data_temp.len] = '\0';
            memcpy(p_str,data_temp.data, data_temp.len);

            printf("{datasize: %zu; data: %s}]", data_temp.len, p_str );

            free(p_str);
            break;
        case CT_KEYS:
        {
            printf("<KEYS_BELOW>\n");
            for( int i = 0; i < p_msg->p_MessageT->n_keys; i++ )
            {
                printf("%s\n", p_msg->p_MessageT->keys[i] );
            }
            break;
        }
        case CT_VALUES:
        {
            printf("<VALUES_BELOW>\n");
            for( int i = 0; i < p_msg->p_MessageT->n_datas; i++ )
            {
                ProtobufCBinaryData *p_data_temp = &p_msg->p_MessageT->datas[i];

                char *p_str = malloc( sizeof( char ) * (p_data_temp->len + 1 ) );
                p_str[p_data_temp->len] = '\0';
                memcpy(p_str, p_data_temp->data, p_data_temp->len);
                printf("{datasize: %zu; data: %s}\n", p_data_temp->len, p_str );
                free(p_str);
            }
            break;
        }
        default:
            break;
    }

    printf("\n");
}
