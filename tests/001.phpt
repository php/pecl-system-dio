--TEST--
Test dio legacy open
--SKIPIF--
<?php if (!extension_loaded("dio")) print "skip"; ?>
--FILE--
<?php 
	$f = dio_open("/dev/null",O_RDONLY);
	if ($f) {
		echo "Legacy open passed";
	} else {
		echo "Legacy open failed";
	}
?>
--EXPECT--
Legacy open passed
