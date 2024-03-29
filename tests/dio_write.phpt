--TEST--
Test dio_write, dio_read, dio_truncate, dio_seek
--SKIPIF--
<?php
if (!extension_loaded("dio")) die("skip extension missing");
?>
--FILE--
<?php 
	$filename = __DIR__ . "/write.txt";
	
	echo "+ open\n";
	var_dump($f = dio_open($filename, O_CREAT | O_RDWR, 0644));
	if ($f) {
		$s = dio_stat($f);
		printf("Size=%d\n", $s['size']);

		echo "+ write\n";
		var_dump(dio_write($f, "foobar"));
		$s = dio_stat($f);
		printf("Size=%d\n", $s['size']);

		echo "+ seek\n";
		var_dump(dio_seek($f, 0));

		echo "+ read\n";
		var_dump(dio_read($f, 1));
		var_dump(dio_read($f, 2));
		var_dump(dio_read($f));
		var_dump(dio_read($f));

		echo "+ close\n";
		dio_close($f);
	}
?>
Done
--CLEAN--
<?php
@unlink(__DIR__ . "/write.txt");
?>
--EXPECTF--
+ open
resource(%d) of type (Direct I/O File Descriptor)
Size=0
+ write
int(6)
Size=6
+ seek
int(0)
+ read
string(1) "f"
string(2) "oo"
string(3) "bar"
NULL
+ close
Done
