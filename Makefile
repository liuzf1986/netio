CC=g++
LD=g++
CFLAGS=-c -g -std=c++11 -Iinclude
LDFLAGS=-Wl,--no-as-needed -lpthread

OBJS=test.o InetAddr.o LogFile.o TimeUtil.o Logger.o
TARGET=test

$(TARGET):$(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) $<

%.o : %.cpp
	$(CC) $(CFLAGS) $<

test.o : include/Logger.hpp

.PHONY:clean
clean:
	rm -f $(TARGET) $(OBJS) core
