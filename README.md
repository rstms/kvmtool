# kvmtool

Windows command line tool for scanning local network for PiKVM MAC addresses


## Details

Uses built-in Windows ipconfig, ping, and arp. 

## Example: 
```
C:\Users\mkrueger> kvmtool
Scanning IPv4 network for hosts with MAC vendor [B8-27-EB, DC-A6-32, E4-5F-01]...
Sending ICMP ECHO_REQUEST to each host in 192.168.66.255...
Searching ARP table...
192.168.66.85

C:\Users\mkrueger> kvmtool -q
192.168.66.85
```
