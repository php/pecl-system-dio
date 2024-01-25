--TEST--
Test dio_dup
--SKIPIF--
<?php
if (!extension_loaded("dio")) die("skip extension missing");
if (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN') print "skip Linux only";
?>
--FILE--
<?php 
	$f = dio_open("/dev/null",O_RDONLY);
	if ($f) {
		$df = dio_dup($f);
		if ($df) {
			echo "dio_dup passed";
			dio_close($df);
		} else {
			echo "dio_dup failed";
		}
		dio_close($f);
	} else {
		echo "Legacy dup precondition failed";
	}
?>
--EXPECT--
dio_dup passed
