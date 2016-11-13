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


/* {{{ dio_assoc_array_get_basic_options
 * Retrieves the basic open option values from an associative array
 */
void dio_assoc_array_get_basic_options(zval *options, php_dio_stream_data *data) {
#if defined(DIO_HAS_FILEPERMS) || defined(DIO_NONBLOCK)
	zval      *tmpzval;
	HashTable *opthash;

	opthash = HASH_OF(options);
#endif

#ifdef DIO_HAS_FILEPERMS
	/* This is the file mode flags used by open(). */
	if ((tmpzval = zend_hash_str_find(opthash, "perms", sizeof("perms") -1)) != NULL) {
		data->perms = (int)zval_get_long(tmpzval);
		data->has_perms = 1;
	}
#endif

#ifdef DIO_NONBLOCK
	/* This sets the underlying stream to be blocking/non
	   block (i.e. O_NONBLOCK) */
	if ((tmpzval = zend_hash_str_find(opthash, "is_blocking", sizeof("is_blocking") -1)) != NULL) {
		data->is_blocking = zval_get_long(tmpzval) ? 1 : 0;
	}

	/* This is the timeout value for reads in seconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if ((tmpzval = zend_hash_str_find(opthash, "timeout_secs", sizeof("timeout_secs") -1)) != NULL) {
		data->timeout_sec = zval_get_long(tmpzval);
	}

	/* This is the timeout value for reads in microseconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if ((tmpzval = zend_hash_str_find(opthash, "timeout_usecs", sizeof("timeout_usecs") -1)) != NULL) {
		data->timeout_usec = zval_get_long(tmpzval);
	}

	data->has_timeout = (data->timeout_sec | data->timeout_usec) ? 1 : 0;
#endif
}
/* }}} */

/* {{{ dio_assoc_array_get_serial_options
 * Retrieves the serial open option values from an associative array
 */
void dio_assoc_array_get_serial_options(zval *options, php_dio_stream_data *data) {
	zval *tmpzval;
	HashTable *opthash;

	opthash = HASH_OF(options);

	if ((tmpzval = zend_hash_str_find(opthash, "data_rate", sizeof("data_rate") -1)) != NULL) {
		data->data_rate = zval_get_long(tmpzval);
	}

	if ((tmpzval = zend_hash_str_find(opthash, "data_bits", sizeof("data_bits") -1)) != NULL) {
		data->data_bits = (int)zval_get_long(tmpzval);
	}

	if ((tmpzval = zend_hash_str_find(opthash, "stop_bits", sizeof("stop_bits") -1)) != NULL) {
		data->stop_bits = (int)zval_get_long(tmpzval);
	}

	if ((tmpzval = zend_hash_str_find(opthash, "parity", sizeof("parity") -1)) != NULL) {
		data->parity = (int)zval_get_long(tmpzval);
	}

	if ((tmpzval = zend_hash_str_find(opthash, "flow_control", sizeof("flow_control") -1)) != NULL) {
		data->flow_control = (int)(zval_get_long(tmpzval) ? 1 : 0);
	}

	if ((tmpzval = zend_hash_str_find(opthash, "is_canonical", sizeof("is_canonical") -1)) != NULL) {
		data->canonical = (int)(zval_get_long(tmpzval) ? 1 : 0);
	}
}
/* }}} */

/* {{{ dio_stream_context_get_raw_options
 * Extracts the option values for dio.raw mode from a context
 */
void dio_stream_context_get_basic_options(php_stream_context *context, php_dio_stream_data *data) {
#if defined(DIO_HAS_FILEPERMS) || defined(DIO_NONBLOCK)
	zval *tmpzval;
#endif

#ifdef DIO_HAS_FILEPERMS
	/* This is the file mode flags used by open(). */
	if ((tmpzval = php_stream_context_get_option(context, "dio", "perms")) != NULL) {
		data->perms = (int)zval_get_long(tmpzval);
		data->has_perms = 1;
	}
#endif

#ifdef DIO_NONBLOCK
	/* This sets the underlying stream to be blocking/non
	   block (i.e. O_NONBLOCK) */
	if ((tmpzval = php_stream_context_get_option(context, "dio", "is_blocking")) != NULL) {
		data->is_blocking = zval_get_long(tmpzval) ? 1 : 0;
	}

	/* This is the timeout value for reads in seconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if ((tmpzval = php_stream_context_get_option(context, "dio", "timeout_secs")) != NULL) {
		data->timeout_sec = zval_get_long(tmpzval);
	}

	/* This is the timeout value for reads in microseconds.  Only one of
	   timeout_secs or timeout_usecs need be defined to define a timeout. */
	if ((tmpzval = php_stream_context_get_option(context, "dio", "timeout_usecs")) != NULL) {
		data->timeout_usec = zval_get_long(tmpzval);
	}

	data->has_timeout = (data->timeout_sec | data->timeout_usec) ? 1 : 0;
#endif
}
/* }}} */

/* {{{ dio_stream_context_get_serial_options
 * Extracts the option values for dio.serial mode from a context
 */
void dio_stream_context_get_serial_options(php_stream_context *context, php_dio_stream_data *data) {
	zval *tmpzval;

	if ((tmpzval = php_stream_context_get_option(context, "dio", "data_rate")) != NULL) {
		data->data_rate = zval_get_long(tmpzval);
	}

	if ((tmpzval = php_stream_context_get_option(context, "dio", "data_bits")) != NULL) {
		data->data_bits = (int)zval_get_long(tmpzval);
	}

	if ((tmpzval = php_stream_context_get_option(context, "dio", "stop_bits")) != NULL) {
		data->stop_bits = (int)zval_get_long(tmpzval);
	}

	if ((tmpzval = php_stream_context_get_option(context, "dio", "parity")) != NULL) {
		data->parity = (int)zval_get_long(tmpzval);
	}

	if ((tmpzval = php_stream_context_get_option(context, "dio", "flow_control")) != NULL) {
		data->flow_control = (int)(zval_get_long(tmpzval) ? 1 : 0);
	}

	if ((tmpzval = php_stream_context_get_option(context, "dio", "is_canonical")) != NULL) {
		data->canonical = (int)(zval_get_long(tmpzval) ? 1 : 0);
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

