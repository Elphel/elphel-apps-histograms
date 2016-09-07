AXIS_USABLE_LIBS = UCLIBC GLIBC
AXIS_AUTO_DEPEND = yes
include $(AXIS_TOP_DIR)/tools/build/Rules.axis

# makefile for libpng using gcc (generic, static library)
# Copyright (C) 2000 Cosmin Truta
# Copyright (C) 1995 Guy Eric Schalnat, Group 42, Inc.
# For conditions of distribution and use, see copyright notice in png.h

# Location of the zlib library and include files
#ZLIBINC = zlib
#ZLIBLIB = zlib

PNGINC = libpng
PNGLIB = libpng


#ZLIB = zlib
PNGLIB  = libpng
INCDIR    = $(AXIS_KERNEL_DIR)/include

INSTDIR   = $(prefix)/usr/local/bin
CGIDIR    = $(prefix)/usr/html
INSTMODE  = 0755
INSTDOCMODE  = 0644

INSTOWNER = root
INSTGROUP = root

#PGMNAME = pnghist

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

CFLAGS   += -Wall -g -I$(INCDIR) -I$(AXIS_KERNEL_DIR)/include
LDFLAGS  += -L$(LIBDIR)

CFLAGS   += -Wall -g -I$(INCDIR) -I$(PNGINC) -I$(AXIS_KERNEL_DIR)/include
LDFLAGS  += -L. -L$(PNGLIB) -L$(LIBDIR) -lpng -lz -lm

ifndef UCLIBC
# -lm - to use math library
LDLIBS = -lcrypt -lm
endif


# Variables
PROGS=pnghist.cgi
PNGSRCS = $(PNGLIB)/png.c $(PNGLIB)/pngerror.c $(PNGLIB)/pngget.c $(PNGLIB)/pngmem.c $(PNGLIB)/pngpread.c \
	$(PNGLIB)/pngread.c $(PNGLIB)/pngrio.c $(PNGLIB)/pngrtran.c $(PNGLIB)/pngrutil.c $(PNGLIB)/pngset.c \
	$(PNGLIB)/pngtrans.c $(PNGLIB)/pngwio.c $(PNGLIB)/pngwrite.c $(PNGLIB)/pngwtran.c $(PNGLIB)/pngwutil.c
PNGOBJS = $(PNGLIB)/png.o $(PNGLIB)/pngerror.o $(PNGLIB)/pngget.o $(PNGLIB)/pngmem.o $(PNGLIB)/pngpread.o \
	$(PNGLIB)/pngread.o $(PNGLIB)/pngrio.o $(PNGLIB)/pngrtran.o $(PNGLIB)/pngrutil.o $(PNGLIB)/pngset.o \
	$(PNGLIB)/pngtrans.o $(PNGLIB)/pngwio.o $(PNGLIB)/pngwrite.o $(PNGLIB)/pngwtran.o $(PNGLIB)/pngwutil.o

SRCS = pnghist.c $(PNGSRCS) $(ZSRCS)
OBJS = pnghist.o $(PNGOBJS) $(ZOBJS)

#LIBS = $(PNGLIB)/libpng.a $(ZLIB)/libz.a  
LIBS = $(PNGLIB)/libpng.a
# Targets
#all: dependency $(PROGS) $(PNGLIB)/libpng.a $(ZLIB)/libz.a
all: dependency $(PROGS)

$(PNGLIB)/libpng.a: $(PNGOBJS)
	$(AR) $@ $(PNGOBJS)
	$(RANLIB) $@

#$(ZLIB)/libz.a: $(ZOBJS) 
#	$(AR) $@ $(ZOBJS) 
#	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1
   
pnghist.cgi: pnghist.o $(PNGLIB)/libpng.a
	$(LD) $^ $(LDLIBS) -o $@ $(LDFLAGS)
	cris-strip -s $@
   
install: $(LIBS) $(PROGS)
	$(INSTALL) -d $(INSTDIR)
	$(INSTALL) -m $(INSTMODE) -o $(INSTOWNER) -g $(INSTGROUP) pnghist.cgi $(CGIDIR)

clean:
	$(RM) $(PNGLIB)/*.o $(PNGLIB)/*~ $(PNGLIB)/*.bak $(PNGLIB)/libpng.a $(PNGLIB)/pngtest $(PNGLIB)/pngout.png \
	*.o *~ *.bak dependency  $(PROGS)

configsubs:
