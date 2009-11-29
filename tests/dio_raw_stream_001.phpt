--TEST--
Test dio raw stream open
--SKIPIF--
<?php if (!extension_loaded("dio")) print "skip"; ?>
--FILE--
<?php
	$ostype = "windows"; 
	if (shell_exec("which uname")) {
		$ostype = shell_exec("uname");
	}
	if ($ostype != "windows") {
		$filename = "dio.raw:///dev/null";
	} else {
		$filename = "dio.raw://\\Device\\Null";
	}
	$f = fopen($filename, "r+");
	if ($f) {
		echo "Raw open passed";
		fclose($f);
	}
?>
--EXPECT--
Raw open passed
