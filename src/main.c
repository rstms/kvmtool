//
// findpikvm - attempt to find the IP address of a PiKVM on the local network 
//

//#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "network.h"

bool verbose = false;
bool quiet = false;

// https://docs.pikvm.org/first_steps/#first-power-on
char *pikvm_mac_prefix[] = {
    "B8-27-EB",
    "DC-A6-32",
    "E4-5F-01",
    NULL
};


char *rtrim(char *str) {
    
    char *pos = str + strlen(str);
    while(pos >= str) {
	if (isspace(*pos)) {
	    *pos = 0;
	    break;
	}
	--pos;
    }
    return str;
}

void replace(char **str, char *value) 
{
    // if str is not NULL, it is a strdup return, so free it
    if(*str) free(*str);
    if(value) {
	// if value is not NULL, strdup it
	*str = strdup(value);
    } else {
	*str = NULL;
    }
}

char *parse_interface(char *line, char **result)
{
    char *pos = strstr(line, "adapter");
    if (pos) {
	if (strtok(pos, " \t")) {
	    replace(result, strtok(NULL, ": \t"));
	    return *result;
	}
    }
    return NULL;
}

char *parse(char *line, char *pattern, char *label, char **result)
{
    char *pos;

    if (strstr(line, pattern)) {
	if (strtok(line, ":")) {
	    if(pos = strtok(NULL, ": (")) {
		replace(result, pos);
		return *result;
	    }
	}
    }
    return NULL;
}

void print_header()
{
    printf("Scanning IPv4 network for hosts with MAC vendor [");
    bool first = true;
    for(char **prefix = pikvm_mac_prefix; *prefix; ++prefix, first=false) {
	if (!first) {
	    printf(", ");
	}
	printf("%s", *prefix);
    }
    printf("]...\n");
}

int main(int argc, char*argv[]) 
{
    char template[] = "findkvmXXXXXX";
    char buf[1024];

    char pathname[L_tmpnam+1];
    char *ofname;

    char *line;
    char *pos;

    char *if_name = NULL;
    char *ip_addr = NULL;
    char *gw_addr = NULL;
    char *mask = NULL;
    char *broadcast = NULL;

    for(int i=1; i<argc; i++) {
	if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) verbose=true;
	if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet")) quiet=true;
    }

    if (!quiet) print_header();

    network_startup();

    ofname = tmpnam(pathname);

    if (verbose) printf("Reading network config...\n");
    snprintf(buf, sizeof(buf), "ipconfig >%s", ofname);
    system(buf);

    FILE *ifp = fopen(ofname, "r");
    while(line=fgets(buf, sizeof(buf), ifp)) {

	line=rtrim(line);

	if (isalpha(line[0])) {
		if (gw_addr) break;
		parse_interface(line, &if_name);
	}
	parse(line, "IPv4 Address", "ip_addr", &ip_addr);
	parse(line, "Default Gateway", "gw_addr", &gw_addr);
	parse(line, "Subnet Mask", "mask", &mask);
    }
    fclose(ifp);

    broadcast = calculate_broadcast_address(ip_addr, mask);

    if (verbose) {
	printf("  interface : %s\n", if_name);
	printf("  address   : %s\n", ip_addr);
	printf("  gateway   : %s\n", gw_addr);
	printf("  netmask   : %s\n", mask);
	printf("  broadcast : %s\n", broadcast);
    }

    if (!quiet) printf("Sending ICMP ECHO_REQUEST to each host in %s...", broadcast);
    if (!verbose && !quiet) putchar('\n');
    ping_all_hosts(ip_addr, mask, verbose);

    if (!quiet) printf("Searching ARP table...\n");
    snprintf(buf, sizeof(buf), "arp -a > %s", ofname);
    system(buf);

    ifp = fopen(ofname, "r");
    bool found=false;
    while(line=fgets(buf, sizeof(buf), ifp)) {
	line=rtrim(line);
	char *pi_addr = strtok(line, " \t");
	if (!pi_addr) continue;
	char *mac= strtok(NULL, " \t");
	if (mac && strlen(mac) > 2 && mac[2]=='-') {
	    strupr(mac);
	    for(char **prefix = pikvm_mac_prefix; *prefix; ++prefix) {
		if (!strncmp(*prefix, mac, strlen(*prefix))) {
		    if (verbose) {
			printf("\nPossible PiKVM found at %s with MAC %s\n", pi_addr, mac);
		    }
		    else {
			puts(pi_addr);
		    }
		    found = true;
		}
	    } 
	}
    }
    fclose(ifp);

    unlink(ofname);
    if (if_name) free(if_name);
    if (ip_addr) free(ip_addr);
    if (gw_addr) free(gw_addr);
    if (mask) free(mask);
    if (broadcast) free(broadcast);

    network_shutdown();

    if (found) {
	return 0;
    } else {
	if (!quiet) fprintf(stderr, "No matches found.\n");
	return -1;
    }
}
