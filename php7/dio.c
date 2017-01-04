/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2009 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Sterling Hughes <sterling@php.net>                           |
   | Author: Melanie Rhianna Lewis <cyberspice@php.net>                   |
   +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

#include "php_dio.h"
#include "php_dio_stream_wrappers.h"

#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#ifndef PHP_WIN32
#include <termios.h>
#endif

/* e.g. IRIX does not have CRTSCTS */
#ifndef CRTSCTS
# ifdef CNEW_RTSCTS
#  define CRTSCTS CNEW_RTSCTS
# else
#  define CRTSCTS 0
# endif /* CNEW_RTSCTS */
#endif /* !CRTSCTS */

/*
   +----------------------------------------------------------------------+
   |                       DEPRECATED FUNCTIONALITY                       |
   +----------------------------------------------------------------------+
   | The functions below are from the earlier DIO versions.  They will    |
   | continue to be maintained but not extended.  It is thoroughly        |
   | recommended that you should use either the stream wrappers or the    |
   | DIO classes in new code. - Melanie                                   |
   +----------------------------------------------------------------------+
 */

#define le_fd_name "Direct I/O File Descriptor"
static int le_fd;

static int new_php_fd(php_fd_t **f, int fd)
{
	if (!(*f = malloc(sizeof(php_fd_t)))) {
		return 0;
	}
	(*f)->fd = fd;
	return 1;
}

static void _dio_close_fd(zend_resource *rsrc)
{
	php_fd_t *f = (php_fd_t *) rsrc->ptr;
	if (f) {
		close(f->fd);
		free(f);
	}
}

/* {{{ proto resource dio_open(string filename, int flags[, int mode])
   Open a new filename with specified permissions of flags and creation permissions of mode */
PHP_FUNCTION(dio_open)
{
	php_fd_t *f;
	char     *file_name;
	size_t       file_name_length;
	zend_long      flags;
	zend_long      mode = 0;
	int       fd;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sl|l", &file_name, &file_name_length, &flags, &mode) == FAILURE) {
		return;
	}

	if (php_check_open_basedir(file_name) || DIO_SAFE_MODE_CHECK(file_name, "wb+")) {
		RETURN_FALSE;
	}

	if (ZEND_NUM_ARGS() == 3) {
		fd = open(file_name, flags, mode);
	} else {
		fd = open(file_name, flags);
	}

	if (fd == -1) {
		php_error_docref(NULL, E_WARNING, "cannot open file %s with flags %ld and permissions %ld: %s", file_name, flags, mode, strerror(errno));
		RETURN_FALSE;
	}

	if (!new_php_fd(&f, fd)) {
		RETURN_FALSE;
	}

	RETURN_RES(zend_register_resource(f, le_fd));
}
/* }}} */

#ifndef PHP_WIN32

/* {{{ proto resource dio_fdopen(int fd)
   Returns a resource for the specified file descriptor. */
PHP_FUNCTION(dio_fdopen)
{
	php_fd_t *f;
	zend_long lfd;
	int fd;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &lfd) == FAILURE) {
		return;
	}

	fd = (int)lfd;

	if ((fcntl(fd, F_GETFL, 0) == -1) && (errno == EBADF)) {
		php_error_docref(NULL, E_WARNING, "Bad file descriptor %d", fd);
		RETURN_FALSE;
	}

	if (!new_php_fd(&f, fd)) {
		RETURN_FALSE;
	}

	RETURN_RES(zend_register_resource(f, le_fd));
}
/* }}} */


/* {{{ proto resource dio_dup(resource fd)
   Opens a duplicate of the specified open resource. */
PHP_FUNCTION(dio_dup)
{
	zval     *r_fd;
	php_fd_t *f, *df;
	int dfd;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &r_fd) == FAILURE) {
		return;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}

	dfd = dup(f->fd);
	if (dfd == -1) {
		php_error_docref(NULL, E_WARNING, "cannot duplication file descriptor %d: %s", f->fd, strerror(errno));
		RETURN_FALSE;
	}

	if (!new_php_fd(&df, dfd)) {
		RETURN_FALSE;
	}

	RETURN_RES(zend_register_resource(df, le_fd));
}
/* }}} */
#endif

/* {{{ proto string dio_read(resource fd[, int n])
   Read n bytes from fd and return them, if n is not specified, read 1k */
PHP_FUNCTION(dio_read)
{
	zval     *r_fd;
	php_fd_t *f;
	char     *data;
	zend_long      bytes = 1024;
	ssize_t   res;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "r|l", &r_fd, &bytes) == FAILURE) {
		return;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}

	if (bytes <= 0) {
		php_error_docref(NULL, E_WARNING, "Length parameter must be greater than 0.");
		RETURN_FALSE;
	}

	data = emalloc(bytes + 1);
	res = read(f->fd, data, bytes);
	if (res <= 0) {
		efree(data);
		RETURN_NULL();
	}

	data = erealloc(data, res + 1);
	data[res] = 0;

	RETURN_STRINGL(data, res);
	efree(data);
}
/* }}} */

/* {{{ proto int dio_write(resource fd, string data[, int len])
   Write data to fd with optional truncation at length */
PHP_FUNCTION(dio_write)
{
	zval     *r_fd;
	php_fd_t *f;
	char     *data;
	size_t       data_len;
	zend_long      trunc_len = 0;
	ssize_t   res;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "rs|l", &r_fd, &data, &data_len, &trunc_len) == FAILURE) {
		return;
	}

	if (trunc_len < 0 || trunc_len > data_len) {
		php_error_docref(NULL, E_WARNING, "length must be greater or equal to zero and less then the length of the specified string.");
		RETURN_FALSE;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}
	

	res = write(f->fd, data, trunc_len ? trunc_len : data_len);
	if (res == -1) {
		php_error_docref(NULL, E_WARNING, "cannot write data to file descriptor %d: %s", f->fd, strerror(errno));
	}

	RETURN_LONG(res);
}
/* }}} */

#ifndef PHP_WIN32

/* {{{ proto bool dio_truncate(resource fd, int offset)
   Truncate file descriptor fd to offset bytes */
PHP_FUNCTION(dio_truncate)
{
	zval     *r_fd;
	php_fd_t *f;
	zend_long      offset;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl", &r_fd, &offset) == FAILURE) {
		return;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}

	if (ftruncate(f->fd, offset) == -1) {
		php_error_docref(NULL, E_WARNING, "couldn't truncate %d to %ld bytes: %s", f->fd, offset, strerror(errno));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */
#endif

#define ADD_FIELD(f, v) add_assoc_long_ex(return_value, (f), sizeof(f), v);

/* {{{ proto array dio_stat(resource fd)
   Get stat information about the file descriptor fd */
PHP_FUNCTION(dio_stat)
{
	zval        *r_fd;
	php_fd_t    *f;
	struct stat  s;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &r_fd) == FAILURE) {
		return;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}

	if (fstat(f->fd, &s) == -1) {
		php_error_docref(NULL, E_WARNING, "cannot stat %d: %s", f->fd, strerror(errno));
		RETURN_FALSE;
	}

	array_init(return_value);
	ADD_FIELD("device", s.st_dev);
	ADD_FIELD("inode", s.st_ino);
	ADD_FIELD("mode", s.st_mode);
	ADD_FIELD("nlink", s.st_nlink);
	ADD_FIELD("uid", s.st_uid);
	ADD_FIELD("gid", s.st_gid);
	ADD_FIELD("device_type", s.st_rdev);
	ADD_FIELD("size", s.st_size);
#ifndef PHP_WIN32
	ADD_FIELD("block_size", s.st_blksize);
	ADD_FIELD("blocks", s.st_blocks);
#endif
	ADD_FIELD("atime", s.st_atime);
	ADD_FIELD("mtime", s.st_mtime);
	ADD_FIELD("ctime", s.st_ctime);
}
/* }}} */

/* {{{ proto int dio_seek(resource fd, int pos, int whence)
   Seek to pos on fd from whence */
PHP_FUNCTION(dio_seek)
{
	zval     *r_fd;
	php_fd_t *f;
	zend_long      offset;
	zend_long      whence = SEEK_SET;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl|l", &r_fd, &offset, &whence) == FAILURE) {
		return;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}

	RETURN_LONG(zend_lseek(f->fd, offset, whence));
}
/* }}} */

#ifndef PHP_WIN32

/* {{{ proto mixed dio_fcntl(resource fd, int cmd[, mixed arg])
   Perform a c library fcntl on fd */
PHP_FUNCTION(dio_fcntl)
{
	zval     *r_fd;
	zval     *arg = NULL;
	php_fd_t *f;
	zend_long      cmd;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl|z", &r_fd, &cmd, &arg) == FAILURE) {
		return;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}

	switch (cmd) {
		case F_SETLK:
		case F_SETLKW: {
			zval           *element;
			struct flock    lk = {0};
			HashTable      *fh;

			if (!arg) {
				php_error_docref(NULL, E_WARNING, "expects argument 3 to be array or int, none given");
				RETURN_FALSE;
			}
			if (Z_TYPE_P(arg) == IS_ARRAY) {
				fh = HASH_OF(arg);
				if ((element = zend_hash_str_find(fh, "start", sizeof("start") - 1)) == NULL) {
					lk.l_start = 0;
				} else {
					lk.l_start = Z_LVAL_P(element);
				}

				if ((element = zend_hash_str_find(fh, "length", sizeof("length") - 1)) == NULL) {
					lk.l_len = 0;
				} else {
					lk.l_len = Z_LVAL_P(element);
				}

				if ((element = zend_hash_str_find(fh, "whence", sizeof("whence") - 1)) == NULL) {
					lk.l_whence = 0;
				} else {
					lk.l_whence = Z_LVAL_P(element);
				}

				if ((element = zend_hash_str_find(fh, "type", sizeof("type") - 1)) == NULL) {
					lk.l_type = 0;
				} else {
					lk.l_type = Z_LVAL_P(element);
				}
			} else if (Z_TYPE_P(arg) == IS_LONG) {
				lk.l_start  = 0;
				lk.l_len    = 0;
				lk.l_whence = SEEK_SET;
				lk.l_type   = Z_LVAL_P(arg);
			} else {
				php_error_docref(NULL, E_WARNING, "expects argument 3 to be array or int, %s given", zend_zval_type_name(arg));
				RETURN_FALSE;
			}

			RETURN_LONG(fcntl(f->fd, cmd, &lk));
			break;
		}
		case F_GETLK: {
			struct flock lk = {0};

			fcntl(f->fd, cmd, &lk);

			array_init(return_value);
			add_assoc_long(return_value, "type", lk.l_type);
			add_assoc_long(return_value, "whence", lk.l_whence);
			add_assoc_long(return_value, "start", lk.l_start);
			add_assoc_long(return_value, "length", lk.l_len);
			add_assoc_long(return_value, "pid", lk.l_pid);

			break;
		}
		case F_DUPFD: {
			php_fd_t *new_f;

			if (!arg || Z_TYPE_P(arg) != IS_LONG) {
				php_error_docref(NULL, E_WARNING, "expects argument 3 to be int");
				RETURN_FALSE;
			}

			if (!new_php_fd(&new_f, fcntl(f->fd, cmd, Z_LVAL_P(arg)))) {
				RETURN_FALSE;
			}
			zend_register_resource(new_f, le_fd);
			break;
		}
		default:
			if (!arg || Z_TYPE_P(arg) != IS_LONG) {
				php_error_docref(NULL, E_WARNING, "expects argument 3 to be int");
				RETURN_FALSE;
			}

			RETURN_LONG(fcntl(f->fd, cmd, Z_LVAL_P(arg)));
	}
}
/* }}} */
#endif

#ifndef PHP_WIN32

/* {{{ proto mixed dio_tcsetattr(resource fd,  array args )
   Perform a c library tcsetattr on fd */
PHP_FUNCTION(dio_tcsetattr)
{
	zval     *r_fd;
	zval     *arg = NULL;
	php_fd_t *f;
	struct termios newtio;
	int Baud_Rate, Data_Bits=8, Stop_Bits=1, Parity=0, Flow_Control=1, Is_Canonical=1;
	long BAUD,DATABITS,STOPBITS,PARITYON,PARITY;
	HashTable      *fh;
	zval           *element;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "rz", &r_fd, &arg) == FAILURE) {
		return;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}

	if (Z_TYPE_P(arg) != IS_ARRAY) {
		php_error_docref(NULL, E_WARNING,"tcsetattr, third argument should be an associative array");
		return;
	}

	fh = HASH_OF(arg);

	if ((element = zend_hash_str_find(fh, "baud", sizeof("baud") - 1)) == NULL) {
		Baud_Rate = 9600;
	} else {
		Baud_Rate = Z_LVAL_P(element);
	}

	if ((element = zend_hash_str_find(fh, "bits", sizeof("bits") - 1)) == NULL) {
		Data_Bits = 8;
	} else {
		Data_Bits = Z_LVAL_P(element);
	}

	if ((element = zend_hash_str_find(fh, "stop", sizeof("stop") - 1)) == NULL) {
		Stop_Bits = 1;
	} else {
		Stop_Bits = Z_LVAL_P(element);
	}

	if ((element = zend_hash_str_find(fh, "parity", sizeof("parity") - 1)) == NULL) {
		Parity = 0;
	} else {
		Parity = Z_LVAL_P(element);
	}

	if ((element = zend_hash_str_find(fh, "flow_control", sizeof("flow_control") - 1)) == NULL) {
		Flow_Control = 1;
	} else {
		Flow_Control = Z_LVAL_P(element);
	}

	if ((element = zend_hash_str_find(fh, "is_canonical", sizeof("is_canonical") - 1)) == NULL) {
		Is_Canonical = 0;
	} else {
		Is_Canonical = Z_LVAL_P(element);
	}

	/* assign to correct values... */
	switch (Baud_Rate)  {
#ifdef B460800
		case 460800:
			BAUD = B460800;
			break;
#endif
#ifdef B230400
		case 230400:
			BAUD = B230400;
			break;
#endif
#ifdef B115200
		case 115200:
			BAUD = B115200;
			break;
#endif
#ifdef B57600
		case 57600:
			BAUD = B57600;
			break;
#endif
		case 38400:
			BAUD = B38400;
			break;
		case 19200:
			BAUD = B19200;
			break;
		case 9600:
			BAUD = B9600;
			break;
		case 4800:
			BAUD = B4800;
			break;
		case 2400:
			BAUD = B2400;
			break;
		case 1800:
			BAUD = B1800;
			break;
		case 1200:
			BAUD = B1200;
			break;
		case 600:
			BAUD = B600;
			break;
		case 300:
			BAUD = B300;
			break;
		case 200:
			BAUD = B200;
			break;
		case 150:
			BAUD = B150;
			break;
		case 134:
			BAUD = B134;
			break;
		case 110:
			BAUD = B110;
			break;
		case 75:
			BAUD = B75;
			break;
		case 50:
			BAUD = B50;
			break;
		default:
			php_error_docref(NULL, E_WARNING, "invalid baud rate %d", Baud_Rate);
			RETURN_FALSE;
	}
	switch (Data_Bits) {
		case 8:
			DATABITS = CS8;
			break;
		case 7:
			DATABITS = CS7;
			break;
		case 6:
			DATABITS = CS6;
			break;
		case 5:
			DATABITS = CS5;
			break;
		default:
			php_error_docref(NULL, E_WARNING, "invalid data bits %d", Data_Bits);
			RETURN_FALSE;
	}
	switch (Stop_Bits) {
		case 1:
			STOPBITS = 0;
			break;
		case 2:
			STOPBITS = CSTOPB;
			break;
		default:
			php_error_docref(NULL, E_WARNING, "invalid stop bits %d", Stop_Bits);
			RETURN_FALSE;
	}

	switch (Parity) {
		case 0:
			PARITYON = 0;
			PARITY = 0;
			break;
		case 1:
			PARITYON = PARENB;
			PARITY = PARODD;
			break;
		case 2:
			PARITYON = PARENB;
			PARITY = 0;
			break;
		default:
			php_error_docref(NULL, E_WARNING, "invalid parity %d", Parity);
			RETURN_FALSE;
	}

	memset(&newtio, 0, sizeof(newtio));
	tcgetattr(f->fd, &newtio);

	if (Is_Canonical) {
		newtio.c_iflag = IGNPAR | ICRNL;
		newtio.c_oflag = 0;
		newtio.c_lflag = ICANON;
	} else {
		cfmakeraw(&newtio);
	}

	newtio.c_cflag = BAUD | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL | CREAD;

#ifdef CRTSCTS
	if (Flow_Control) {
		newtio.c_cflag |= CRTSCTS;
	}
#endif

	if (Is_Canonical)

	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 0;
	tcflush(f->fd, TCIFLUSH);
	tcsetattr(f->fd,TCSANOW,&newtio);

	RETURN_TRUE;
}
/* }}} */
#endif

/* {{{ proto void dio_close(resource fd)
   Close the file descriptor given by fd */
PHP_FUNCTION(dio_close)
{
	zval     *r_fd;
	php_fd_t *f;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &r_fd) == FAILURE) {
		return;
	}

	if ((f = (php_fd_t *) zend_fetch_resource(Z_RES_P(r_fd), le_fd_name, le_fd)) == NULL) {
		RETURN_FALSE;
	}

	zend_list_close(Z_RES_P(r_fd));
}
/* }}} */

#define RDIOC(c) REGISTER_LONG_CONSTANT(#c, c, CONST_CS | CONST_PERSISTENT)

/* {{{ dio_init_legacy_defines
 * Initialises the legacy PHP defines
 */
static void dio_init_legacy_defines(int module_number) {
	RDIOC(O_RDONLY);
	RDIOC(O_WRONLY);
	RDIOC(O_RDWR);
	RDIOC(O_CREAT);
	RDIOC(O_EXCL);
	RDIOC(O_TRUNC);
	RDIOC(O_APPEND);
#ifdef O_NONBLOCK
	RDIOC(O_NONBLOCK);
#endif
#ifdef O_NDELAY
	RDIOC(O_NDELAY);
#endif
#ifdef O_SYNC
	RDIOC(O_SYNC);
#endif
#ifdef O_ASYNC
	RDIOC(O_ASYNC);
#endif
#ifdef O_NOCTTY
	RDIOC(O_NOCTTY);
#endif
#ifndef PHP_WIN32
	RDIOC(S_IRWXU);
	RDIOC(S_IRUSR);
	RDIOC(S_IWUSR);
	RDIOC(S_IXUSR);
	RDIOC(S_IRWXG);
	RDIOC(S_IRGRP);
	RDIOC(S_IWGRP);
	RDIOC(S_IXGRP);
	RDIOC(S_IRWXO);
	RDIOC(S_IROTH);
	RDIOC(S_IWOTH);
	RDIOC(S_IXOTH);
	RDIOC(F_DUPFD);
	RDIOC(F_GETFD);
	RDIOC(F_GETFL);
	RDIOC(F_SETFL);
	RDIOC(F_GETLK);
	RDIOC(F_SETLK);
	RDIOC(F_SETLKW);
	RDIOC(F_SETOWN);
	RDIOC(F_GETOWN);
	RDIOC(F_UNLCK);
	RDIOC(F_RDLCK);
	RDIOC(F_WRLCK);
#endif
}

ZEND_BEGIN_ARG_INFO_EX(dio_open_args, 0, 0, 2)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, flags)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_fdopen_args, 0, 0, 1)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_dup_args, 0, 0, 1)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(dio_read_args, 0, 0, 1)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_INFO(0, n)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_write_args, 0, 0, 2)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, len)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_stat_args, 0, 0, 1)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_truncate_args, 0, 0, 2)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_seek_args, 0, 0, 3)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_INFO(0, pos)
	ZEND_ARG_INFO(0, whence)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_fcntl_args, 0, 0, 2)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_INFO(0, cmd)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_tcsetattr_args, 0, 0, 2)
	ZEND_ARG_INFO(0, fd)
	ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_close_args, 0, 0, 1)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()

/*
   +----------------------------------------------------------------------+
   |                   END OF DEPRECATED FUNCTIONALITY                    |
   +----------------------------------------------------------------------+
 */

ZEND_BEGIN_ARG_INFO_EX(dio_raw_args, 0, 0, 2)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, mode)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dio_serial_args, 0, 0, 2)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, mode)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

/* not used static zend_object_handlers dio_raw_object_handlers; */

static zend_function_entry dio_functions[] = {
	/* Class functions. */

	/* Legacy functions (Deprecated - See dio_legacy.c) */
	PHP_FE(dio_open, dio_open_args)
#ifndef PHP_WIN32
	PHP_FE(dio_fdopen, dio_fdopen_args)
	PHP_FE(dio_dup, dio_dup_args)
	PHP_FE(dio_truncate, dio_truncate_args)
#endif
	PHP_FE(dio_stat, dio_stat_args)
	PHP_FE(dio_seek, dio_seek_args)
#ifndef PHP_WIN32
	PHP_FE(dio_fcntl, dio_fcntl_args)
#endif
	PHP_FE(dio_read, dio_read_args)
	PHP_FE(dio_write, dio_write_args)
	PHP_FE(dio_close, dio_close_args)
#ifndef PHP_WIN32
	PHP_FE(dio_tcsetattr, dio_tcsetattr_args)
#endif

	/* Stream functions */
	PHP_FE(dio_raw, dio_raw_args)
	PHP_FE(dio_serial, dio_serial_args)

	/* End of functions */
	{NULL, NULL, NULL}
};

zend_module_entry dio_module_entry = {
	STANDARD_MODULE_HEADER,
	"dio",
	dio_functions,
	PHP_MINIT(dio),
	NULL,
	NULL,	
	NULL,
	PHP_MINFO(dio),
	PHP_DIO_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_DIO
ZEND_GET_MODULE(dio)
#endif

#define DIO_UNDEF_CONST -1

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(dio)
{
	/* Legacy resource destructor. */
	le_fd = zend_register_list_destructors_ex(_dio_close_fd, NULL, le_fd_name, module_number);

	dio_init_legacy_defines(module_number);

	/* Register the stream wrappers */
	return (php_register_url_stream_wrapper(DIO_RAW_STREAM_NAME, &php_dio_raw_stream_wrapper) == SUCCESS &&
			php_register_url_stream_wrapper(DIO_SERIAL_STREAM_NAME, &php_dio_serial_stream_wrapper) == SUCCESS) ? SUCCESS : FAILURE;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(dio)
{
	return (php_unregister_url_stream_wrapper(DIO_RAW_STREAM_NAME) == SUCCESS &&
			php_unregister_url_stream_wrapper(DIO_SERIAL_STREAM_NAME) == SUCCESS) ? SUCCESS : FAILURE;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(dio)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "dio support", "enabled");
	php_info_print_table_row(2, "version", PHP_DIO_VERSION);
	php_info_print_table_end();
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
