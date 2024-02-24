//
// network functions
//

#include <stdbool.h>

void network_startup(void); 
void network_shutdown(void);
char *calculate_broadcast_address(const char* addr, const char* mask);
int ping_all_hosts(char *addr, char *netmask, bool verbose);
