--TEST--
Test dio_raw() call
--SKIPIF--
<?php if (!extension_loaded("dio")) print "skip"; ?>
--FILE--
<?php
	$ostype = "windows"; 
	if (shell_exec("which uname")) {
		$ostype = shell_exec("uname");
	}
	if ($ostype != "windows") {
		$filename = "/dev/null";
	} else {
		$filename = "\\Device\\Null";
	}
	$f = dio_raw($filename, "r+");
	if ($f) {
		echo "dio_raw passed";
		fclose($f);
	}
?>
--EXPECT--
dio_raw passed
