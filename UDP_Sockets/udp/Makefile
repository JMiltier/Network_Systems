# --------------------------------------------------------
# Makefile - versitility to use again for C or C++ files
# 'make'        builds executable targets
# 'make clean'  removes all .o and executable files
# --------------------------------------------------------

# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g			adds debugging information to the executable file (dSYM files)
#  -Wall	turns on most, but not all, compiler warnings
#  -w			inhibits all warning messages
CFLAGS  = -w

# the build target executable:
CLIENT = client/udp_client
SERVER = server/udp_server

all: clean $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT).c
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).c

$(SERVER): $(SERVER).c
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).c

clean:
	$(RM) $(CLIENT) $(SERVER)