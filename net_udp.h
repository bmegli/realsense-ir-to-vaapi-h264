#pragma once

#include <netinet/in.h> //socaddr_in

void InitNetworkUDP(int *sock,struct sockaddr_in *si_dest,  const char *host, int port, int timeout_ms);
void CloseNetworkUDP(int sock);
void SendToUDP(int sock, const struct sockaddr_in &dest, const char *data, int data_size);

