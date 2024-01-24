<?php

/** 
 * @generate-class-entries
 * @generate-legacy-arginfo 80000
 */

/**
 * @return resource|false
 */
function dio_open(string $filename, int $flags, int $mode=0) {}

#ifndef PHP_WIN32
/**
 * @param resource $fd
 * @return resource|false
 */
function dio_fdopen($fd) {}

/**
 * @param resource $fd
 * @return resource|false
 */
function dio_dup($fd) {}

/**
 * @param resource $fd
 */
function dio_truncate($fd, int $offset): bool {}
#endif

/**
 * @param resource $fd
 */
function dio_stat($fd): array|false {}

/**
 * @param resource $fd
 */
function dio_seek($fd, int $pos, int $whence=SEEK_SET): int {}

#ifndef PHP_WIN32
/**
 * @param resource $fd
 * @param mixed $arg
 * @return mixed
 */
function dio_fcntl($fd, int $cmd, $arg=NULL) {}
#endif

/**
 * @param resource $fd
 */
function dio_read($fd, int $n=1024): ?string {}

/**
 * @param resource $fd
 */
function dio_write($fd, string $data, int $len=0): int {}

/**
 * @param resource $fd
 */
function dio_close($fd): void {}

#ifndef PHP_WIN32
/**
 * @param resource $fd
 * @param mixed $arg
 * @return mixed
 */
function dio_tcsetattr($fd, array $args): bool {}
#endif

/**
 * @return resource|false
 */
function dio_raw(string $filename, string $mode, ?array $options=NULL) {}

/**
 * @return resource|false
 */
function dio_serial(string $filename, string $mode, ?array $options=NULL) {}

