PHP_ARG_ENABLE(dio, whether to enable direct I/O support,
[  --enable-dio            Enable direct I/O support])

if test "$PHP_DIO" != "no"; then
  subdir=src
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
