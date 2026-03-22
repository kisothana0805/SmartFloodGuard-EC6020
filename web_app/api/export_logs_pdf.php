<?php
require_once "db.php";
date_default_timezone_set("Asia/Colombo");

// FPDF
require_once __DIR__ . "/../lib/fpdf/fpdf.php";

$device_id = $_GET["device_id"] ?? "";
$from = $_GET["from"] ?? ""; // YYYY-MM-DDTHH:MM
$to   = $_GET["to"] ?? "";
$flood_only = (int)($_GET["flood_only"] ?? 0);

if ($device_id === "" || $from === "" || $to === "") {
    http_response_code(400);
    echo "Missing required parameters.";
    exit;
}

// Convert datetime-local format (2025-12-25T19:30) => (2025-12-25 19:30:00)
$from_sql = str_replace("T", " ", $from) . ":00";
$to_sql   = str_replace("T", " ", $to) . ":59";

// Basic sanity (from must be <= to)
if (strtotime($from_sql) === false || strtotime($to_sql) === false || strtotime($from_sql) > strtotime($to_sql)) {
    http_response_code(400);
    echo "Invalid date range.";
    exit;
}

// Query
$sql = "
SELECT id, flood_detected, height_cm, humidity, created_at
FROM sensor_readings
WHERE device_id = ?
  AND created_at BETWEEN ? AND ?
";
if ($flood_only === 1) {
    $sql .= " AND flood_detected = 1";
}
$sql .= " ORDER BY created_at ASC LIMIT 2000"; // safety cap

$stmt = $conn->prepare($sql);
$stmt->bind_param("sss", $device_id, $from_sql, $to_sql);
$stmt->execute();
$res = $stmt->get_result();

$rows = [];
while ($r = $res->fetch_assoc()) $rows[] = $r;

// ---- Generate PDF ----
$pdf = new FPDF("P", "mm", "A4");
$pdf->AddPage();
$pdf->SetAutoPageBreak(true, 15);

// Title
$pdf->SetFont("Arial", "B", 14);
$pdf->Cell(0, 8, "Indoor Flood Monitor - Log Report", 0, 1, "C");

$pdf->SetFont("Arial", "", 10);
$pdf->Cell(0, 6, "Device: $device_id", 0, 1, "L");
$pdf->Cell(0, 6, "Range: $from_sql  to  $to_sql", 0, 1, "L");
$pdf->Cell(0, 6, "Filter: " . ($flood_only ? "Flood only" : "All readings"), 0, 1, "L");
$pdf->Ln(2);

// Table header
$pdf->SetFont("Arial", "B", 10);
$pdf->Cell(12, 7, "#", 1, 0, "C");
$pdf->Cell(40, 7, "Date/Time", 1, 0, "C");
$pdf->Cell(20, 7, "Flood", 1, 0, "C");
$pdf->Cell(30, 7, "Height (cm)", 1, 0, "C");
$pdf->Cell(30, 7, "Humidity (%)", 1, 1, "C");

$pdf->SetFont("Arial", "", 10);

if (count($rows) === 0) {
    $pdf->Cell(132, 8, "No logs found in this selected range.", 1, 1, "C");
} else {
    $i = 1;
    foreach ($rows as $r) {
        $flood = ((int)$r["flood_detected"] === 1) ? "YES" : "NO";
        $height = number_format((float)$r["height_cm"], 1);
        $hum = ($r["humidity"] === null) ? "--" : number_format((float)$r["humidity"], 1);

        $pdf->Cell(12, 7, $i, 1, 0, "C");
        $pdf->Cell(40, 7, $r["created_at"], 1, 0, "C");
        $pdf->Cell(20, 7, $flood, 1, 0, "C");
        $pdf->Cell(30, 7, $height, 1, 0, "C");
        $pdf->Cell(30, 7, $hum, 1, 1, "C");

        $i++;
    }
}

// Output as download
$filename = "logs_" . $device_id . "_" . date("Ymd_His") . ".pdf";
header("Content-Type: application/pdf");
header("Content-Disposition: attachment; filename=\"$filename\"");
$pdf->Output("I", $filename);