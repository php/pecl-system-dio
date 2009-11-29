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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>

#include "php_dio_common.h"

/* {{{ dio_convert_to_long
 * Returns as a long, the value of the zval regardless of its type.
 */
long dio_convert_to_long(zval *val) {
	zval *copyval;
	long  longval;

	ALLOC_INIT_ZVAL(copyval);
	*copyval = *val;
	convert_to_long(copyval);
	longval = Z_LVAL_P(copyval);
	zval_ptr_dtor(&copyval);

	return longval;
}
/* }}} */

/* {{{ dio_stream_mode_to_flags
 * Convert an fopen() mode string to open() flags
 */
int dio_stream_mode_to_flags(const char *mode) {
	int  flags = 0, ch = 0, bin = 1;

#ifndef PHP_WIN32
	switch(mode[ch++]) {
		case 'r':
			flags = 0;
			break;
		case 'w':
			flags = O_TRUNC | O_CREAT;
			break;
		case 'a':
			flags = O_APPEND | O_CREAT;
			break;
		case 'x':
			flags = O_EXCL | O_CREAT;
			break;
	}

	if (mode[ch] != '+') {
		bin = (mode[ch++] == 'b');
	}

	if (mode[ch] == '+') {
		flags |= O_RDWR;
	} else if (flags) {
		flags |= O_WRONLY;
	} else {
		flags |= O_RDONLY;
	}

#if defined(_O_TEXT) && defined(O_BINARY)
	if (bin) {
		flags |= O_BINARY;
	} else {
		flags |= _O_TEXT;
	}
#endif

#else
	/* TODO: Windows version */
#endif

	return flags;
}
/* }}} */

#ifndef PHP_WIN32

/* {{{ dio_data_rate_to_define
 * Converts a numeric data rate to a termios define
 */
int dio_data_rate_to_define(long rate, speed_t *def) {
	speed_t val;

	switch (rate) {
		case 0:
			val = 0;
			break;
		case 50:
			val = B50;
			break;
		case 75:
			val = B75;
			break;
		case 110:
			val = B110;
			break;
		case 134:
			val = B134;
			break;
		case 150:
			val = B150;
			break;
		case 200:
			val = B200;
			break;
		case 300:
			val = B300;
			break;
		case 600:
			val = B600;
			break;
		case 1200:
			val = B1200;
			break;
		case 1800:
			val = B1800;
			break;
		case 2400:
			val = B2400;
			break;
		case 4800:
			val = B4800;
			break;
		case 9600:
			val = B9600;
			break;
		case 19200:
			val = B19200;
			break;
		case 38400:
			val = B38400;
			break;
#ifdef B57600
		case 57600:
			val = B57600;
			break;
#endif
#ifdef B115200
		case 115200:
			val = B115200;
			break;
#endif
#ifdef B230400
		case 230400:
			val = B230400;
			break;
#endif
#ifdef B460800
		case 460800:
			val = B460800;
			break;
#endif
		default:
			return 0;
	}

	*def = val;
	return 1;
}

/* {{{ dio_data_bits_to_define
 * Converts a number of data bits to a termios define
 */
int dio_data_bits_to_define(int data_bits, int *def) {
	int val;

	switch (data_bits) {
		case 8:
			val = CS8;
			break;
		case 7:
			val = CS7;
			break;
		case 6:
			val = CS6;
			break;
		case 5:
			val = CS5;
			break;
		default:
			return 0;
	}

	*def = val;
	return 1;
}
/* }}} */

/* {{{ dio_stop_bits_to_define
 * Converts a number of stop bits to a termios define
 */
int dio_stop_bits_to_define(int stop_bits, int *def) {
	int val;

	switch (stop_bits) {
		case 1:
			val = 0;
			break;
		case 2:
			val = CSTOPB;
			break;
		default:
			return 0;
	}

	*def = val;
	return 1;
}
/* }}} */

/* {{{ dio_parity_to_define
 * Converts a parity type to a termios define
 */
int dio_parity_to_define(int parity, int *def) {
	int val;

	switch (parity) {
		case 0:
			val = 0;
			break;
		case 1:
			val = PARENB | PARODD;
			break;
		case 2:
			val = PARENB;
			break;
		default:
			return 0;
	}

	*def = val;
	return 1;
}
/* }}} */

#endif

/* {{{ dio_assoc_array_get_basic_options
 * Retrives the basic open option values from an associative array
 */
int dio_assoc_array_get_basic_options(zval *options, php_dio_stream_data *data) {
	zval **tmpzval;
	HashTable *opthash;

	opthash = HASH_OF(options);

	/* This is the file mode flags used by open(). */
	if (zend_hash_find(opthash, "perms", sizeof("perms"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->perms = (mode_t)dio_convert_to_long(*tmpzval);
		data->has_perms = 1;
	}

#ifdef O_NONBLOCK
	/* This sets the underlying stream to be blocking/non
	   block (i.e. O_NONBLOCK) */
	if (zend_hash_find(opthash, "is_blocking", sizeof("is_blocking"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->is_blocking = dio_convert_to_long(*tmpzval) ? 1 : 0;
	}

	/* This is the timeout value for reads in seconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if (zend_hash_find(opthash, "timeout_secs", sizeof("timeout_secs"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->timeout.tv_sec = dio_convert_to_long(*tmpzval);
	}

	/* This is the timeout value for reads in microseconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if (zend_hash_find(opthash, "timeout_usecs", sizeof("timeout_usecs"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->timeout.tv_usec = dio_convert_to_long(*tmpzval);
	}

	data->has_timeout = (data->timeout.tv_sec | data->timeout.tv_usec) ? 1 : 0;
#endif
}
/* }}} */

/* {{{ dio_assoc_array_get_serial_options
 * Retrives the serial open option values from an associative array
 */
int dio_assoc_array_get_serial_options(zval *options, php_dio_stream_data *data) {
	zval **tmpzval;
	HashTable *opthash;

	opthash = HASH_OF(options);

	if (zend_hash_find(opthash, "data_rate", sizeof("data_rate"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->data_rate = dio_convert_to_long(*tmpzval);
	}

	if (zend_hash_find(opthash, "data_bits", sizeof("data_bits"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->data_bits = (int)dio_convert_to_long(*tmpzval);
	}

	if (zend_hash_find(opthash, "stop_bits", sizeof("stop_bits"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->stop_bits = (int)dio_convert_to_long(*tmpzval);
	}

	if (zend_hash_find(opthash, "parity", sizeof("parity"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->parity = (int)dio_convert_to_long(*tmpzval);
	}

	if (zend_hash_find(opthash, "rtscts", sizeof("rtscts"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->rtscts = (int)(dio_convert_to_long(*tmpzval) ? 1 : 0);
	}
}
/* }}} */

/* {{{ dio_common_write
 * Writes count chars from the buffer to the stream described by the stream data.
 */
size_t dio_common_write(php_dio_stream_data *data, const char *buf, size_t count) {
	size_t ret;

#ifndef PHP_WIN32
	/* Blocking writes can be interrupted by signals etc. If
	 * interrupted try again. Not sure about non-blocking
	 * writes but it doesn't hurt to check. */
	do {
		ret = write(data->fd, buf, count);
		if (ret > 0) {
			return ret;
		}
	} while (errno == EINTR);
	return 0;
#else
	return 0;
#endif
}
/* }}} */

#ifdef O_NONBLOCK
/* {{{ dio_timeval_subtract
 * Calculates the difference between two timevals returning the result in the
 * structure pointed to by diffptr.  Returns -1 as error if late time is
 * earlier than early time.
 */
static int dio_timeval_subtract(struct timeval *late, struct timeval *early, struct timeval *diff) {
	struct timeval *tmp;

	if ((late->tv_sec < early->tv_sec) ||
		((late->tv_usec == early->tv_sec) && (late->tv_usec < early->tv_usec))) {
		return 1;
	}

	/* Handle any carry.  If later usec is smaller than earlier usec simple
	 * subtraction will result in negative value.  Since usec has a maximum
	 * of one second by adding another second before the subtraction the
	 * result will always be positive. */
	if (late->tv_usec < early->tv_usec) {
		late->tv_usec  += 1000000;
		late->tv_sec--;
	}

	/* Once adjusted can just subtract values. */
	diff->tv_sec  = late->tv_sec  - early->tv_sec;
	diff->tv_usec = late->tv_usec - early->tv_usec;

	return 0;
}
#endif

/* {{{ dio_common_read
 * Reads count chars to the buffer to the stream described by the stream data.
 */
size_t dio_common_read(php_dio_stream_data *data, const char *buf, size_t count) {
	size_t ret, total = 0;
	char *ptr = (char*)buf;

#ifndef PHP_WIN32
	struct timeval timeout, timeouttmp, before, after, diff;
	fd_set rfds;

	if (!data->has_timeout) {
		/* Blocking reads can be interrupted by signals etc. If
		 * interrupted try again. Not sure about non-blocking
		 * reads but it doesn't hurt to check. */
		do {
			ret = read(data->fd, (char*)buf, count);
			if (ret > 0) {
				return ret;
			} else if (!ret) {
				data->end_of_file = 1;
			}
		} while ((errno == EINTR) && !data->end_of_file);
		return 0;
	}
#ifdef O_NONBLOCK
	else {
		/* The initial timeout value */
		timeout = data->timeout;

		do {
			/* The symantics of select() are that you cannot guarantee
			 * that the timeval structure passed in has not been changed by
			 * the select call.  So you keep a copy. */
			timeouttmp = timeout;

			/* The time before we wait for data. */
			(void) gettimeofday(&before, NULL);

			/* Wait for an event on our file descriptor. */
			FD_ZERO(&rfds);
			FD_SET(data->fd, &rfds);

			ret = select(data->fd + 1, &rfds, NULL, NULL, &timeouttmp);
			/* An error. */
			if ((ret < 0) && (errno != EINTR) && (errno != EAGAIN)) {
				return 0;
			}

			/* We have data to read. */
			if ((ret > 0) && FD_ISSET(data->fd, &rfds)) {
				ret = read(data->fd, ptr, count);
				/* Another error */
				if ((ret < 0) && (errno != EINTR) && (errno != EAGAIN)) {
					return 0;
				}

				if (ret > 0) {
					/* Got data, add it to the buffer. */
					ptr   += ret;
					total += ret;
					count -= ret;
				} else if (!ret) {
					/* This should never happen since how can we have
					 * data to read at an end of file, but still
					 * just in case! */
					data->end_of_file = 1;
				}
			}

			/* If not timed out and not end of file and not all data read
			 * calculate how long it took us and loop if we still have time
			 * out time left. */
			if (count && ret) {
				(void) gettimeofday(&after, NULL);

				/* Diff the timevals */
				(void) dio_timeval_subtract(&after, &before, &diff);

				/* Now adjust the timeout.  If it errors we've run out
				 * of time. */
				if (dio_timeval_subtract(&timeout, &diff, &timeout)) {
					break;
				}
			}
		} while (count && ret); /* Until time out or end of file or all data read. */

		return total;
	}
#endif
#else
	return 0;
#endif
}
/* }}} */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */

