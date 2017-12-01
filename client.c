//========================================= file = udpFileSend.c ==============
//=  A file transfer program using TCP - this is the file send                =
//=============================================================================
//=  Notes:                                                                   =
//=    1) This program conditionally compiles for Winsock and BSD sockets.    =
//=       Set the initial #define to WIN or BSD as appropriate.               =
//=    2) This program sends a file to udpFileSend                            =
//=    3) This program take command line input as shown below                 =
//=    4) Ignore build warnings on unused retcode and options.                =
//=---------------------------------------------------------------------------=
//=  Example execution: (for udpFileSend sendFile.dat 127.0.0.1 1050)         =
//=    Starting file transfer...                                              =
//=    File transfer is complete                                              =
//=---------------------------------------------------------------------------=
//=  Build: makefile                                                          =
//=---------------------------------------------------------------------------=
//=  Execute: udpFileSend                                                     =
//=---------------------------------------------------------------------------=
//=  Author:                                                                  =
//=          University of South Florida                                      =
//=          Email:                                                           =
//=---------------------------------------------------------------------------=
//=  History:  KJC (__/__/17) - Genesis                                       =
//=            Lily Tang (10/05/17) - Added function int sendFile             =
//=            Lily Tang (10/31/17) - Added function getTime                  =
//=            Lily Tang (11/02/17) - Deleted function getTime :(             =
//=            Lily Tang (11/02/17) - Added function searchList and others    =
//=            Lily Tang (11/22/17) - Put in more comments so it looks better =
//=            Lily Tang (11/22/17) - Cleaned up unused stuff                 =
//=            Lily Tang (11/24/17) - Deleted inefficient code for high speed =
//=============================================================================
#define  BSD                // WIN for Winsock and BSD for BSD sockets

//----- Include files ---------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <string.h>         // Needed for memcpy() and strcpy()
#include <stdlib.h>         // Needed for exit()
#include <fcntl.h>          // Needed for file i/o constants
#include <sys/stat.h>       // Needed for file i/o constants
#include <pthread.h>        // Needed for threads 
#include <unistd.h>         // Needed for threads
#include <sys/time.h>       // Needed for timing 
#include <time.h>           // Needed for timing
#include <sys/mman.h>       // Needed for memory management
#include <math.h>           // Quick maffs

#ifdef WIN
  #include <windows.h>      // Needed for all Winsock stuff
#endif
#ifdef BSD
  #include <sys/types.h>    // Needed for sockets stuff
  #include <netinet/in.h>   // Needed for sockets stuff
  #include <sys/socket.h>   // Needed for sockets stuff
  #include <arpa/inet.h>    // Needed for sockets stuff
  #include <fcntl.h>        // Needed for sockets stuff
  #include <netdb.h>        // Needed for sockets stuff
#endif

#include "discard.h"        // This code belongs to Dr. Christensen (not mine, don't sue)
                            // Source: http://www.csee.usf.edu/~kchriste/class2/discard.c
#include "packet.h"         // Packet stuff and maybe shared functions


//----- Prototypes ------------------------------------------------------------
int sendFile(char *fileName, char *destIpAddr, int port);
void *retransmit(void* arg);
void deliverChunks(int windowSize, int seq);

//----- Globals ------------------------------------------------------------
char *buffer;                                        // buffer
char *seqNumList;                                    // seq_no tracker
struct sockaddr_in sockaddr_in;                      // socket address
struct stat bufferStatus;                            // buffer status
int client_socket;                                   // client socket
int searchIndex;                                    // iterator
int fileSize = 0;                                    // file size
int errno= -1;                                       // error number
pthread_t udpThread[1];                              // pthread
int totalArrived = 0, totalExpected = 0;             // expected vs real
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;    // pthread mutex

//===== Main program ==========================================================
int main(int argc, char *argv[])
{   
  // Usage and parsing command line arguments
  if (argc < 3)
  {
    printf("usage: 'udpFileSend sendFile recvIpAddr recvPort' where        \n");
    printf("       sendFile is the filename of an existing file to be sent \n");
    printf("       to the receiver, recvIpAddr is the IP address of the    \n");
    printf("       receiver, and recvPort is the port number for the       \n");
    printf("       receiver where udpFileRecv is running.                  \n");
    return(0);
  }
  // Display connection information
  printf("File:                    %s\n", argv[1]);
  printf("Server IP:               %s\n", argv[2]);
  printf("Port:                    %s\n", argv[3]);

  // Set parameters
  char *fileName = argv[1];
  char *recvIpAddr = argv[2];
  int port = atoi(argv[3]);

  // Send the file
  printf("\nStarting file transfer...\n");
  int retcode;
  retcode = sendFile(fileName, recvIpAddr, port);  
  printf("File transfer is complete ...\n\n");

  // Goodbye
  return retcode;
}

//=============================================================================
//=  Function to send a file using TCP                                        =
//=============================================================================
//=  Inputs:                                                                  =
//=    fileName ----- Name of file to open, read, and send                    =
//=    recvIpAddr --- IP address or receiver                                  =
//=    port     ----- Port number receiver is listening on                    =
//=    options ------ Options (not implemented)                               =
//=---------------------------------------------------------------------------=
//=  Outputs:                                                                 =
//=    Returns -1 for fail and 0 for success                                  =
//=---------------------------------------------------------------------------=
//=  Side effects:                                                            =
//=    None known                                                             =
//=---------------------------------------------------------------------------=
//=  Bugs:                                                                    =
//=---------------------------------------------------------------------------=
int sendFile(char *fileName, char *recvIpAddr, int port){

  in_addr_t in_addr;                                   // IPv4 binary address
  int fileDesc;                                        // file descriptor
  struct hostent *hostname;                            // hostname
  unsigned short server_port = (unsigned short) port;  // server port num
  int bytes_read;                                      // recv() and sendto() 
  udpPacket serverPacket;                              // packet from the server
  int ackMessage = -1;                                 // the "ACK" message
  struct stat status;                                  // getting the file status 
  int i = 0, seq = 0, windowSize =0;                   // misc. integers
  
  socklen_t fromlen;
    
  // Open the file
  fileDesc = open(fileName, O_RDONLY);
  if (fileDesc == -1)
  {
    printf("*** Error - open() file failed \n");
    exit(-1);
  }
  // Get the file status
  if(fstat(fileDesc,&bufferStatus) < 0)
  {
     printf("*** Error - fstat() failed \n");
     exit(-1);
  }
  // Create a UDP socket
  client_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (client_socket == -1)
  {
    printf("*** Error - socket() failed \n");
    exit(-1);
  }
  // Get the host via IP Address 
  hostname = gethostbyname(recvIpAddr);
  if (hostname == NULL)
  {
    printf("*** Error - gethostbyname(%s) failed \n", recvIpAddr);
    exit(-1);
  }
  // Convert the IPv4 dot notation IP Address into binary data
  in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostname->h_addr_list)));
  
  // Fill-in the server's address information.
  bzero((char *) &sockaddr_in, sizeof(sockaddr_in)); 
  sockaddr_in.sin_family = AF_INET;
  sockaddr_in.sin_port = htons(server_port);
  sockaddr_in.sin_addr.s_addr = in_addr;    
  
  // Get the file size
  if(stat(fileName,&status)==0)
  {
    fileSize = status.st_size;
    printf("The file is %d bytes long.\n", fileSize);
  }
  // Tell the server the size of the file
  for(i=0; i<10; i++){
    bytes_read = packetLoser(client_socket,&fileSize,sizeof(fileSize), 0,(struct sockaddr *) &sockaddr_in,sizeof(sockaddr_in));
    if (bytes_read < 0)
    {
      printf("*** Error - packetLoser() failed \n");
      exit(-1);
    }
  }
  // One thread at a time
  pthread_mutex_lock(&lock);
  // The buffer is a pointer to the entire file being mapped
  // mmap is preferred over file IO because
  // a) we want to share the file betweeen processes 
  // b) we want to beat TCP in speed
  // Source: https://stackoverflow.com/a/9818286
  buffer = mmap((caddr_t)0, bufferStatus.st_size , PROT_READ , MAP_SHARED , fileDesc , 0 ) ;
  
  if(buffer == MAP_FAILED)
  {
    printf("*** Error - mmap() failed \n");
    exit(-1);
  }
  // Set the data size
  windowSize = bufferStatus.st_size;
  
  pthread_mutex_unlock(&lock);
  
  // Create a new thread that will check if the server had any
  // missing packets. If so, we must retransmit
  if((errno = pthread_create(&udpThread[0], 0, retransmit , (void*)0 ))){
    fprintf(stderr, "pthread_create[0] %s\n",strerror(errno));
    pthread_exit(0);
  } 
  // Blast out packets to the server in chunks
  deliverChunks(windowSize, seq);
  
  // Wait for threads to terminate  
  pthread_join(udpThread[0], 0);
   
  // Delete the mapping to the file 
  munmap(buffer, bufferStatus.st_size);
    
  close(client_socket);
  close(fileDesc);
  return 0;
}

// This function checks if additional packets need to be transmitted
void *retransmit(void* arg){
    int totalSeqNum, lastSeqNum, packetSize, bytes_left, bytes_read = 0;
    udpPacket extraPacket;
    int packetmiss;
    socklen_t serverLength = sizeof(sockaddr_in);
    
    printf("Retransmitting lost packets\n");
    while (1) {
        // Get the reported missing packet sequence numbers from the server
        bytes_read = recvfrom(client_socket,&packetmiss,sizeof(int),0,(struct sockaddr *)&sockaddr_in, &serverLength);
        if(bytes_read < 0){
           printf("*** Error - recv() failed \n");
           exit(-1);
        }
        // No errors! Done transmitting
        if( packetmiss < 0){
            printf("No errors! Done transmitting\n");
            pthread_exit(0);
        }
        totalSeqNum =  fileSize / BUFFERSIZE;
        bytes_left = fileSize % BUFFERSIZE;
        packetSize = BUFFERSIZE;
        if(bytes_left != 0 && totalSeqNum ==  packetmiss)
        {
        	packetSize = bytes_left;
        }
        pthread_mutex_lock(&lock);
        
        memcpy( extraPacket.dataBuffer, &buffer[packetmiss*BUFFERSIZE] , packetSize );
        pthread_mutex_unlock(&lock);
        
        // Create the packet
        extraPacket.seqNum = packetmiss;
        extraPacket.dataSize = packetSize;        
        // There was a packet loss, so we retransmit the packet
        bytes_read = packetLoser(client_socket,&extraPacket,sizeof(udpPacket), 0,(struct sockaddr *) &sockaddr_in,serverLength);
        if (bytes_read < 0){
           printf("*** Error - packetLoser() failed \n");
           exit(-1);  
        }
    }
}
// Blast the packets out until windowSize
// is exhausted
void deliverChunks(int windowSize, int seq){
  while (windowSize > 0) {
        int chunk = BUFFERSIZE;
        int leftover = windowSize;
        udpPacket sentPacket;
        memset(&sentPacket , 0 , sizeof(udpPacket));
        
        if(leftover - chunk < 0){
            chunk = leftover;
        } else {
            leftover = leftover - chunk;
        }
        // One thread at a time
        pthread_mutex_lock(&lock);
        memcpy(sentPacket.dataBuffer, &buffer[seq*BUFFERSIZE], chunk);
        pthread_mutex_unlock(&lock);
        // Create a packet
        sentPacket.seqNum = seq;
        sentPacket.dataSize = chunk;
        usleep(100);
        // Send the packet out to the server
        packetLoser(client_socket , &sentPacket , sizeof(sentPacket) , 0 , (struct sockaddr *) &sockaddr_in,sizeof(sockaddr_in));
        seq++;
        windowSize -= chunk;
    }
}