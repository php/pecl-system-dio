--TEST--
Test dio_open
--SKIPIF--
<?php
if (!extension_loaded("dio")) die("skip extension missing");
?>
--FILE--
<?php 
	$iswin = (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN'); 

	if (!$iswin) {
		$filename = "/dev/null";
	} else {
		$filename = "NUL";
	}
	
	$f = dio_open($filename,O_RDONLY);
	if ($f) {
		echo "dio_open passed";
		dio_close($f);
	} else {
		echo "dio_open failed";
	}
?>
--EXPECT--
dio_open passed
