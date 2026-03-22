<?php
header("Content-Type: application/json");
require_once "db.php";

$device_id = $_GET["device_id"] ?? "";

if ($device_id === "") {
  http_response_code(400);
  echo json_encode(["error" => "device_id required"]);
  exit;
}

// Get the oldest pending command
$stmt = $conn->prepare("
  SELECT id, command, payload, created_at
  FROM commands
  WHERE device_id = ? AND status = 'PENDING'
  ORDER BY created_at ASC
  LIMIT 1
");
$stmt->bind_param("s", $device_id);
$stmt->execute();
$res = $stmt->get_result();
$row = $res->fetch_assoc();

if (!$row) {
  echo json_encode(["data" => null]);
  exit;
}

// Mark as sent (optional but nice)
$upd = $conn->prepare("UPDATE commands SET status='SENT' WHERE id=?");
$upd->bind_param("i", $row["id"]);
$upd->execute();

$row["payload"] = $row["payload"] ? json_decode($row["payload"], true) : null;

echo json_encode(["data" => $row]);
?>