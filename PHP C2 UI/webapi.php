<?php

ini_set('display_errors', '1');
ini_set('display_startup_errors', '1');
error_reporting(E_ALL);


include_once "db/Info.php";

$bdd = $GLOBALS["bdd"];

if (!isset($_GET["key"]) or $_GET["key"] != "Yq5GYNwzLcZVe9iHiWEbQVVtv6W44hGZvsCUVXVWmZkh6Mru8qFTEToVndDkZ5LH") {
    echo json_encode(array("status" => "error"));
    exit();
}

if (!isset($_GET["ipv4"]) || !isset($_GET["host"]) || !filter_var($_GET["ipv4"], FILTER_VALIDATE_IP, FILTER_FLAG_IPV4)) {
    echo json_encode(array("status" => "error"));
    exit();
}

$data = $bdd->query("SELECT * FROM infected WHERE ip = ? AND host = ?", $_GET["ipv4"], $_GET["host"])->fetchAll();

if (!empty($data)) {
    echo json_encode(array("status" => "success") + array("data" => $data[0]));
} else {
    echo json_encode(array("status" => "error"));
}
