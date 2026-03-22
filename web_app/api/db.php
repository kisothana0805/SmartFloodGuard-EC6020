<?php
$host = "localhost";
$user = "root";
$pass = "";
$dbname = "flood_db";

$conn = new mysqli($host, $user, $pass, $dbname);

if ($conn->connect_error) {
    http_response_code(500);
    die(json_encode(["error" => "DB connection failed"]));
}

$conn->set_charset("utf8");
?>