--TEST--
Test dio_stat
--SKIPIF--
<?php
if (!extension_loaded("dio")) print "skip";
if (strtoupper(substr(PHP_OS, 0, 3)) != 'WIN') print "skip Windows only";
?>
--FILE--
<?php 
	$filename = "NUL";
	
	$f = dio_open($filename, O_RDONLY);
	var_dump(array_keys(dio_stat($f)));
	dio_close($f);
?>
Done
--EXPECT--
array(11) {
  [0]=>
  string(6) "device"
  [1]=>
  string(5) "inode"
  [2]=>
  string(4) "mode"
  [3]=>
  string(5) "nlink"
  [4]=>
  string(3) "uid"
  [5]=>
  string(3) "gid"
  [6]=>
  string(11) "device_type"
  [7]=>
  string(4) "size"
  [8]=>
  string(5) "atime"
  [9]=>
  string(5) "mtime"
  [10]=>
  string(5) "ctime"
}
Done
