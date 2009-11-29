--TEST--
Test dio raw stream write
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
		$data = str_repeat("+", 2048);
		if (fwrite($f, $data)) {
			echo "Raw write passed";
		}
		fclose($f);
	}
?>
--EXPECT--
Raw write passed
