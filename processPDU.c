#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "safeUtil.h"

int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData)
{
    uint16_t pduLen = lengthOfData + 2;
    uint8_t pduBuff[pduLen];

    uint16_t pduLenNet = htons(pduLen);
    memcpy(pduBuff, &pduLenNet,2);
    memcpy(pduBuff+2, dataBuffer,lengthOfData);

    return (safeSend(clientSocket, pduBuff, pduLen, 0)-2);
}

int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize)
{
    uint16_t pduLen = 0;
    // if client closed return 0 immediately
    if (safeRecv(socketNumber, (uint8_t *)(&pduLen), 2, MSG_WAITALL) == 0)
        return 0;
    int dataLen = ntohs(pduLen) - 2;
    if (bufferSize < dataLen)
    {
        printf("recvPDU: bufferSize not big enough. Msg len = %u bytes\n", pduLen);
        exit(-1);
    }

    return safeRecv(socketNumber, dataBuffer, dataLen, MSG_WAITALL);
}
