/******************************************************************************
* myServer.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

#include "server.h"

int main(int argc, char *argv[])
{
	int mainServerSocket = 0;   //socket descriptor for the server socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

    // setup polling
    setupPollSet();
    addToPollSet(mainServerSocket);

    serverControl(mainServerSocket);

	// NOT CLEANING UP - server will recieve intterupt
	
	return 0;
}

void serverControl(int mainServerSocket)
{
    uint8_t recvBuf[MAXBUF];
    int recvLen;
    s_clientTable clientTable = createClientTable();

    int pollSocket;
    while(1)
    {
        pollSocket = pollCall(-1);
        if (pollSocket == mainServerSocket)
            addNewSocket(mainServerSocket);
        else
        {
            recvLen = recvFromClient(pollSocket, recvBuf, &clientTable);
            if (recvLen)
                processMsgFromClient(pollSocket, recvBuf, recvLen, &clientTable);
        }
    }
}

s_clientTable createClientTable()
{
    s_clientTable clientTable;
    clientTable.items = 0;
    clientTable.activeClients = 0;
    clientTable.size = 10;
    clientTable.table = sCalloc(clientTable.size, sizeof(s_client));
    return clientTable;
}

void resizeClientTable(s_clientTable *clientTable)
{
    clientTable->size = clientTable->size * 2;
    clientTable->table = srealloc(clientTable->table, clientTable->size * sizeof(s_client));
}
 
void processMsgFromClient(int clientSocket, uint8_t *recvBuf, int recvLen, s_clientTable *clientTable)
{
    int recvOffset = 0;
    uint8_t recvFlag = recvBuf[recvOffset++];

    switch (recvFlag)
    {
        case FLAG_CONNECT_REQ:
            processConnectionReq(clientSocket, recvBuf, recvOffset, clientTable);
            break;
        case FLAG_BROADCAST:
            processBroadcast(clientSocket, recvBuf, recvLen, recvOffset, clientTable);
            break;
        case FLAG_UNICAST:
        case FLAG_MULTICAST:
            processMulticast(clientSocket, recvBuf, recvLen, recvOffset, clientTable);
            break;
        case FLAG_EXIT_REQ:
            processExitReq(clientSocket, clientTable);
            break;
        case FLAG_HANDLES_REQ:
            processListHandles(clientSocket, clientTable);
            break;
        default:
            printf("Error: Unexpected flag: %u\n", recvFlag);
    }
}

void processListHandles(int senderSocket, s_clientTable *clientTable)
{
    uint8_t sendBuf[MAXBUF];
    int sendLen = 0;

    sendBuf[sendLen++] = FLAG_HANDLES_COUNT;
    uint32_t n_activeClients = htonl(clientTable->activeClients);
    memcpy(sendBuf + sendLen, &n_activeClients, 4);
    sendLen += 4;
    sendPDU(senderSocket, sendBuf, sendLen);
    
    s_client client;
    int i;
    for (i=0; i<clientTable->items;i++)
    {
        client = clientTable->table[i];
        if (client.valid)
        {
            sendLen = 0;
            sendBuf[sendLen++] = FLAG_HANDLES_MEMBER;
            sendLen += addHandleToBuff(sendBuf, sendLen, client.handle, strlen(client.handle));
            sendPDU(senderSocket, sendBuf, sendLen);
        }
    }

    sendLen = 0;
    sendBuf[sendLen++] = FLAG_HANDLES_END;
    sendPDU(senderSocket, sendBuf, sendLen);
}

void processExitReq(int clientSocket, s_clientTable *clientTable)
{
    uint8_t sendBuf[MAXBUF];
    int sendLen = 0;

    sendBuf[sendLen++] = FLAG_EXIT_ACK;
    sendPDU(clientSocket, sendBuf, sendLen);
    removeClient(clientSocket, clientTable);
}

void processMulticast(int senderSocket, uint8_t *recvBuf, int recvLen, int recvOffset, s_clientTable *clientTable)
{
    uint8_t sendBuf[MAXBUF];
    int sendLen = 0;

    // skip the sender handle
    recvOffset = skipHandles(recvBuf, recvOffset, 1);

    uint8_t numDestinations = recvBuf[recvOffset++];

    s_client *client;
    char clientHandle[MAXHANDLE];
    int i;
    for (i=0; i<numDestinations; i++)
    {
        recvOffset = readHandle(recvBuf, recvOffset, clientHandle);
        client = getClientFromHandle(clientHandle, clientTable);
        if (client)
            sendPDU(client->socket, recvBuf, recvLen);

        // if a handle doesn't exist in table send out flag DEST_ERR
        else
        {
            sendBuf[sendLen++] = FLAG_DEST_ERR;
            uint8_t handleLen = strlen(clientHandle);
            sendBuf[sendLen++] = handleLen;
            memcpy(sendBuf + sendLen, clientHandle, handleLen);
            sendLen += handleLen;
            sendPDU(senderSocket, sendBuf, sendLen);
        }
    }
}

void processBroadcast(int senderSocket, uint8_t *recvBuf, int recvLen, int recvOffset, s_clientTable *clientTable)
{
    s_client client;
    int i;
    for (i=0; i<clientTable->items;i++)
    {  
        client = clientTable->table[i];
        if (client.valid && client.socket != senderSocket)
            sendPDU(client.socket, recvBuf, recvLen);
    }
}

void processConnectionReq(int clientSocket, uint8_t *recvBuf, int recvOffset, s_clientTable *clientTable)
{
    uint8_t sendBuf[MAXBUF];
    int sendLen = 0;

    char requestHandle[MAXHANDLE];
    recvOffset = readHandle(recvBuf, recvOffset, requestHandle);

    // if client exists in table
    if (getClientFromHandle(requestHandle, clientTable))
    {
        sendBuf[sendLen++] = FLAG_CONNECT_ERR;
        sendPDU(clientSocket, sendBuf, sendLen);

        // client should exit after server denies
        removeFromPollSet(clientSocket);
        close(clientSocket);
    }
    else
    {
        sendBuf[sendLen++] = FLAG_CONNECT_SUC;
        insertClientIntoTable(clientSocket, requestHandle, clientTable);
        sendPDU(clientSocket, sendBuf, sendLen);
    }
}

void insertClientIntoTable(int clientSocket, char* clientHandle, s_clientTable *clientTable)
{
    if (clientTable->items == clientTable->size)
        resizeClientTable(clientTable);
    
    s_client newClient;
    strcpy(newClient.handle, clientHandle);
    newClient.socket = clientSocket;
    newClient.valid = 1;
    clientTable->table[clientTable->items++] = newClient;
    clientTable->activeClients++;
}

void addNewSocket(int mainServerSocket)
{
    int clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
    addToPollSet(clientSocket);
}

int recvFromClient(int clientSocket, uint8_t *recvBuf, s_clientTable *clientTable)
{
	int messageLen = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, recvBuf, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

    if (DEBUG_FLAG)
    {
        printf("Recieved: 0x");
        int i;
        for (i=0;i<messageLen;i++)
        {
            printf("%02x", recvBuf[i]);
        }
        printf("\n");
    }

    // if client closed unexpectedly
	if (messageLen < 1)
	{
		printf("Unexpected client termination\n");
        removeClient(clientSocket, clientTable);
	}
    return messageLen;
}

// closes, removes from poll set, marks as invalid
void removeClient(int clientSocket, s_clientTable *clientTable)
{
    removeFromPollSet(clientSocket);

    s_client *client = getClientFromSocket(clientSocket, clientTable);
    
    // if client was put in table with handle
    if (client)
    {
        client->valid = 0;
        clientTable->activeClients--;
    }

    close(clientSocket);
}

// gets client structure from socket number
s_client *getClientFromSocket(int socket, s_clientTable *clientTable)
{
    s_client *client;
    int i;
    for (i=0;i<clientTable->items;i++)
    {
        client = (clientTable->table) + i;
        if (client->valid && client->socket == socket)
            return client;
    }
    return NULL;
}

// gets client structure from handle
s_client *getClientFromHandle(char *handle, s_clientTable *clientTable)
{
    s_client *client;
    int i;
    for (i=0;i<clientTable->items;i++)
    {
        client = (clientTable->table) + i;
        if (client->valid && strcmp(client->handle, handle) == 0)
            return client;
    }
    return NULL;
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}

