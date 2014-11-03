TARGETS	= $(OBJDIR)index.html $(OBJDIR)locker.1 $(OBJDIR)locker.html $(OBJDIR)lockerctl.1 $(OBJDIR)lockerctl.html
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)index.html: index.xml index.xsl locker.xml lockerctl.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)index.html"

$(OBJDIR)locker.1: locker.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)locker.1"

$(OBJDIR)locker.html: locker.css.xml locker.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)locker.html"

$(OBJDIR)lockerctl.1: lockerctl.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)lockerctl.1"

$(OBJDIR)lockerctl.html: lockerctl.css.xml lockerctl.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)lockerctl.html"

clean:
	$(RM) -- $(index.html_OBJS) $(locker.1_OBJS) $(locker.html_OBJS) $(lockerctl.1_OBJS) $(lockerctl.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "index.html"
	./docbook.sh -c -P "$(PREFIX)" -- "locker.1"
	./docbook.sh -c -P "$(PREFIX)" -- "locker.html"
	./docbook.sh -c -P "$(PREFIX)" -- "lockerctl.1"
	./docbook.sh -c -P "$(PREFIX)" -- "lockerctl.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)locker.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)locker.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)lockerctl.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)lockerctl.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "locker.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "locker.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "lockerctl.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "lockerctl.html"

.PHONY: all clean distclean install uninstall
