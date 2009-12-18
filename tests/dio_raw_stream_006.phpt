--TEST--
Test dio raw stream set blocking
--SKIPIF--
<?php if (!extension_loaded("dio")) print "skip"; ?>
--FILE--
<?php 
	$iswin = (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN'); 

	if (!$iswin) {
		$filename = "dio.raw:///dev/null";
	} else {
		$filename = "dio.raw://c:\\ntdetect.com";
	}

	$f = fopen($filename, "r");
	if ($f) {
		if (stream_set_blocking($f, 1)) {
			echo "Set blocking passed";
		}
		fclose($f);
	}
?>
--EXPECT--
Set blocking passed
