dnl
dnl $Id$
dnl

PHP_ARG_ENABLE(pfs_client,whether to enable System V shared memory support,
[  --enable-pfs_client	Enable the System V shared memory support])

if test "$PHP_SYSVSHM" != "no"; then
  PHP_SUBST(PFS_CLIENT_SHARED_LIBADD)
  AC_DEFINE(HAVE_PFS, 1, [ ])
  PHP_ADD_INCLUDE(../lib)
  PHP_ADD_LIBRARY_WITH_PATH(pfs, ../lib, PFS_CLIENT_SHARED_LIBADD)
  PHP_NEW_EXTENSION(pfs_client, pfs_client.c, $ext_shared)
fi
