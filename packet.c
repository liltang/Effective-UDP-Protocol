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

#include <sys/types.h>    // Needed for sockets stuff
#include <netinet/in.h>   // Needed for sockets stuff
#include <sys/socket.h>   // Needed for sockets stuff
#include <arpa/inet.h>    // Needed for sockets stuff
#include <fcntl.h>        // Needed for sockets stuff
#include <netdb.h>        // Needed for sockets stuff

#include "packet.h"

// There used to be things here but now I deleted it -- Lily (11/24)