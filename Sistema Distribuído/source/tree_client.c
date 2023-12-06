
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "client_stub.h"
#include "client_stub-private.h"

#define g_MAX_USER_INPUT_SIZE 1024

void print_commands(){
    printf("\n--------------List of Commands------------------------------\n");
    printf("help               || show available commands\n");
    printf("size               || returns size of the tree\n");
    printf("height             || returns height of the tree\n");
    printf("del <key>          || deletes entry of the corresponding key\n");
    printf("get <key>          || returns entry of the corresponding key\n");
    printf("put <key> <data>   || puts entry(key,data) on the tree\n");
    printf("getkeys            || returns all the keys from the tree\n");
    printf("getvalues          || returns all the values from the tree\n");
    printf("verify <op_n>      || verifies if operation was finished\n");
    printf("quit               || exits the program\n");
}

int main( int argc, char **argv )
{
    // Verifiy if there's two arguments.
    if ( argc != 2 )
    {
        printf( "Usage: ./tree-client <server>:<port>\n" );
        printf( "Example: ./tree-client 127.0.0.1:1234 \n" );
        exit( EXIT_FAILURE );
    }

    printf("SIGINT signal ignored. To exit the program type 'quit'.\n");

    // Ignore SIGPIPE signal.
    signal( SIGPIPE, SIG_IGN );
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);


    // Connection to server.
    struct rtree_t *p_rtree;

    if ( !(p_rtree = rtree_connect(argv[1] )) )
    {
        fprintf( stderr, "Connection with the server was not possible.\n" );
        exit( EXIT_FAILURE );
    }

    print_commands();

    int has_quit = 0;
    char *p_user_input = NULL;
    char *pp_user_input = NULL;  // will save p_user_input[0] pointer.

    while ( !has_quit )
    {
        errno = 0;
        printf( "\nInsert command: \n" );
        

        if ( pp_user_input ) free( pp_user_input );

        // Allocate memory for user input string
        if ( !(p_user_input = (char *)malloc( g_MAX_USER_INPUT_SIZE * sizeof( char ) )) )
        {
            fprintf( stderr, "%s: it was not possible to malloc().\n", strerror( errno ) );
            exit( EXIT_FAILURE );
        }

        // Get user input.
        fgets( p_user_input, g_MAX_USER_INPUT_SIZE, stdin );

        /*
         * Get user input arguments.
         */

        // Save first position for later free.
        pp_user_input = p_user_input;

        // Remove '\n'
        strtok( p_user_input, (const char *)"\n" );

        // First argument
        p_user_input = strtok( p_user_input, " " );
        char *p_first_arg = p_user_input;

        // First argument to lowercase
        for ( int i = 0; p_first_arg[i]; i++ ) p_first_arg[i] = (char)tolower( p_first_arg[i] );

        // Number of user input arguments after the first
        int n_args = 0;

        // Second argument
        if ( (p_user_input = strtok( NULL, " " )) ) ++n_args;
        char *p_second_arg = p_user_input;

        // Third argument
        if ( (p_user_input = strtok( NULL, "" )) ) ++n_args;
        char *p_third_arg = p_user_input;
        /*
         * Choose command based on the arguments.
         */

        // Show menu.
        if (strcmp( p_first_arg, "help" ) == 0 )
        {
            print_commands();
        }
        // Size command.
        else if ( strcmp( p_first_arg, "size" ) == 0 )
        {
            if ( n_args != 0 )
            {
                printf( "Size command does not have arguments.\n" );
                continue;
            }

            int tree_size = rtree_size(p_rtree );

            if (tree_size < 0 )
                printf( "Error obtaining the tree size.\n" );
            else
                printf("Tree size: %d\n", tree_size );

        }
        // Height command.
        else if ( strcmp( p_first_arg, "height" ) == 0 )
        {
            if ( n_args != 0 )
            {
                printf( "Height command does not have arguments.\n" );
                continue;
            }

            int tree_height = rtree_height(p_rtree );

            if (tree_height < 0 )
                printf( "Error obtaining the tree height.\n" );
            else
                printf("Tree height: %d\n", tree_height );

        }
            // Del command.
        else if ( strcmp( p_first_arg, "del" ) == 0 )
        {
            if ( n_args != 1 )
            {
                printf( "Del command only has one argument (e.g. del <key> ).\n" );
                continue;
            }

            int result;
            if ( (result = rtree_del(p_rtree, p_second_arg )) < 0 )
            {
                printf( "Key was not found on tree.\n" );
            }
            else
            {
                printf( "Operation number: %d\n", result );
            }
        } // Get command.
        else if ( strcmp( p_first_arg, "get" ) == 0 )
        {
            if ( n_args != 1 )
            {
                printf( "Get command only has one argument (e.g. get <key> ).\n" );
                continue;
            }

            struct data_t *p_data;
            if ((p_data = rtree_get(p_rtree, p_second_arg )) == NULL )
            {
                printf( "Key specified is not in the tree.\n" );
            }
            else
            {
                char *p_str = malloc( sizeof( char ) * (p_data->datasize + 1 ) );
                p_str[p_data->datasize] = '\0';
                memcpy(p_str,p_data->data, p_data->datasize);
                printf( "datasize: %d ; data: %s\n", p_data->datasize, p_str );
                free(p_str);
                data_destroy( p_data );
            }
        } // Put command.
        else if ( strcmp( p_first_arg, "put" ) == 0 )
        {
            if ( n_args < 2 )
            {
                printf( "Put command has two arguments (e.g. put <key> <data> ).\n" );
                continue;
            }

            struct data_t *p_data = data_create2( (int)strlen( p_third_arg ), strdup( p_third_arg ) );
            struct entry_t *p_entry = entry_create( strdup( p_second_arg ), p_data );
            int result = rtree_put(p_rtree, p_entry );

            if ( result < 0 )
                printf( "It was not possible to insert entry on the tree.\n" );
            else
                printf( "Operation number: %d\n", result );

            entry_destroy( p_entry );

        }
        else if ( strcmp( p_first_arg, "getkeys" ) == 0 )
        {
            if ( n_args != 0 )
            {
                printf( "Getkeys command does not need arguments.\n" );
            }
            else
            {
                char **pp_keys = rtree_get_keys(p_rtree );

                // Tree is empty.
                if ( !pp_keys )
                {
                    printf( "No key found (empty tree).\n" );

                }
                else
                {
                    printf( "Keys found:\n" );
                    for ( int i = 0; pp_keys[i]; i++ )
                    {
                        printf( "%s\n", pp_keys[i] );
                        free( pp_keys[i] );
                    }
                }

                if ( pp_keys ) free( pp_keys );
            }
        }
        else if ( strcmp( p_first_arg, "getvalues" ) == 0 )
        {
            if ( n_args != 0 )
            {
                printf( "Getvalues command does not need arguments.\n" );
            }
            else
            {
                void **pp_values = rtree_get_values(p_rtree ); // 1)

                // Tree is empty.
                if ( !pp_values )
                {
                    printf( "No key found (empty tree).\n" );

                }
                else
                {
                    printf( "Values found:\n" );
                    for ( int i = 0; pp_values[i]; i++ )
                    {
                        struct data_t* p_data = (struct data_t*)pp_values[i];
                        char *p_str = malloc( sizeof( char ) * (p_data->datasize + 1 ) ); // 2)
                        p_str[p_data->datasize] = '\0';
                        memcpy(p_str, p_data->data, p_data->datasize);
                        printf("{datasize: %d; data: %s}\n", p_data->datasize, p_str );
                        free(p_str); // 2) freed
                    }
                }


                for ( int i = 0; pp_values[i]; i++ )
                {
                    data_destroy( (struct data_t*)pp_values[i] ); // 1) freed
                }
                free( pp_values );
            }
        }
        else if ( strcmp( p_first_arg, "verify" ) == 0 )
        {
            if ( n_args != 1 )
            {
                printf( "Verify command only has one argument.\n" );
                continue;
            }

            char *p_additional_chars = NULL;
            long operation_number = strtol( p_second_arg, &p_additional_chars, 10 );

            // argumento contem caracteres que nao sao digitos
            if ( *p_additional_chars != 0 )
            {
                errno = EINVAL;
                printf( "Error: Verfiy's argument has non digit characters.\n" );
                continue;
            }

            int result = rtree_verify( p_rtree, (int)operation_number );

            if ( errno == EBADMSG )
                printf( "\nInvalid operation number. No operation with specified number was assigned.\n" );
            else if ( result < 0 )
                printf( "\nOperation did not finish yet.\n" );
            else
                printf( "\nOperation did finish.\n" );
        }
	// Quit command.
        else if ( strcmp( p_first_arg, "quit" ) == 0 )
        {
            has_quit = 1;
            rtree_quit(p_rtree );
            free( pp_user_input );
        } // Unknown command.
        else
        {
            printf( "Command not existent.\n" );
        }
    }

    // Disconnect from server.
    if (rtree_disconnect(p_rtree ) < 0 )
    {
        fprintf( stderr, "Error disconnecting from the server.\n" );
        return -1;
    }

    exit( EXIT_SUCCESS );
}

