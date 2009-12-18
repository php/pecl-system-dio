--TEST--
Test dio raw stream end of file
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
		if (!feof($f)) {
			echo "Raw end of file passed";
		}
		fclose($f);
	}
?>
--EXPECT--
Raw end of file passed
