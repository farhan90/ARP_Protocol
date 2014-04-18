ME=$(USER)
CC=gcc
UNP_DIR=/home/courses/cse533/Stevens/unpv13e
LIBS=/home/courses/cse533/Stevens/unpv13e/libunp.a
FLAGS=-g -O2 -Wall -Werror 
CFLAGS=$(FLAGS) -pthread -I$(UNP_DIR)/lib
EXE=$(ME)_tour $(ME)_arp test
INCLUDES=common.h me.h uds.h

all: $(EXE)

$(ME)_tour: tour.o route.o common.o ping.o uds.o
	gcc $(CFLAGS) -o $@ $^ $(LIBS)


$(ME)_arp : arp.o arplist.o get_hw_addrs.o common.o arpmsg.o uds.o
	gcc ${CFLAG} -o $@ $^ $(LIBS)


test: test.o uds.o common.o ping.o
	gcc ${CFLAG} -o $@ $^ $(LIBS)

%.o: %.c %.h $(INCLUDES)
	gcc $(CFLAGS) -c $^

clean:
	rm -f *.o *.out *.gch $(EXE) *~
