#include "cclient.h"

int main(int argc, char * argv[])
{
	int serverSock = 0;         //socket descriptor
	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	serverSock = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

    setupPollSet();
    addToPollSet(serverSock);
    addToPollSet(STDIN_FILENO);
	
    clientControl(argv[1], serverSock);

    removeFromPollSet(serverSock);
    close(serverSock);
	
	return 0;
}

void clientControl(char *handle, int serverSock)
{

    uint8_t sendBuf[MAXBUF];    // send buffer
    uint8_t recvBuf[MAXBUF];    // recieve buffer
    int recvLen;
    uint8_t inputBuf[MAXBUF];   // stdin buffer
    int inputLen;

    if (!initialHandshake(serverSock, handle, sendBuf, recvBuf))
        return;
 
    int readySocket;
    while (1)
    {
        printf("$: ");
        fflush(stdout);
        readySocket = pollCall(-1);
        if (readySocket == STDIN_FILENO)
        {
            // this will block but prof says its ok
            inputLen = readFromStdin(inputBuf);
            if (inputLen >= MAXBUF)
                printf("\nError: Input exceeds max buffer size\n");
            else
                processStdin(serverSock, inputBuf, inputLen, sendBuf, handle);
            // printf("read: %s string len: %d (including null)\n", inputBuf, inputLen);

        }
        else
        {
            recvLen = recvPDU(serverSock, recvBuf, MAXBUF);
            if (recvLen < 1)
            {
                printf("\nServer terminated\n");
                return;
            }
            else if (recvBuf[0] == FLAG_EXIT_ACK)
            {
                printf("\n");
                return;
            }

            processMsgFromServer(serverSock, recvBuf, recvLen);
        }
    }
}

int initialHandshake(int serverSock, char *handle, uint8_t *sendBuf, uint8_t *recvBuf)
{
    // send out initial connection request
    uint8_t handleLen = strlen(handle);
    int bytes = 0;

    sendBuf[bytes++] = FLAG_CONNECT_REQ;

    bytes += addHandleToBuff(sendBuf, bytes, handle, handleLen);
    sendPDU(serverSock, sendBuf, bytes);

    // must block until receive connection response
    recvPDU(serverSock, recvBuf, MAXBUF);
    if (recvBuf[0] == FLAG_CONNECT_SUC)
        return 1;
    else if (recvBuf[0] == FLAG_CONNECT_ERR)
    {
        printf("\nHandle already in use: %s\n", handle);
        return 0;
    }
    else
    {
        printf("\nUnknown connection response flag %u\n", recvBuf[0]);
        return 0;
    }
}

int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void processMsgFromServer(int serverSock, uint8_t *recvBuf, int recvLen)
{
    int byteOffset = 0;
    int flag = recvBuf[byteOffset++];
    char handle[MAXHANDLE+1];
    uint8_t handlesToSkip;
    switch (flag)
    {
        case FLAG_BROADCAST: 
            byteOffset = readHandle(recvBuf, byteOffset, handle);
            printf("\n%s: %s\n", handle, recvBuf+byteOffset);
            break;
        case FLAG_UNICAST: 
        case FLAG_MULTICAST:
            byteOffset = readHandle(recvBuf, byteOffset, handle);
            handlesToSkip = recvBuf[byteOffset++];
            byteOffset = skipHandles(recvBuf, byteOffset, handlesToSkip);
            printf("\n%s: %s\n", handle, recvBuf+byteOffset);
            break;
        case FLAG_DEST_ERR:
            byteOffset = readHandle(recvBuf, byteOffset, handle);
            printf("\nClient with handle %s does not exist\n", handle);
            break;
        case FLAG_HANDLES_COUNT:
            recieveHandles(serverSock, recvBuf);
            break;
        default:
            printf("\nError: Recieved unexpected flag: %u\n", flag);
    }
}

void recieveHandles(int serverSock, uint8_t *recvBuf)
{
    uint32_t handlesCount;
    memcpy(&handlesCount, recvBuf+1, 4);
    handlesCount = ntohl(handlesCount);
    printf("\nNumber of clients: %u", handlesCount);

    int byteOffset;
    uint8_t flag;
    char handle[MAXHANDLE];
    int i;
    for (i=0;i<handlesCount;i++)
    {
        recvPDU(serverSock, recvBuf, MAXBUF);
        byteOffset = 0;
        flag = recvBuf[byteOffset++];
        if (flag != FLAG_HANDLES_MEMBER)
        {
            printf("\nError: expected flag %u, got %u\n", FLAG_HANDLES_MEMBER, flag);
            return;
        }
        byteOffset = readHandle(recvBuf, 1, handle);
        printf("\n\t%s", handle);
    }

    recvPDU(serverSock, recvBuf, MAXBUF);
    byteOffset = 0;
    flag = recvBuf[byteOffset++];
    if (flag != FLAG_HANDLES_END)
    {
        printf("\nError: expected flag %u, got %u\n", FLAG_HANDLES_END,flag);
        return;
    }
    printf("\n");
}

void processStdin(int serverSock, uint8_t *inputBuf, int inputLen, uint8_t *sendBuf, char *handle)
{
    uint8_t handleLen = strlen(handle);
    int packetLen = 0;

    // find first keyword, skipping whitespace
    char * keyword = findKeyword((char *)inputBuf);
    int keylen = findKeylen(keyword);
    if (keylen != 2)
    {
        printf("Invalid command\n");
        return;
    }

    keyword[1] = tolower(keyword[1]);
    
    if (strncmp(keyword, "%m", keylen) == 0)
        sendUnicast(serverSock, inputBuf, sendBuf, handle, handleLen);
    else if (strncmp(keyword, "\%c", keylen) == 0)
        sendMulticast(serverSock, inputBuf, sendBuf, handle, handleLen);
    else if (strncmp(keyword, "%b", keylen) == 0)
        sendBroadcast(serverSock, inputBuf, sendBuf, handle, handleLen);
    else if (strncmp(keyword, "%l", keylen) == 0)
    {
        sendBuf[packetLen++] = FLAG_HANDLES_REQ;
        sendPDU(serverSock, sendBuf, packetLen);
    }
    else if (strncmp(keyword, "\%e", keylen) == 0)
    {
        sendBuf[packetLen++] = FLAG_EXIT_REQ;
        sendPDU(serverSock, sendBuf, packetLen);
    }
    else
    {
        printf("Invalid command\n");
        return;
    }
}

void sendMulticast(int serverSock, uint8_t *inputBuf, uint8_t *sendBuf, char *handle, uint8_t handleLen)
{
    int byteOffset = 0;
    sendBuf[byteOffset++] = FLAG_MULTICAST;

    byteOffset += addHandleToBuff(sendBuf, byteOffset, handle, handleLen);

    // get pointer to number of destinations (first 2 chars are message type)
    char *numDestinationsStr = findKeyword((char *)inputBuf + 2);
    uint8_t numDestinationsStrLen = findKeylen(numDestinationsStr);
    if (numDestinationsStrLen == 0 || !isdigit(numDestinationsStr[0]))
    {
        printf("Invalid command format\n");
        return;
    }
    uint8_t numDestinations = atoi(numDestinationsStr);
    if (numDestinations < 2 || numDestinations > 9)
    {
        printf("Multicast must have 2-9 destinations\n");
        return;
    }
    sendBuf[byteOffset++] = numDestinations;

    // setting start of handle to previous user input
    char *destHandle = numDestinationsStr;
    uint8_t destHandleLen = numDestinationsStrLen;

    // loop through handles in user input
    int i;
    for (i=0;i<numDestinations;i++)
    {
        destHandle = findKeyword(destHandle + destHandleLen);
        destHandleLen = findKeylen(destHandle);
        // check if there's a handle where there should be
        if (destHandleLen == 0)
        {
            printf("Invalid command format\n");
            return;
        }
        byteOffset += addHandleToBuff(sendBuf, byteOffset, destHandle, destHandleLen);
    }

    char *textBuf = findKeyword(destHandle + destHandleLen);

    sendTextMessages(serverSock, textBuf, sendBuf, byteOffset);
}

void sendUnicast(int serverSock, uint8_t *inputBuf, uint8_t *sendBuf, char *handle, uint8_t handleLen)
{
    int byteOffset = 0;
    sendBuf[byteOffset++] = FLAG_UNICAST;

    byteOffset += addHandleToBuff(sendBuf, byteOffset, handle, handleLen);

    sendBuf[byteOffset++] = 1; // 1 destination for unicast

    // get pointer dest handle (first 2 chars are message type)
    char *destHandle = findKeyword((char *)inputBuf + 2);
    uint8_t destHandleLen = findKeylen(destHandle);
    if (destHandleLen == 0)
    {
        printf("Invalid command format\n");
        return;
    }
    byteOffset += addHandleToBuff(sendBuf, byteOffset, destHandle, destHandleLen);

    char *textBuf = findKeyword(destHandle + destHandleLen);

    sendTextMessages(serverSock, textBuf, sendBuf, byteOffset);
}

void sendBroadcast(int serverSock, uint8_t *inputBuf, uint8_t *sendBuf, char *handle, uint8_t handleLen)
{
    int byteOffset = 0;
    sendBuf[byteOffset++] = FLAG_BROADCAST;

    byteOffset += addHandleToBuff(sendBuf, byteOffset, handle, handleLen);

    // get pointer to start of text (first 2 chars are message type)
    char *textBuf = findKeyword((char *)inputBuf + 2);
    sendTextMessages(serverSock, textBuf, sendBuf, byteOffset);
}

void sendTextMessages(int serverSock, char *textBuf, uint8_t *sendBuf, int textOffset)
{
    // these dont include null byte
    int totalTextLen = strlen(textBuf);
    int totalTextSent = 0;

    // this is just to ensure we can send an empty message
    int packetsSent = 0;

    int packetTextLen;
    while (packetsSent == 0 || totalTextSent < totalTextLen)
    {
        // send max of 199 chars of text (not including null)
        packetTextLen = MIN(totalTextLen - totalTextSent, 199);
        // copy to current send buffer
        memcpy(sendBuf + textOffset, textBuf + totalTextSent, packetTextLen);
        // null terminate packet
        sendBuf[textOffset + packetTextLen] = '\0';
        // send full packet
        sendPDU(serverSock, sendBuf, textOffset + packetTextLen + 1);
        
        totalTextSent += packetTextLen;
        packetsSent++;
    }
}

// first nonwhitespace in string (NOT NULL TERMINATED)
char * findKeyword(char *str)
{
    int i = 0;
    while (str[i] == ' ')
        i++;

    return str + i;
}

// length of non whitespace string
int findKeylen(char *str)
{
    int i = 0;
    while (str[i] && str[i] != ' ')
        i++;

    return i;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle server-name server-port \n", argv[0]);
		exit(1);
	}
    if (strlen(argv[1]) > MAXHANDLE)
    {
        printf("Invalid handle, handle longer than %u characters: %s\n", MAXHANDLE, argv[1]);
        exit(1);
    }
    if (isdigit(argv[1][0]))
    {
        printf("Invalid handle, handle starts with a number\n");
        exit(1);
    }
}
