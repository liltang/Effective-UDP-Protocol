#ifndef __PACKET__
#define __PACKET__

#define FILENAME "100MB_file.dat"
#define RECV_FILE "recvFile.dat"
#define MAXSEND 20
#define BUFFERSIZE 1400

typedef struct udpPacket {
    uint32_t seqNum;                // Sequence Number
    uint32_t dataSize;              // Size of the data being stored
    char dataBuffer[BUFFERSIZE];    // Actual packet data 
}udpPacket;

#endif