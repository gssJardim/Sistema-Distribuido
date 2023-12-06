

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "network_client.h"
#include "message-private.h"
#include "client_stub-private.h"

int network_connect( struct rtree_t *p_rtree )
{
    if ( !p_rtree )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : p_rtree argument is null.\n", strerror( errno ) );
        return -1;
    }

    int sockfd;

    // Socket TCP.
    if ( (sockfd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
    {
        fprintf( stderr, "%s : error creating socket.\n", strerror( errno ) );
        return -1;
    }

    // Connection with server specified.
    if (connect(sockfd, (struct sockaddr *)p_rtree->p_sockaddr, sizeof( *p_rtree->p_sockaddr ) ) < 0 )
    {
        fprintf( stderr, "%s : error connecting to server.\n", strerror( errno ) );
        return -1;
    }

    p_rtree->sockfd = sockfd;
    return 0;
}

struct message_t *network_send_receive(struct rtree_t *p_rtree, struct message_t *p_msg )
{
    if (!p_rtree || !p_msg )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : at least one network_send_recieve() argument is NULL.\n", strerror( errno ) );
        return NULL;
    }

    // Reserve memory for the buffer.
    size_t buffer_len = message_t__get_packed_size( p_msg->p_MessageT );
    void *p_buffer;
    if ( !(p_buffer = malloc( buffer_len )) )
    {
        fprintf( stderr, "%s: it was not possible to malloc().\n", strerror( errno ) );
        return NULL;
    }

    // Serialize msg to buffer.
    message_t__pack( p_msg->p_MessageT, p_buffer );

    // Send message through server socket.
    if (write_all(p_rtree->sockfd, p_buffer, buffer_len ) != buffer_len )
    {
        fprintf( stderr, "%s : message sent length does not coincide with buffer length.\n", strerror( errno ) );
        free( p_buffer );
        return NULL;
    }

    // Free buffer containing the message is sent.
    free( p_buffer );

    // Receive message.
    char** pp_received_buffer;
    if ( !( pp_received_buffer = (char**)malloc( sizeof( char *) ) ) )
        return NULL;

    // Receive message from server socket.
    if (read_all(p_rtree->sockfd, pp_received_buffer ) < 0 )
    {
        free( pp_received_buffer );
        return NULL;
    }

    size_t msg_len = strlen( *pp_received_buffer );

    p_buffer = (uint8_t *)malloc( sizeof( uint8_t ) * ( msg_len + 1 ) );
    memcpy( p_buffer, *pp_received_buffer, msg_len + 1 );

    free( *pp_received_buffer );
    free( pp_received_buffer );

    // Message was not received.
    if ( msg_len == 0 )
    {
        errno = ENODATA;
        fprintf( stderr, "%s : message received is empty.\n", strerror( errno ) );
        free( p_buffer );
        return NULL;
    }

    // Deserialize message received from the server.
    MessageT *p_MessageT;
    if ( !(p_MessageT = message_t__unpack( NULL, msg_len, p_buffer )) )
    {
        fprintf( stderr, "%s : error deserializing message received from the server.\n", strerror( errno ) );
        message_t__free_unpacked( p_msg->p_MessageT, NULL );
        free( p_buffer );
        return NULL;
    }

    p_msg->p_MessageT = p_MessageT;

    // Clean memory.
    free( p_buffer );

    return p_msg;
}

int network_close( struct rtree_t *p_rtree )
{
    if ( !p_rtree )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : p_rtree argument is null.\n", strerror( errno ) );
        return -1;
    }

    // Keep
    int sockfd = p_rtree->sockfd;

    // Free the tree.
    if (p_rtree->p_sockaddr != NULL ) free(p_rtree->p_sockaddr );
    free(p_rtree );

    // Close the socket.
    if ( close( sockfd ) < 0 )
    {
        fprintf( stderr, "%s : error closing socket.\n", strerror( errno ) );
        return -1;
    }

    return 0;
}