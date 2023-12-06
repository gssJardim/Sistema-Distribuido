
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "shared-private.h"
#include "network_server.h"
#include "tree_skel-private.h"

void sigint_handler()
{
    printf("SIGINT received. Terminating and cleaning server...\n");
    tree_skel_request_queue_threads_destroy();
}

int main(int argc, char **argv)
{
    // Ignore SIGPIPE signal.
    signal( SIGPIPE, SIG_IGN );

    // Verifiy if the argument are present.
    if ( argc != 3 )
    {
        printf( "Usage: ./tree-server <port> <n_threads>\n" );
        printf( "Example: ./tree-server 1234 5\n" );
        exit( EXIT_FAILURE );
    }

    // Verify and parse port.
    short server_port;

    if ( (server_port = parse_port( argv[1] )) < 0 )
    {
        exit( EXIT_FAILURE );
    }

    // Verify and parse number of threads.
    int n_threads;

    if ( (n_threads = parse_int( argv[2] )) < 0 )
    {
        exit( EXIT_FAILURE );
    }

    if ( n_threads < 1 )
    {
        fprintf( stderr, "The number of threads must be greater than 0.\n" );
        exit( EXIT_FAILURE );
    }

    // Init server.
    int sockfd;

    if ( (sockfd = network_server_init( server_port )) < 0 )
    {
        fprintf( stderr, "%s : error starting network server.\n", strerror( errno ) );
        exit( EXIT_FAILURE );
    }

    signal( SIGINT, sigint_handler );

    // Start tree.
    if ( tree_skel_init(n_threads) < 0 )
    {
        fprintf( stderr, "%s : error starting tree skel.\n", strerror( errno ) );
        exit( EXIT_FAILURE );
    }

    // Start server main loop.
    if ( network_main_loop( sockfd ) < 0 )
    {
        fprintf( stderr, "%s : error on main loop.\n", strerror( errno ) );
        tree_skel_destroy();
        tree_skel_request_queue_threads_destroy(n_threads);
        exit( EXIT_FAILURE );
    }

    // Destroy tree.
    tree_skel_destroy();
    tree_skel_request_queue_threads_destroy(n_threads);

    exit( EXIT_SUCCESS );
}