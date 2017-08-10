SRC_UDP=$(wildcard *_udp.c)
SRC_UDP+=$(wildcard *_test.c)
SRC_NHX=$(wildcard *_nhx.c)
SRC_NHX+=$(wildcard *_test.c)

OBJS_UDP=$(SRC_UDP:.c=.udp.test)
OBJS_NHX=$(SRC_NHX:.c=.nhx.test)

GRAPES ?= $(PWD)/../../../../grapes

TARGET=../libnethelper.a
TARGET_SRC_UDP=$(wildcard ../*.c)
TARGET_SRC_NHX=$(wildcard ../*.c)
TARGET_SRC_NHX+=$(wildcard ../nhx/*.c)

GRAPESLIB=$(GRAPES)/src/libgrapes.a

CFLAGS += -g -W -Wall -I$(GRAPES)/include -I../include -I../nhx
LDFLAGS += -l nethelper -L ../ -lgrapes -L $(GRAPES)/src 

all: $(GRAPESLIB) $(OBJS_NHX) $(OBJS_UDP)

nhx_lib: $(TARGET_SRC_NHX)
	rm -f $(TARGET)
	DEBUG=1 GRAPES=$(GRAPES) $(MAKE) -C ../

udp_lib: $(TARGET_SRC_UDP)
	rm -f $(TARGET)
	DEBUG=1 GRAPES=$(GRAPES) NET_UDP=1 $(MAKE) -C ../

$(GRAPESLIB):
	$(MAKE) -C $(GRAPES)

%.udp.test: %.c udp_lib $(GRAPESLIB) $(TARGET_SRC_UDP)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

%.nhx.test: %.c nhx_lib $(GRAPESLIB) $(TARGET_SRC_NHX)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.test

.PHONY: all clean

