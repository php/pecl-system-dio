/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
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

#ifndef PHP_DIO_POSIX_H_
#define PHP_DIO_POSIX_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <termios.h>


/**
 * Detect if we can support non blocking IO.
 */
#ifdef O_NONBLOCK
#define DIO_NONBLOCK O_NONBLOCK
#else
#ifdef O_NDELAY
#define DIO_NONBLOCK O_NDELAY
#endif
#endif

/**
 * POSIXy platforms have file permissions
 */
#define DIO_HAS_FILEPERMS

#include "php_dio_common_data.h"

typedef struct _php_dio_posix_stream_data {
	php_dio_stream_data common;
	int fd;
	int flags;
	/* Serial options */
	struct termios oldtio;
} php_dio_posix_stream_data ;

#endif /* PHP_DIO_POSIX_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
