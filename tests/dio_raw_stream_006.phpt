--TEST--
Test dio raw stream set blocking
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
		if (stream_set_blocking($f, 1)) {
			echo "Set blocking passed";
		}
		fclose($f);
	}
?>
--EXPECT--
Set blocking passed
