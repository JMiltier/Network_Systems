# --------------------------------------------------------
# Makefile - versitility to use again for C or C++ files
# 'make'        builds executable targets
# 'make DFC'    creates DF client executable object
# 'make DFS'    creates DF server executable object
# 'make clean'  removes all .o and executable files
# --------------------------------------------------------

# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g			adds debugging information to the executable file (dSYM files)
#  -Wall	turns on most, but not all, compiler warnings
#  -w			inhibits all warning messages
CFLAGS = -w

DFC=dfc
DFCSRC=dfc.c
DFS=dfs
DFSSRC=dfs.c

.PHONY: clean all

all: clean $(DFC) $(DFS)

$(DFC): $(DFCSRC)
	$(CC) $(CFGLAGS) $(DFCSRC) -o $(DFC)

$(DFS): $(DFSSRC)
	$(CC) $(CFGLAGS) $(DFSSRC) -o $(DFS)

clean:
	rm -rf $(DFC) $(DFS)