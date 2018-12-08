# Makefile for XPENGUINS
# Copyright (C) 1999, 2000  Robin Hogan

CC = gcc
RPM_OPT_FLAGS =
CFLAGS = -Wall $(RPM_OPT_FLAGS) 

XLIBS = -lX11 -lXpm -lXext
XLIBDIR = -L/usr/X11R6/lib -L/usr/local/lib 
XINCLUDEDIRS = -I/usr/X11R6/include -I/usr/local/include 

OBJS = xsimpsons.o toon.o
PROGRAM = xsimpsons

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(XLIBDIR) $(XLIBS) $(OBJS) -o $(PROGRAM)

%.o: %.c
	$(CC) $(CFLAGS) $(XINCLUDEDIRS) -c $<

clean:
	-rm -f $(PROGRAM) $(OBJS)

$(OBJS): toon.h penguins/def.h penguins/*.xpm

chvar:
	mv penguins tmp
	mv variant2 penguins
	mv tmp variant2