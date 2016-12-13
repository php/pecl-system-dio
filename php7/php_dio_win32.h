/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 2009 Melanie Rhianna Lewis                             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
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

#ifndef PHP_DIO_WIN32_H_
#define PHP_DIO_WIN32_H_

#include <windows.h>

/* Windows platform can do non blocking. */
#define DIO_NONBLOCK

#include "php_dio_common_data.h"

#define DIO_WIN32_CANON_BUF_SIZE 8192

/* This is the buffer information when reading in canonical mode.  Data is 
   read right up to either buffer being full or a newline being read.  Excess
   data will be retained in the buffer until the next read. */
typedef struct _php_dio_win32_canon_data {
	size_t size;
	size_t read_pos;
	size_t write_pos;
	char buf[DIO_WIN32_CANON_BUF_SIZE];

} php_dio_win32_canon_data;

typedef struct _php_dio_win32_stream_data {
	php_dio_stream_data common;
	HANDLE handle;
	DWORD desired_access;
	DWORD creation_disposition;
	DCB olddcb;
	COMMTIMEOUTS oldcto;
	php_dio_win32_canon_data *canon_data;

} php_dio_win32_stream_data ;

#endif /* PHP_DIO_WIN32_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
