#ifndef CONNCTION_CASH_H
#define CONNCTION_CASH_H

int get_connection(const char ip[50], int port);
int release_connection(int socket_fd);
void close_all_connections();


#endif
