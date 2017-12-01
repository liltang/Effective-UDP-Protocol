# Effective-UDP-Protocol
Scalable and fast UDP file transfer protocol design

In this network programming project, the goal was to design and implement a file transfer program utilizing UDP, while giving competitive performance and reliability to comparable TCP based
solutions. Toward that end, this experimental protocol combines elements of traditional selective repeat protocols, and Network Block Transfer (NETBLT) protocol. 

It further modernizes the techniques found in these protocols, mainly through the use of POSIX threads, to allow previously discrete and serialized protocol states to be performed in parallel, greatly increasing performance.
Implementation and testing has shown that this protocol provides reliable file transfer, in approximately half the transfer time of a TCP-based solution.

Experimental Results:
Step #	Disc.	 Time 1	  Time 2	Time 3	Mean	% of TCP Time
1	TCP local	    25.233	25.296	25.287	25.272	100%
2	No loss local	14.989	14.059	13.960	14.336	57%
3	2% loss local	14.147	14.161	14.150	14.153	56%
4	TCP WiFi	    41.767	39.795	40.679	40.747	100%
5	WiFi	        20.287	19.105	19.534	19.642	48%
						
	SR6X WiFi	22.587	24.260	23.339		
  
![alt text][logo]

[logo]: https://github.com/adam-p/markdown-here/raw/master/src/common/images/icon48.png "Logo Title Text 2"
