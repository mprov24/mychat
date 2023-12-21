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
#include <ctype.h>

#include "networks.h"
#include "safeUtil.h"
#include "processPDU.h"
#include "pollLib.h"

#define MAXBUF 1400
#define DEBUG_FLAG 0


void clientControl(char *handle, int serverSock);
int initialHandshake(int serverSock, char *handle, uint8_t *sendBuf, uint8_t *recvBuf);

// reading messages from server
void processMsgFromServer(int serverSock, uint8_t *recvBuf, int recvLen);
void recieveHandles(int serverSock, uint8_t *recvBuf);

// stdin stuff
int readFromStdin(uint8_t * buffer);
void processStdin(int serverSock, uint8_t *inputBuf, int inputLen, uint8_t *sendBuf, char *handle);

// sending out packets
void sendMulticast(int serverSock, uint8_t *inputBuf, uint8_t *sendBuf, char *handle, uint8_t handleLen);
void sendUnicast(int serverSock, uint8_t *inputBuf, uint8_t *sendBuf, char *handle, uint8_t handleLen);
void sendBroadcast(int serverSock, uint8_t *inputBuf, uint8_t *sendBuf, char *handle, uint8_t handleLen);
void sendTextMessages(int serverSock, char *textBuf, uint8_t *sendBuf, int textOffset);

// for reading stdin, ignoring whitespace
char * findKeyword(char *str);
int findKeylen(char *str);

void checkArgs(int argc, char * argv[]);
