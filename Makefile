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

tests:
	$(MAKE) -C test
	test/run_tests.sh

clean:
	$(MAKE) -C src/ clean
	$(MAKE) -C test/ clean
	rm -f pstreamer

.PHONY: clean

