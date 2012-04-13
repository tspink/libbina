TOPDIR 	:= $(CURDIR)
DISTDIR := $(TOPDIR)/dist
SRCDIR	:= $(TOPDIR)/src
TESTDIR	:= $(TOPDIR)/test
INCDIR	:= $(TOPDIR)/include

target		:= libbina.so.1.0
target-obj	:= bina.o bblock.o loops.o trace.o arch/x86/disasm-32.o
target-obj	+= arch/dex/disasm-dex.o

test		:= bina-test
test-obj	:= bina-test.o

real-target		:= $(DISTDIR)/$(target)
real-target-obj		:= $(foreach T,$(target-obj),$(SRCDIR)/$(T))

real-test		:= $(DISTDIR)/$(test)
real-test-obj		:= $(foreach T,$(test-obj),$(TESTDIR)/$(T))

LDFLAGS	:= -Wl,-soname,libbina.so.1 -L/usr/local/lib -ldisasm
CFLAGS	:= -g -Wall -D__BINA_LIBRARY__

LN := ln

all:	$(real-target) $(real-test)

$(real-target): $(real-target-obj)
	$(CC) -shared -o $@ $(LDFLAGS) $(real-target-obj)
	
$(real-test): $(real-target) $(real-test-obj)
	$(CC) -o $@ $(real-test-obj) -L$(DISTDIR) -lbina -lelf

%.o: %.c
	$(CC) -c -o $@ -fPIC -I$(INCDIR) $(CFLAGS) $<

clean:
	$(RM) $(real-target) $(real-target-obj) $(real-test) $(real-test-obj)
