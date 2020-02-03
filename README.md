DeforaOS Locker
===============

About Locker
------------

Locker is a screensaver.

It can be extended through authentication backends, additional animations, and
general-purpose plug-ins.

Locker is part of the DeforaOS Project, found at https://www.defora.org/.


Compiling Locker
-----------------

Locker depends on the following components:

 * Gtk+ 2.4 or later, or Gtk+ 3.0 or later
 * DeforaOS libDesktop
 * an implementation of `make`
 * gettext (libintl) for translations
 * DocBook-XSL (for the manual pages)
 * GTK-Doc (for the API documentation)

With GCC, this should then be enough to compile and install Locker:

    $ make install

To install (or package) Locker in a different location:

    $ make clean
    $ make PREFIX="/another/prefix" install

Locker also supports `DESTDIR`, to be installed in a staging directory; for
instance:

    $ make DESTDIR="/staging/directory" PREFIX="/another/prefix" install


Documentation
-------------

Manual pages for each of the executables installed are available in the `doc`
folder. They are written in the DocBook-XML format, and need libxslt and
DocBook-XSL to be installed for conversion to the HTML or man file format.

Likewise, the API reference for Locker (plug-ins) is available in the
`doc/gtkdoc` folder, and is generated using gtk-doc.

Distributing Locker
--------------------

DeforaOS Locker is subject to the terms of the 2-clause BSD license. Please
see the `COPYING` file for more information.
