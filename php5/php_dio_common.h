/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 2009 Melanie Rhianna Lewis                             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Melanie Rhianna Lewis <cyberspice@php.net>                   |
   +----------------------------------------------------------------------+
 */

#ifndef PHP_DIO_COMMON_H_
#define PHP_DIO_COMMON_H_

#ifdef PHP_WIN32
#define PHP_DIO_API __declspec(dllexport)
#else
#define PHP_DIO_API
#endif

#ifdef PHP_WIN32
#include "php_dio_win32.h"
#else
#include "php_dio_posix.h"
#endif

#define DIO_STREAM_TYPE_NONE   0
#define DIO_STREAM_TYPE_RAW    1
#define DIO_STREAM_TYPE_SERIAL 2

long dio_convert_to_long(zval *val);

php_dio_stream_data * dio_create_stream_data(void);

void dio_init_stream_data(php_dio_stream_data *data);

void dio_assoc_array_get_basic_options(zval *options, php_dio_stream_data *data TSRMLS_DC);

void dio_assoc_array_get_serial_options(zval *options, php_dio_stream_data *data TSRMLS_DC);

void dio_stream_context_get_basic_options(php_stream_context *context, php_dio_stream_data *data TSRMLS_DC);

void dio_stream_context_get_serial_options(php_stream_context *context, php_dio_stream_data *data TSRMLS_DC);

size_t dio_common_write(php_dio_stream_data *data, const char *buf, size_t count);

size_t dio_common_read(php_dio_stream_data *data, const char *buf, size_t count);

int dio_common_close(php_dio_stream_data *data);

int dio_common_set_option(php_dio_stream_data *data, int option, int value, void *ptrparam);

int dio_raw_open_stream(char *filename, char *mode, php_dio_stream_data *data TSRMLS_DC);

int dio_serial_uninit(php_dio_stream_data *data);

int dio_serial_purge(php_dio_stream_data *data);

int dio_serial_open_stream(char *filename, char *mode, php_dio_stream_data *data TSRMLS_DC);

#endif /* PHP_DIO_COMMON_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
