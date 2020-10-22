# --------------------------------------------------------
# Makefile - versitility to use again for C or C++ files
# 'make'            builds executable targets
# 'make webserver'  creates webserver executable object
# 'make echoserver' creates echoserver executable object
# 'make clean'      removes all .o and executable files
# --------------------------------------------------------

# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g			adds debugging information to the executable file (dSYM files)
#  -Wall	turns on most, but not all, compiler warnings
#  -w			inhibits all warning messages
CFLAGS = -w

WEBSVR=webserver
ECHSVR=echoserver
WEBSVRSRC=webserver.c
ECHSVRSRC=httpechosrv.c

.PHONY: clean all

all: $(WEBSVR)

$(WEBSVR):  $(WEBSVRSRC)
	$(CC) $(CFGLAGS) $(WEBSVRSRC) -o $(WEBSVR)

$(ECHSVR): $(ECHSVRSRC)
	$(CC) $(CFGLAGS) $(ECHSVRSRC) -o $(ECHSVR)

clean:
	rm -rf $(WEBSVR) $(ECHSVR)