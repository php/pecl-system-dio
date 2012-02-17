--TEST--
Test dio legacy open
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
	
	$f = dio_open($filename,O_RDONLY);
	if ($f) {
		echo "Legacy open passed";
	} else {
		echo "Legacy open failed";
	}
?>
--EXPECT--
Legacy open passed
