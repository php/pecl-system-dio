/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: c960cbb93c3f8966636813ca2346e35030451bf4 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_dio_open, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

#if !defined(PHP_WIN32)
ZEND_BEGIN_ARG_INFO_EX(arginfo_dio_fdopen, 0, 0, 1)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()
#endif

#if !defined(PHP_WIN32)
#define arginfo_dio_dup arginfo_dio_fdopen
#endif

#if !defined(PHP_WIN32)
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dio_truncate, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_dio_stat, 0, 1, MAY_BE_ARRAY|MAY_BE_FALSE)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_dio_seek, 0, 2, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_TYPE_INFO(0, pos, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, whence, IS_LONG, 0, "SEEK_SET")
ZEND_END_ARG_INFO()

#if !defined(PHP_WIN32)
ZEND_BEGIN_ARG_INFO_EX(arginfo_dio_fcntl, 0, 0, 2)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_TYPE_INFO(0, cmd, IS_LONG, 0)
	ZEND_ARG_INFO_WITH_DEFAULT_VALUE(0, arg, "NULL")
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_dio_read, 0, 1, MAY_BE_LONG|MAY_BE_NULL|MAY_BE_FALSE)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, n, IS_LONG, 0, "1024")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_dio_write, 0, 2, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, len, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dio_close, 0, 1, IS_FALSE, 1)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()

#if !defined(PHP_WIN32)
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_dio_tcsetattr, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_TYPE_INFO(0, args, IS_ARRAY, 0)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_dio_raw, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, mode, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 1, "NULL")
ZEND_END_ARG_INFO()

#define arginfo_dio_serial arginfo_dio_raw


ZEND_FUNCTION(dio_open);
#if !defined(PHP_WIN32)
ZEND_FUNCTION(dio_fdopen);
#endif
#if !defined(PHP_WIN32)
ZEND_FUNCTION(dio_dup);
#endif
#if !defined(PHP_WIN32)
ZEND_FUNCTION(dio_truncate);
#endif
ZEND_FUNCTION(dio_stat);
ZEND_FUNCTION(dio_seek);
#if !defined(PHP_WIN32)
ZEND_FUNCTION(dio_fcntl);
#endif
ZEND_FUNCTION(dio_read);
ZEND_FUNCTION(dio_write);
ZEND_FUNCTION(dio_close);
#if !defined(PHP_WIN32)
ZEND_FUNCTION(dio_tcsetattr);
#endif
ZEND_FUNCTION(dio_raw);
ZEND_FUNCTION(dio_serial);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(dio_open, arginfo_dio_open)
#if !defined(PHP_WIN32)
	ZEND_FE(dio_fdopen, arginfo_dio_fdopen)
#endif
#if !defined(PHP_WIN32)
	ZEND_FE(dio_dup, arginfo_dio_dup)
#endif
#if !defined(PHP_WIN32)
	ZEND_FE(dio_truncate, arginfo_dio_truncate)
#endif
	ZEND_FE(dio_stat, arginfo_dio_stat)
	ZEND_FE(dio_seek, arginfo_dio_seek)
#if !defined(PHP_WIN32)
	ZEND_FE(dio_fcntl, arginfo_dio_fcntl)
#endif
	ZEND_FE(dio_read, arginfo_dio_read)
	ZEND_FE(dio_write, arginfo_dio_write)
	ZEND_FE(dio_close, arginfo_dio_close)
#if !defined(PHP_WIN32)
	ZEND_FE(dio_tcsetattr, arginfo_dio_tcsetattr)
#endif
	ZEND_FE(dio_raw, arginfo_dio_raw)
	ZEND_FE(dio_serial, arginfo_dio_serial)
	ZEND_FE_END
};
