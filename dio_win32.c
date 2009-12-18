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

/* {{{ dio_data_rate_to_define
 * Converts a numeric data rate to a termios define
 */
static int dio_data_rate_to_define(long rate, DWORD *def) {
	switch (rate) {
		case 75:
		case 110:
		case 134:
		case 150:
		case 300:
		case 600:
		case 1200:
		case 1800:
		case 2400:
		case 4800:
		case 7200:
		case 9600:
		case 14400:
		case 19200:
		case 38400:
		case 57600:
		case 115200:
		case 56000:
		case 128000:
			break;
		default:
			return 0;
	}

	*def = (DWORD)rate;
	return 1;
}
/* }}} */


/* {{{ dio_data_bits_to_define
 * Converts a number of data bits to a termios define
 */
static int dio_data_bits_to_define(int data_bits, DWORD *def) {
	switch (data_bits) {
		case 8:
		case 7:
		case 6:
		case 5:
		case 4:
			break;
		default:
			return 0;
	}

	*def = (DWORD)data_bits;
	return 1;
}
/* }}} */

/* {{{ dio_stop_bits_to_define
 * Converts a number of stop bits to a termios define
 */
static int dio_stop_bits_to_define(int stop_bits, DWORD *def) {
	DWORD val;

	switch (stop_bits) {
		case 1:
			val = 0;
			break;
		case 2:
			val = 2;
			break;
		case 3:
			val = 1;
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
static int dio_parity_to_define(int parity, DWORD *def) {
	switch (parity) {
		case 0:
		case 1:
		case 2:
			break;
		default:
			return 0;
	}

	*def = (DWORD)parity;
	return 1;
}
/* }}} */

/* {{{ dio_create_stream_data
 * Creates an initialised stream data structure.  Free with efree().
 */
php_dio_stream_data * dio_create_stream_data(void) {
	php_dio_win32_stream_data * data = emalloc(sizeof(php_dio_win32_stream_data));
	dio_init_stream_data(&(data->common));
	data->handle = INVALID_HANDLE_VALUE;
	data->desired_access = 0;
	data->creation_disposition = 0;
	memset(&(data->olddcb), 0, sizeof(DCB));
	data->olddcb.DCBlength = sizeof(DCB);
	memset(&(data->oldcto), 0, sizeof(COMMTIMEOUTS));

	return (php_dio_stream_data *)data;
}
/* }}} */

/* {{{ dio_common_write
 * Writes count chars from the buffer to the stream described by the stream data.
 */
size_t dio_common_write(php_dio_stream_data *data, const char *buf, size_t count) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	DWORD total = 0;

	if (WriteFile(wdata->handle, buf, (DWORD)count, &total, NULL)) {
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
	DWORD err, total = 0;

	if (ReadFile(wdata->handle, buf, (DWORD)count, &total, NULL)) {
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
	DWORD err;

	switch(*mode) {
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
	mode ++;

	if (*mode && (*mode != '+')) {
		mode++;
	}

	if (*mode && (*mode == '+')) {
		wdata->desired_access = GENERIC_READ | GENERIC_WRITE;
	} else if (OPEN_EXISTING == wdata->creation_disposition) {
		wdata->desired_access = GENERIC_READ;
	} else {
		wdata->desired_access = GENERIC_WRITE;
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
						err = GetLastError();
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

/* {{{ dio_serial_init
 * Initialises the serial port
 */
static int dio_serial_init(php_dio_stream_data *data TSRMLS_DC) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	DWORD err, rate_def, data_bits_def, stop_bits_def, parity_def;
	DCB dcb;

	if (!dio_data_rate_to_define(data->data_rate, &rate_def)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid data_rate value (%d) (%d)", data->data_rate, __LINE__);
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

	if (!GetCommState(wdata->handle, &(wdata->olddcb))) {
		err = GetLastError();
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "GetCommState() failed! (%d)", err);
		return 0;
	}

	/* Init the DCB structure */
	memset(&dcb, 0, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	/* Set the communication parameters */
	dcb.fBinary  = 1;
	dcb.BaudRate = rate_def;
	dcb.ByteSize = (BYTE)data_bits_def;
	dcb.StopBits = (BYTE)stop_bits_def;
	dcb.Parity   = (BYTE)parity_def;

	/* Set the control line parameters */
	dcb.fDtrControl       = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity   = FALSE;
	dcb.fOutxDsrFlow      = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX             = FALSE;
	dcb.fInX              = FALSE;
	dcb.fErrorChar        = FALSE;
	dcb.fNull             = FALSE;
	dcb.fAbortOnError     = FALSE;

	/* Hardware flow control */
	if (data->flow_control) {
		dcb.fOutxCtsFlow = TRUE;
		dcb.fRtsControl  = RTS_CONTROL_HANDSHAKE;
	} else {
		dcb.fOutxCtsFlow = FALSE;
		dcb.fRtsControl  = RTS_CONTROL_DISABLE;
	}

	if (!SetCommState(wdata->handle, &dcb)) {
		return 0;
	}

	return 1;
}
/* }}} */


/* {{{ dio_serial_uninit
 * Restores the serial settings back to their original state.
 */
int dio_serial_uninit(php_dio_stream_data *data) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;

	if (!SetCommState(wdata->handle, &(wdata->olddcb))) {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ dio_serial_flush
 * Purges the serial buffers of data.
 */
int dio_serial_purge(php_dio_stream_data *data) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	BOOL ret;

	if ((wdata->desired_access & (GENERIC_READ|GENERIC_WRITE)) == (GENERIC_READ|GENERIC_WRITE)) {
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

	if (!GetCommTimeouts(wdata->handle, &(wdata->oldcto))) {
		err = GetLastError();
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "SetCommTimeouts() failed! (%d) Not a comm port?", err);
		CloseHandle(wdata->handle);
		return 0;
	}

	/* If we're not blocking but don't have a timeout
	   set to return immediately */
	if (!data->is_blocking && !data->has_timeout) {
		cto.ReadIntervalTimeout = MAXDWORD;
	}
	
	/* If we have a timeout ignore the blocking and set
	   the total time in which to read the data */
	if (data->has_timeout) {
		cto.ReadIntervalTimeout = MAXDWORD;
		cto.ReadTotalTimeoutMultiplier  = MAXDWORD;
		cto.ReadTotalTimeoutConstant = (data->timeout_usec / 1000) + 
			(data->timeout_sec * 1000);
	}

	if (!SetCommTimeouts(wdata->handle, &cto)) {
		CloseHandle(wdata->handle);
		return 0;
	}

	if (!dio_serial_init(data TSRMLS_CC)) {
		CloseHandle(wdata->handle);
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
