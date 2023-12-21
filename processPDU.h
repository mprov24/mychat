#ifndef __PROCESSPDU_H__
#define __PROCESSPDU_H__

#include <netinet/in.h>

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize);

#endif