## UDP Sockets
A connectionless transmission model with a minimum protocol mechanism. Unlike TCP, UDP (Use Datagram Protocol) have no handshaking/acknowledgements of data (No ACKs). Because of this, it's not guaranteed that any data is received by the end host. The up side, this avoids some delays due to it's simplicity.

### Program
A *C Language* program that sends UDP packets to a specified server host from a client host.

### Makefile
- To *create* executable objects of client and server, run `make` within the **udp** directory.
    - Note: Compiler warning messages are disabled.
- To *remove/clean* objects created, run `make clean`.

### Commands
Client takes two command line arguments. The first is the IP address, and the second is the port number the server application is using. To find the IP address of the current server, try `hostname -i` (Linux).  
    -Complile the *C*-program  
        `gcc udp_client.c -o udp_client`  
        `gcc udp_server.c -o udp_server`   
        *OR* run [makefile](#makefile) commands (above)

#### Connect to one of CUB's elra servers:
SSH into Linux VM: `ssh <IDENTIKEY>@elra-##.cs.colorado.edu`, ## can be 01, 02, 03, or 04  
*Password* is same as IDENTIKEYs  
Find IP of server: `ip addr`

#### Sending echo requests to server
  1. Create object files, listed under [Commands](#commands) above.
  2. Open port on server to connect incoming traffic to `./server <port>`
  3. Send an **echo** request to the server `./client <server ip> <port>`
  4. Enter message when prompted on the client
  5. *Note: this can be run locally, using different ports for the server and client*
