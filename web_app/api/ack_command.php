<?php
header("Content-Type: application/json");
require_once "db.php";

$raw = file_get_contents("php://input");
$data = json_decode($raw, true);

if (!is_array($data)) {
  http_response_code(400);
  echo json_encode(["ok" => false, "error" => "Invalid JSON"]);
  exit;
}

$command_id = isset($data["command_id"]) ? (int)$data["command_id"] : 0;
$status     = strtoupper(trim($data["status"] ?? "EXECUTED")); // EXECUTED / FAILED
$message    = trim($data["message"] ?? ""); // optional debug note

if ($command_id <= 0) {
  http_response_code(400);
  echo json_encode(["ok" => false, "error" => "command_id required"]);
  exit;
}

if (!in_array($status, ["EXECUTED", "FAILED"], true)) {
  http_response_code(400);
  echo json_encode(["ok" => false, "error" => "Invalid status. Use EXECUTED or FAILED"]);
  exit;
}

// Update only if the command was SENT (or already EXECUTED/FAILED)
$stmt = $conn->prepare("
  UPDATE commands
  SET status = ?, executed_at = NOW()
  WHERE id = ? AND status IN ('SENT','EXECUTED','FAILED')
");
$stmt->bind_param("si", $status, $command_id);

if (!$stmt->execute()) {
  http_response_code(500);
  echo json_encode(["ok" => false, "error" => "DB execute failed"]);
  exit;
}

if ($stmt->affected_rows === 0) {
  // not found or not in correct state
  http_response_code(409);
  echo json_encode([
    "ok" => false,
    "error" => "Command not in SENT/EXECUTED/FAILED state (maybe not delivered yet)",
    "command_id" => $command_id
  ]);
  exit;
}

// Optional: store message if you add a 'result_message' column later.
// For now, just return it in response.
echo json_encode([
  "ok" => true,
  "command_id" => $command_id,
  "status" => $status,
  "message" => $message
]);
?>