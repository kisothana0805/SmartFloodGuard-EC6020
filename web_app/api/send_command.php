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
$command   = $data["command"] ?? "";
$payload   = $data["payload"] ?? null;

$allowed = ["BUZZER_RESET"];

if ($device_id === "" || $command === "") {
  http_response_code(400);
  echo json_encode(["error" => "Missing device_id or command"]);
  exit;
}

if (!in_array($command, $allowed, true)) {
  http_response_code(400);
  echo json_encode(["error" => "Command not allowed"]);
  exit;
}

$payload_json = $payload ? json_encode($payload) : null;

$stmt = $conn->prepare("INSERT INTO commands (device_id, command, payload) VALUES (?, ?, ?)");
$stmt->bind_param("sss", $device_id, $command, $payload_json);

if ($stmt->execute()) {
  echo json_encode(["ok" => true, "command_id" => $stmt->insert_id]);
} else {
  http_response_code(500);
  echo json_encode(["error" => "Insert failed"]);
}
?>