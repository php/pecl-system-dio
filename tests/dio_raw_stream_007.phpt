--TEST--
Test dio_raw() call
--SKIPIF--
<?php if (!extension_loaded("dio")) print "skip"; ?>
--FILE--
<?php
	$iswin = (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN'); 

	if (!$iswin) {
		$filename = "/dev/null";
	} else {
		$filename = "NUL";
	}

	$f = dio_raw($filename, "r+");
	if ($f) {
		echo "dio_raw passed";
		fclose($f);
	}
?>
--EXPECT--
dio_raw passed
