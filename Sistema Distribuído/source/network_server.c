

#include <signal.h>
#include "tree_skel.h"
#include <stdio.h>
#include <stdlib.h>
#include "network_server.h"
#include "message-private.h"
#include "tree_skel-private.h"
#include <string.h>
#include <errno.h>
#include <poll.h>

// Number of sockets. One for listening and the last to close connections with the client if the limit is reached.
#define NFDESC 20
#define TIMEOUT 50000000 // ms



// To use it on network_server_close() in order to be able to close the socket cleanly.
int g_SERVER_SOCKFD;

int network_server_init( short port )
{
    int sockfd;
    struct sockaddr_in server;

    // Create TCP socket.
    if ((sockfd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
    {
        fprintf( stderr, "%s : error creating socket.\n", strerror(errno));
        return -1;
    }

    g_SERVER_SOCKFD = sockfd;

    // Socket can re-use addresses.
    int option = 1;

    if ( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof( option )) < 0 )
    {
        fprintf( stderr, "%s : error setting socket option SO_REUSEADDR.\n", strerror(errno));
        return -1;
    }

    // Fill the server structure for the bind.
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket.
    if ( bind( sockfd, (struct sockaddr *) &server, sizeof( server )) < 0 )
    {
        fprintf( stderr, "%s : error binding socket.\n", strerror(errno));
        return -1;
    }

    // Enable socket listening.
    if ( listen( sockfd, 0 ) < 0 )
    {
        fprintf( stderr, "%s : error socket listening.\n", strerror(errno));
        return -1;
    }

    return sockfd;
}


int network_main_loop( int listening_sockfd )
{

    if ( listening_sockfd < 0 )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : network_main_loop() argument sockfd is not valid.\n", strerror(errno));
        return -1;
    }

    printf( "Server awaiting connections.\n" );

    struct pollfd connections[NFDESC];
    int socket_ids[NFDESC];
    int nfds, kfds, i;

    struct sockaddr_in client;
    socklen_t size_client;
    size_client = sizeof( client );

    // Initialize array of connections.
    memset( connections, 0, sizeof( connections ));

    for ( i = 1; i < NFDESC; i++ )
    {
        connections[i].fd = -1; // poll ingores structures with fd < 0
        socket_ids[i] = i;
    }

    connections[0].fd = listening_sockfd;
    connections[0].events = POLLIN;

    nfds = 1; // number of file descriptors in use

    // Connection loop. Await for data in open sockets.
    while ((kfds = poll( connections, nfds, TIMEOUT )) >= 0 )
    {
        if ( kfds > 0 )
        {
            // Check if there's a new connection request.
            if ((connections[0].revents & POLLIN) && (nfds < NFDESC))
            {
                if ((connections[nfds].fd = accept( connections[0].fd, (struct sockaddr *) &client, &size_client )) >
                    0 )
                {
                    // Print which client connected, in which socket id.
                    printf( "\nConnection accepted with client %s on socket %d.\n",
                            inet_ntoa( client.sin_addr ),
                            socket_ids[nfds] );

                    connections[nfds].events = POLLIN;
                    nfds++;
                }
            }
        }

        // Verify if there's new data on the other connections.
        for ( i = 1; i < nfds; i++ )
        {
            // Limit of sockets occupied reached. Open and immediately close a new socket to force the client to close the connection.
            if ( i == NFDESC - 1 )
            {
                printf( "\nNumber of maximum occupied sockets reached. "
                        "Closing connection with client immediately...\n" );
                goto connection_close;
            }

            // New data.
            if ( connections[i].revents & POLLIN )
            {
                struct message_t *p_msg;

                if ( !(p_msg = network_receive( connections[i].fd )))
                {
                    errno = ENODATA;
                    fprintf( stderr, "%s : no message was received from connected client.\n", strerror(errno));
                    goto connection_close;
                }

                // Print received message.
                print_message( p_msg, 1 );

                // Connection was closed.
                if ( p_msg->p_MessageT->opcode == OP_BAD )
                {
                    printf( "Connection with client closed.\n" );
                    message_t__free_unpacked( p_msg->p_MessageT, NULL);
                    free( p_msg );
                    goto connection_close;
                }

                // Invoke message received
                if ( imvoke( p_msg ) < 0 )
                {
                    fprintf( stderr, "%s : error invoking received message command.\n", strerror(errno));
                    message_t__free_unpacked( p_msg->p_MessageT, NULL);
                    free( p_msg );
                    goto connection_close;
                }

                // Send client response
                if ( network_send( connections[i].fd, p_msg ) < 0 )
                {
                    fprintf( stderr, "%s : error sending response to client.\n", strerror(errno));
                    message_t__free_unpacked( p_msg->p_MessageT, NULL);
                    free( p_msg );
                    goto connection_close;
                }

                // Print sent message
                print_message( p_msg, 0 );
                message_t__free_unpacked( p_msg->p_MessageT, NULL);
                free( p_msg );
            }
        }
        // Connection closed.
        if ( connections[i].revents & POLLHUP )
        {
            connection_close:

            printf( "\nConnection closed with client %s on socket %d.\n",
                    inet_ntoa( client.sin_addr ),
                    socket_ids[i] );

            close( connections[i].fd );

            int current_socket_number = socket_ids[i];

            // Remove connection from array. Shift all connections after it.
            for ( int j = i; j < NFDESC - 1; j++ )
            {
                connections[j] = connections[j + 1];
                socket_ids[j] = socket_ids[j + 1];
            }

            connections[NFDESC - 1].fd = -1;
            socket_ids[NFDESC - 1] = current_socket_number;
            nfds--;
        }

    }

    return 0;
}


struct message_t *network_receive( int client_sockfd )
{

    if ( client_sockfd < 0 )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : invalid client_sockfd argument.\n", strerror(errno));
        return NULL;
    }

    // Receive message.
    char **pp_received_buffer;
    if ( !(pp_received_buffer = (char **) malloc( sizeof( char * ))))
        return NULL;

    if ( read_all( client_sockfd, pp_received_buffer ) < 0 )
    {
        fprintf( stderr, "%s : error receiving serialized message from client.\n", strerror(errno));
        free( pp_received_buffer );
        return NULL;
    }

    // Create new message.
    size_t msg_len = strlen( *pp_received_buffer );

    // Put received string on a buffer.
    uint8_t buffer[msg_len + 1];
    memcpy( buffer, *pp_received_buffer, msg_len + 1 );

    // Clean received string.
    free( *pp_received_buffer );
    free( pp_received_buffer );

    // Unpack the message.
    MessageT *p_MessageT;
    p_MessageT = message_t__unpack(NULL, msg_len, buffer );

    if ( !p_MessageT )
    {
        fprintf( stderr, "%s : error deserializing received message.\n", strerror(errno));
        return NULL;
    }

    struct message_t *p_msg;

    if ( !(p_msg = (struct message_t *) malloc( sizeof( struct message_t ))))
        return NULL;

    p_msg->p_MessageT = p_MessageT;

    return p_msg;
}


int network_send( int client_sockfd, struct message_t *p_msg )
{

    if ( client_sockfd < 0 || !p_msg )
    {
        errno = EINVAL;
        fprintf( stderr, "%s : at least on of network_send() arguments is invalid.\n", strerror(errno));
        return -1;
    }

    // Create buffer and serialize message.
    void *p_buffer;
    size_t buffer_len = message_t__get_packed_size( p_msg->p_MessageT );

    if ( !(p_buffer = malloc( buffer_len )))
    {
        fprintf( stderr, "%s: it was not possible to malloc().\n", strerror(errno));
        return -1;
    }

    message_t__pack( p_msg->p_MessageT, p_buffer );

    // Send buffer to client.
    if ( write_all( client_sockfd, p_buffer, buffer_len ) < 0 )
    {
        fprintf( stderr, "%s : error sending serialized message to client.\n", strerror(errno));
        free( p_buffer );
        return -1;
    }

    free( p_buffer );

    return 0;
}


int network_server_close()
{
    if ( close( g_SERVER_SOCKFD ) < 0 )
    {
        fprintf( stderr, "%s : error closing socket.\n", strerror(errno));
        return -1;
    }

    tree_skel_destroy();
    return 0;
}