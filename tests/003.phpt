--TEST--
Test dio legacy dup
--SKIPIF--
<?php 
	if (!extension_loaded("dio")) print "skip"; 
	if (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN') print "skip";
?>
--FILE--
<?php 
	$f = dio_open("/dev/null",O_RDONLY);
	if ($f) {
		$df = dio_dup($f);
		if ($f) {
			echo "Legacy dup passed";
		} else {
			echo "Legacy dup failed";
		}
	} else {
		echo "Legacy dup precondition failed";
	}
?>
--EXPECT--
Legacy dup passed
