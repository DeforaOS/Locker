TARGETS	= index.html locker.1 locker.html lockerctl.1 lockerctl.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

index.html: index.xml index.xsl locker.xml lockerctl.xml
	./docbook.sh -P "$(PREFIX)" -- "index.html"

locker.1: locker.xml
	./docbook.sh -P "$(PREFIX)" -- "locker.1"

locker.html: locker.css.xml locker.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "locker.html"

lockerctl.1: lockerctl.xml
	./docbook.sh -P "$(PREFIX)" -- "lockerctl.1"

lockerctl.html: lockerctl.css.xml lockerctl.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "lockerctl.html"

clean:
	$(RM) -- $(index.html_OBJS) $(locker.1_OBJS) $(locker.html_OBJS) $(lockerctl.1_OBJS) $(lockerctl.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "lockerctl.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "locker.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "locker.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "lockerctl.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "lockerctl.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "locker.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "locker.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "lockerctl.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "lockerctl.html"

.PHONY: all clean distclean install uninstall
