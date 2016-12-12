PHP_ARG_ENABLE(dio, whether to enable direct I/O support,
[  --enable-dio            Enable direct I/O support])

if test "$PHP_DIO" != "no"; then
  AC_MSG_CHECKING(PHP version)

  export OLD_CPPFLAGS="$CPPFLAGS"
  export CPPFLAGS="$CPPFLAGS $INCLUDES"
  AC_TRY_COMPILE([#include <php_version.h>], [
#if PHP_MAJOR_VERSION > 5
#error  PHP > 5
#endif
  ], [
    subdir=php5
    AC_MSG_RESULT([PHP 5.x])
  ], [
    subdir=php7
    AC_MSG_RESULT([PHP 7.x])
  ])
  export CPPFLAGS="$OLD_CPPFLAGS"
  PHP_DIO_SOURCES="
    $subdir/dio.c \
    $subdir/dio_common.c \
    $subdir/dio_posix.c \
    $subdir/dio_stream_wrappers.c \
  "

  PHP_NEW_EXTENSION(dio, $PHP_DIO_SOURCES, $ext_shared)
  PHP_ADD_BUILD_DIR($abs_builddir/$subdir, 1)
  PHP_ADD_INCLUDE([$ext_srcdir/$subdir])
fi
