AXIS_USABLE_LIBS = UCLIBC GLIBC
include $(AXIS_TOP_DIR)/tools/build/Rules.axis

# makefile for libpng using gcc (generic, static library)
# Copyright (C) 2000 Cosmin Truta
# Copyright (C) 1995 Guy Eric Schalnat, Group 42, Inc.
# For conditions of distribution and use, see copyright notice in png.h

# Location of the zlib library and include files
ZLIBINC = ../zlib
ZLIBLIB = ../zlib
INCDIR    = $(AXIS_KERNEL_DIR)/include

INSTDIR   = $(prefix)/usr/local/bin
INSTMODE  = 0755
INSTDOCMODE  = 0644

INSTOWNER = root
INSTGROUP = root



# Compiler, linker, lib and other tools
# CC = gcc
LD = $(CC)
LN= $(CC)
AR = ar rcs
RANLIB = ranlib
RM = rm -f

CDEBUG = -g -DPNG_DEBUG=5
LDDEBUG =
CRELEASE = -O2
LDRELEASE = -s
#CFLAGS = -I$(INCDIR) -I$(ZLIBINC) -Wall $(CRELEASE)
#LDFLAGS = -L. -L$(ZLIBLIB) -lpng -lz -lm $(LDRELEASE)

CFLAGS   += -Wall -g -I$(INCDIR) -I$(ZLIBINC) -I$(AXIS_KERNEL_DIR)/include $(CRELEASE)
LDFLAGS  += -L. -L$(ZLIBLIB) -L$(LIBDIR) -lpng -lz -lm $(LDRELEASE)

ifndef UCLIBC
# -lm - to use math library
LDLIBS = -lcrypt -lm
endif


# Variables
PROGS=pngtest
DOCS=pngtest.png
SRCS = png.c pngerror.c pngget.c pngmem.c pngpread.c \
	pngread.c pngrio.c pngrtran.c pngrutil.c pngset.c \
	pngtrans.c pngwio.c pngwrite.c pngwtran.c pngwutil.c pngtest.c
OBJS = png.o pngerror.o pngget.o pngmem.o pngpread.o \
	pngread.o pngrio.o pngrtran.o pngrutil.o pngset.o \
	pngtrans.o pngwio.o pngwrite.o pngwtran.o pngwutil.o

# Targets
all: dependency libpng.a pngtest

libpng.a: $(OBJS)
	$(AR) $@ $(OBJS)
	$(RANLIB) $@

#test: pngtest
#	./pngtest
aaa:
	echo $(CFLAGS)
pngtest: pngtest.o libpng.a
#	$(LD) -o $@ pngtest.o $(LDFLAGS)
	$(LD) $^ $(LDLIBS) -o $@ $(LDFLAGS)
	cris-strip -s $@
install: $(LIBS)
	$(INSTALL) -d $(INSTDIR)
	$(INSTALL) -m $(INSTMODE) -o $(INSTOWNER) -g $(INSTGROUP) $(PROGS) $(INSTDIR)
	$(INSTALL) -m $(INSTDOCMODE) -o $(INSTOWNER) -g $(INSTGROUP) $(DOCS) $(INSTDIR)

clean:
	$(RM) *.o libpng.a pngtest pngout.png

dependency:
	make depend
depend:
	makedepend -Y -- $(CFLAGS) -- $(SRCS) 2>/dev/null
	touch dependency

# DO NOT DELETE THIS LINE -- make depend depends on it.

png.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngerror.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngget.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngmem.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngpread.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngread.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngrio.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngrtran.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngrutil.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngset.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngtrans.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngwio.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngwrite.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngwtran.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngwutil.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
pngtest.o: png.h ../zlib/zlib.h ../zlib/zconf.h pngconf.h
