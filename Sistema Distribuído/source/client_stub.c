

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errno.h"

#include "network_client.h"
#include "message-private.h"
#include "client_stub-private.h"
#include "shared-private.h"

struct rtree_t *rtree_connect( const char *p_address_port )
{
    if ( !p_address_port )
    {
        errno = EINVAL;
        fprintf( stderr, "%s: argument address_port is null.\n", strerror( errno ) );
        return NULL;
    }

    char *p_address_port_copy = NULL; // copy of the argument
    char *p_addr, *p_port;
    short parsed_port;
    struct rtree_t *p_rtree = NULL; // contains the variables needed for the connection between server and client

    // Get address and port.
    if ( !(p_address_port_copy = (char *)malloc( (strlen( p_address_port ) + 1) * sizeof( char ) )) )
    {
        fprintf( stderr, "%s: it was not possible to malloc().\n", strerror( errno ) );
        return NULL;
    }

    strcpy( p_address_port_copy, p_address_port );

    // Separate address and port to each variable.
    p_addr = strtok( p_address_port_copy, ":" );
    p_port = strtok( NULL, ":" );

    if ( !p_addr || !p_port )
    {
        errno = EINVAL;
        fprintf( stderr, "%s: invalid <address>:port argument: %s", strerror( errno ), p_address_port_copy );
        goto error_clean;
    }

    // Check if port is valid.
    if ( (parsed_port = parse_port( p_port )) < 0 )
    {
        goto error_clean;
    }

    // Initialize the tree
    if ( !(p_rtree = (struct rtree_t *)malloc(sizeof( struct rtree_t ) )) )
    {
        fprintf( stderr, "%s: it was not possible to malloc().\n", strerror( errno ) );
        goto error_clean;
    }
    if ( !(p_rtree->p_sockaddr = (struct sockaddr_in *)malloc(sizeof( struct sockaddr_in ) )) )
    {
        fprintf( stderr, "%s: it was not possible to malloc().\n", strerror( errno ) );
        goto error_clean;
    }

    // Check if server addr is valid.
    if (inet_pton( AF_INET, p_addr, &p_rtree->p_sockaddr->sin_addr ) < 1 )
    {
        errno = EAFNOSUPPORT;
        fprintf( stderr, "%s : error converting IP address.\n", strerror( errno ) );
        goto error_clean;
    }

    p_rtree->p_sockaddr->sin_family = AF_INET;
    p_rtree->p_sockaddr->sin_port = htons(parsed_port );

    // Connect to server.
    if (network_connect(p_rtree ) < 0 )
    {
        fprintf( stderr, "%s : could not connect to server.\n", strerror( errno ) );
        goto error_clean;
    }

    // Clean now unneeded function argument copy.
    free( p_address_port_copy );

    return p_rtree;

    // Cleans memory and returns NULL (error) value.
    error_clean:

    free( p_address_port_copy );

    if ( p_rtree )
    {
        if ( p_rtree->p_sockaddr ) free(p_rtree->p_sockaddr );
        free(p_rtree );
    }

    return NULL;
}

int rtree_disconnect( struct rtree_t *p_rtree )
{
    if ( !p_rtree )
    {
        errno = EINVAL;
        fprintf( stderr, "%s: p_rtree argument is null .\n", strerror( errno ) );
        return -1;
    }

    return network_close(p_rtree );
}

int rtree_put(struct rtree_t *p_rtree, struct entry_t *p_entry) {
    if ( !p_rtree || !p_entry )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : rtree_put at least one null argument found.\n", strerror( errno ) );
        return -1;
    }

    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command condes.
    msg.opcode = OP_PUT;
    msg.c_type = CT_ENTRY;

    // Entry to send.
    MessageT__Entry entry_temp;
    message_t__entry__init(&entry_temp);

    ProtobufCBinaryData data_temp;

    entry_temp.key = (char *)malloc( sizeof( char ) * ( strlen( p_entry->key ) + 1 ) );
    strcpy( entry_temp.key, p_entry->key );

    data_temp.len = p_entry->value->datasize;
    data_temp.data = calloc( sizeof( void ), data_temp.len );
    memcpy(data_temp.data, p_entry->value->data, data_temp.len);

    entry_temp.data = data_temp;
    p_MessageT->entry = &entry_temp;

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Send and receive answer.
    if ((p_msg = network_send_receive(p_rtree, p_msg )) == NULL )
    {
        fprintf( stderr, "%s : error sending/receving to/from server.\n", strerror( errno ) );
        free( data_temp.data );
        free( entry_temp.key );
        free(p_msg);
        return -1;
    };

    // Clean temp entry and data.
    free( data_temp.data );
    free( entry_temp.key );

    int result = p_msg->p_MessageT->result;

    // Clean memory.
    message_t__free_unpacked( p_msg->p_MessageT, NULL );
    free( p_msg );

    return result;
}

struct data_t *rtree_get(struct rtree_t *p_rtree, char *p_key) {
    if ( !p_rtree || !p_key )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : rtree_get at least one null argument found.\n", strerror( errno ) );
        return NULL;
    }

    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command codes.
    msg.opcode = OP_GET;
    msg.c_type = CT_KEY;

    // Key to send.
    msg.key = (char *)malloc( sizeof( char) * ( strlen( p_key ) + 1 ) );
    strcpy( msg.key, p_key );

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Send and receive answer.
    if ((p_msg = network_send_receive(p_rtree, p_msg )) == NULL )
    {
        fprintf( stderr, "%s : error sending/receving to/from server.\n", strerror( errno ) );
        free( msg.key );
        free(p_msg);
        return NULL;
    }

    if(p_msg->p_MessageT->data.len == 0){
        message_t__free_unpacked( p_msg->p_MessageT, NULL );
        free(p_msg);
        free(msg.key);
    	return NULL;
    }

    struct data_t *p_data_result = data_create((int)p_msg->p_MessageT->data.len );
    memcpy(p_data_result->data, p_msg->p_MessageT->data.data, p_data_result->datasize);

    // Clean memory.
    message_t__free_unpacked( p_msg->p_MessageT, NULL );
    free( msg.key );
    free( p_msg );

    return p_data_result;
}

int rtree_del(struct rtree_t *p_rtree, char *p_key) {

	if ( !p_rtree || !p_key )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : rtree_get at least one null argument found.\n", strerror( errno ) );
        return -1;
    }

    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command codes.
    msg.opcode = OP_DEL;
    msg.c_type = CT_KEY;

    // Key to send.
    msg.key = (char *)malloc( sizeof( char ) * (strlen( p_key ) + 1)  );
    strcpy( msg.key, p_key );

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Send and receive answer.
    if ((p_msg = network_send_receive(p_rtree, p_msg )) == NULL )
    {
        fprintf( stderr, "%s : error sending/receving to/from server.\n", strerror( errno ) );
        free(p_msg);
        return -1;
    };

    int result = p_msg->p_MessageT->result;

    // Clean memory.
    message_t__free_unpacked( p_msg->p_MessageT, NULL );
    free( msg.key );
    free( p_msg );

    return result;

}

int rtree_size(struct rtree_t *p_rtree) { 

	if ( !p_rtree )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : rtree_get at least one null argument found.\n", strerror( errno ) );
        return -1;
    }

    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command codes.
    msg.opcode = OP_SIZE;
    msg.c_type = CT_NONE;

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Send and receive answer.
    if ((p_msg = network_send_receive(p_rtree, p_msg )) == NULL )
    {
        fprintf( stderr, "%s : error sending/receving to/from server.\n", strerror( errno ) );
        free(p_msg);
        return -1;
    };

    int result = -1;
    
    if ( p_msg->p_MessageT->opcode == OP_SIZE + 1 ) result = 0;
    result = p_msg->p_MessageT->result;

    // Clean memory.
    message_t__free_unpacked( p_msg->p_MessageT, NULL );
    free( p_msg );

    return result;

}

int rtree_height(struct rtree_t *p_rtree) { 

	if ( !p_rtree )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : rtree_get at least one null argument found.\n", strerror( errno ) );
        return -1;
    }

    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command codes.
    msg.opcode = OP_HEIGHT;
    msg.c_type = CT_NONE;

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Send and receive answer.
    if ((p_msg = network_send_receive(p_rtree, p_msg )) == NULL )
    {
        fprintf( stderr, "%s : error sending/receving to/from server.\n", strerror( errno ) );
        free(p_msg);
        return -1;
    };

    int result = -1;
    
    if ( p_msg->p_MessageT->opcode == OP_HEIGHT + 1 ) result = 0;
    result = p_msg->p_MessageT->result;

    // Clean memory.
    message_t__free_unpacked( p_msg->p_MessageT, NULL );
    free( p_msg );

    return result;

}

char **rtree_get_keys(struct rtree_t *p_rtree) {
    if ( !p_rtree )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : rtree_get_keys has a null argument.\n", strerror( errno ) );
        return NULL;
    }

    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command codes.
    msg.opcode = OP_GETKEYS;
    msg.c_type = CT_NONE;

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Send and receive answer.
    if ((p_msg = network_send_receive(p_rtree, p_msg )) == NULL )
    {
        fprintf( stderr, "%s : error sending/receving to/from server.\n", strerror( errno ) );
        free(p_msg);
        return NULL;
    };

    int num_keys = p_msg->p_MessageT->n_keys;
    char **pp_keys = (char **) malloc(sizeof(char *) * ( num_keys + 1 ));
    pp_keys[num_keys] = NULL;

    // Iterate through all strings on the message.
    for ( int i = 0; i < num_keys; i++ )
    {
        pp_keys[i] = strdup( p_msg->p_MessageT->keys[i] );
    }

    // Clean memory.
    message_t__free_unpacked( p_msg->p_MessageT, NULL );
    free( p_msg );

    return pp_keys;
}

void **rtree_get_values(struct rtree_t *p_rtree) { 

	if ( !p_rtree )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : rtree_get_keys has a null argument.\n", strerror( errno ) );
        return NULL;
    }

    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command codes.
    msg.opcode = OP_GETVALUES;
    msg.c_type = CT_NONE;

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Send and receive answer.
    if ((p_msg = network_send_receive(p_rtree, p_msg )) == NULL )
    {
        fprintf( stderr, "%s : error sending/receving to/from server.\n", strerror( errno ) );
        free(p_msg);
        return NULL;
    }

    int num_values = p_msg->p_MessageT->n_datas;
    void **pp_values = (void **) malloc(sizeof(char *) * ( num_values + 1 ));
    pp_values[num_values] = NULL;

    // Iterate through all strings on the message.
    for ( int i = 0; i < num_values; i++ )
    {
        struct data_t *p_data = data_create( p_msg->p_MessageT->datas[i].len );
        memcpy( p_data->data, p_msg->p_MessageT->datas[i].data, p_data->datasize );
        pp_values[i] = p_data;
    }

    // Clean memory.
    message_t__free_unpacked( p_msg->p_MessageT, NULL );
    free( p_msg );

    return pp_values;
}

int rtree_verify( struct rtree_t *p_rtree, int op_n )
{
    if ( p_rtree == NULL || op_n < 0 )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : null parameter on rtree_verify.\n", strerror( errno ) );
        return -1;
    }

    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command codes.
    msg.opcode = OP_VERIFY;
    msg.c_type = CT_RESULT;
    msg.result = op_n;

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Send and receive answer.
    if ((p_msg = network_send_receive(p_rtree, p_msg )) == NULL )
    {
        fprintf( stderr, "%s : error sending/receving to/from server.\n", strerror( errno ) );
        free(p_msg);
        return -1;
    }

    if ( p_msg->p_MessageT->opcode == OP_ERROR ) errno = EBADMSG;
    int result = p_msg->p_MessageT->result;

    // Clean memory.
    message_t__free_unpacked( p_msg->p_MessageT, NULL );
    free( p_msg );

    return result;
}

void rtree_quit( struct rtree_t *p_rtree )
{
    MessageT msg;
    message_t__init( &msg );
    MessageT *p_MessageT = &msg;

    // Command codes.
    msg.opcode = OP_BAD;
    msg.c_type = CT_RESULT;
    msg.result = 0;

    struct message_t* p_msg = (struct message_t*) malloc( sizeof( struct message_t ) );
    p_msg->p_MessageT = p_MessageT;

    // Reserve memory for the buffer.
    size_t buffer_len = message_t__get_packed_size( p_msg->p_MessageT );
    void *p_buffer;
    if ( !(p_buffer = malloc( buffer_len )) )
    {
        fprintf( stderr, "%s: it was not possible to malloc().\n", strerror( errno ) );
        return;
    }

    // Serialize msg to buffer.
    message_t__pack( p_msg->p_MessageT, p_buffer );

    // Send message through server socket.
    if (write_all(p_rtree->sockfd, p_buffer, buffer_len ) != buffer_len )
    {
        fprintf( stderr, "%s : message sent length does not coincide with buffer length.\n", strerror( errno ) );
        free( p_buffer );
        return;
    }

    // Clean memory.
    free( p_buffer );
    free( p_msg );
}

