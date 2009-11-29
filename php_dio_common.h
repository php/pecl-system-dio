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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#ifndef PHP_WIN32
#include <termios.h>
#endif

#include "php.h"

typedef struct _php_dio_stream_data {
#ifndef PHP_WIN32
	int fd;
#else
	int fh;
#endif
	int flags;
	int end_of_file;
	int has_perms;
	mode_t perms;
#ifdef O_NONBLOCK
	int is_blocking;
	int has_timeout;
	struct timeval timeout;
#endif

	/* Serial options */
	long data_rate;
	int data_bits;
	int stop_bits;
	int parity;
	int rtscts;
#ifndef PHP_WIN32
	struct termios oldtio;
#endif

} php_dio_stream_data ;

long dio_convert_to_long(zval *val);

int dio_data_rate_to_define(long rate, speed_t *def);

int dio_data_bits_to_define(int bits, int *def);

int dio_stop_bits_to_define(int stop_bits, int *def);

int dio_parity_to_define(int parity, int *def);

int dio_assoc_array_get_basic_options(zval *options, php_dio_stream_data *data);

int dio_assoc_array_get_serial_options(zval *options, php_dio_stream_data *data);

int dio_stream_mode_to_flags(const char *mode);

size_t dio_common_write(php_dio_stream_data *data, const char *buf, size_t count);

size_t dio_common_read(php_dio_stream_data *data, const char *buf, size_t count);

#endif /* PHP_DIO_COMMON_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
