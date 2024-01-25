--TEST--
Test dio legacy fdopen
--SKIPIF--
<?php 
	if (!extension_loaded("dio")) print "skip"; 
	if (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN') print "skip";
?>
--FILE--
<?php 
	$f = dio_fdopen(1);
	if ($f) {
		echo "Legacy fdopen passed";
	} else {
		echo "Legacy fdopen failed";
	}
?>
--EXPECT--
Legacy fdopen passed
