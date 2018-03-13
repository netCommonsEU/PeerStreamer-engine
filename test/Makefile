SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.test)

GRAPES ?= $(PWD)/../../grapes
NET_HELPER ?= $(PWD)/../Lib/net_helper

TARGET=../src/libpstreamer.a
TARGET_SRC=$(wildcard ../src/*.c)

GRAPESLIB=$(GRAPES)/src/libgrapes.a
NETHELPERLIB=$(NET_HELPER)/libnethelper.a

CFLAGS += -g -W -Wall -I ../include -I../src -I$(GRAPES)/include -I$(NET_HELPER)/include
LDFLAGS += -l pstreamer -L ../src -lnethelper -L$(NET_HELPER) -lgrapes -L $(GRAPES)/src

all: $(TARGET) $(OBJS)

$(TARGET): $(NETHELPERLIB) $(TARGET_SRC) $(GRAPESLIB)
	DEBUG=1 NET_HELPER=$(NET_HELPER) GRAPES=$(GRAPES) $(MAKE) -C ../

$(GRAPESLIB):
	$(MAKE) -C $(GRAPES)

$(NETHELPERLIB):
	GRAPES=$(GRAPES) $(MAKE) -C $(NET_HELPER)

%.test: %.c $(TARGET) $(GRAPESLIB) $(TARGET_SRC) $(OPTIONAL_OBJS) 
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.test

.PHONY: all clean

