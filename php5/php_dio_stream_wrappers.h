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

#ifndef PHP_DIO_STREAM_WRAPPERS_H_
#define PHP_DIO_STREAM_WRAPPERS_H_

#define DIO_RAW_STREAM_NAME         "dio.raw"
#define DIO_RAW_STREAM_PROTOCOL     "dio.raw://"
#define DIO_SERIAL_STREAM_NAME      "dio.serial"
#define DIO_SERIAL_STREAM_PROTOCOL  "dio.serial://"

/* To support PHP 5.4 and later */
#if PHP_VERSION_ID < 50399
#define DIO_SAFE_MODE_CHECK(f, m) (PG(safe_mode) && !php_checkuid(f, m, CHECKUID_CHECK_MODE_PARAM))
#else
#define DIO_SAFE_MODE_CHECK(f, m) (0)
#endif

extern php_stream_wrapper php_dio_raw_stream_wrapper;

PHP_FUNCTION(dio_raw);

extern php_stream_wrapper php_dio_serial_stream_wrapper;

PHP_FUNCTION(dio_serial);

#endif /* PHP_DIO_STREAM_WRAPPERS_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
