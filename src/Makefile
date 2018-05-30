SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.o)

GRAPES ?= ../../grapes
NET_HELPER ?= ../Lib/net_helper
TARGET=libpstreamer.a

ifdef DEBUG
CFLAGS += -g -W -Wall -O0 -DDEBUG -Wno-unused-parameter -DLOG_CHUNK -DLOG_SIGNAL -Wno-unused-function -DMULTIFLOW
endif
CFLAGS += -I./ -I$(GRAPES)/include -I../include -I$(NET_HELPER)/include -DMULTIFLOW

$(TARGET): $(OBJS) $(SRC)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS) 

clean:
	rm -f *.o $(TARGET)

.PHONY: clean

