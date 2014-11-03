TARGETS	= $(OBJDIR)colors.so $(OBJDIR)gtk-demo.so $(OBJDIR)logo.so $(OBJDIR)template.so $(OBJDIR)xscreensaver.so $(OBJDIR)xterm.so
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libSystem gtk+-2.0 x11`
CFLAGS	= -Wall -g -O2 -pedantic -fPIC
LDFLAGSF= -W `pkg-config --libs libSystem gtk+-2.0 x11`
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

colors_OBJS = $(OBJDIR)colors.o
colors_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
colors_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)colors.so: $(colors_OBJS)
	$(CCSHARED) -o $(OBJDIR)colors.so $(colors_OBJS) $(colors_LDFLAGS)

gtk-demo_OBJS = $(OBJDIR)gtk-demo.o
gtk-demo_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
gtk-demo_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)gtk-demo.so: $(gtk-demo_OBJS)
	$(CCSHARED) -o $(OBJDIR)gtk-demo.so $(gtk-demo_OBJS) $(gtk-demo_LDFLAGS)

logo_OBJS = $(OBJDIR)logo.o
logo_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
logo_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)logo.so: $(logo_OBJS)
	$(CCSHARED) -o $(OBJDIR)logo.so $(logo_OBJS) $(logo_LDFLAGS)

template_OBJS = $(OBJDIR)template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)template.so: $(template_OBJS)
	$(CCSHARED) -o $(OBJDIR)template.so $(template_OBJS) $(template_LDFLAGS)

xscreensaver_OBJS = $(OBJDIR)xscreensaver.o
xscreensaver_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
xscreensaver_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)xscreensaver.so: $(xscreensaver_OBJS)
	$(CCSHARED) -o $(OBJDIR)xscreensaver.so $(xscreensaver_OBJS) $(xscreensaver_LDFLAGS)

xterm_OBJS = $(OBJDIR)xterm.o
xterm_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
xterm_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)xterm.so: $(xterm_OBJS)
	$(CCSHARED) -o $(OBJDIR)xterm.so $(xterm_OBJS) $(xterm_LDFLAGS)

$(OBJDIR)colors.o: colors.c ../../include/Locker.h ../../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(colors_CFLAGS) -o $(OBJDIR)colors.o -c colors.c

$(OBJDIR)gtk-demo.o: gtk-demo.c ../../include/Locker/demo.h ../../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(gtk-demo_CFLAGS) -o $(OBJDIR)gtk-demo.o -c gtk-demo.c

$(OBJDIR)logo.o: logo.c ../../include/Locker/demo.h ../../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(logo_CFLAGS) -o $(OBJDIR)logo.o -c logo.c

$(OBJDIR)template.o: template.c
	$(CC) $(template_CFLAGS) -o $(OBJDIR)template.o -c template.c

$(OBJDIR)xscreensaver.o: xscreensaver.c ../../include/Locker.h ../../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(xscreensaver_CFLAGS) -o $(OBJDIR)xscreensaver.o -c xscreensaver.c

$(OBJDIR)xterm.o: xterm.c ../../include/Locker.h ../../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(xterm_CFLAGS) -o $(OBJDIR)xterm.o -c xterm.c

clean:
	$(RM) -- $(colors_OBJS) $(gtk-demo_OBJS) $(logo_OBJS) $(template_OBJS) $(xscreensaver_OBJS) $(xterm_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/demos
	$(INSTALL) -m 0644 $(OBJDIR)colors.so $(DESTDIR)$(LIBDIR)/Locker/demos/colors.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/demos
	$(INSTALL) -m 0644 $(OBJDIR)gtk-demo.so $(DESTDIR)$(LIBDIR)/Locker/demos/gtk-demo.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/demos
	$(INSTALL) -m 0644 $(OBJDIR)logo.so $(DESTDIR)$(LIBDIR)/Locker/demos/logo.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/demos
	$(INSTALL) -m 0644 $(OBJDIR)xscreensaver.so $(DESTDIR)$(LIBDIR)/Locker/demos/xscreensaver.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/demos
	$(INSTALL) -m 0644 $(OBJDIR)xterm.so $(DESTDIR)$(LIBDIR)/Locker/demos/xterm.so

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/demos/colors.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/demos/gtk-demo.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/demos/logo.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/demos/xscreensaver.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/demos/xterm.so

.PHONY: all clean distclean install uninstall
