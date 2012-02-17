--TEST--
Test dio raw read
--SKIPIF--
<?php
	if (!extension_loaded('dio')) print 'skip';
?>
--FILE--
<?php 
	// Create a temp file with some content to read

	// Create the temp file name
	if (!function_exists('sys_get_temp_dir')) {
		if (!($tmpdir = getenv('TEMP'))) {
			$tmpdir = '';
		}
	} else {
		$tmpdir = sys_get_temp_dir();
	}
	$filename = tempnam($tmpdir, 'dio_raw_stream_004.tmp');
			
	// Create the temp file
	$tf = fopen($filename, "w");
	if ($tf) {
	 	fwrite($tf, str_repeat('*', 2048));	
		fclose($tf);
	} else {
		echo "Can\'t create temp file";
	}

	$f = fopen('dio.raw://' . $filename, "r");
	if ($f) {
		$data = fread($f, 2048);
		if ($data && (strlen($data) == 2048)) {
			echo "Raw read passed";
		} else {
			echo "Raw read failed";
		}
		fclose($f);
	} else {
		echo "Raw open failed";
	}

	unlink($filename);
?>
--EXPECT--
Raw read passed
