// Grupo 55
// Jose Alves nº 44898
// Gustavo Jardim nº 48483
// Henrique Lopes nº 52840

#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

struct rtree_t
{
    struct sockaddr_in *p_sockaddr;
    int sockfd;
};

void rtree_quit( struct rtree_t *p_rtree );

#endif