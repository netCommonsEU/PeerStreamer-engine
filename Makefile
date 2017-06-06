GRAPES ?= ../GRAPES
LIBGRAPES=$(GRAPES)/src/libgrapes.a
LIBPS=src/libpstreamer.a
LIBPS_SRC=$(wildcard src/*.c)

pstreamer: pstreamer.c $(LIBPS) $(LIBGRAPES)
	cc pstreamer.c -o pstreamer -I $(GRAPES)/include -I include/ -l pstreamer -L src -l grapes -L $(GRAPES)/src

$(LIBPS): $(LIBPS_SRC)
	$(MAKE) -C src/

$(LIBGRAPES):
	$(MAKE) -C $(GRAPES)

clean:
	$(MAKE) -C src/ clean
	rm -f pstreamer

.PHONY: clean

