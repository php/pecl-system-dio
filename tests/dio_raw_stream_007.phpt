--TEST--
Test dio_raw() call
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

	$f = dio_raw($filename, "r+");
	if ($f) {
		echo "dio_raw passed\n";
		fclose($f);
	}
	$f = dio_raw($filename, "r+", ['is_blocking' => 1]);
	if ($f) {
		echo "dio_raw passed\n";
		fclose($f);
	}
	$f = dio_raw($filename, "r+", NULL);
	if ($f) {
		echo "dio_raw passed\n";
		fclose($f);
	}
	try {
		$f = dio_raw($filename, "r+", "foo");
	} catch(TypeError $e) {
		echo $e->getMessage() . "\n";
	}
?>
Done
--EXPECT--
dio_raw passed
dio_raw passed
dio_raw passed
dio_raw(): Argument #3 ($options) must be of type ?array, string given
Done
