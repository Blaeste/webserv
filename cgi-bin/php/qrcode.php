#!/usr/bin/php-cgi
<?php
header("Content-Type: application/json; charset=utf-8");

// Get text from query string
$text = isset($_GET['text']) ? $_GET['text'] : '';

if (empty($text)) {
    echo json_encode(array('error' => 'Please enter some text'));
    exit;
}

// Init response
$response = array();

// Limit input to 500 char
if (strlen($text) > 500) {
	$text = substr($text, 0, 500);
	$response['warning'] = "⚠️ Text truncated to 500 characters";
}

// Sanitize text
$text = htmlspecialchars($text, ENT_QUOTES, 'UTF-8');

// Generate Qr Code Url
$qrUrl = 'https://api.qrserver.com/v1/create-qr-code/?size=300x300&data=' . urlencode($text);

// Build response
$response['url'] = $qrUrl;
$response['text'] = $text;

echo json_encode($response);
?>
