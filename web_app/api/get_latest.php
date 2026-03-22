<?php
header("Content-Type: application/json");
header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
header("Pragma: no-cache");
header("Expires: 0");
require_once "db.php";

$device_id = $_GET["device_id"] ?? "";

if ($device_id === "") {
    http_response_code(400);
    echo json_encode(["error" => "device_id required"]);
    exit;
}

$stmt = $conn->prepare("
    SELECT id, device_id, flood_detected, height_cm, humidity, drip_triggered, created_at
    FROM sensor_readings
    WHERE device_id = ?
    ORDER BY created_at DESC
    LIMIT 1
");
$stmt->bind_param("s", $device_id);
$stmt->execute();

$res = $stmt->get_result();
$row = $res->fetch_assoc();

echo json_encode(["data" => $row]);
?>