GRAPES ?= $(PWD)/../GRAPES
NET_HELPER ?= $(PWD)/Lib/net_helper
LIBNETHELPER=$(NET_HELPER)/libnethelper.a
LIBGRAPES=$(GRAPES)/src/libgrapes.a
LIBPS=src/libpstreamer.a
LIBPS_SRC=$(wildcard src/*.c)
LDFLAGS+=-l pstreamer -L src -l grapes -L $(GRAPES)/src -l nethelper -L$(NET_HELPER) -DMULTIFLOW
CFLAGS+=-DMULTIFLOW
pstreamer: pstreamer.c $(LIBPS) $(LIBGRAPES) $(LIBNETHELPER)
	cc pstreamer.c -o pstreamer -I $(GRAPES)/include -I include/ $(LDFLAGS)

tests: $(LIBPS)
	NET_HELPER=$(NET_HELPER) GRAPES=$(GRAPES) $(MAKE) -C test/
	GRAPES=$(GRAPES) $(MAKE) -C $(NET_HELPER) tests
	test/run_tests.sh && $(NET_HELPER)/test/run_tests.sh

$(LIBPS): $(LIBPS_SRC)
	NET_HELPER=$(NET_HELPER) GRAPES=$(GRAPES) $(MAKE) -C src/

$(LIBGRAPES):
	CFLAGS='-DMULTIFLOW' $(MAKE) -C $(GRAPES)

$(LIBNETHELPER):
	GRAPES=$(GRAPES) $(MAKE) -C $(NET_HELPER)

clean:
	$(MAKE) -C $(NET_HELPER)/ clean
	$(MAKE) -C src/ clean
	$(MAKE) -C test/ clean
	rm -f pstreamer

.PHONY: clean

