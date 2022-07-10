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
 * DocBook-XSL for the manual pages
 * GTK-Doc for the API documentation

With these installed, the following command should be enough to compile and
install Locker on most systems:

    $ make install

To install (or package) Locker in a different location, use the `PREFIX` option
as follows:

    $ make PREFIX="/another/prefix" install

Locker also supports `DESTDIR`, to be installed in a staging directory; for
instance:

    $ make DESTDIR="/staging/directory" PREFIX="/another/prefix" install

The compilation process supports a number of other options, such as OBJDIR for
compilation outside of the source tree for instance.

On some systems, the Makefiles shipped may have to be re-generated accordingly.
This can be performed with the DeforaOS configure tool.


Documentation
-------------

Manual pages for each of the executables installed are available in the `doc`
folder. They are written in the DocBook-XML format, and need libxslt and
DocBook-XSL to be installed for conversion to the HTML or man file format.

Likewise, the API reference for Locker (plug-ins) is available in the
`doc/gtkdoc` folder, and is generated using gtk-doc.

Extending Locker
----------------

Plug-ins for the Locker program can be written according to the API definitions
installed and found in `<Desktop/Locker/*.h>` (in the source code in the
`include/Locker/*.h` files).

Sample plug-ins can be found in `src/auth/template.c`, `src/demos/template.c`,
and `src/plugins/template.c`.

Further plug-ins may also be found in the
[DeforaOS Integration](https://www.defora.org/os/project/4339/Integration)
project.

Distributing Locker
--------------------

DeforaOS Locker is subject to the terms of the 2-clause BSD license. Please
see the `COPYING` file for more information.
