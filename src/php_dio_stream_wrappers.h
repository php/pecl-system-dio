/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt.                                 |
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

extern php_stream_wrapper php_dio_raw_stream_wrapper;

extern php_stream_wrapper php_dio_serial_stream_wrapper;

#endif /* PHP_DIO_STREAM_WRAPPERS_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
