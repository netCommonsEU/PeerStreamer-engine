GRAPES ?= $(PWD)/../../../grapes
TARGET=libnethelper.a

SRC=net_helpers.c
CFLAGS+=-I./ -I$(GRAPES)/include
ifdef DEBUG
CFLAGS+=-g -W -Wall -O0 -DDEBUG -Wno-unused-parameter -DLOG_CHUNK -DLOG_SIGNAL -Wno-unused-function 
endif

ifdef NET_UDP
	SRC+=net_helper-udp.c
else
	SRC+=net_helper-x.c
	SRC+=$(wildcard nhx/*.c)
	CFLAGS+=-Inhx/
endif

OBJS=$(SRC:.c=.o)

$(TARGET): $(OBJS) $(SRC)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS) 

tests:
	GRAPES=$(GRAPES) make -C test/

clean:
	rm -f $(OBJS) $(TARGET)
	make -C test/ clean

.PHONY: clean
