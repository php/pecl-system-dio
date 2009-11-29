--TEST--
Test dio raw stream end of file
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
		if (!feof($f)) {
			echo "Raw end of file passed";
		}
		fclose($f);
	}
?>
--EXPECT--
Raw end of file passed
