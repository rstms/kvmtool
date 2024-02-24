#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

typedef uint32_t in_addr_t;

#include "network.h"

void network_startup(void) 
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
}

void network_shutdown(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}


// Function to calculate the broadcast address from an IP address and subnet mask
char* calculate_broadcast_address(const char* addr, const char* mask) {
    // Convert IP address and subnet mask from dotted-decimal to binary representation
    in_addr_t bin_addr = inet_addr(addr);
    in_addr_t bin_mask = inet_addr(mask);

    // Calculate broadcast address: IP OR (NOT subnet mask)
    in_addr_t bin_broadcast = bin_addr | (~bin_mask);

    // Convert binary broadcast address back to dotted-decimal notation
    struct in_addr broadcast_addr;
    broadcast_addr.s_addr = bin_broadcast;

    // Use inet_ntoa to convert to string; note: this returns a statically allocated string
    char* broadcast_str = inet_ntoa(broadcast_addr);
    
    char* result = strdup(broadcast_str); // Caller must free this memory
    return result;
}


// Function to calculate the first and last host IP in the subnet
void calculate_ip_range(const char *ip_str, const char *netmask_str, struct in_addr *start, struct in_addr *end) {
    struct in_addr ip, netmask, network, broadcast;

    // Convert IP and netmask from dotted decimal to binary
    ip.s_addr = inet_addr(ip_str);
    netmask.s_addr = inet_addr(netmask_str);

    // Calculate network address: IP & netmask
    network.s_addr = ip.s_addr & netmask.s_addr;

    // Calculate broadcast address: network | ~netmask
    broadcast.s_addr = network.s_addr | ~netmask.s_addr;

    // First addressable host: network address + 1
    start->s_addr = htonl(ntohl(network.s_addr) + 1);

    // Last addressable host: broadcast address - 1
    end->s_addr = htonl(ntohl(broadcast.s_addr) - 1);
}

void *ping_thread(void *arg) 
{
    system((char *)arg);
    return arg;
}

int ping_all_hosts(char *addr, char *netmask, bool verbose) 
{
    char command[256];
    struct in_addr start, end;
    calculate_ip_range(addr, netmask, &start, &end);

    uint32_t start_host = ntohl(start.s_addr);
    uint32_t end_host = ntohl(end.s_addr);

    int num_ips = end_host - start_host;

    pthread_t threads[num_ips];

    unsigned long ping_addr = start_host;
    for (int i=0; i<num_ips; i++) {
	struct in_addr current;
	current.s_addr = htonl(start_host + i);
	snprintf(command, sizeof(command), "ping -n 1 %s >nul", inet_ntoa(current));
	char *arg = strdup(command);
	if (pthread_create(&threads[i], NULL, ping_thread, (void *)arg)) {
	    perror("thread create");
	    exit(errno);
	}
    }

    for (int i=0; i<num_ips; i++) {
	char *ret;
	if (pthread_join(threads[i], (void **)&ret)) {
	    perror("thread join");
	    exit(errno);
	}
	free(ret);
	if(verbose) {
	    putchar('.');
	}
    }
    if (verbose) putchar('\n');


    return 0;
}

