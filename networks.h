
// 	Writen - HMS April 2017
//  Supports TCP and UDP - both client and server


#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define LISTEN_BACKLOG 10

#define FLAG_CONNECT_REQ 1
#define FLAG_CONNECT_SUC 2
#define FLAG_CONNECT_ERR 3
#define FLAG_BROADCAST 4
#define FLAG_UNICAST 5
#define FLAG_MULTICAST 6
#define FLAG_DEST_ERR 7
#define FLAG_EXIT_REQ 8
#define FLAG_EXIT_ACK 9
#define FLAG_HANDLES_REQ 10
#define FLAG_HANDLES_COUNT 11
#define FLAG_HANDLES_MEMBER 12
#define FLAG_HANDLES_END 13

#define MAXHANDLE 100

// for the TCP server side
int tcpServerSetup(int serverPort);
int tcpAccept(int mainServerSocket, int debugFlag);

// for the TCP client side
int tcpClientSetup(char * serverName, char * serverPort, int debugFlag);

// For UDP Server and Client
int udpServerSetup(int serverPort);
int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char * hostName, int serverPort);

// shared between client and server
int readHandle(uint8_t *buff, int pos, char *handle);
int addHandleToBuff(uint8_t *buff, int pos, char *handle, uint8_t handleLen);
int skipHandles(uint8_t *recvBuf, int byteOffset, int handlesToSkip);

#endif
