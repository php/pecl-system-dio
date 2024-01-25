--TEST--
Test dio_fdopen
--SKIPIF--
<?php
if (!extension_loaded("dio")) die("skip extension missing");
if (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN') print "skip Linux only";
?>
--FILE--
<?php 
	$f = dio_fdopen(1);
	if ($f) {
		echo "dio_fdopen passed";
		dio_close($f);
	} else {
		echo "dio_fdopen failed";
	}
?>
--EXPECT--
dio_fdopen passed
