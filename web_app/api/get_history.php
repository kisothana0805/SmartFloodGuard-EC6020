<?php
header("Content-Type: application/json");
require_once "db.php";

$device_id = $_GET["device_id"] ?? "";
$limit = (int)($_GET["limit"] ?? 100);

if ($device_id === "") {
    http_response_code(400);
    echo json_encode(["error" => "device_id required"]);
    exit;
}

if ($limit <= 0 || $limit > 1000) $limit = 100;

// SAFE because we clamp limit to 1..1000 and cast to int
$sql = "
    SELECT id, flood_detected, height_cm, humidity, drip_triggered, created_at
    FROM sensor_readings
    WHERE device_id = ?
    ORDER BY created_at DESC
    LIMIT $limit
";

$stmt = $conn->prepare($sql);
$stmt->bind_param("s", $device_id);
$stmt->execute();

$res = $stmt->get_result();
$out = [];
while ($r = $res->fetch_assoc()) $out[] = $r;

echo json_encode(["data" => $out]);
?>