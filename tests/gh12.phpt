--TEST--
GH-12: Memory leak in dio_read
--SKIPIF--
<?php
if (!extension_loaded("dio")) die("skip extension missing");
?>
--INI--
memory_limit=10M
--FILE--
<?php
file_put_contents(__DIR__ . '/dio-test-file', str_repeat('0', 10 * 1024));
$hFile = dio_open(__DIR__ . '/dio-test-file', O_RDONLY);

for($i=0 ; $i<10000; $i++) {
	dio_seek($hFile, 0);
	$data = dio_read($hFile, 10000);
}
?>
Done
--CLEAN--
<?php
@unlink(__DIR__ . '/dio-test-file');
?>
--EXPECT--
Done
