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
	php_dio_stream_data* data = (php_dio_stream_data*)stream->abstract;
	size_t bytes = dio_common_read(data, buf, count);
	stream->eof = data->end_of_file;

	return bytes;
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

/* {{{ dio_stream_close
 * Close the stream
 */
static int dio_stream_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	php_dio_stream_data *abstract = (php_dio_stream_data*)stream->abstract;

	if (!dio_common_close(abstract)) {
		return 0;
	}

	efree(abstract);
	return 1;
}
/* }}} */

/* {{{ dio_stream_set_option
 * Set the stream options.
 */
static int dio_stream_set_option(php_stream *stream, int option, int value, void *ptrparam TSRMLS_DC)
{
	php_dio_stream_data *abstract = (php_dio_stream_data*)stream->abstract;

	switch (option) {
		case PHP_STREAM_OPTION_META_DATA_API:
#ifdef DIO_NONBLOCK
			add_assoc_bool((zval *)ptrparam, "timed_out", abstract->timed_out);
			add_assoc_bool((zval *)ptrparam, "blocked", abstract->is_blocking);
#endif
			add_assoc_bool((zval *)ptrparam, "eof", stream->eof);
			return PHP_STREAM_OPTION_RETURN_OK;

#if PHP_MAJOR_VERSION >= 5
		case PHP_STREAM_OPTION_CHECK_LIVENESS:
			stream->eof = abstract->end_of_file;
			return PHP_STREAM_OPTION_RETURN_OK;
#endif /* PHP_MAJOR_VERSION >= 5 */

		default:
			break;
	}

	return dio_common_set_option(abstract, option, value, ptrparam);
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
	if (php_check_open_basedir(filename TSRMLS_CC) || DIO_SAFE_MODE_CHECK(filename, mode)) {
		return NULL;
	}

	data = dio_create_stream_data();
	data->stream_type = DIO_STREAM_TYPE_RAW;

	/* Parse the context. */
	if (context) {
		dio_stream_context_get_basic_options(context, data TSRMLS_CC);
	}

	/* Try and open a raw stream. */
	if (!dio_raw_open_stream(filename, mode, data TSRMLS_CC)) {
		return NULL;
	}

	/* Create a PHP stream based on raw stream */
	stream = php_stream_alloc(&dio_raw_stream_ops, data, 0, mode);
	if (!stream) {
		(void) dio_common_close(data);
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

	char *filename;
	int   filename_len;
	char *mode;
	int   mode_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|z", &filename, &filename_len, &mode, &mode_len, &options) == FAILURE) {
		RETURN_FALSE;
	}

	/* Check the third argument is an array. */
	if (options && (Z_TYPE_P(options) != IS_ARRAY)) {
		RETURN_FALSE;
	}

	/* Check we can actually access the file. */
	if (php_check_open_basedir(filename TSRMLS_CC) || DIO_SAFE_MODE_CHECK(filename, mode)) {
		RETURN_FALSE;
	}

	data = dio_create_stream_data();
	data->stream_type = DIO_STREAM_TYPE_RAW;

	if (options) {
		dio_assoc_array_get_basic_options(options, data TSRMLS_CC);
	}

	/* Try and open a raw stream. */
	if (dio_raw_open_stream(filename, mode, data TSRMLS_CC)) {
		stream = php_stream_alloc(&dio_raw_stream_ops, data, 0, mode);
		if (!stream) {
			(void) dio_common_close(data);
			efree(data);
			RETURN_FALSE;
		}
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
	return dio_serial_purge((php_dio_stream_data*)stream->abstract);
}
/* }}} */

/* {{{ dio_stream_close
 * Close the stream.  Restores the serial settings to their value before
 * the stream was open.
 */
static int dio_serial_stream_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	php_dio_stream_data *abstract = (php_dio_stream_data*)stream->abstract;

	if (!dio_serial_uninit(abstract)) {
		return 0;
	}

	if (!dio_common_close(abstract)) {
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
	if (php_check_open_basedir(filename TSRMLS_CC) || DIO_SAFE_MODE_CHECK(filename, mode)) {
		return NULL;
	}

	data = dio_create_stream_data();
	data->stream_type = DIO_STREAM_TYPE_SERIAL;

	/* Parse the context. */
	if (context) {
		dio_stream_context_get_basic_options(context, data TSRMLS_CC);
		dio_stream_context_get_serial_options(context, data TSRMLS_CC);
	}

	/* Try and open a serial stream. */
	if (!dio_serial_open_stream(filename, mode, data TSRMLS_CC)) {
		return NULL;
	}

	stream = php_stream_alloc(&dio_serial_stream_ops, data, 0, mode);
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
	if (php_check_open_basedir(filename TSRMLS_CC) || DIO_SAFE_MODE_CHECK(filename, mode)) {
		RETURN_FALSE;
	}

	data = dio_create_stream_data();
	data->stream_type = DIO_STREAM_TYPE_SERIAL;

	if (options) {
		dio_assoc_array_get_basic_options(options, data TSRMLS_CC);
		dio_assoc_array_get_serial_options(options, data TSRMLS_CC);
	}

	/* Try and open a serial stream. */
	if (dio_serial_open_stream(filename, mode, data TSRMLS_CC)) {
		stream = php_stream_alloc(&dio_serial_stream_ops, data, 0, mode);
		if (!stream) {
			efree(data);
			RETURN_FALSE;
		}
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
