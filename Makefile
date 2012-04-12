TOPDIR 	:= $(CURDIR)
DISTDIR := $(TOPDIR)/dist
SRCDIR	:= $(TOPDIR)/src
INCDIR	:= $(TOPDIR)/include

target		:= libbina.so.1.0
target-obj	:= bina.o bblock.o loops.o arch/x86/disasm-32.o

real-target		:= $(DISTDIR)/$(target)
real-target-obj	:= $(foreach T,$(target-obj),$(SRCDIR)/$(T))

LDFLAGS	:= -Wl,-soname,libbina.so.1 -L/usr/local/lib -ldisasm
CFLAGS	:= -g -Wall -D__BINA_LIBRARY__

LN := ln

all:	$(real-target)

$(real-target): $(real-target-obj)
	$(CC) -shared -o $@ $(LDFLAGS) $(real-target-obj)

%.o: %.c
	$(CC) -c -o $@ -fPIC -I$(INCDIR) $(CFLAGS) $<

clean:
	$(RM) $(real-target) $(real-target-obj)
