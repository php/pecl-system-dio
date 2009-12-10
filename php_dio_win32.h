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

#ifndef PHP_DIO_WIN32_H_
#define PHP_DIO_WIN32_H_

#include <windows.h>

/* Windows platform can do non blocking. */
#define DIO_HAS_NONBLOCK

#include "php_dio_common_data.h"

typedef struct _php_dio_win32_stream_data {
	php_dio_stream_data common;
	HANDLE handle;
	DWORD desired_access;
	DWORD creation_disposition;
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
