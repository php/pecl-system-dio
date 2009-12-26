--TEST--
Test dio raw stream close
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
		if (fclose($f)) {
			echo "Raw close passed";
		} else {
			echo "Raw close failed";
		}
	} else {
		echo "Raw open failed";
	}
?>
--EXPECT--
Raw close passed
