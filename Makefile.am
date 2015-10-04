#AM_CFLAGS= @DBUS_CFLAGS@ -Wall -g -DDBG
AM_CFLAGS= @DBUS_CFLAGS@ -Wall -g 
AM_LDFLAGS= @DBUS_LIBS@ -lpthread
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4 

bin_PROGRAMS = dbus_pam
dbus_pam_SOURCES = dbus_pam_verify.c logger.c logger.h watcher.c


lib_LTLIBRARIES = libpam_dbus_verify.la
libpam_dbus_verify_la_SOURCES = pam_dbus_verify.c logger.c
libpam_dbus_verify_la_LIBADD = @DBUS_LIBS@ 
libpam_dbus_verify_la_LDFLAGS = @DBUS_LIBS@ 
libpam_dbus_verify_la_CFLAGS = @DBUS_CFLAGS@ 
EXTRA_DIST = test.conf autogen.sh blacklist
