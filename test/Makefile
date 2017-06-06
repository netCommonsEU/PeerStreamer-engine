SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.test)

GRAPES ?= ../../GRAPES

TARGET=../src/libpstreamer.a
TARGET_SRC=$(wildcard ../src/*.c)
GRAPESLIB=$(GRAPES)/src/libgrapes.a

CFLAGS += -g -W -Wall -I ../include
LDFLAGS += -l pstreamer -L ../src -lgrapes -L $(GRAPES)/src

all: $(OBJS)

$(TARGET): $(TARGET_SRC)
	DEBUG=1 $(MAKE) -C ../

$(GRAPESLIB):
	$(MAKE) -C $(GRAPES)

%.test: %.c $(TARGET) $(GRAPESLIB) $(TARGET_SRC)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.test

.PHONY: all clean

