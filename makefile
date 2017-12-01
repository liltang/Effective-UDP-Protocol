# variables
CC = gcc
CFLAGS = -g -pthread

UDPSERVER = server
UDPCLIENT = client
PACKET = packet

# rules
all: clean ${UDPSERVER} ${UDPCLIENT} cleanobj

udp: clean ${UDPSERVER} ${UDPCLIENT} cleanobj

${UDPSERVER}: ${PACKET}.o discard.o -lm

${UDPCLIENT}: ${PACKET}.o discard.o -lm

discard.o: discard.h

${PACKET}.o: ${PACKET}.h

clean:
	rm -f *.o ${UDPSERVER} ${UDPCLIENT}
cleanobj:
	rm -f *.o

