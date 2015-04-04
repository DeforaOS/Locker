TARGETS	= $(OBJDIR)debug.so $(OBJDIR)openmoko.so $(OBJDIR)suspend.so $(OBJDIR)systray.so $(OBJDIR)template.so
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libSystem gtk+-2.0`
CFLAGS	= -Wall -g -O2 -pedantic -fPIC
LDFLAGSF= -W `pkg-config --libs libSystem gtk+-2.0`
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

debug_OBJS = $(OBJDIR)debug.o
debug_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
debug_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)debug.so: $(debug_OBJS)
	$(CCSHARED) -o $(OBJDIR)debug.so $(debug_OBJS) $(debug_LDFLAGS)

openmoko_OBJS = $(OBJDIR)openmoko.o
openmoko_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
openmoko_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)openmoko.so: $(openmoko_OBJS)
	$(CCSHARED) -o $(OBJDIR)openmoko.so $(openmoko_OBJS) $(openmoko_LDFLAGS)

suspend_OBJS = $(OBJDIR)suspend.o
suspend_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
suspend_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)suspend.so: $(suspend_OBJS)
	$(CCSHARED) -o $(OBJDIR)suspend.so $(suspend_OBJS) $(suspend_LDFLAGS)

systray_OBJS = $(OBJDIR)systray.o
systray_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop`
systray_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop`

$(OBJDIR)systray.so: $(systray_OBJS)
	$(CCSHARED) -o $(OBJDIR)systray.so $(systray_OBJS) $(systray_LDFLAGS)

template_OBJS = $(OBJDIR)template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)template.so: $(template_OBJS)
	$(CCSHARED) -o $(OBJDIR)template.so $(template_OBJS) $(template_LDFLAGS)

$(OBJDIR)debug.o: debug.c ../../include/Locker.h
	$(CC) $(debug_CFLAGS) -o $(OBJDIR)debug.o -c debug.c

$(OBJDIR)openmoko.o: openmoko.c ../../include/Locker.h
	$(CC) $(openmoko_CFLAGS) -o $(OBJDIR)openmoko.o -c openmoko.c

$(OBJDIR)suspend.o: suspend.c ../../include/Locker.h
	$(CC) $(suspend_CFLAGS) -o $(OBJDIR)suspend.o -c suspend.c

$(OBJDIR)systray.o: systray.c ../../include/Locker.h
	$(CC) $(systray_CFLAGS) -o $(OBJDIR)systray.o -c systray.c

$(OBJDIR)template.o: template.c ../../include/Locker.h
	$(CC) $(template_CFLAGS) -o $(OBJDIR)template.o -c template.c

clean:
	$(RM) -- $(debug_OBJS) $(openmoko_OBJS) $(suspend_OBJS) $(systray_OBJS) $(template_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/plugins
	$(INSTALL) -m 0755 $(OBJDIR)openmoko.so $(DESTDIR)$(LIBDIR)/Locker/plugins/openmoko.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/plugins
	$(INSTALL) -m 0755 $(OBJDIR)suspend.so $(DESTDIR)$(LIBDIR)/Locker/plugins/suspend.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/plugins
	$(INSTALL) -m 0755 $(OBJDIR)systray.so $(DESTDIR)$(LIBDIR)/Locker/plugins/systray.so

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/plugins/openmoko.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/plugins/suspend.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/plugins/systray.so

.PHONY: all clean distclean install uninstall
