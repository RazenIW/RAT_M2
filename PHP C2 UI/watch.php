<?php
	session_start();
	
	if (isset($_POST["login"]) and isset($_POST["pass"])) {
		if ($_POST["login"] == "razen" and $_POST["pass"] == "skffqzbACtG47enUvggkQUpJ5FKR3UExNVGCDb542emA3siUzjF2SC67G4fVAWp8") {
			$_SESSION["logged_on"] = 1;
		}
	}
	
	if (isset($_POST["disconnect"])) {
		unset($_SESSION["logged_on"]);
		echo "unset";
	}
	
	$img_opened = isset($_GET["host"]) and isset($_SESSION["logged_on"]);
?>

<html>
	<head>
		<style>
			* {
				margin: 0;
				padding: 0;
				font-family: Verdana, sans-serif;
				<?php
					if ($img_opened) {
						echo "background-color: #161616;";
					}
				?>
			}
			
			.imgbox {
				display: grid;
				height: 100%;
			}
			
			.center-fit {
				max-width: 100%;
				max-height: 100vh;
				margin: auto;
			}
			
			.center {
				position: absolute;
                top: 50%;
                left: 50%;
                -moz-transform: translateX(-50%) translateY(-50%);
                -webkit-transform: translateX(-50%) translateY(-50%);
                transform: translateX(-50%) translateY(-50%);
                padding: 10px;
			}
		</style>
	</head>
	<body>

		<?php
			if (!isset($_SESSION["logged_on"])) {
				echo "<div class='center'><form method='post'>Login<br><input name='login' type='text' /><br><br>Password<br><input name='pass' type='password' />";
				echo "<br><br><input type='submit' value='Login'></form></div>";
			} else {
				if (!isset($_GET["host"])) {
					$image_folder = "/home/images/";
					$images = glob($image_folder . '*.jpeg');
					$links = array();
					
					foreach ($images as $image) {
						$file_info = pathinfo($image);
						$timestamp = filemtime($image);
						$data = explode("@", $file_info["filename"]);
						$host = $data[0];
						$ip = $data[1];
						$date = date("d/m/Y H:i:s", $timestamp);

						$links[] = "<a href='?host={$host}'>{$host} from {$ip} (latest upload {$date})</a>";
					}
					
					echo "<div class='center'><h3>Available hosts :</h3><br><ul>";
					
					foreach ($links as $link) {
						echo "<li>{$link}</li>";
					}

					echo "</ul><form method='post' style='margin-top: 50px;'><input type='submit' name='disconnect' value='Disconnect'></form></div>";
				} else {
					$host = $_GET["host"];
					$key = "5F4p42L6Qwu8LFjAm9dqiXnWUWAhs5mtg24pHnR3tEj6PimrrpXCrUAAzf4cmX9B";
					echo "<div class='imgbox'>";
					echo "<img id='screenshot' src='http://185.143.220.132/latest.php?host={$host}&key={$key}' class='center-fit'>";
					echo "</div>";
				}
			}
		?>

	</body>
	<script>
		if (window.history.replaceState) {
			window.history.replaceState(null, null, window.location.href);
		}
	</script>
	<?php
		if ($img_opened) {
			$host = $_GET["host"]; 
			echo "
			<script>
			  function refreshImage() {
				var img = document.getElementById('screenshot');
				img.src = 'http://185.143.220.132/latest.php?host={$host}&key=5F4p42L6Qwu8LFjAm9dqiXnWUWAhs5mtg24pHnR3tEj6PimrrpXCrUAAzf4cmX9B&' + new Date().getTime();
			  }
			  
			  setInterval(refreshImage, 3000);
			</script>
			";
		}
	?>
</html>


