--TEST--
Test dio raw read
--SKIPIF--
<?php if (!extension_loaded("dio")) print "skip"; ?>
--FILE--
<?php 
	$ostype = "windows"; 
	if (shell_exec("which uname")) {
		$ostype = shell_exec("uname");
	}
	if ($ostype != "windows") {
		$filename = "dio.raw:///dev/zero";
	} else {
		$filename = "dio.raw://\\Device\\Null";
	}
	$f = fopen($filename, "r+");
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
