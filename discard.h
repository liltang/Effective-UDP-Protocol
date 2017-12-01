#ifndef _DISCARD_H_
#define _DISCARD_H_

double rand_val(void);
ssize_t packetLoser(int s, const void *msg, size_t len,int flags,const struct sockaddr *to,int tolen);

#endif
