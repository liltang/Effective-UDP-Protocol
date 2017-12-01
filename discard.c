//==================================================== file = discard.c =====
//=  Program to discard a percentage of messages                            =
//===========================================================================
//=  Notes:                                                                 =
//=    1) This program is a useful example for the class project            =
//=       to emulate a packet loss rate.                                    =
//=-------------------------------------------------------------------------=
//=  Build: gcc dicard.c -odiscard, cl discard.c                            =
//=-------------------------------------------------------------------------=
//=  Execute: discard                                                       =
//=-------------------------------------------------------------------------=
//=  Author: Ken Christensen                                                =
//=          University of South Florida                                    =
//=          WWW: http://www.csee.usf.edu/~christen                         =
//=          Email: christen@csee.usf.edu                                   =
//=-------------------------------------------------------------------------=
//=  History: KJC (10/17/17) - Genesis (from rand.c)                        =
//=  History: Lily (11/17/17) - Added packetLoser                           =
//===========================================================================

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#define DISCARD_RATE  0.02

// Random Number Generator
double rand_val(void)
{
  const long  a =      16807;  // Multiplier
  const long  m = 2147483647;  // Modulus
  const long  q =     127773;  // m div a
  const long  r =       2836;  // m mod a
  static long x = 1;           // Random int value (seed is set to 1)
  long        x_div_q;         // x divided by q
  long        x_mod_q;         // x modulo q
  long        x_new;           // New x value

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0)
    x = x_new;
  else
    x = x_new + m;

  // Return a random value between 0.0 and 1.0
  return((double) x / m);
}

// Sends the packet if random >= DISCARD_RATE
// Otherwise, the packet is lost and a dummy value is sent
ssize_t packetLoser(
      int socket_ptr, 
      const void *data, 
      size_t dataLength,
      int flags,
      const struct sockaddr *recv_addr,
      int recvLength
    ) {
    double random;
    int bytes_sent;
     
    random = rand_val();
    
    if (random < DISCARD_RATE) {  
    // Packet is lost --- do nothing, but make it look like success
	   return(dataLength);
    }
    
    // Otherwise: Nothing bad happened -- calling sendto like normal
    bytes_sent = sendto(socket_ptr,data,dataLength,flags,recv_addr,recvLength);
    return(bytes_sent);
} 

