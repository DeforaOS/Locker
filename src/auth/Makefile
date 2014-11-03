TARGETS	= $(OBJDIR)password.so $(OBJDIR)slider.so $(OBJDIR)template.so
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

password_OBJS = $(OBJDIR)password.o
password_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
password_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) -lcrypt

$(OBJDIR)password.so: $(password_OBJS)
	$(CCSHARED) -o $(OBJDIR)password.so $(password_OBJS) $(password_LDFLAGS)

slider_OBJS = $(OBJDIR)slider.o
slider_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
slider_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)slider.so: $(slider_OBJS)
	$(CCSHARED) -o $(OBJDIR)slider.so $(slider_OBJS) $(slider_LDFLAGS)

template_OBJS = $(OBJDIR)template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)template.so: $(template_OBJS)
	$(CCSHARED) -o $(OBJDIR)template.so $(template_OBJS) $(template_LDFLAGS)

$(OBJDIR)password.o: password.c ../../include/Locker.h ../../config.h
	$(CC) $(password_CFLAGS) -o $(OBJDIR)password.o -c password.c

$(OBJDIR)slider.o: slider.c ../../include/Locker.h
	$(CC) $(slider_CFLAGS) -o $(OBJDIR)slider.o -c slider.c

$(OBJDIR)template.o: template.c ../../include/Locker.h
	$(CC) $(template_CFLAGS) -o $(OBJDIR)template.o -c template.c

clean:
	$(RM) -- $(password_OBJS) $(slider_OBJS) $(template_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/auth
	$(INSTALL) -m 0644 $(OBJDIR)password.so $(DESTDIR)$(LIBDIR)/Locker/auth/password.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Locker/auth
	$(INSTALL) -m 0644 $(OBJDIR)slider.so $(DESTDIR)$(LIBDIR)/Locker/auth/slider.so

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/auth/password.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Locker/auth/slider.so

.PHONY: all clean distclean install uninstall
