<?php
header("Content-Type: application/json");
require_once "db.php";

$raw = file_get_contents("php://input");
$data = json_decode($raw, true);

if (!$data) {
    http_response_code(400);
    echo json_encode(["error" => "Invalid JSON"]);
    exit;
}

$device_id = $data["device_id"] ?? "";
$flood_detected = $data["flood_detected"] ?? null;
$height_cm = $data["height_cm"] ?? null;
$humidity = $data["humidity"] ?? null;
$drip_triggered = $data["drip_triggered"] ?? 0;

if ($device_id === "" || $flood_detected === null || $height_cm === null) {
    http_response_code(400);
    echo json_encode(["error" => "Missing required fields"]);
    exit;
}

$stmt = $conn->prepare("INSERT INTO sensor_readings (device_id, flood_detected, height_cm, humidity, drip_triggered) VALUES (?, ?, ?, ?, ?)");
$stmt->bind_param("siddi", $device_id, $flood_detected, $height_cm, $humidity, $drip_triggered);

if ($stmt->execute()) {
    // auto alert if flood detected
    if ((int)$flood_detected === 1) {
        $msg = "Flood entry detected. Height: " . $height_cm . " cm";
        $alert = $conn->prepare("INSERT INTO alerts (device_id, alert_type, message) VALUES (?, 'FLOOD', ?)");
        $alert->bind_param("ss", $device_id, $msg);
        $alert->execute();
    }

    echo json_encode(["ok" => true, "insert_id" => $stmt->insert_id]);
} else {
    http_response_code(500);
    echo json_encode(["error" => "Insert failed"]);
}
?>