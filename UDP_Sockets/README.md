## UDP Sockets
A connectionless transmission model with a minimum protocol mechanism. Unlike TCP, UDP (Use Datagram Protocol) have no handshaking/acknowledgements of data (No ACKs). Because of this, it's not guaranteed that any data is received by the end host. The up side, this avoids some delays due to it's simplicity.

### Program
A *C Language* program that sends UDP packets to a specified host.


### Commands
1. Client takes two command line arguments. The first is the IP address, and the second is the port number the server application is using. To find the IP address of the current server, try `hostname -i` (Linux).
  - Complile the *C*-program
    `gcc uftp_client.c -o client`
  - Run client with a given server IP and port number
    `./client 192.168.1.101 5001`


#### Connect to CU Boulder's server:
SSH into server: `ssh <IDENTIKEY>@elra-03.cs.colorado.edu`, password is same as IDENTIKEYs