PROGS = pnghist.cgi

SRCS = pnghist.c
OBJS = pnghist.o
#OBJS = $(SRC:.c=.o)

#CFLAGS   += -Wall -I$(ELPHEL_KERNEL_DIR)/include/uapi/elphel
CFLAGS   += -Wall -I$(STAGING_DIR_HOST)/usr/include-uapi 
#LDLIBS   += -lpng -lm -lz
LDLIBS   += -lpng -lm

#INSTALL    = install
INSTMODE   = 0755
INSTDOCS   = 0644
OWN = -o root -g root

#SYSCONFDIR = /etc/
#BINDIR     = /usr/bin/
#WWW_PAGES  = /www/pages
#CGIDIR    = $(prefix)/usr/html
CGIDIR    = /www/pages
all: $(PROGS)

$(PROGS): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

install: $(PROGS) $(PHPSCRIPTS) $(CONFIGS)
	$(INSTALL) $(OWN) -d $(DESTDIR)
	$(INSTALL) $(OWN) -d $(DESTDIR)$(CGIDIR)
	$(INSTALL) $(OWN) -m $(INSTMODE) $(PROGS) $(DESTDIR)$(CGIDIR)

clean:
	rm -rf $(PROGS) *.o *~
depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ > ./.depend;

include .depend
