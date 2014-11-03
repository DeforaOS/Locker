SUBDIRS	= auth demos plugins
TARGETS	= $(OBJDIR)locker $(OBJDIR)lockerctl
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs libDesktop` -lintl
LDFLAGS	= 
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: subdirs $(TARGETS)

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

locker_OBJS = $(OBJDIR)locker.o $(OBJDIR)main.o
locker_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags x11 xscrnsaver`
locker_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs x11 xscrnsaver`

$(OBJDIR)locker: $(locker_OBJS)
	$(CC) -o $(OBJDIR)locker $(locker_OBJS) $(locker_LDFLAGS)

lockerctl_OBJS = $(OBJDIR)lockerctl.o
lockerctl_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
lockerctl_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)lockerctl: $(lockerctl_OBJS)
	$(CC) -o $(OBJDIR)lockerctl $(lockerctl_OBJS) $(lockerctl_LDFLAGS)

$(OBJDIR)locker.o: locker.c locker.h ../include/Locker.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(locker_CFLAGS) -o $(OBJDIR)locker.o -c locker.c

$(OBJDIR)main.o: main.c locker.h ../include/Locker.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(locker_CFLAGS) -o $(OBJDIR)main.o -c main.c

$(OBJDIR)lockerctl.o: lockerctl.c ../include/Locker.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(lockerctl_CFLAGS) -o $(OBJDIR)lockerctl.o -c lockerctl.c

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(locker_OBJS) $(lockerctl_OBJS)

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(locker_OBJS) $(lockerctl_OBJS)
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)locker $(DESTDIR)$(BINDIR)/locker
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)lockerctl $(DESTDIR)$(BINDIR)/lockerctl

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(BINDIR)/locker
	$(RM) -- $(DESTDIR)$(BINDIR)/lockerctl

.PHONY: all subdirs clean distclean install uninstall
