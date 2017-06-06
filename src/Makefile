SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.o)

GRAPES ?= ../../GRAPES
TARGET=libpstreamer.a

ifdef DEBUG
CFLAGS += -g -W -Wall -O0 -DDEBUG -Wno-unused-parameter -DLOG_CHUNK -DLOG_SIGNAL -Wno-unused-function 
endif
CFLAGS += -I./ -I$(GRAPES)/include -I../include

$(TARGET): $(OBJS) $(SRC)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS) 

clean:
	rm -f *.o $(TARGET)

.PHONY: clean

