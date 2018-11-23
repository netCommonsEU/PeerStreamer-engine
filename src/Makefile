SRC=$(wildcard *.c)
SRC+=$(wildcard rbtree/*.c)
OBJS=$(SRC:.c=.o)

GRAPES ?= ../../grapes
NET_HELPER ?= ../Lib/net_helper
TARGET=libpstreamer.a

ifdef DEBUG
CFLAGS += -g -W -Wall -O0 -DDEBUG -Wno-unused-parameter -DLOG_CHUNK -DLOG_SIGNAL -Wno-unused-function 
endif
CFLAGS += -I./ -I$(GRAPES)/include -I../include -I$(NET_HELPER)/include 

$(TARGET): $(OBJS) $(SRC)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS) 

clean:
	rm -f rbtree/*.o *.o $(TARGET)

.PHONY: clean

