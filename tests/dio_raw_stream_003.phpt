--TEST--
Test dio raw read
--SKIPIF--
<?php if (!extension_loaded("dio")) print "skip"; ?>
--FILE--
<?php 
	$iswin = (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN'); 

	if (!$iswin) {
		$filename = "dio.raw:///dev/zero";
	} else {
		$filename = "dio.raw://c:\\ntdetect.com";
	}

	$f = fopen($filename, "r");
	if ($f) {
		$data = fread($f, 2048);
		if ($data && (strlen($data) == 2048)) {
			echo "Raw read passed";
		}
		fclose($f);
	}
?>
--EXPECT--
Raw read passed
