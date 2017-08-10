SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.test)

GRAPES ?= $(PWD)/../../grapes
NET_HELPER ?= ../Lib/net_helper

TARGET=../src/libpstreamer.a
TARGET_SRC=$(wildcard ../src/*.c)

GRAPESLIB=$(GRAPES)/src/libgrapes.a
NETHELPERLIB=$(NET_HELPER)/libnethelper.a

CFLAGS += -g -W -Wall -I ../include -I../src -I$(GRAPES)/include -I$(NET_HELPER)
LDFLAGS += -l pstreamer -L ../src -lgrapes -L $(GRAPES)/src -L$(NET_HELPER) -lnethelper

all: $(TARGET) $(OBJS)

$(TARGET): $(NETHELPERLIB) $(TARGET_SRC)
	DEBUG=1 GRAPES=$(GRAPES) $(MAKE) -C ../

# $(GRAPESLIB):
# 	$(MAKE) -C $(GRAPES)
# 
$(NETHELPERLIB):
	NET_UDP=1 GRAPES=$(GRAPES) $(MAKE) -C $(NET_HELPER)

%.test: %.c $(TARGET) $(GRAPESLIB) $(TARGET_SRC) $(OPTIONAL_OBJS) 
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.test

.PHONY: all clean

