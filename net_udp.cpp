#include "net_udp.h"

#include "misc.h"
#include <string.h> //memset
#include <arpa/inet.h> //inet_pton, etc
#include <unistd.h> //close

int InitSocketUDP(int port, int timeout_ms)
{
	struct sockaddr_in si_me;
	struct timeval tv;   
	
	int s;

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP)) == -1)    
        DieErrno("InitSocketUDP, socket");
    		
	//set timeout if necessary
	if(timeout_ms > 0)
	{
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms-(timeout_ms/1000)*1000) * 1000;		
				
		if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
			DieErrno("InitSocketUDP, setsockopt");				
	}		
			
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
	//prepare address 
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    //if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
      //  DieErrno("InitSocketUDP, bind");
    
	return s;
}

void InitDestinationUDP(struct sockaddr_in *si_dest, const char *host, int port)
{
	si_dest->sin_family = AF_INET;
	if (!inet_pton(AF_INET, host, &si_dest->sin_addr))
		DieErrno("InitDestinationUDP, inet_pton");
	si_dest->sin_port = htons(port);
}

void InitNetworkUDP(int *sock, struct sockaddr_in *si_dest,  const char *host, int port, int timeout_ms)
{
	*sock=InitSocketUDP(port, timeout_ms);
	
	if(host)
		InitDestinationUDP(si_dest, host, port);
}
void CloseNetworkUDP(int sock)
{
	if( close(sock) == -1 )
		Die("CloseNetworkUDP, close");
}

void SendToUDP(int sock, const struct sockaddr_in &dest, const char *data, int data_size)
{
	int result;
	int written=0;
	
	while(written<data_size)
	{
		if ((result = sendto(sock, data+written, data_size-written, 0, (struct sockaddr*)&dest, sizeof(dest))) == -1)
			DieErrno("SendToUDP, sendto failed");
		written += result;		
	}
}