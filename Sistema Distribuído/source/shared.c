
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>

short parse_port(char *p_input_str)
{
    char *p_additional_chars = NULL;

    long server_port = strtol(p_input_str, &p_additional_chars, 10);

    // Argument contains characters which are not digits.
    if (strlen(p_additional_chars) > 0)
    {
        errno = EINVAL;
        fprintf(stderr, "%s : <port> contains invalid characters: %s \n", strerror(errno), p_additional_chars);
        return -1;
    } // Overflow, por with value above 32767
    else if (server_port > SHRT_MAX)
    {
        errno = ERANGE;
        fprintf(stderr, "%s : <port> = %ld (port is above the limit 32767).\n", strerror(errno), server_port);
        return -1;
    } // Do not create ports on the reserved 1-1023; 0 is also invalid.
    else if (0 <= server_port && server_port <= 1023)
    {
        errno = EINVAL;
        fprintf(stderr,
                "%s : <port> = %ld (ports between 0 e 1023 are system reserved).\n", strerror(errno), server_port);
        return -1;
    }

    return (short) server_port;
}

int parse_int(char *p_input_str)
{
    char *p_additional_chars = NULL;

    long parsed_int = strtol( p_input_str, &p_additional_chars, 10);

    // Argument contains characters which are not digits.
    if (strlen(p_additional_chars) > 0)
    {
        errno = EINVAL;
        fprintf(stderr, "%s : <n_threads> contains invalid characters: %s \n", strerror(errno), p_additional_chars);
        return -1;
    } // Overflow, value above
    else if ( parsed_int > INT_MAX)
    {
        errno = ERANGE;
        fprintf( stderr, "%s : <n_threads> = %ld (port is above the limit of a Int can hold).\n", strerror(errno), parsed_int);
        return -1;
    }

    return (int) parsed_int;
}