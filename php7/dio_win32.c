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
#include "php_dio_common.h"

#ifndef ZEND_WIN32
#error ZEND_WIN32 not defined!
#endif

/* {{{ dio_last_error_php_error
 * Generates a PHP error message based upon the last Windows error.
 */
static void dio_last_error_php_error(int level, char * message) {
	LPVOID msgbuf;
	DWORD  msgbuflen;
	char * errmsg;
	DWORD  err;

#ifdef UNICODE
	DWORD  errmsglen;
#endif

	err = GetLastError();
	msgbuflen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|
		FORMAT_MESSAGE_FROM_SYSTEM|
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&msgbuf,
		0,
		NULL);

#ifdef UNICODE

	/* Get the length of the converted message */
	errmsglen = WideCharToMultibyte(
		CP_ACP,
		0
		(LPCWSTR)msgbuf,
		-1,
		(LPSTR)errmsg,
		0,
		NULL,
		NULL);

	/* Allocate a buffer */
	errmsg = emalloc(errmsglen);
	if (!errmsg) {
		php_error_docref(NULL, E_ERROR, "Out of memory in dio_last_error_php_error()!");
		LocalFree(msgbuf);
		return;
	}

	/* Convert the message */
	errmsglen = WideCharToMultibyte(
		CP_ACP,
		0
		(LPCWSTR)msgbuf,
		-1,
		(LPSTR)errmsg,
		errmsglen,
		NULL,
		NULL);

#else
	errmsg = (char *)msgbuf;
#endif

	php_error_docref(NULL, E_WARNING, "%s[ERROR %d] %s", message, err, errmsg);

	LocalFree(msgbuf);
#ifdef UNICODE
	efree(errmsg);
#endif
}

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
		case 256000:
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
			val = ONESTOPBIT;
			break;
		case 2:
			val = TWOSTOPBITS;
			break;
		case 3:
			val = ONE5STOPBITS;
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
	DWORD val;

	switch (parity) {
		case 0:
			val = NOPARITY;
			break;
		case 1:
			val = ODDPARITY;
			break;
		case 2:
			val = EVENPARITY;
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
	php_dio_win32_stream_data * data = emalloc(sizeof(php_dio_win32_stream_data));
	memset(data, 0, sizeof(php_dio_win32_stream_data));
	dio_init_stream_data(&(data->common));
	data->handle = INVALID_HANDLE_VALUE;
	data->desired_access = 0;
	data->creation_disposition = 0;
	data->olddcb.DCBlength = sizeof(DCB);

	return (php_dio_stream_data *)data;
}
/* }}} */

/* {{{ dio_common_write
 * Writes count chars from the buffer to the stream described by the stream data.
 */
#if PHP_VERSION_ID < 70400
size_t dio_common_write(php_dio_stream_data *data, const char *buf, size_t count) {
#else
ssize_t dio_common_write(php_dio_stream_data *data, const char *buf, size_t count) {
#endif
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	DWORD total = 0;

	if (WriteFile(wdata->handle, buf, (DWORD)count, &total, NULL)) {
		return (size_t)total;
	}

#if PHP_VERSION_ID < 70400
	return 0;
#else
	return -1;
#endif
}
/* }}} */

/* {{{ dio_buffer_read
 * Reads any available chars from the canonical buffer.
 */
static size_t dio_buffer_read(php_dio_win32_stream_data *wdata, const char *buf, size_t count) {
	php_dio_win32_canon_data *canon_data = wdata->canon_data;
	size_t total = 0;

	/* Read always follows write.  I.e. if read ptr > write ptr buffer has
	   wrapped and so we need to copy two blocks of data. */
	if (canon_data->read_pos > canon_data->write_pos) {

		/* Check we actually need to copy both blocks */
		if ((canon_data->size - canon_data->read_pos) > count) {

			/* No we don't.  Just copy as much as we were asked for. */
			memcpy((char*)buf, 
				   &(canon_data->buf[canon_data->read_pos]), 
				   count);
			/* Update the read pointer. */
			canon_data->read_pos += count;

			/* Return the amount read. */
			return count;
		} else {

			/* We need to copy both blocks so copy data up to the end of 
			   the buffer. */
			total = canon_data->size - canon_data->read_pos;
			memcpy((char*)buf, 
				   &(canon_data->buf[canon_data->read_pos]), 
				   total);
			canon_data->read_pos = 0;
			count -= total;

			/* Now copy the data from the start of the buffer either up
			   count or the number of bytes in the buffer. */

			if (canon_data->write_pos > count) {
				memcpy((char*)buf, canon_data->buf, count);
				canon_data->read_pos = count;
				total += count;

				return total;
			} else {
				memcpy((char*)buf, canon_data->buf, canon_data->write_pos);
				canon_data->read_pos = canon_data->write_pos;
				total += canon_data->write_pos;

				return total;
			}
		}

	/* Else if write follows read.  This is a simpler case.  We just copy 
	   either all the data buffered or count, which ever is smaller. */
	} else if (canon_data->write_pos > canon_data->read_pos) {
		if ((canon_data->write_pos - canon_data->read_pos) > count) {
			memcpy((char*)buf, 
				   &(canon_data->buf[canon_data->read_pos]), 
				   count);
			canon_data->read_pos += count;

			return count;
		} else {
			total = canon_data->write_pos - canon_data->read_pos;
			memcpy((char*)buf, 
				   &(canon_data->buf[canon_data->read_pos]), 
				   total);
			canon_data->read_pos += total;

			return total;
		}
	}

	/* Else we need to read more data from the data port. */
	return 0;
}

/* {{{ dio_com_read
 * Read chars from the data port.
 */
#if PHP_VERSION_ID < 70400
static size_t dio_com_read(php_dio_stream_data *data, const char *buf, size_t count) {
#else
static ssize_t dio_com_read(php_dio_stream_data *data, const char *buf, size_t count) {
#endif
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	DWORD err, total = 0;

	if (ReadFile(wdata->handle, (void*)buf, (DWORD)count, &total, NULL)) {

		if (total) {
			return (size_t)total;
		}

		data->end_of_file = 1;
	}

	if (!data->end_of_file) {
		err = GetLastError();

		if (ERROR_HANDLE_EOF == err) {
			data->end_of_file = 1;
		}
	}

#if PHP_VERSION_ID < 70400
	return 0;
#else
	return (data->end_of_file ? 0 : -1);
#endif
}

/* {{{ dio_canonical_read
 * Reads chars from the input stream until the internal buffer is full or a new
 * line is reached.
 */
#if PHP_VERSION_ID < 70400
static size_t dio_canonical_read(php_dio_win32_stream_data *wdata, const char *buf, size_t count) {
#else
static ssize_t dio_canonical_read(php_dio_win32_stream_data *wdata, const char *buf, size_t count) {
#endif
	php_dio_win32_canon_data *canon_data = wdata->canon_data;
	size_t total = 0;
	char ch;

	/* See if there's any buffered data and copy it. */
	total = dio_buffer_read(wdata, buf, count);
	if (total) {
		return total;
	}

	/* Need to read more data from the data port.  Buffer should be empty(er)
	   by now. */
	do {
		/* Is the buffer full? */
		if (((canon_data->write_pos + 1) % canon_data->size) == 
			canon_data->read_pos) {
			break;
		}

		/* Read a byte from the input checking for EOF. */
		if (dio_com_read((php_dio_stream_data*)wdata, &ch, 1) < 1) {
			break;
		}

		/* Translate CR to newlines (same as ICRNL in POSIX) */
		ch = (ch != '\r') ? ch : '\n';

		/* We read a character!  So buffer it. */
		canon_data->buf[canon_data->write_pos++] = ch;
		if (canon_data->write_pos >= canon_data->size) {
			canon_data->write_pos = 0;
		}

		/* End of line/input (^D)? */
	} while ((ch != '\n') && (ch != 0x04));

	return dio_buffer_read(wdata, buf, count);
}
/* }}} */

/* {{{ dio_common_read
 * Reads count chars to the buffer to the stream described by the stream data.
 */
#if PHP_VERSION_ID < 70400
size_t dio_common_read(php_dio_stream_data *data, const char *buf, size_t count) {
#else
ssize_t dio_common_read(php_dio_stream_data *data, const char *buf, size_t count) {
#endif

	/* You ask for no bytes you'll get none :-) */
	if (!count) {
		return 0;
	}

	if (data->canonical) {
		return dio_canonical_read((php_dio_win32_stream_data*)data, buf, count);
	} else {
		return dio_com_read(data, buf, count);
	}
}
/* }}} */

/* {{{ php_dio_stream_data
 * Closes the php_stream.
 */
int dio_common_close(php_dio_stream_data *data) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;

	if (data->canonical) {
		efree(wdata->canon_data);
	}

	if (!CloseHandle(wdata->handle)) {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ dio_common_set_option
 * Sets/gets stream options
 */
int dio_common_set_option(php_dio_stream_data *data, int option, int value, void *ptrparam) {
	COMMTIMEOUTS cto = { 0, 0, 0, 0, 0 };
	int old_is_blocking = 0;

	/* Can't do timeouts or non blocking with raw windows streams :-( */
	if (DIO_STREAM_TYPE_SERIAL == data->stream_type) {
		switch (option) {
			case PHP_STREAM_OPTION_BLOCKING:
				old_is_blocking   = data->is_blocking;
				data->is_blocking = value ? 1 : 0;

				/* Only change values if we need to change them. */
				if (data->is_blocking != old_is_blocking) {
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

					if (!SetCommTimeouts(((php_dio_win32_stream_data*)data)->handle, &cto)) {
						return PHP_STREAM_OPTION_RETURN_ERR;
					}
				}
				return old_is_blocking ? PHP_STREAM_OPTION_RETURN_OK : PHP_STREAM_OPTION_RETURN_ERR;

			case PHP_STREAM_OPTION_READ_TIMEOUT:
				if (ptrparam) {
					/* struct timeval is supported with PHP_WIN32 defined. */
					struct timeval *tv = (struct timeval*)ptrparam;

					/* A timeout of zero seconds and zero microseconds disables
					   any existing timeout. */
					if (tv->tv_sec || tv->tv_usec) {
						data->timeout_sec = tv->tv_sec;
						data->timeout_usec = tv->tv_usec;
						data->has_timeout = -1;

						cto.ReadIntervalTimeout = MAXDWORD;
						cto.ReadTotalTimeoutMultiplier  = MAXDWORD;
						cto.ReadTotalTimeoutConstant = (data->timeout_usec / 1000) +
							(data->timeout_sec * 1000);
					} else {
						data->timeout_sec = 0;
						data->timeout_usec = 0;
						data->has_timeout = 0;
						data->timed_out = 0;

						/* If we're not blocking but don't have a timeout
						   set to return immediately */
						if (!data->is_blocking) {
							cto.ReadIntervalTimeout = MAXDWORD;
						}
					}

					if (!SetCommTimeouts(((php_dio_win32_stream_data*)data)->handle, &cto)) {
						return PHP_STREAM_OPTION_RETURN_ERR;
					} else {
						return PHP_STREAM_OPTION_RETURN_OK;
					}
				} else {
					return PHP_STREAM_OPTION_RETURN_ERR;
				}

			default:
				break;
		}
	}

	return 1;
}
/* }}} */

/* {{{ dio_raw_open_stream
 * Opens the underlying stream.
 */
int dio_raw_open_stream(const char *filename, const char *mode, php_dio_stream_data *data) {
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
				php_error_docref(NULL, E_WARNING, "File exists!");
				return 0;

			case ERROR_FILE_NOT_FOUND:
				/* ERROR_FILE_NOT_FOUND with TRUNCATE_EXISTING means that
				 * the file doesn't exist so now try to create it. */
				if (TRUNCATE_EXISTING == wdata->creation_disposition) {
					php_error_docref(NULL, E_NOTICE, "File does not exist, creating new file!");

					wdata->handle = CreateFile(filename, wdata->desired_access, 0,
								NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (INVALID_HANDLE_VALUE == wdata->handle) {
						dio_last_error_php_error(E_WARNING, "CreateFile() failed:");
						return 0;
					}
				} else {
					php_error_docref(NULL, E_WARNING, "File not found!");
					return 0;
				}
				break;

			default:
				dio_last_error_php_error(E_WARNING, "CreateFile() failed:");
				return 0;
		}
	}

	/* If canonical allocate the canonical buffer. */
	if (data->canonical) {
		wdata->canon_data = emalloc(sizeof(php_dio_win32_canon_data));
		memset(wdata->canon_data, 0, sizeof(php_dio_win32_canon_data));
		wdata->canon_data->size = DIO_WIN32_CANON_BUF_SIZE;
	}

	return 1;
}
/* }}} */

/* {{{ dio_serial_init
 * Initialises the serial port
 */
static int dio_serial_init(php_dio_stream_data *data) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	DWORD rate_def, data_bits_def, stop_bits_def, parity_def;
	DCB dcb;

	if (!dio_data_rate_to_define(data->data_rate, &rate_def)) {
		php_error_docref(NULL, E_WARNING, "invalid data_rate value (%d)", data->data_rate);
		return 0;
	}

	if (!dio_data_bits_to_define(data->data_bits, &data_bits_def)) {
		php_error_docref(NULL, E_WARNING, "invalid data_bits value (%d)", data->data_bits);
		return 0;
	}

	if (!dio_stop_bits_to_define(data->stop_bits, &stop_bits_def)) {
		php_error_docref(NULL, E_WARNING, "invalid stop_bits value (%d)", data->stop_bits);
		return 0;
	}

	if (!dio_parity_to_define(data->parity, &parity_def)) {
		php_error_docref(NULL, E_WARNING, "invalid parity value (%d)", data->parity);
		return 0;
	}

	if (!GetCommState(wdata->handle, &(wdata->olddcb))) {
		dio_last_error_php_error(E_WARNING, "GetCommState() failed:");
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
		dio_last_error_php_error(E_WARNING, "SetCommState() failed:");
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

	/* Purge the canonical buffer if required */
	if (data->canonical && ((wdata->desired_access & GENERIC_READ) == GENERIC_READ)) {
		wdata->canon_data->read_pos  = 0;
		wdata->canon_data->write_pos = 0;
	}

	/* Purge the com port */
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
int dio_serial_open_stream(const char *filename, const char *mode, php_dio_stream_data *data) {
	php_dio_win32_stream_data *wdata = (php_dio_win32_stream_data*)data;
	COMMTIMEOUTS cto = { 0, 0, 0, 0, 0 };

	php_error_docref(NULL, E_NOTICE, "Opening \"%s\" as a serial port (mode=\"%s\").", filename, mode);

	if (*mode != 'r') {
		php_error_docref(NULL, E_WARNING, "You must open serial ports in read or read/write mode!");
		return 0;
	}

	if (!dio_raw_open_stream(filename, mode, data)) {
		return 0;
	}

	if (!GetCommTimeouts(wdata->handle, &(wdata->oldcto))) {
		dio_last_error_php_error(E_WARNING, "GetCommTimeouts() failed (Not a comm port?):");
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
		dio_last_error_php_error(E_WARNING, "SetCommTimeouts() failed:");
		CloseHandle(wdata->handle);
		return 0;
	}

	if (!dio_serial_init(data)) {
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
