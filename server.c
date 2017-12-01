//============================== file = udpFileRecv.c =========================
//=  A file transfer program using UDP - this is the file receiver            =
//=============================================================================
//=  Notes:                                                                   =
//=    1) This program conditionally compiles for Winsock and BSD sockets.    =
//=       Set the initial #define to WIN or BSD as appropriate.               =
//=    2) This program receives and writes a file sent from tcpFileSend.      =
//=    3) This program must be running first for tcpFileSend to connect to.   =
//=    4) Ignore build warnings on unused retcode, maxSize, and options.      =
//=---------------------------------------------------------------------------=
//=  Example execution:                                                       =
//=    Starting file transfer...                                              =
//=    File transfer is complete                                              =
//=---------------------------------------------------------------------------=
//=  Build: makefile                                                          =
//=---------------------------------------------------------------------------=
//=  Execute: udpFileRecv                                                     =
//=---------------------------------------------------------------------------=
//=  Author:                                                                  =
//=          University of South Florida                                      =
//=          Email:                                                           =
//=---------------------------------------------------------------------------=
//=  History:  KJC (__/__/17) - Genesis                                       =
//=            Lily Tang (10/31/17) - Added function int recvFile             =
//=            Lily Tang (11/02/17) - Added function transmit                 =
//=            Lily Tang (11/21/17) - Added function deliverChunks            =
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
#include "packet.h"         // Packet stuff and shared functions

//----- Globals ------------------------------------------------------------
char *buffer;                                        // buffer
char *seqNumList;                                    // seq_no tracker
struct sockaddr_in sockaddr_in;                      // socket address
struct stat bufferStatus;                            // buffer status
int server_socket;                                   // client socket
int searchIndex;                                    // iterator
int fileSize = 0;                                    // file size
int errno= -1;                                       // error number
pthread_t udpThread[1];                              // pthread
int totalArrived = 0, totalExpected = 0;             // expected vs real
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;    // pthread mutex


char *data;
int datarecv = 0;

//----- Prototypes ------------------------------------------------------------
char *fileManager(char *str);
int searchList();
void *checkForMissing(void* arg);
int isReceived(int seqNum);
int recvFile(int port);

//===== Main program ==========================================================
int main(int argc, char *argv[])
{
  // Usage and parsing command line arguments
  if (argc < 1)
  {
    printf("usage: 'udpFileRecv portNumber' where         \n");
    printf("       recvPort is the port number for the    \n");
    printf("       receiver where udpFileSend is running. \n");
    return(0);
  }

  // Display connection information
  printf("Port:                    %s\n", argv[1]);

  // Set parameters
  int port = atoi(argv[1]);

  // Send the file
  printf("\nStarting file transfer...\n");
  int retcode;
  retcode = recvFile(port);  
  printf("File transfer is complete ...\n\n");

  // Goodbye
  return retcode;
}

//=============================================================================
//=  Function to receive a file using UDP                                     =
//=============================================================================
//=  Inputs:                                                                  =
//=    port --- Port number to listen and receive on                          =
//=---------------------------------------------------------------------------=
//=  Outputs:                                                                 =
//=    Returns -1 for fail and 0 for success                                  =
//=---------------------------------------------------------------------------=
//=  Side effects:                                                            =
//=    Stress                                                                 =
//=---------------------------------------------------------------------------=
//=  Bugs:                                                                    =
//=---------------------------------------------------------------------------=
int recvFile(int port){
  in_addr_t in_addr;                                   // IPv4 binary address
  int fileDesc;                                        // file descriptor
  unsigned short server_port = (unsigned short) port;  // server port num
  int bytes_read;                                      // recv() and sendto() 
  udpPacket serverPacket;                              // packet from the server
  struct stat status;                                  // getting the file status 
  FILE *fp;                                            // file pointer
  int i = 0, seq = 0, windowSize =0;                   // misc. integers
  int ackMessage = -1;                                 // the "ACK" message
  socklen_t senderLength;                              // sender's socklen
  udpPacket recvPacket;                                    // upd packet
    
  // Create a UDP socket
  server_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_socket == -1)
  {
    perror("bc:");
    printf("*** Error - socket() failed \n");
    exit(-1);
  }
  
  // Fill-in the server's address information.
  bzero((char *) &sockaddr_in, sizeof(sockaddr_in)); 
  sockaddr_in.sin_family = AF_INET;
  sockaddr_in.sin_port = htons(port);
  sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
 
  // Bind the socket
  if (bind(server_socket,(struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in)) < 0)
  {
          perror("*** Error - bind() failed");
          exit(-1);
  }
  
  // Get the receiver sockaddr length
  senderLength = sizeof(struct sockaddr_in);

  // Get the file size from the client
  bytes_read = recvfrom(server_socket,&fileSize,sizeof(fileSize),0,(struct sockaddr *)&sockaddr_in,&senderLength);
  // Calculate the expected total sequence numbers
  totalExpected = ceil(fileSize / BUFFERSIZE);
  printf("Total expected sequence numbers: %d\n",totalExpected);
    
  // Write to the output file 
  fp = fopen(RECV_FILE, "w+");
    
  // Create a new thread and make it check for missing packets with sequence numbers
  if((errno = pthread_create(&udpThread[0], 0, checkForMissing , (void*)0 ))){
    fprintf(stderr, "pthread_create[0] %s\n",strerror(errno));
    pthread_exit(0);
  }
  // An array initialized with all zeros
  seqNumList = calloc(totalExpected , sizeof(char));
  
  // Recv packets from the client
  while (1) {
    bytes_read = recvfrom(server_socket,&recvPacket,sizeof(udpPacket),0,(struct sockaddr *)&sockaddr_in,&senderLength);
    if(bytes_read == sizeof(int) ){
      continue;
	 }
    if (bytes_read < 0){
      printf("*** Error - recvfrom() failed \n");
      exit(-1);
    }
    
    // Check if packet with sequence number seqNum has arrived    
    pthread_mutex_lock(&lock);
    if(isReceived(recvPacket.seqNum)){
      fseek(fp , BUFFERSIZE*recvPacket.seqNum , SEEK_SET);
      fwrite(&recvPacket.dataBuffer, recvPacket.dataSize , 1 , fp);
      fflush(fp);
      totalArrived++;
    } 
    pthread_mutex_unlock(&lock);
    
    // If we got the whole file, that means we're done!
    if(totalArrived == totalExpected+1 ){
      printf("We got the whole file\n");
        for(i=0 ; i<MAXSEND ; i++){
          printf("ACKing...\n");
          bytes_read = packetLoser(server_socket,&ackMessage,sizeof(int), 0,(struct sockaddr *) &sockaddr_in,sizeof(sockaddr_in));
          if (bytes_read < 0){
            printf("*** Error - packetLoser() failed \n");
            exit(-1);
          }
        }
        break;
    }
}

// Goodbye
pthread_join(udpThread[0], 0);
munmap(data, bufferStatus.st_size);
fclose(fp);
close(server_socket);

return 0;
}
// Set value to 1 if the packet has been recieved 
// Return 1 if sequence number is a new packet
// Returns 0 if sequence number has already been received
int isReceived(int seqNum){
  if( seqNum >=0 && seqNum <=totalExpected){
    if( seqNumList[seqNum] == 0 ){
		seqNumList[seqNum] = 1;
      return 1;
	 } else {
		return 0;
	 }
  }
  return 0;
}
// This function searches the list for
// the location of the missing packet
int searchList(){
	int i;
	if(seqNumList == NULL){
		return -1;
	}
	for ( i = searchIndex; i <= totalExpected ; i++)
	{
		if (seqNumList[i] == 0 )
		{
			if (i==totalExpected)
				searchIndex=0;
			else
				searchIndex=(i+1);
         return i;
		}
	}
	searchIndex=0;
	return -1;
}
// Handles file IO stuff
char *fileManager(char *str){
  int filePtr;
  char *data;
    
  filePtr = open(str, O_RDONLY);
  if(filePtr < 0 ){
  printf("*** Error - open() failed \n");
  exit(-1);
  }
  if(fstat(filePtr,&bufferStatus) < 0){
    printf("*** Error - fstat() failed \n");
    exit(-1);
  }  
  // Data is a pointer to the entire file being mapped
  // mmap is preferred over file IO because
  // a) we want to share the file betweeen processes 
  // b) we want to beat TCP in speed
  // Source: https://stackoverflow.com/a/9818286
  data = mmap((caddr_t)0, bufferStatus.st_size , PROT_READ , MAP_SHARED , filePtr , 0 ) ;  
  return data;
}
// Checks the sequence numbers of the packets. If all packets have arrived, exit the thread 
// Else, send the sequence numbers of the packets that are missing
// The idea to use threads came from this source:
// https://stackoverflow.com/a/40033557
void* checkForMissing(void* arg){
    int bytes_read = 0;
    int key;
    printf("Checking for received packets...\n");
    while(1)
    {
        if(totalArrived == totalExpected+1 ) {
            printf("All packets received!\n");
            pthread_exit(0);
        }
        usleep(100);
        pthread_mutex_lock(&lock);
        key = searchList();
        pthread_mutex_unlock(&lock);
        if(key >= 0 && key <=totalExpected){
            // Send the index of the missing packet 
            bytes_read = packetLoser(server_socket, &key ,sizeof(int),0,(struct sockaddr *)&sockaddr_in,sizeof(sockaddr_in));
          if (bytes_read < 0){
          printf("*** Error - packetLoser() failed \n");
          exit(-1);
          }
        }
        
    }
}
