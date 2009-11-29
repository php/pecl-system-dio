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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/url.h"

#include "php_dio.h"
#include "php_dio_common.h"
#include "php_dio_stream_wrappers.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#ifndef PHP_WIN32
#include <termios.h>
#endif

/* {{{ dio_stream_context_get_raw_options
 * Extracts the option values for dio.raw mode from a context
 */
static void dio_stream_context_get_basic_options(php_stream_context *context, php_dio_stream_data *data) {
	zval **tmpzval;

	/* This is the file mode flags used by open(). */
	if (php_stream_context_get_option(context, "dio", "perms", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->perms = (mode_t)dio_convert_to_long(*tmpzval);
		data->has_perms = 1;
	}

#ifdef O_NONBLOCK
	/* This sets the underlying stream to be blocking/non
	   block (i.e. O_NONBLOCK) */
	if (php_stream_context_get_option(context, "dio", "is_blocking", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->is_blocking = dio_convert_to_long(*tmpzval) ? 1 : 0;
	}

	/* This is the timeout value for reads in seconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if (php_stream_context_get_option(context, "dio", "timeout_secs", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->timeout.tv_sec = dio_convert_to_long(*tmpzval);
	}

	/* This is the timeout value for reads in microseconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if (php_stream_context_get_option(context, "dio", "timeout_usecs", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->timeout.tv_usec = dio_convert_to_long(*tmpzval);
	}

	data->has_timeout = (data->timeout.tv_sec | data->timeout.tv_usec) ? 1 : 0;
#endif
}
/* }}} */

/* {{{ dio_stream_context_get_serial_options
 * Extracts the option values for dio.serial mode from a context
 */
static void dio_stream_context_get_serial_options(php_stream_context *context, php_dio_stream_data *data) {
	zval **tmpzval;

	if (php_stream_context_get_option(context, "dio", "data_rate", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->data_rate = dio_convert_to_long(*tmpzval);
	}

	if (php_stream_context_get_option(context, "dio", "data_bits", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->data_bits = (int)dio_convert_to_long(*tmpzval);
	}

	if (php_stream_context_get_option(context, "dio", "stop_bits", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->stop_bits = (int)dio_convert_to_long(*tmpzval);
	}

	if (php_stream_context_get_option(context, "dio", "parity", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->parity = (int)dio_convert_to_long(*tmpzval);
	}

	if (php_stream_context_get_option(context, "dio", "rtscts", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->rtscts = (int)(dio_convert_to_long(*tmpzval) ? 1 : 0);
	}
}
/* }}} */

/*
   +----------------------------------------------------------------------+
   | Raw stream handling                                                  |
   +----------------------------------------------------------------------+
*/

/* {{{ dio_stream_write
 * Write to the stream
 */
static size_t dio_stream_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC)
{
	return dio_common_write((php_dio_stream_data*)stream->abstract, buf, count);
}
/* }}} */

/* {{{ dio_stream_read
 * Read from the stream
 */
static size_t dio_stream_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
{
	return dio_common_read((php_dio_stream_data*)stream->abstract, buf, count);
}
/* }}} */

/* {{{ dio_stream_close
 * Close the stream
 */
static int dio_stream_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	php_dio_stream_data *abstract = (php_dio_stream_data*)stream->abstract;

	if (close(abstract->fd) < 0) {
		return 0;
	}

	efree(abstract);
	return 1;
}
/* }}} */

/* {{{ dio_stream_flush
 * Flush the stream.  For raw streams this does nothing.
 */
static int dio_stream_flush(php_stream *stream TSRMLS_DC)
{
	return 1;
}
/* }}} */

/* {{{ dio_stream_set_option
 * Set the stream options.
 */
static int dio_stream_set_option(php_stream *stream, int option, int value, void *ptrparam TSRMLS_DC)
{
	php_dio_stream_data *abstract = (php_dio_stream_data*)stream->abstract;
	int old_is_blocking;
	int flags;

	switch (option) {

#ifdef O_NONBLOCK
		case PHP_STREAM_OPTION_READ_TIMEOUT:
			if (ptrparam) {
				struct timeval *tv = (struct timeval*)ptrparam;

				flags = fcntl(abstract->fd, F_GETFL, 0);

				/* A timeout of zero seconds and zero micro seconds disables
				   any existing timeout. */
				if (tv->tv_sec || tv->tv_usec) {
					abstract->timeout.tv_sec = tv->tv_sec;
					abstract->timeout.tv_usec = tv->tv_usec;
					abstract->has_timeout = -1;
					(void) fnctl(abstract->fd, F_SETFL, flags & ~O_NONBLOCK);
				} else {
					abstract->timeout.tv_sec = 0;
					abstract->timeout.tv_usec = 0;
					abstract->has_timeout = 0;
					(void) fnctl(abstract->fd, F_SETFL, flags | O_NONBLOCK);
				}
			} else {
				return 0;
			}
			break;

		case PHP_STREAM_OPTION_BLOCKING:
			flags = fcntl(abstract->fd, F_GETFL, 0);
			if (value) {
				flags &= ~O_NONBLOCK;
			} else {
				flags |= O_NONBLOCK;
			}
			(void) fcntl(abstract->fd, F_SETFL, flags);

			old_is_blocking = abstract->is_blocking;
			abstract->is_blocking = value;
			return old_is_blocking;
#endif

#if PHP_MAJOR_VERSION >= 5
		case PHP_STREAM_OPTION_CHECK_LIVENESS:
			stream->eof = abstract->end_of_file;
			return stream->eof;
#endif
	}

	return 1;
}
/* }}} */

php_stream_ops dio_raw_stream_ops = {
	dio_stream_write,
	dio_stream_read,
	dio_stream_close,
	dio_stream_flush,
	"dio",
	NULL, /* seek */
	NULL, /* cast */
	NULL, /* stat */
	dio_stream_set_option,
};

/* {{{ dio_raw_open_stream
 * Opens the php_stream.
 */
static php_stream *dio_raw_open_stream(char *filename, char *mode, php_dio_stream_data *data) {
	data->flags = dio_stream_mode_to_flags(mode);

#ifdef O_NONBLOCK
	if (!data->is_blocking || data->has_timeout) {
		data->flags |= O_NONBLOCK;
	}
#endif

	/* Open the file and handle any errors. */
	if (data->has_perms) {
		data->fd = open(filename, data->flags, data->perms);
	} else {
		data->fd = open(filename, data->flags);
	}

	if (data->fd < 0) {
		switch (errno) {
			case EEXIST:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "File exists!");
				return NULL;
			default:
				return NULL;
		}
	}

	return php_stream_alloc(&dio_raw_stream_ops, data, 0, mode);
}
/* }}} */

/* {{{ dio_raw_fopen_wrapper
 * fopen for the dio.raw stream.
 */
static php_stream *dio_raw_fopen_wrapper(php_stream_wrapper *wrapper,
                                         char *path, char *mode,
                                         int options, char **opened_path,
                                         php_stream_context *context STREAMS_DC TSRMLS_DC) {
	php_dio_stream_data *data;
	php_stream *stream;
	char *filename;

	/* Check it was actually for us (not a corrupted function pointer
	   somewhere!). */
	if (strncmp(path, DIO_RAW_STREAM_PROTOCOL, sizeof(DIO_RAW_STREAM_PROTOCOL) - 1)) {
		return NULL;
	}

	/* Get the actually file system name/path. */
	filename = path + sizeof(DIO_RAW_STREAM_PROTOCOL) - 1;

	/* Check we can actually access it. */
	if (php_check_open_basedir(filename TSRMLS_CC) ||
		(PG(safe_mode) && !php_checkuid(filename, mode, CHECKUID_CHECK_MODE_PARAM))) {
		return NULL;
	}

	/* Initialise the stream data structure. */
	data = emalloc(sizeof(php_dio_stream_data));
	data->has_perms = 0;
	data->perms = 0;
	data->end_of_file = 0;
#ifdef O_NONBLOCK;
	data->is_blocking = 1;
	data->has_timeout = 0;
	data->timeout.tv_sec = 0;
	data->timeout.tv_usec = 0;
#endif

	/* Parse the context. */
	if (context) {
		dio_stream_context_get_basic_options(context, data);
	}

	/* Try and open a raw stream. */
	stream = dio_raw_open_stream(filename, mode, data);
	if (!stream) {
		efree(data);
	}

	return stream;
}
/* }}} */

static php_stream_wrapper_ops dio_raw_stream_wops = {
	dio_raw_fopen_wrapper,
	NULL, /* stream_close */
	NULL, /* stat */
	NULL, /* stat_url */
	NULL, /* opendir */
	DIO_RAW_STREAM_NAME
};

php_stream_wrapper php_dio_raw_stream_wrapper = {
	&dio_raw_stream_wops,
	NULL,
	0
};

/* {{{ proto dio_raw(string filename, string mode[, array options])
 * Opens a raw direct IO stream.
 */
PHP_FUNCTION(dio_raw) {
	zval *options = NULL;
	php_dio_stream_data *data;
	php_stream *stream;
	HashTable *opthash;

	char *filename;
	int   filename_len;
	char *mode;
	int   mode_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|z", &filename, &filename_len, &mode, &mode_len, &options) == FAILURE) {
		RETURN_FALSE;
	}

	/* Check the third argument is an array. */
	if (options && (Z_TYPE_P(options) != IS_ARRAY)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,"dio_raw, the third argument should be an array of options");
		RETURN_FALSE;
	}

	/* Check we can actually access the file. */
	if (php_check_open_basedir(filename TSRMLS_CC) ||
		(PG(safe_mode) && !php_checkuid(filename, mode, CHECKUID_CHECK_MODE_PARAM))) {
		RETURN_FALSE;
	}

	/* Initialise the stream data structure. */
	data = emalloc(sizeof(php_dio_stream_data));
	data->has_perms = 0;
	data->perms = 0;
	data->end_of_file = 0;
#ifdef O_NONBLOCK
	data->is_blocking = 1;
	data->has_timeout = 0;
	data->timeout.tv_sec = 0;
	data->timeout.tv_usec = 0;
#endif

	if (options) {
		dio_assoc_array_get_basic_options(options, data);
	}

	/* Try and open a raw stream. */
	stream = dio_raw_open_stream(filename, mode, data);
	if (!stream) {
		efree(data);
		RETURN_FALSE;
	}

	php_stream_to_zval(stream, return_value);
}
/* }}} */

/*
   +----------------------------------------------------------------------+
   | Serial stream handling                                               |
   +----------------------------------------------------------------------+
*/

/* {{{ dio_stream_flush
 * Flush the stream.  If the stream is read only, it flushes the read
 * stream, if it is write only it flushes the write, otherwise it flushes
 * both.
 */
static int dio_serial_stream_flush(php_stream *stream TSRMLS_DC)
{
	php_dio_stream_data *abstract = (php_dio_stream_data*)stream->abstract;
	int ret;

#ifndef PHP_WIN32
	if ((abstract->flags & O_RDWR) == O_RDWR) {
		ret = tcflush(abstract->fd, TCIOFLUSH);
	} else if ((abstract->flags & O_WRONLY) == O_WRONLY) {
		ret = tcflush(abstract->fd, TCOFLUSH);
	} else if ((abstract->flags & O_RDONLY) == O_RDONLY) {
		ret = tcflush(abstract->fd, TCIFLUSH);
	}

	if (ret < 0) {
		return 0;
	}
#endif

	return 1;
}
/* }}} */

/* {{{ dio_stream_close
 * Close the stream.  Restores the serial settings to their value before
 * the stream was open.
 */
static int dio_serial_stream_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	php_dio_stream_data *abstract = (php_dio_stream_data*)stream->abstract;
	int ret;

#ifndef PHP_WIN32
	do {
		ret = tcsetattr(abstract->fd, TCSAFLUSH, &(abstract->oldtio));
	} while ((ret < 0) && (errno == EINTR));
#endif

	if (close(abstract->fd) < 0) {
		return 0;
	}

	efree(abstract);
	return 1;
}
/* }}} */

php_stream_ops dio_serial_stream_ops = {
	dio_stream_write,
	dio_stream_read,
	dio_serial_stream_close,
	dio_serial_stream_flush,
	"dio",
	NULL, /* seek */
	NULL, /* cast */
	NULL, /* stat */
	dio_stream_set_option,
};

#ifndef PHP_WIN32

/* {{{ dio_serial_init
 * Initialises the serial settings storing the original settings before hand.
 */
static int dio_serial_init(php_dio_stream_data *data, speed_t data_rate_in, speed_t data_rate_out,
		                   int data_bits, int stop_bits, int parity) {
	struct termios tio;
	int ret = 0;

	ret = tcgetattr(data->fd, &(data->oldtio));
	if (ret < 0) {
		if ((errno == ENOTTY) || (errno == ENODEV)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not a serial port or terminal!");
		}
		return 0;
	}

	ret = tcgetattr(data->fd, &tio);
	if (ret < 0) {
		return 0;
	}

	tio.c_ispeed = data_rate_in;
	tio.c_ospeed = data_rate_out;
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= data_bits;
	tio.c_cflag &= ~CSTOPB;
	tio.c_cflag |= stop_bits;
	tio.c_cflag &= ~(PARENB|PARODD);
	tio.c_cflag |= parity;

	tio.c_cflag &= ~CRTSCTS;
	if (data->rtscts) {
		tio.c_cflag |= CRTSCTS;
	}

	ret = tcsetattr(data->fd, TCSAFLUSH, &tio);
	if (ret < 0) {
		return 0;
	}

	return 1;
}
/* }}} */

#endif

/* {{{ dio_raw_open_stream
 * Opens the php_stream.
 */
static php_stream *dio_serial_open_stream(char *filename, char *mode, php_dio_stream_data *data) {
	speed_t rate_def;
	int data_bits_def;
	int stop_bits_def;
	int parity_def;

	data->flags = dio_stream_mode_to_flags(mode);

#ifdef O_NONBLOCK
	if (!data->is_blocking || data->has_timeout) {
		data->flags |= O_NONBLOCK;
	}
#endif

	if (!dio_data_rate_to_define(data->data_rate, &rate_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid data_rate value (%ld)", data->data_rate);
		return NULL;
	}

	if (!dio_data_bits_to_define(data->data_bits, &data_bits_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid data_bits value (%d)", data->data_bits);
		return NULL;
	}

	if (!dio_stop_bits_to_define(data->stop_bits, &stop_bits_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid stop_bits value (%d)", data->stop_bits);
		return NULL;
	}

	if (!dio_parity_to_define(data->parity, &parity_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid parity value (%d)", data->parity);
		return NULL;
	}

	/* Open the file and handle any errors. */
	if (data->has_perms) {
		data->fd = open(filename, data->flags, data->perms);
	} else {
		data->fd = open(filename, data->flags);
	}

	if (data->fd < 0) {
		switch (errno) {
			case EEXIST:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "File exists!");
				return NULL;
			default:
				return NULL;
		}
	}

#ifndef PHP_WIN32
	if (!dio_serial_init(data, rate_def, rate_def, data_bits_def, stop_bits_def, parity_def)) {
		close(data->fd);
		return NULL;
	}
#else
	/* TODO: Windows version */
	return NULL;
#endif

	return php_stream_alloc(&dio_serial_stream_ops, data, 0, mode);
}
/* }}} */

/* {{{ dio_raw_fopen_wrapper
 * fopen for the dio.raw stream.
 */
static php_stream *dio_serial_fopen_wrapper(php_stream_wrapper *wrapper,
                                         char *path, char *mode,
                                         int options, char **opened_path,
                                         php_stream_context *context STREAMS_DC TSRMLS_DC) {
	php_dio_stream_data *data;
	php_stream *stream;
	char *filename;

	/* Check it was actually for us (not a corrupted function pointer
	   somewhere!). */
	if (strncmp(path, DIO_SERIAL_STREAM_PROTOCOL, sizeof(DIO_SERIAL_STREAM_PROTOCOL) - 1)) {
		return NULL;
	}

	/* Get the actually file system name/path. */
	filename = path + sizeof(DIO_SERIAL_STREAM_PROTOCOL) - 1;

	/* Check we can actually access it. */
	if (php_check_open_basedir(filename TSRMLS_CC) ||
		(PG(safe_mode) && !php_checkuid(filename, mode, CHECKUID_CHECK_MODE_PARAM))) {
		return NULL;
	}

	/* Initialise the stream data structure. */
	data = emalloc(sizeof(php_dio_stream_data));
	data->has_perms = 0;
	data->perms = 0;
	data->end_of_file = 0;
#ifdef O_NONBLOCK
	data->is_blocking = 1;
	data->has_timeout = 0;
	data->timeout.tv_sec = 0;
	data->timeout.tv_usec = 0;
#endif
	data->data_rate = 9600;
	data->data_bits = 8;
	data->stop_bits = 1;
	data->parity = 0;
	data->rtscts = 1;

	/* Parse the context. */
	if (context) {
		dio_stream_context_get_basic_options(context, data);
		dio_stream_context_get_serial_options(context, data);
	}

	/* Try and open a raw stream. */
	stream = dio_serial_open_stream(filename, mode, data);
	if (!stream) {
		efree(data);
	}

	return stream;
}
/* }}} */

static php_stream_wrapper_ops dio_serial_stream_wops = {
	dio_serial_fopen_wrapper,
	NULL, /* stream_close */
	NULL, /* stat */
	NULL, /* stat_url */
	NULL, /* opendir */
	DIO_SERIAL_STREAM_NAME
};

php_stream_wrapper php_dio_serial_stream_wrapper = {
	&dio_serial_stream_wops,
	NULL,
	0
};

/* {{{ proto dio_serial(string filename, string mode[, array options])
 * Opens a serial direct IO stream.
 */
PHP_FUNCTION(dio_serial) {
	zval *options = NULL;
	php_dio_stream_data *data;
	php_stream *stream;

	char *filename;
	int   filename_len;
	char *mode;
	int   mode_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|z", &filename, &filename_len, &mode, &mode_len, &options) == FAILURE) {
		RETURN_FALSE;
	}

	/* Check the third argument is an array. */
	if (options && (Z_TYPE_P(options) != IS_ARRAY)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,"dio_serial, the third argument should be an array of options");
		RETURN_FALSE;
	}

	/* Check we can actually access the file. */
	if (php_check_open_basedir(filename TSRMLS_CC) ||
		(PG(safe_mode) && !php_checkuid(filename, mode, CHECKUID_CHECK_MODE_PARAM))) {
		RETURN_FALSE;
	}

	/* Initialise the stream data structure. */
	data = emalloc(sizeof(php_dio_stream_data));
	data->has_perms = 0;
	data->perms = 0;
	data->end_of_file = 0;
#ifdef O_NONBLOCK
	data->is_blocking = 1;
	data->has_timeout = 0;
	data->timeout.tv_sec = 0;
	data->timeout.tv_usec = 0;
#endif
	data->data_rate = 9600;
	data->data_bits = 8;
	data->stop_bits = 1;
	data->parity = 0;
	data->rtscts = 1;

	if (options) {
		dio_assoc_array_get_basic_options(options, data);
		dio_assoc_array_get_serial_options(options, data);
	}

	/* Try and open a raw stream. */
	stream = dio_serial_open_stream(filename, mode, data);
	if (!stream) {
		efree(data);
		RETURN_FALSE;
	}

	php_stream_to_zval(stream, return_value);
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
