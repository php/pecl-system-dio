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

#include "php_dio_common.h"

/* {{{ dio_create_stream_data
 * Creates an initialised stream data structure.  Free with efree().
 */
php_dio_stream_data * dio_create_stream_data(void) {
	php_dio_win32_stream_data * data = emalloc(sizeof(php_dio_win32_stream_data));
	dio_init_stream_data(&(data->common));
	data->handle = INVALID_HANDLE_VALUE;
	data->desired_access = 0;
	data->creation_disposition = 0;

	return (php_dio_stream_data *)data;
}
/* }}} */

/* {{{ dio_common_write
 * Writes count chars from the buffer to the stream described by the stream data.
 */
size_t dio_common_write(php_dio_stream_data *data, const char *buf, size_t count) {
	char *ptr = (char*)buf;
	DWORD total = 0;

	if (WriteFile(((php_dio_win32_stream_data*)data)->handle, ptr, (DWORD)count, &total, NULL)) {
		return (size_t)total;
	}

	return 0;
}
/* }}} */

/* {{{ dio_common_read
 * Reads count chars to the buffer to the stream described by the stream data.
 */
size_t dio_common_read(php_dio_stream_data *data, const char *buf, size_t count) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	char *ptr = (char*)buf;
	DWORD total = 0;
	DWORD err;

	if (ReadFile(wdata->handle, ptr, (DWORD)count, &total, NULL)) {
		if (!total) {
			data->end_of_file = 1;
		}
		return (size_t)total;
	}

	err = GetLastError();

	if (ERROR_HANDLE_EOF == err) {
		data->end_of_file = 1;
	}

	return 0;
}
/* }}} */

/* {{{ php_dio_stream_data
 * Closes the php_stream.
 */
int dio_common_close(php_dio_stream_data *data) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;

	if (!CloseHandle(wdata->handle)) {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ dio_raw_open_stream
 * Opens the underlying stream.
 */
int dio_raw_open_stream(char *filename, char *mode, php_dio_stream_data *data TSRMLS_DC) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	int ch = 0;
	DWORD err;

	switch(mode[ch++]) {
		case 'r':
			wdata->creation_disposition = OPEN_EXISTING;
			break;
		case 'w':
			wdata->creation_disposition = TRUNCATE_EXISTING;
			break;
		case 'a':
			wdata->creation_disposition = OPEN_ALWAYS;
			break;
		case 'x':
			wdata->creation_disposition = CREATE_NEW;
			break;
	}

	if (mode[ch] != '+') {
		mode[ch++];
	}

	if (mode[ch] == '+') {
		wdata->desired_access = GENERIC_READ | GENERIC_WRITE;
	} else if (OPEN_EXISTING != wdata->creation_disposition) {
		wdata->desired_access = GENERIC_WRITE;
	} else {
		wdata->desired_access = GENERIC_READ;
	}

	wdata->handle = CreateFile(filename, wdata->desired_access, 0,
			NULL, wdata->creation_disposition, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == wdata->handle) {
		err = GetLastError();
		switch (err) {
			case ERROR_FILE_EXISTS:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "File exists!");
				return 0;

			case ERROR_FILE_NOT_FOUND:
				/* ERROR_FILE_NOT_FOUND with TRUNCATE_EXISTING means that
				 * the file doesn't exist so now try to create it. */
				if (TRUNCATE_EXISTING == wdata->creation_disposition) {
					wdata->handle = CreateFile(filename, wdata->desired_access, 0,
								NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (INVALID_HANDLE_VALUE == wdata->handle) {
						return 0;
					}
				} else {
					return 0;
				}
				break;

			default:
				return 0;
		}
	}

	return 1;
}
/* }}} */

/* {{{ dio_serial_uninit
 * Restores the serial settings back to their original state.
 */
int dio_serial_uninit(php_dio_stream_data *data) {

	return 1;
}
/* }}} */

/* {{{ dio_serial_flush
 * Purges the serial buffers of data.
 */
int dio_serial_purge(php_dio_stream_data *data) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	BOOL ret;

	if ((data->desired_access & (GENERIC_READ|GENERIC_WRITE)) == (GENERIC_READ|GENERIC_WRITE)) {
		ret = PurgeComm(wdata->handle, PURGE_RXCLEAR|PURGE_TXCLEAR);
	} else if ((wdata->desired_access & GENERIC_WRITE) == GENERIC_WRITE) {
		ret = PurgeComm(wdata->handle, PURGE_TXCLEAR);
	} else if ((wdata->desired_access & GENERIC_READ) == GENERIC_READ) {
		ret = PurgeComm(wdata->handle, PURGE_RXCLEAR);
	}

	return ret;
}
/* }}} */

/* {{{ dio_serial_open_stream
 * Opens the underlying stream.
 */
int dio_serial_open_stream(char *filename, char *mode, php_dio_stream_data *data TSRMLS_DC) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	COMMTIMEOUTS cto = { 0, 0, 0, 0, 0 };
	DWORD err;

	if (!dio_raw_open_stream(filename, mode, data TSRMLS_CC)) {
		return 0;
	}

	if (!data->is_blocking) {
		cto.ReadIntervalTimeout = MAXDWORD;
		if (data->has_timeout) {
			cto.ReadTotalTimeoutMultiplier = MAXDWORD;
			cto.ReadTotalTimeoutConstant =
					(data->timeout_usec / 1000) + (data->timeout_sec * 1000);
		}
	}

	if (!SetCommTimeouts(wdata->handle, &cto)) {
		err = GetLastError();
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "SetCommTimeouts() failed! (%d)", err);
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
