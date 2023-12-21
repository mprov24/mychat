#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>

#include "networks.h"
#include "safeUtil.h"
#include "processPDU.h"
#include "pollLib.h"

#define MAXBUF 1024
#define DEBUG_FLAG 0

// client table structs
struct client
{
    int socket;
    char handle[MAXHANDLE+1];
    int valid;
};
typedef struct client s_client;

struct clientTable
{
    s_client *table;
    int items;
    uint32_t activeClients;
    int size;
};
typedef struct clientTable s_clientTable;

void serverControl(int mainServerSocket);

// processing messages betweeen clients
void processMsgFromClient(int clientSocket, uint8_t *recvBuf, int recvLen, s_clientTable *clientTable);
void processListHandles(int senderSocket, s_clientTable *clientTable);
void processExitReq(int clientSocket, s_clientTable *clientTable);
void processMulticast(int senderSocket, uint8_t *recvBuf, int recvLen, int recvOffset, s_clientTable *clientTable);
void processBroadcast(int senderSocket, uint8_t *recvBuf, int recvLen, int recvOffset, s_clientTable *clientTable);
void processConnectionReq(int clientSocket, uint8_t *recvBuf, int recvOffset, s_clientTable *clientTable);

// dealing with client table
s_clientTable createClientTable();
void resizeClientTable(s_clientTable *clientTable);
void insertClientIntoTable(int clientSocket, char* clientHandle, s_clientTable *clientTable);
s_client *getClientFromSocket(int socket, s_clientTable *clientTable);
s_client *getClientFromHandle(char *handle, s_clientTable *clientTable);
void removeClient(int clientSocket, s_clientTable *clientTable);

void addNewSocket(int mainServerSocket);
int recvFromClient(int clientSocket, uint8_t *recvBuf, s_clientTable *clientTable);

int checkArgs(int argc, char *argv[]);
