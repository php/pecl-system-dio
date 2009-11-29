--TEST--
Test dio raw stream close
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
		if (fclose($f)) {
			echo "Raw close passed";
		}
	}
?>
--EXPECT--
Raw close passed
