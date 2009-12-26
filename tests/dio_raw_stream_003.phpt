--TEST--
Test dio raw stream write
--SKIPIF--
<?php if (!extension_loaded("dio")) print "skip"; ?>
--FILE--
<?php 
	$iswin = (strtoupper(substr(PHP_OS, 0, 3)) == 'WIN'); 

	if (!$iswin) {
		$filename = "dio.raw:///dev/null";
	} else {
		$filename = "dio.raw://NUL";
	}

	$f = fopen($filename, "r+");
	if ($f) {
		$data = str_repeat("+", 2048);
		if (fwrite($f, $data)) {
			echo "Raw write passed";
		} else {
			echo "Raw write failed";
		}
		fclose($f);
	} else {
		echo "Raw open failed";
	}
?>
--EXPECT--
Raw write passed
