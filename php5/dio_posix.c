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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#include "php_dio_common.h"

/* {{{ dio_stream_mode_to_flags
 * Convert an fopen() mode string to open() flags
 */
static int dio_stream_mode_to_flags(const char *mode) {
	int  flags = 0, ch = 0;
#if defined(_O_TEXT) && defined(O_BINARY)
	int  bin = 1;
#endif

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

#if defined(_O_TEXT) && defined(O_BINARY)
	if (mode[ch] != '+') {
		bin = (mode[ch++] == 'b');
	}
#endif

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

	return flags;
}
/* }}} */

/* {{{ dio_data_rate_to_define
 * Converts a numeric data rate to a termios define
 */
static int dio_data_rate_to_define(long rate, speed_t *def) {
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
static int dio_data_bits_to_define(int data_bits, int *def) {
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
static int dio_stop_bits_to_define(int stop_bits, int *def) {
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
static int dio_parity_to_define(int parity, int *def) {
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

/* {{{ dio_create_stream_data
 * Creates an initialised stream data structure.  Free with efree().
 */
php_dio_stream_data * dio_create_stream_data(void) {
	php_dio_posix_stream_data * data = emalloc(sizeof(php_dio_posix_stream_data));
	dio_init_stream_data(&(data->common));
	data->fd = -1;
	data->flags = 0;

	return (php_dio_stream_data *)data;
}
/* }}} */

/* {{{ dio_common_write
 * Writes count chars from the buffer to the stream described by the stream data.
 */
size_t dio_common_write(php_dio_stream_data *data, const char *buf, size_t count) {
	size_t ret;

	/* Blocking writes can be interrupted by signals etc. If
	 * interrupted try again. Not sure about non-blocking
	 * writes but it doesn't hurt to check. */
	do {
		ret = write(((php_dio_posix_stream_data*)data)->fd, buf, count);
		if (ret > 0) {
			return ret;
		}
	} while (errno == EINTR);
	return 0;
}
/* }}} */

#ifdef DIO_NONBLOCK
/* {{{ dio_timeval_subtract
 * Calculates the difference between two timevals returning the result in the
 * structure pointed to by diffptr.  Returns -1 as error if late time is
 * earlier than early time.
 */
static int dio_timeval_subtract(struct timeval *late, struct timeval *early, struct timeval *diff) {

	/* Handle negatives */
	if (late->tv_sec < early->tv_sec) {
		return 0;
	}

	if ((late->tv_sec == early->tv_sec) && (late->tv_usec < early->tv_usec)) {
		return 0;
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

	return 1;
}
#endif

/* {{{ dio_common_read
 * Reads count chars to the buffer to the stream described by the stream data.
 */
size_t dio_common_read(php_dio_stream_data *data, const char *buf, size_t count) {
	int fd = ((php_dio_posix_stream_data*)data)->fd;
	size_t ret, total = 0;
	char *ptr = (char*)buf;

	struct timeval timeout, timeouttmp, before, after, diff;
	fd_set rfds;

	if (!data->has_timeout) {
		/* Blocking reads can be interrupted by signals etc. If
		 * interrupted try again. Not sure about non-blocking
		 * reads but it doesn't hurt to check. */
		do {
			ret = read(fd, (char*)ptr, count);
			if (ret > 0) {
				return ret;
			} else if (!ret) {
				data->end_of_file = 1;
			}
		} while ((errno == EINTR) && !data->end_of_file);
		return 0;
	}
#ifdef DIO_NONBLOCK
	else {
		/* Clear timed out flag */
		data->timed_out = 0;

		/* The initial timeout value */
		timeout.tv_sec  = data->timeout_sec;
		timeout.tv_usec = data->timeout_usec;

		do {
			/* The semantics of select() are that you cannot guarantee
			 * that the timeval structure passed in has not been changed by
			 * the select call.  So you keep a copy. */
			timeouttmp = timeout;

			/* The time before we wait for data. */
			(void) gettimeofday(&before, NULL);

			/* Wait for an event on our file descriptor. */
			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);

			ret = select(fd + 1, &rfds, NULL, NULL, &timeouttmp);
			/* An error. */
			if ((ret < 0) && (errno != EINTR) && (errno != EAGAIN)) {
				return 0;
			}

			/* We have data to read. */
			if ((ret > 0) && FD_ISSET(fd, &rfds)) {
				ret = read(fd, ptr, count);
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
					break;
				}
			}

			/* If not timed out and not end of file and not all data read
			 * calculate how long it took us and loop if we still have time
			 * out time left. */
			if (count) {
				(void) gettimeofday(&after, NULL);

				/* Diff the timevals */
				(void) dio_timeval_subtract(&after, &before, &diff);

				/* Now adjust the timeout. */
				if (!dio_timeval_subtract(&timeout, &diff, &timeout)) {
					/* If it errors we've run out of time. */
					data->timed_out = 1;
					break;
				} else if (!timeout.tv_sec && !(timeout.tv_usec / 1000)) {
					/* Check for rounding issues (millisecond accuracy) */
					data->timed_out = 1;
					break;
				}
			}
		} while (count); /* Until time out or end of file or all data read. */

		return total;
	}
#endif
}
/* }}} */

/* {{{ php_dio_stream_data
 * Closes the php_stream.
 */
int dio_common_close(php_dio_stream_data *data) {
	if (close(((php_dio_posix_stream_data*)data)->fd) < 0) {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ dio_common_set_option
 * Sets/gets stream options
 */
int dio_common_set_option(php_dio_stream_data *data, int option, int value, void *ptrparam) {
	int fd = ((php_dio_posix_stream_data*)data)->fd;
	int old_is_blocking;
	int flags;

	switch (option) {
#ifdef DIO_NONBLOCK
		case PHP_STREAM_OPTION_READ_TIMEOUT:
			if (ptrparam) {
				struct timeval *tv = (struct timeval*)ptrparam;

				flags = fcntl(fd, F_GETFL, 0);

				/* A timeout of zero seconds and zero microseconds disables
				   any existing timeout. */
				if (tv->tv_sec || tv->tv_usec) {
					data->timeout_sec = tv->tv_sec;
					data->timeout_usec = tv->tv_usec;
					data->has_timeout = -1;
					(void) fcntl(fd, F_SETFL, flags & ~DIO_NONBLOCK);
				} else {
					data->timeout_sec = 0;
					data->timeout_usec = 0;
					data->has_timeout = 0;
					data->timed_out = 0;
					(void) fcntl(fd, F_SETFL, flags | DIO_NONBLOCK);
				}

				return PHP_STREAM_OPTION_RETURN_OK;
			} else {
				return PHP_STREAM_OPTION_RETURN_ERR;
			}

		case PHP_STREAM_OPTION_BLOCKING:
			flags = fcntl(fd, F_GETFL, 0);
			if (value) {
				flags &= ~DIO_NONBLOCK;
			} else {
				flags |= DIO_NONBLOCK;
			}
			(void) fcntl(fd, F_SETFL, flags);

			old_is_blocking = data->is_blocking;
			data->is_blocking = value;
			return old_is_blocking ? PHP_STREAM_OPTION_RETURN_OK : PHP_STREAM_OPTION_RETURN_ERR;
#endif /* O_NONBLOCK */

		default:
			break;
	}

	return 1;
}
/* }}} */

/* {{{ dio_raw_open_stream
 * Opens the underlying stream.
 */
int dio_raw_open_stream(char *filename, char *mode, php_dio_stream_data *data TSRMLS_DC) {
	php_dio_posix_stream_data *pdata = (php_dio_posix_stream_data*)data;
	pdata->flags = dio_stream_mode_to_flags(mode);

#ifdef DIO_NONBLOCK
	if (!data->is_blocking || data->has_timeout) {
		pdata->flags |= DIO_NONBLOCK;
	}
#endif

	/* Open the file and handle any errors. */
#ifdef DIO_HAS_FILEPERMS
	if (data->has_perms) {
		pdata->fd = open(filename, pdata->flags, (mode_t)data->perms);
	} else {
		pdata->fd = open(filename, pdata->flags);
	}
#else
	pdata->fd = open(filename, pdata->flags);
#endif

	if (pdata->fd < 0) {
		switch (errno) {
			case EEXIST:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "File exists!");
				return 0;
			default:
				return 0;
		}
	}

	return 1;
}
/* }}} */

/* {{{ dio_serial_init
 * Initialises the serial settings storing the original settings before hand.
 */
static int dio_serial_init(php_dio_stream_data *data TSRMLS_DC) {
	php_dio_posix_stream_data *pdata = (php_dio_posix_stream_data*)data;
	int ret = 0, data_bits_def, stop_bits_def, parity_def;
	struct termios tio;
	speed_t rate_def;

	if (!dio_data_rate_to_define(data->data_rate, &rate_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid data_rate value (%ld)", data->data_rate);
		return 0;
	}

	if (!dio_data_bits_to_define(data->data_bits, &data_bits_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid data_bits value (%d)", data->data_bits);
		return 0;
	}

	if (!dio_stop_bits_to_define(data->stop_bits, &stop_bits_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid stop_bits value (%d)", data->stop_bits);
		return 0;
	}

	if (!dio_parity_to_define(data->parity, &parity_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid parity value (%d)", data->parity);
		return 0;
	}

	ret = tcgetattr(pdata->fd, &(pdata->oldtio));
	if (ret < 0) {
		if ((errno == ENOTTY) || (errno == ENODEV)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not a serial port or terminal!");
		}
		return 0;
	}

	ret = tcgetattr(pdata->fd, &tio);
	if (ret < 0) {
		return 0;
	}

	if (data->canonical) {
		tio.c_iflag = IGNPAR | ICRNL;
		tio.c_oflag = 0;
		tio.c_lflag = ICANON;
	} else {
		cfmakeraw(&tio);
	}

	cfsetispeed(&tio, rate_def);
	cfsetospeed(&tio, rate_def);

	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= data_bits_def;
	tio.c_cflag &= ~CSTOPB;
	tio.c_cflag |= stop_bits_def;
	tio.c_cflag &= ~(PARENB|PARODD);
	tio.c_cflag |= parity_def;

#ifdef CRTSCTS
	tio.c_cflag &= ~(CLOCAL | CRTSCTS);
#else
	tio.c_cflag &= ~CLOCAL;
#endif
	if (!data->flow_control) {
		tio.c_cflag |= CLOCAL;
#ifdef CRTSCTS
	} else {
		tio.c_cflag |= CRTSCTS;
#endif
	}

	ret = tcsetattr(pdata->fd, TCSANOW, &tio);
	if (ret < 0) {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ dio_serial_uninit
 * Restores the serial settings back to their original state.
 */
int dio_serial_uninit(php_dio_stream_data *data) {
	php_dio_posix_stream_data *pdata = (php_dio_posix_stream_data*)data;
	int ret;

	do {
		ret = tcsetattr(pdata->fd, TCSANOW, &(pdata->oldtio));
	} while ((ret < 0) && (errno == EINTR));

	return 1;
}
/* }}} */

/* {{{ dio_serial_flush
 * Purges the serial buffers of data.
 */
int dio_serial_purge(php_dio_stream_data *data) {
	php_dio_posix_stream_data *pdata = (php_dio_posix_stream_data*)data;
	int ret;

	if ((pdata->flags & O_RDWR) == O_RDWR) {
		ret = tcflush(pdata->fd, TCIOFLUSH);
	} else if ((pdata->flags & O_WRONLY) == O_WRONLY) {
		ret = tcflush(pdata->fd, TCOFLUSH);
	} else if ((pdata->flags & O_RDONLY) == O_RDONLY) {
		ret = tcflush(pdata->fd, TCIFLUSH);
	}

	if (ret < 0) {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ dio_serial_open_stream
 * Opens the underlying stream.
 */
int dio_serial_open_stream(char *filename, char *mode, php_dio_stream_data *data TSRMLS_DC) {
	php_dio_posix_stream_data *pdata = (php_dio_posix_stream_data*)data;

#ifdef O_NOCTTY
	/* We don't want a controlling TTY */
	pdata->flags |= O_NOCTTY;
#endif

	if (!dio_raw_open_stream(filename, mode, data TSRMLS_CC)) {
		return 0;
	}

	if (!dio_serial_init(data TSRMLS_CC)) {
		close(pdata->fd);
		return 0;
	}

	return 1;
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
