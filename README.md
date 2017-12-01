# Effective-UDP-Protocol
Scalable and fast UDP file transfer protocol design

In this network programming project, the goal was to design and implement a file transfer program utilizing UDP, while giving competitive performance and reliability to comparable TCP based
solutions. Toward that end, this experimental protocol combines elements of traditional selective repeat protocols, and Network Block Transfer (NETBLT) protocol. 

It further modernizes the techniques found in these protocols, mainly through the use of POSIX threads, to allow previously discrete and serialized protocol states to be performed in parallel, greatly increasing performance.
Implementation and testing has shown that this protocol provides reliable file transfer, in approximately half the transfer time of a TCP-based solution.

Experimental Results:
  
![alt text][logo2]

[logo2]: https://github.com/liltang/Effective-UDP-Protocol/blob/master/2017-12-01_1408.png "Logo Title Text 3"
