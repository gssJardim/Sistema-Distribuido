// Grupo 55
// Jose Alves nº 44898
// Gustavo Jardim nº 48483
// Henrique Lopes nº 52840

#ifndef _SHARED_PRIVATE_H
#define _SHARED_PRIVATE_H

/**
 * Checks if string given is a valid port and returns its value.
 *
 * Parameters:
 *      p_input_str: input string to test.
 *
 * Returns:
 *      The port value to be returned; -1 if there's an error.
 */
short parse_port( char *p_input_str );

int parse_int(char *p_input_str);

#endif
