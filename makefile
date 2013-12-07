# makefile for all sample code

VERSION:=0.2-alpha
DIST:=beindexed-$(VERSION)

SUBDIRS = \
	src

# IMKIT_HEADERS=$(addprefix /boot/home/config/include/, $(wildcard libim/*.h))

.PHONY: default clean install dist common

default .DEFAULT : /boot/home/config/include/libim $(IMKIT_HEADERS)
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile $@ || exit -1; \
	done

clean:
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile clean || exit -1; \
	done

COMMON_LIB:=$(shell finddir B_COMMON_LIB_DIRECTORY)
COMMON_SERVERS:=$(shell finddir B_COMMON_SERVERS_DIRECTORY)
COMMON_ADDONS:=$(shell finddir B_COMMON_ADDONS_DIRECTORY)
BUILD:=$(shell pwd)/build
