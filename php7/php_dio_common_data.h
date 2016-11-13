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

#ifndef PHP_DIO_COMMON_DATA_H_
#define PHP_DIO_COMMON_DATA_H_

/* This is the data structure 'base class'.  It is common data fields used
 * by all versions of DIO.
 */
typedef struct _php_dio_stream_data {
	/* Stream type */
	int stream_type;
	/* Stream options */
	int end_of_file;
#ifdef DIO_HAS_FILEPERMS
	int has_perms;
	int perms;
#endif
#ifdef DIO_NONBLOCK
	int is_blocking;
	int has_timeout;
	long timeout_sec;
	long timeout_usec;
	int timed_out;
#endif
	/* Serial options */
	long data_rate;
	int data_bits;
	int stop_bits;
	int parity;
	int flow_control;
	int canonical;
} php_dio_stream_data ;

#endif /* PHP_DIO_COMMON_DATA_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
