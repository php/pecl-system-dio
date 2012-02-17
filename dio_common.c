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

#include "php_dio.h"
#include "php_dio_common.h"

/* {{{ dio_init_stream_data
 * Initialises the command parts of the stream data.
 */
void dio_init_stream_data(php_dio_stream_data *data) {
	data->stream_type = DIO_STREAM_TYPE_NONE;
	data->end_of_file = 0;
#ifdef DIO_HAS_FILEPERMS
	data->has_perms = 0;
	data->perms = 0;
#endif
#ifdef DIO_NONBLOCK
	data->is_blocking = 1;
	data->has_timeout = 0;
	data->timeout_sec = 0;
	data->timeout_usec = 0;
	data->timed_out = 0;
#endif
	/* Serial options */
	data->data_rate = 9600;
	data->data_bits = 8;
	data->stop_bits = 1;
	data->parity = 0;
	data->flow_control = 1;
	data->canonical = 1;
}
/* }}} */

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

/* {{{ dio_assoc_array_get_basic_options
 * Retrieves the basic open option values from an associative array
 */
void dio_assoc_array_get_basic_options(zval *options, php_dio_stream_data *data TSRMLS_DC) {
#if defined(DIO_HAS_FILEPERMS) || defined(DIO_NONBLOCK)
	zval **tmpzval;
	HashTable *opthash;

	opthash = HASH_OF(options);
#endif

#ifdef DIO_HAS_FILEPERMS
	/* This is the file mode flags used by open(). */
	if (zend_hash_find(opthash, "perms", sizeof("perms"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->perms = (int)dio_convert_to_long(*tmpzval);
		data->has_perms = 1;
	}
#endif

#ifdef DIO_NONBLOCK
	/* This sets the underlying stream to be blocking/non
	   block (i.e. O_NONBLOCK) */
	if (zend_hash_find(opthash, "is_blocking", sizeof("is_blocking"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->is_blocking = dio_convert_to_long(*tmpzval) ? 1 : 0;
	}

	/* This is the timeout value for reads in seconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if (zend_hash_find(opthash, "timeout_secs", sizeof("timeout_secs"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->timeout_sec = dio_convert_to_long(*tmpzval);
	}

	/* This is the timeout value for reads in microseconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if (zend_hash_find(opthash, "timeout_usecs", sizeof("timeout_usecs"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->timeout_usec = dio_convert_to_long(*tmpzval);
	}

	data->has_timeout = (data->timeout_sec | data->timeout_usec) ? 1 : 0;
#endif
}
/* }}} */

/* {{{ dio_assoc_array_get_serial_options
 * Retrieves the serial open option values from an associative array
 */
void dio_assoc_array_get_serial_options(zval *options, php_dio_stream_data *data TSRMLS_DC) {
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

	if (zend_hash_find(opthash, "flow_control", sizeof("flow_control"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->flow_control = (int)(dio_convert_to_long(*tmpzval) ? 1 : 0);
	}

	if (zend_hash_find(opthash, "is_canonical", sizeof("is_canonical"), (void **)&tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->canonical = (int)(dio_convert_to_long(*tmpzval) ? 1 : 0);
	}
}
/* }}} */

/* {{{ dio_stream_context_get_raw_options
 * Extracts the option values for dio.raw mode from a context
 */
void dio_stream_context_get_basic_options(php_stream_context *context, php_dio_stream_data *data TSRMLS_DC) {
#if defined(DIO_HAS_FILEPERMS) || defined(DIO_NONBLOCK)
	zval **tmpzval;
#endif

#ifdef DIO_HAS_FILEPERMS
	/* This is the file mode flags used by open(). */
	if (php_stream_context_get_option(context, "dio", "perms", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->perms = (int)dio_convert_to_long(*tmpzval);
		data->has_perms = 1;
	}
#endif

#ifdef DIO_NONBLOCK
	/* This sets the underlying stream to be blocking/non
	   block (i.e. O_NONBLOCK) */
	if (php_stream_context_get_option(context, "dio", "is_blocking", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->is_blocking = dio_convert_to_long(*tmpzval) ? 1 : 0;
	}

	/* This is the timeout value for reads in seconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if (php_stream_context_get_option(context, "dio", "timeout_secs", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->timeout_sec = dio_convert_to_long(*tmpzval);
	}

	/* This is the timeout value for reads in microseconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if (php_stream_context_get_option(context, "dio", "timeout_usecs", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->timeout_usec = dio_convert_to_long(*tmpzval);
	}

	data->has_timeout = (data->timeout_sec | data->timeout_usec) ? 1 : 0;
#endif
}
/* }}} */

/* {{{ dio_stream_context_get_serial_options
 * Extracts the option values for dio.serial mode from a context
 */
void dio_stream_context_get_serial_options(php_stream_context *context, php_dio_stream_data *data TSRMLS_DC) {
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

	if (php_stream_context_get_option(context, "dio", "flow_control", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->flow_control = (int)(dio_convert_to_long(*tmpzval) ? 1 : 0);
	}

	if (php_stream_context_get_option(context, "dio", "is_canonical", &tmpzval) == SUCCESS && tmpzval && *tmpzval) {
		data->canonical = (int)(dio_convert_to_long(*tmpzval) ? 1 : 0);
	}
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

